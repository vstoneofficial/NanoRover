/**
 * スケッチ名：nanorover_encoder
 * バージョン：1.00
 * 概要：
 * 　ナノローバーを5cm走行させ、停止させるサンプルプログラムです。
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

  //USBシリアルセットアップ
  Serial.begin(115200);

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

  int initialEncL = 0;   //左車輪のエンコーダ値
  int initialEncR = 0;   //右車輪のエンコーダ値

  readEnc(&initialEncL, &initialEncR);  //エンコーダ値の取得

  //Serial.println(initialEncL);

  wheelRun(-20, 20);      //20mm/sで前進

  int encL = 0;
  int encR = 0;
  while(1){
    readEnc(&encL, &encR);  //エンコーダ値の取得

    encL -= initialEncL;
    encR -= initialEncR;

    Serial.println(encR/ENC_PER_MM);

    if(encL/ENC_PER_MM < -50.0 || encR/ENC_PER_MM > 50.0){  //どちらかの車輪が50mm以上進んだ場合はwhileから出る
      break;                                                //ENC_PER_MMは標準状態での1mmあたりのエンコーダ値
    }

    delay(10);
  }

  wheelRun(0, 0);   //5秒間停止
  delay(5000);
  

}
