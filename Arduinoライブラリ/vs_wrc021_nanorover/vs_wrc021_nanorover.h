/**
 * VS-WRC021用ライブラリの各ファイルをまとめてincludeするためのヘッダ
 * およびユーティリティ的な関数集
 */
#ifndef WRC021_H
#define WRC021_H


#include "vs_wrc021_memmap.h"
#include "vs_wrc021_i2c.h"
#include "vs_wrc021_motor.h"
#include "vs_wrc021_serial.h"
#include "vs_wrc021_spi.h"
#include "vs_wrc021_wifi.h"
#include "vs_wrc021_ble.h"
#include "vs_wrc021_btc.h"
#include "vs_wrc031_spiffs.h"
#include "vs_wrc031_led.h"
#include <vector>
#include <string>



enum wrc021StatusCode{
    NO_INPUT     = 0,          //入力無し or 無効な入力
    HTTP_ACCES   = 1,          //1~2はHTTPアクセスによる有効なメモリマップ操作命令
    HTTP_PAD     = 3,          //HTTPコントローラからのボタン入力
    HTTP_SYSTEM  = WIFI_VIN,   //HTMLコントローラに電圧を表示するためのアクセス =4
    HTTP_BAD     = BAD_REQUEST, //HTTPアクセスによるbad request =5    
    SERIAL_ACCES = 6,          //有線シリアル経由による有効なメモリマップ操作命令
    BLE_ACCES    = 7,
    BTC_ACCES    = 8,
    ROS_CTRL     = 9,
    PAD_INPUT    = 10

};

extern uint16_t waitTime;
extern int waitStartTime;

extern ledColor stdLedColor;    //標準LED色
extern bool isInterrupt;        //タイマー割り込み発砲フラグ

void checkResetFlag();
void resetSequence();
void checkPenUpDown();

uint8_t isRoverRunning();
int8_t c2ToHex(int i);

void IRAM_ATTR onTimer();
void setupInterruptTimer();

void setUpSync();
void sync(void *pvParameters);

void wheelRun(int32_t spdL, int32_t spdR);
void readEnc(int32_t *encL, int32_t *encR);
void clearEnc();
void penUpdown(int isUp);
void penUpDown(int isUp);
void ledSet(uint8_t ch, uint8_t r, uint8_t g, uint8_t b);


#endif /* WRC021 */
