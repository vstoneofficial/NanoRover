/**
 * スケッチ名：nanorover_LED
 * バージョン：1.00
 * 概要：
 * 　ナノローバーのLED色を変化させるサンプルプログラムです。
 * 　
 * 　
 */
#include <vs_wrc021_nanorover.h>
#include <Arduino.h>


/*******************************************
 * セットアップ
 */
void setup()
{

  //モータ制御関連パラメータの設定(適切な値を設定してください)
  std_motor_param[MAX_SPEED] = 0.1;     //最大速度[m/s]
  std_motor_param[MAX_RAD]   = 2.51;    //最大旋回速度[rad/s]
  std_motor_param[K_P]       = 1.4;     //Pゲイン
  std_motor_param[K_I]       = 0.2;     //Iゲイン
  std_motor_param[K_D]       = 1.2;     //Dゲイン

  //標準LED色の設定
  stdLedColor.number = 0x00;
  stdLedColor.r      = 23;
  stdLedColor.g      = 23;
  stdLedColor.b      = 23;  

  //I2Cセットアップ
  i2cMasterInit();

  //メモリマップ初期化
  wrc021.initMemmap(8.0);

  //LEDセットアップ
  wrc033LedInit();
  delay(100);

  //Syncタスクセットアップ
  setupInterruptTimer(); 
  setUpSync();
  delay(100);

  //エンコーダの初期化
  clearEnc();
  delay(50);

}


/*******************************************
 * メインループ
 */
void loop(){

  int i;
  for( i = 0; i < 9; i++){
    ledSet(i, 0, 0, 0);     //全てのLEDを消灯する
  }

  delay(1000);              //1秒待機

  ledSet(1, 255, 0, 0);     //LED1の色設定
  delay(500);               //500ミリ秒待機

  ledSet(2, 255, 191, 0);   //LED2の色設定
  delay(500);

  ledSet(3, 127, 255, 0);   //LED3の色設定
  delay(500);

  ledSet(4, 0, 255, 63);    //LED4の色設定
  delay(500);

  ledSet(5, 0, 255, 255);   //LED5の色設定
  delay(500);

  ledSet(6, 0, 63, 255);    //LED6の色設定
  delay(500);

  ledSet(7, 127, 0, 255);   //LED7の色設定
  delay(500);

  ledSet(8, 255, 0, 191);   //LED8の色設定
  delay(500);

  //LED0(TOP)を虹色に変化させながら点灯する
  int r = 240;
  int g = 0;
  int b = 0;
  int deg = 0;
  while(1){
    if(deg > 0 && deg <= 60 ){              //60度ごとにR,G,Bの各色を明滅させる
      g += 4;
    }else if(deg > 60 && deg <= 120){
      r -= 4;
    }else if(deg > 120 && deg <= 180){
      b += 4;
    }else if(deg > 180 && deg <= 240){
      g -= 4;
    }else if(deg > 240 && deg <= 300){
      r += 4;
    }else if(deg > 300 && deg <= 360){
      b -= 4;
    }

    ledSet(0, r, g, b);   //値を設定

    deg += 1;
    if(deg > 360){
      deg = 0;
    }

    delay(10);            //10ミリ秒待機

  }

  
  

}
