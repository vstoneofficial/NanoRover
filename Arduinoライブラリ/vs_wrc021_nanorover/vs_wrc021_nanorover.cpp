#include "vs_wrc021_nanorover.h"
#include "vs_wrc021_memmap.h"
#include "vs_wrc021_i2c.h"
#include "vs_wrc021_motor.h"
#include "vs_wrc021_serial.h"
#include "vs_wrc021_spi.h"
#include "vs_wrc021_wifi.h"
#include "vs_wrc021_ble.h"
#include "vs_wrc021_btc.h"
//#include "vs_wrc031_spiffs.h"
#include "vs_wrc031_led.h"
#include <vector>
#include <string>
#include <math.h>

//LED制御のための定義
#define LEDC_CHANNEL_0     0
#define LEDC_TIMER_8_BIT   8
#define LEDC_BASE_FREQ     5000
#define LED_PIN            4



uint8_t bufValue = 0;
uint16_t waitTime = 0;
int waitStartTime = millis();

//標準LED色
ledColor stdLedColor;


/*******************************************
 * リセットフラグがHIGHになっていたらresetSequence実行
 */
void checkResetFlag(){
  if(wrc021.u8Map(MU8_TRIG2) & 0x02){
    //リセットフラグがHIGH
    resetSequence();
    wrc021.u8Map(MU8_TRIG2, wrc021.u8Map(MU8_TRIG2) & 0xfd);
  }
}

/*******************************************
 * スタートボタンが押された際に現在の状態をリセットして起動時の状態に戻す
 */
void resetSequence(){
  setO_EN(OFF_OFF);
  pen_status = 0x02;
  wrc021.sendWriteMap();
  delay(5);
  wrc021.u8Map(MU8_TRIG, wrc021.u8Map(MU8_TRIG) | 0x80);
  wrc021.u8Map(MU8_TRIG, wrc021.u8Map(MU8_TRIG) | 0x20);
  resetOdom();
  delay(5);
  wrc021.sendWriteMap();
  delay(10);
  wrc021.readAll();
  delay(10);
  wrc021.u8Map(MU8_TRIG, wrc021.u8Map(MU8_TRIG) | 0x10);
  delay(10);
  wrc021.s32Map(MS32_A_POS0, 0);
  wrc021.s32Map(MS32_A_POS1, 0);
  wrc021.u8Map(MU8_TRIG, wrc021.u8Map(MU8_TRIG) | 0x0c);
  wrc021.u16Map(MU16_A_PCTR, 0);
  wrc021.sendWriteMap();
  delay(10);
  wrc021.readAll();
}




/*******************************************
 * メモリマップを確認し、ペンのアップダウンを行う
 */
void checkPenUpDown(){
  static uint8_t pen_command;
  //メモリマップの指示値と現在のステータスが異なる場合
  if(wrc021.u8Map(MU8_PEN_SW) != pen_status){
    if(pen_status == 0x01){
      pen_command = 0x00;
    }else if(pen_status == 0x00){
      pen_command = 0x01;
    }
    if(pen_command == 0x01){  //指示値がアップなら
      penUp();
    }else{  //指示値がダウンなら
      penDawn();
    }
  }
}



//走行中なら1、走行してなければ0を返す
uint8_t isRoverRunning(){
  if(wrc021.u8Map(MU8_P_NOW) == 0x00 && wrc021.u8Map(MU8_P_STTS) == 0x00 && !(wrc021.u8Map(MU8_TRIG) & 0x10) && !(wrc021.u8Map(MU8_TRIG) & 0x40) && !(wrc021.u8Map(MU8_TRIG2) & 0x01)){
    return 0;
  }else{
    return 1;
  }
}

//userProgram内の文字を16進数に変換
int8_t c2ToHex(int i){
  if(isHexadecimalDigit(userProgram[wrc021.u16Map(MU16_A_PCTR)][i]) && isHexadecimalDigit(userProgram[wrc021.u16Map(MU16_A_PCTR)][i+1])){
    bufValue = ((cToHex(userProgram[wrc021.u16Map(MU16_A_PCTR)][i]) << 4) | (cToHex(userProgram[wrc021.u16Map(MU16_A_PCTR)][i+1])));
    return 1;
  }else{
    return 0;
  }        
}
  




/*******************************************
 * タイマー割り込み設定
 */
hw_timer_t *interruptTimer = NULL;
bool isInterrupt = false;

volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer(){
  portENTER_CRITICAL_ISR(&timerMux);



  isInterrupt = true;

  portEXIT_CRITICAL_ISR(&timerMux);
  xSemaphoreGiveFromISR(timerSemaphore, NULL);



}

void setupInterruptTimer(){
  //セマフォを設定
  timerSemaphore = xSemaphoreCreateBinary();

  //4つ中1つ目のタイマーを使用する（カウントは0から）
  //dividerを80に設定
  interruptTimer = timerBegin(0, 80, true);

  //割り込みで発砲する関数にonTimer()を設定
  timerAttachInterrupt(interruptTimer, &onTimer, true);

  //割り込み周期を設定(単位はmicroSec)
  //繰り返し割り込みを許可（第３引数）
  timerAlarmWrite(interruptTimer, 10000, true);

  //割り込み判定開始
  timerAlarmEnable(interruptTimer);

}


/***************************************
 * Syncタスク
 */
//uint32_t time1 = micros();
TaskHandle_t syncTaskHandle;
void sync(void *pvParameters){
  
  BaseType_t xStatus;
  xSemaphoreGive(xMutexHandle);

  //セマフォを初期化
  //xSemaphoreGive(xMutexI2C);
  //xSemaphoreGive(xMutexSerial);
  Serial.println( "SyncTask start." );
  

  while(1){
    xStatus = xSemaphoreTake(xMutexHandle, xTicksToWait);

    if(xStatus == pdTRUE){

    //Serial.println( "SyncTask start." );
    //スタートスイッチが押されていたら
    static uint32_t pushSwitchTime = millis();
    static uint32_t upSwitchTime   = millis();
    
    if(wrc021.u8Map(0x7f) == 0x01){

      //チャタリング対策(前回離されてから300ms以内なら押しっぱなしと判定)
      //Serial.println("ON Switch");
      if(millis() - upSwitchTime > 300){

        int blinkCount = 0;
        for(blinkCount = 0; blinkCount < 5; blinkCount++){
          delay(20);
          setLedColor(blackTop);
          wrc033.sendWriteMap();
          delay(20);
          setLedColor(orangeTop);
          wrc033.sendWriteMap();
        }

        pushSwitchTime = millis();

        //Serial.println("RESET pushSwitchTime");

      }

      //3秒以上長押しで電源OFF
      while(wrc021.u8Map(0x7f) == 0x01){
        wrc021.readMemmap(0x7f, 1);
        if(millis() - pushSwitchTime >= 3000 && !((millis() - pushSwitchTime)%5)){
          changeLed2TurnOff();
        }
        //delay(1);
      }

      upSwitchTime = millis();
      //Serial.println("Remove");
    
    }
    

    if(isInterrupt){
      isInterrupt = false;

      wrc021.readMemmap(0x7f, 1);

      resetOdom();

      checkPenUpDown();

      memCom2V();
      posControl();

      //ESP32のメモリマップのうち、書込み要求のあったものをSTM32のメモリマップへ送信する
      wrc021.sendWriteMap();                //メモリマップの送信を実行する  

      //ESP32のLED用メモリマップのうち、書込み要求のあったものをWRC033に送信する
      wrc033.sendWriteMap();

    }
    }

    xSemaphoreGive(xMutexHandle);
    vTaskDelay(portTICK_RATE_MS*2);
    

  }
  
}

//syncのセットアップ
void setUpSync(){
  
  xMutexHandle = xSemaphoreCreateMutex();

  if(xMutexHandle != NULL){
    Serial.println("Make Multi Task!");

    xTaskCreatePinnedToCore(sync, "sync", 8192, NULL, 1, &syncTaskHandle, 1);

    

  }else {
    while(1){
      Serial.println("rtos mutex create error, stopped");
      delay(1000);
    }
  }

}



/***************************************
 * ユーザーコマンド
 */

//走行速度の設定[mm/s]
void wheelRun(int32_t spdL, int32_t spdR){
  double msSpdL = -1.0*spdL;
  double msSpdR = spdR;

  double roverV = (msSpdR + msSpdL)/2.0;
  double roverO = (msSpdR - msSpdL)/(2.0*ROVER_D);

  //Serial.println(roverV);

  setO_EN(ON_ON);
  wrc021.s16Map(MS16_S_XS, roverV);
  wrc021.s16Map(MS16_S_ZS, roverO);

  return;

}

//エンコーダ値の取得
void readEnc(int32_t *encL, int32_t *encR){
  *encL = -1*wrc021.s32Map(MS32_M_POS0);
  *encR = -1*wrc021.s32Map(MS32_M_POS1);

  return;
}

//エンコーダ値のリセット
void clearEnc(){
  wrc021.u8Map(MU8_TRIG, (wrc021.u8Map(MU8_TRIG) | 0x80));

  return;
}

//ペンアップダウン
void penUpdown(int isUp){
  if(isUp != 0 && isUp != 1){
    return;
  }

  wrc021.u8Map(MU8_PEN_SW, isUp);

  return;  
}

void penUpDown(int isUp){
  if(isUp != 0 && isUp != 1){
    return;
  }

  penUpdown(isUp);  

  while(pen_status != isUp){
    delay(1);
  }

  return;
}

//LEDを指定の明るさにセットする
void ledSet(uint8_t ch, uint8_t r, uint8_t g, uint8_t b){
  if(ch < 0 || ch > 8){
    return;
  }

  ledColor led;
  led.number = ch;
  led.r = r;
  led.g = g;
  led.b = b;

  setLedColor(led);

  return;
}
