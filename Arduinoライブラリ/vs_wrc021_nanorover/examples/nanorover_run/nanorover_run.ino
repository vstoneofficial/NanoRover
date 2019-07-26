/**
 * スケッチ名：nanorover_run
 * バージョン：1.00
 * 概要：
 * 　ナノローバーを単独で走行させるサンプルプログラムです。
 *   2秒前進->1秒停止->2秒後進->1秒停止　を繰り返します。
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

  wheelRun(-20,20);    //20mm/sで前進
  delay(2000);        //2秒間待機

  wheelRun(0,0);      //両輪停止
  delay(1000);        //1秒待機

  wheelRun(20,-20);  //10mm/sで後進
  delay(2000);        //2秒間待機

  wheelRun(0,0);      //両輪停止
  delay(1000);        //1秒待機        

}




