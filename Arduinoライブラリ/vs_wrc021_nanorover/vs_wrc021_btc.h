/**
 * BLEによるUART通信に関連するライブラリ
 * 
 * [通信コマンド]
 * ・メモリマップへの書込み
 * w[][アドレスH][アドレスL][][データ1][データ2][データ3][データ4][改行]
 * 
 * ・メモリマップからの読み込み
 * r[][アドレスH][アドレスL][][バイト数H][バイト数L][改行]
 * 
 */

#ifndef WRC021_BTC_H
#define WRC021_BTC_H

#include "vs_wrc021_memmap.h"
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define VIA_BTC 2

namespace btc {
   void btcInit();
   void btcInit(char name[]);

   int readMsg();
   void sendMsg(const char msg[]);
}

#endif
