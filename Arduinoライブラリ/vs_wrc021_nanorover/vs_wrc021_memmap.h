/**
 * ESP32で使用するメモリマップに関する定義
 * 
 */
#ifndef WRC021_MEMMAP_H
#define WRC021_MEMMAP_H

#include "vs_wrc021_ble.h"
#include "vs_wrc021_btc.h"
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "vs_wrc031_spiffs.h"
//#include "vs_wrc031_led.h"

//メモリマップ 
#define MAP_SIZE 0x100

//メモリマップ読み書きマクロ
#define R_MU8(x)  (*(uint8_t  *)(&wrc021.memMap[x]))
#define R_MU16(x) (*(uint16_t *)(&wrc021.memMap[x]))
#define R_MU32(x) (*(uint32_t *)(&wrc021.memMap[x]))

#define R_MS8(x)  (*(int8_t   *)(&wrc021.memMap[x]))
#define R_MS16(x) (*(int16_t  *)(&wrc021.memMap[x]))
#define R_MS32(x) (*(int32_t  *)(&wrc021.memMap[x]))


extern SemaphoreHandle_t xMutexHandle;
extern const TickType_t  xTicksToWait; //セマフォ取得待機時間1ms


//SerialやBluetooth,Wi-Fiで受信した命令を格納する構造体
struct cmd{
  uint8_t cmdType;   //(読込)r, （書込）w
  uint8_t addr;     //読み書きするメモリマップの先頭アドレス
  uint8_t *value;   //書き込む値
  uint8_t valueCount;      //書き込む値の長さ
  uint8_t readLength;   //読みこみで先頭アドレスから何バイト読み込むか 0x00~0xff
};

enum memmapAddr{
  MU16_SYSNAME  = 0x00,  /* R  システム名     */
  MU16_FIRMREV  = 0x02,  /* R  ファームウェアリビジョン  */
  MU32_TRIPTIME = 0x04,  /* R  経過時間(ms)  */
  MU8_MODE      = 0x0d,  /* R	b3-0:起動時に認識したDIP-SW */
  MU16_POWOFF_T = 0x0e,  /* R/W	 msec後に電源が切れる */
  MU8_O_EN      = 0x10,  /* R/W  b0: 1でCH0出力イネーブル  */
/**/                     /* R/W  b1: 1でCH1出力イネーブル  */
/**/                     /* R/W  b2: 1でCH2出力イネーブル  */
  MU8_TRIG      = 0x11,  /* R/W  b0: 1でA_POS0をT_POS0に加算*/
/**/                     /* R/W  b1: 1でA_POS1をT_POS1に加算*/
/**/                     /* R/W  b2: 1でM_POS0とT_POS0をクリア */
/**/                     /* R/W  b3: 1でM_POS1とT_POS1をクリア */
/**/                     /* R/W  b4: 1で位置制御の目標waypointを更新 */
/**/                     /* R/W  b5: 1で位置制御のwaypointをクリア */
/**/                     /* R/W  b6: 1で回転数直接指定制御の指令値更新 */
/**/                     /* R/W  b7: 1でWP_PX-WP_THとM_POS,T_POSをクリア */                    
  MU16_SD_VI    = 0x12,  /* R/W  シャットダウン電圧*/
  MU16_OD_DI    = 0x14,  /* R/W  対応するM_DIが1ならMU8_O_EN=0になる*/
  MU16_SPD_T0   = 0x16,  /* R/W  速度を計算するための基準時間(ms)*/
  MU16_MOVE_T0  = 0x18,  /* R/W  加算時間*/
  MS16_FB_PG0   = 0x20,  /* R/W  比例ゲイン */
  MS16_FB_PG1   = 0x22,  /* R/W  比例ゲイン */
  MU16_FB_ALIM0 = 0x24,  /* R/W  加速度上限 */
  MU16_FB_ALIM1 = 0x26,  /* R/W  加速度上限 */
  MU16_FB_DLIM0 = 0x28,  /* R/W  減速度上限 */
  MU16_FB_DLIM1 = 0x2a,  /* R/W  減速度上限 */
  MU16_FB_OLIM0 = 0x2c,  /* R/W  出力上限  100%が0x1000*/
  MU16_FB_OLIM1 = 0x2e,  /* R/W  出力上限  100%が0x1000*/
  MU16_FB_PCH0  = 0x30,  /* R/W	 パンチ		 100%が0x1000*/
  MU16_FB_PCH1  = 0x32,  /* R/W	 パンチ		 100%が0x1000*/
  MS32_T_POS0   = 0x40,  /* R/W  目標エンコーダ値  */
  MS32_T_POS1   = 0x44,  /* R/W  目標エンコーダ値  */
  MS32_A_POS0   = 0x48,  /* R/W  T_POSへの加算値  */
  MS32_A_POS1   = 0x4c,  /* R/W  T_POSへの加算値  */
  MS16_T_OUT0   = 0x50,  /* R/W  出力オフセット(CH0) */
  MS16_T_OUT1   = 0x52,  /* R/W  出力オフセット(CH1) */
  MS16_T_OUT2   = 0x54,  /* R/N  出力オフセット(CH2) */
  MS32_M_POS0   = 0x60,  /* R  測定されたエンコーダ値 */
  MS32_M_POS1   = 0x64,  /* R  測定されたエンコーダ値 */
  MS16_M_SPD0   = 0x68,  /* R  測定された速度 M_POS0の差分*/
  MS16_M_SPD1   = 0x6a,  /* R  測定された速度 M_POS1の差分*/
  MS16_M_OUT0   = 0x6c,  /* R  計算されたモータ出力値 100%が0x1000*/
  MS16_M_OUT1   = 0x6e,  /* R  計算されたモータ出力値 100%が0x1000*/
  MU16_M_DI     = 0x7e,  /* R  b0-7:CN5, b8:PWR-BTN  */
  MS32_WP_PX    = 0x80,  /* R  ナノローバーのワールド基準のX座標[μm] */
  MS32_WP_PY    = 0x84,  /* R  ナノローバーのワールド基準のY座標[μm] */
  MS16_WP_TH    = 0x88,  /* R  ナノローバーのワールド基準のZ軸姿勢[mrad] */
  MU16_M_VI     = 0x90,  /* R  入力電圧 3.3V*9=29.7Vが0x0fff(VS-WRC021の場合) */
  MS32_P_DIS    = 0xa0,  /* R/W  位置制御：移動距離[μm] */
  MS16_P_RAD    = 0xa4,  /* R/W  位置制御：旋回量[mrad]   */
  MS16_P_ACC    = 0xa6,  /* R/W  位置制御：加速度[mm/s^2]   */
  MS16_P_SPD    = 0xa8,  /* R/W  位置制御：移動速度[mm/s] */
  MU8_P_STTS    = 0xaa,  /* R  0x01で位置制御中、0x00でwaypoint到達 */
  MS16_S_XS     = 0xac,  /* R/W  速度制御：移動速度[mm/s] */
  MS16_S_ZS     = 0xae,  /* R/W  速度制御：旋回速度[mrad/s] */
  MS32_P_PX0    = 0xb0,  /* R/N  位置制御：waypoint0のX座標 */
  MS32_P_PX1    = 0xb4,  /* R/N  位置制御：waypoint1のX座標 */
  MS32_P_PX2    = 0xb8,  /* R/N  位置制御：waypoint2のX座標 */
  MS32_P_PX3    = 0xbc,  /* R/N  位置制御：waypoint3のX座標 */
  MS32_P_PY0    = 0xc0,  /* R/N  位置制御：waypoint0のY座標 */
  MS32_P_PY1    = 0xc4,  /* R/N  位置制御：waypoint1のY座標 */
  MS32_P_PY2    = 0xc8,  /* R/N  位置制御：waypoint2のY座標 */
  MS32_P_PY3    = 0xcc,  /* R/N  位置制御：waypoint3のY座標 */
  MS16_P_TH0    = 0xd0,  /* R/N  位置制御：waypoint0の姿勢 */
  MS16_P_TH1    = 0xd2,  /* R/N  位置制御：waypoint1の姿勢 */
  MS16_P_TH2    = 0xd4,  /* R/N  位置制御：waypoint2の姿勢 */
  MS16_P_TH3    = 0xd6,  /* R/N  位置制御：waypoint3の姿勢 */
  MS32_P_PXIN   = 0xd8,  /* R/N  位置制御：waypoint追加スロットX座標 */
  MS32_P_PYIN   = 0xdc,  /* R/N  位置制御：waypoint追加スロットY座標 */
  MS16_P_THIN   = 0xe0,  /* R/N  位置制御：waypoint追加スロット姿勢 */
  MU8_P_TOP     = 0xe2,  /* R  waypointの先頭番号 */
  MU8_P_BTM     = 0xe3,  /* R  waypointの末尾番号 */
  MU8_P_NOW     = 0xe4,  /* R  残りのwaypoint数 0以上なら中継点追従制御中  */
  MU8_A_EN      = 0xf0,  /* R/W  b0: 1でスタンドアローンプログラムの実行 */
  /**/                   /* R    b1: 1で中継点ベース制御 */
  /**/                   /* R/W  b2: 1で超信地旋回、直線移動制御 */
  /**/                   /* R    b3: 1で回転数直接指定制御 */
  MU8_PEN_SW    = 0xf1,  /* R/W  b0: 1でペンアップ、0でペンダウン */
  MU16_A_PCTR   = 0xf2,  /* R  プログラムカウンター */
  MU8_TRIG2     = 0xf4   /* R/W  b0: 1でwaypoint追加スロットからスタックに積む */
                         /* R/W  b1: 1で走行に関わるメモリマップの項をリセット */

  
  
  
};

//メモリマップ初期値
//0x10から0x8fまでのメモリマップ初期化用
extern const uint8_t initialMemmap[MAP_SIZE];



//Wrc021のメモリマップクラス
class Wrc021{
  public:
  Wrc021();
  Wrc021(uint8_t addr);

  int sendWriteMapTime;

  void initMemmap(double cutOffLevel);
  void memMapClean();
  virtual int readMemmap(uint8_t addr, uint8_t readLength);
  int readAll();
  int writeMemmap(uint8_t addr, uint8_t data[], uint8_t writeLength);
  int write4Byte(uint8_t addr, int32_t data);
  int write2Byte(uint8_t addr, int16_t data);
  int write1Byte(uint8_t addr, uint8_t data);
  uint8_t getAddr();

  void checkMsg(String rcvMsg);
  void checkMsg(String rcvMsg, WiFiClient* client);
  void checkMsg(String rcvMsg, uint8_t viaBle);
  void checkMsg(String rcvMsg, WiFiClient* client, uint8_t viaBlt);
  void sendEnc2dev();
  void sendEnc2dev(uint8_t viaBlt);
  void sendEnc2dev(WiFiClient* client);
  void sendEnc2dev(WiFiClient* client, uint8_t viaBlt);
  void clearEnc();
  void setWheelSpeed(int32_t left, int32_t right);
  void setWheelSpeed(String spdMsg);
  void setPenUpDown(String penMsg);
  void setLedColorByMsg(String ledMsg);

  void sendMap2pc(struct cmd rcvCmd);
  void sendMap2pc(struct cmd rcvCmd, WiFiClient* client);
  void sendMap2pc(struct cmd rcvCmd, WiFiClient* client, uint8_t viaBle);
  void setWriteMapViaMsg(struct cmd rcvCmd);
  virtual void sendWriteMap();
  


  int8_t   s8Map(uint8_t addr);
  int8_t   s8Map(uint8_t addr, int8_t data);
  uint8_t  u8Map(uint8_t addr);
  uint8_t  u8Map(uint8_t addr, uint8_t data);
  int16_t  s16Map(uint8_t addr);
  int16_t  s16Map(uint8_t addr, int16_t data);
  uint16_t u16Map(uint8_t addr);
  uint16_t u16Map(uint8_t addr, uint16_t data);
  int32_t  s32Map(uint8_t addr);
  int32_t  s32Map(uint8_t addr, int32_t data);
  uint32_t u32Map(uint8_t addr);
  uint32_t u32Map(uint8_t addr, uint32_t data);

  uint8_t checkWriteFlag(uint8_t addr);

  double getVin();

  protected:
  uint8_t devAddr;
  uint8_t memMap[MAP_SIZE];
  uint8_t writeFlag[MAP_SIZE];
};


class Wrc033: public Wrc021{
  public:
  Wrc033(uint8_t addr){
    devAddr = addr;
  };
  int readMemmap(uint8_t addr, uint8_t readLength);
  void sendWriteMap();

};


extern Wrc021 wrc021;
//extern Wrc021 wrc022;
extern Wrc033 wrc033;

//extern static char hexToA[16];
uint8_t cToHex(uint8_t c);
String int2HexLittleString(int data, uint8_t length);
String int2HexBigString(int data, uint8_t length);


int checkI2cAddrOfMsg(String rcvMsg, int rcvMsgCount);
int checkI2cAddrOfMsg(String rcvMsg, int rcvMsgCount, uint8_t viaBle);

uint8_t setRoverParam(String rcvMsg);
uint8_t getRoverParam(String rcvMsg); 

#endif /* MEMMAP_H */


