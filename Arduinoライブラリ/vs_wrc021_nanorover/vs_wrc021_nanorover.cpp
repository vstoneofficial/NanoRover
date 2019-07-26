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
  resetMoveTo();
  delay(5);
  wrc021.u8Map(MU8_TRIG, wrc021.u8Map(MU8_TRIG) | 0x80);
  wrc021.u8Map(MU8_TRIG, wrc021.u8Map(MU8_TRIG) | 0x20);
  clearWaypoint();
  resetOdom();
  delay(5);
  wrc021.sendWriteMap();
  delay(10);
  wrc021.readAll();
  delay(10);
  wrc021.u8Map(MU8_TRIG, wrc021.u8Map(MU8_TRIG) | 0x10);
  delay(10);
  moveTo();
  delay(10);
  wrc021.s32Map(MS32_A_POS0, 0);
  wrc021.s32Map(MS32_A_POS1, 0);
  //wrc021.s32Map(MS32_T_POS0, 0);
  //wrc021.s32Map(MS32_T_POS1, 0);
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


//進行方向に合わせてLEDを光らせる
//アイドル状態なら、白を明滅させる
void setLedByUserProg(){

  //stdLedColor.number != 0xff and != 0x00なら機能使用しない
  if(stdLedColor.number != 0xff && stdLedColor.number != 0x00){
    return;
  }

  static int runCounter = 0;
  if(runCounter < 0){
    runCounter++;
    return;
  }else{
    runCounter = 0;
  }

  double power = 0.0;
  static int idlingPower = 128;
  int i;
  ledColor led;
  
  //ユーザープログラム実行中でないかつ、stdLedColor.number == 0x00ならアイドル中
  if(!(wrc021.u8Map(MU8_A_EN) & 0x01) && stdLedColor.number == 0x00){
    if(idlingPower < 192){
      idlingPower += 1;
    }else{
      idlingPower = 64;
    }

    if(idlingPower < 128){
      power = idlingPower;
    }else{
      power = 255.0 - idlingPower;
    }

    power /= 128.0;

    led.r = (uint8_t)(64.0*power*power*power*power);
    led.g = (uint8_t)(64.0*power*power*power*power);
    led.b = (uint8_t)(64.0*power*power*power*power);

    for(i = 1; i < 9; i++){
      led.number = i;
      setLedColor(led);
    }

    led.r = (uint8_t)((double)0xff*power*power*power*power);
    led.g = (uint8_t)((double)0xff*power*power*power*power);
    led.b = (uint8_t)((double)0xff*power*power*power*power);
    led.number = 0;

    setLedColor(led);

    return;
  }
  idlingPower = 0;

  //位置制御中でなければリターン
  if(wrc021.u8Map(MU8_P_STTS) == 0x00){
    return;
  }

  //stdLedColor.number != 0xffなら
  if(stdLedColor.number != 0xff){
    return;
  }

  //現在の進行方向を判定
  double d_speed = e_run_speed[M_L] + e_run_speed[M_R];
  const double threshold = 10.0;
  uint8_t roverDirection = 0; //0->stop / 1->forward / 2->left / 3->right / 4->back
  static uint8_t oldRoverDirection = 0;
  if(e_run_speed[M_L] > 0.0 && e_run_speed[M_R] < 0.0){
    if(d_speed < -threshold){
      //左旋回
      roverDirection = 2;
    }else if(d_speed > threshold){
      //右旋回
      roverDirection = 3;
    }else{
      //前進
      roverDirection = 1;
    }
  }else if(e_run_speed[M_L] < 0.0 && e_run_speed[M_R] < 0.0){
    //左旋回（r < d）
    roverDirection = 2;
  }else if(e_run_speed[M_L] > 0.0 && e_run_speed[M_R] > 0.0){
    //右旋回（r < d）
    roverDirection = 3;
  }else if(e_run_speed[M_L] < 0.0 && e_run_speed[M_R] > 0.0){
    if(d_speed < -threshold){
      //右旋回
      roverDirection = 3;
    }else if(d_speed > threshold){
      //左旋回
      roverDirection = 2;
    }else{
      //後進
      roverDirection = 4;
    }
  }

  //roverDirectionに合わせてLEDを順に標準色で点灯する
  //LED[1-4] : 右列　LED[5-8] ： 左列
  //
  static int LPower = 0;
  static int RPower = 0;

  if(roverDirection != oldRoverDirection){
    if(roverDirection == 1){
      RPower = 0;
      LPower = 0;
    }else if(roverDirection == 2){
      RPower = 0;
      LPower = 0;
    }else if(roverDirection == 3){
      RPower = 0;
      LPower = 0;
    }else if(roverDirection == 4){
      RPower = 512;
      LPower = 512;
    }
  }
  oldRoverDirection = roverDirection;

  if(roverDirection == 1){
    if(RPower >= 512){
      RPower = 0;
    }
    if(LPower >= 512){
      LPower = 0;
    }
    RPower += 4;
    LPower += 4;
  }else if(roverDirection == 2){
    if(LPower < 512){
      LPower += 4;
    }else{
      RPower = 0;
      LPower = 0;
    }
    
  }else if(roverDirection == 3){
    if(RPower < 512){
      RPower += 4;
    }else{
      RPower = 0;
      LPower = 0;
    }
  }else if(roverDirection == 4){
    if(RPower <= 0){
      RPower = 512;
    }
    if(LPower <= 0){
      LPower = 512;
    }
    RPower -= 4;
    LPower -= 4;
  }

  //右列制御
  
  
  for(i = 4; i > 0; i--){
    power = RPower - (4-i)*128;
    if(power < 0.0){
      power = 0.0;
    }else if(power > 128.0){
      power = 128.0;
    }
    power /= 128.0;

    if(roverDirection == 4){
      power = 1.0 - power;
    }

    led.number = i;
    led.r = (uint8_t)((double)stdLedColor.r*power*power*power*power);
    led.g = (uint8_t)((double)stdLedColor.g*power*power*power*power);
    led.b = (uint8_t)((double)stdLedColor.b*power*power*power*power);

    
    

    setLedColor(led);
  }
  

  //左列制御

  
  for(i = 5; i < 9; i++){
    power = LPower - (i-5)*128;
    if(power < 0.0){
      power = 0.0;
    }else if(power > 128.0){
      power = 128.0;
    }
    power /= 128.0;

    if(roverDirection == 4){
      power = 1.0 - power;
    }

    led.number = i;
    led.r = (uint8_t)((double)stdLedColor.r*power*power*power*power);
    led.g = (uint8_t)((double)stdLedColor.g*power*power*power*power);
    led.b = (uint8_t)((double)stdLedColor.b*power*power*power*power);

    setLedColor(led);
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
    wrc021.readMemmap(0x7f, 1);
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
