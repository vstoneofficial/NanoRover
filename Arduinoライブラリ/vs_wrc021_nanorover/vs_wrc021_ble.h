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

#ifndef WRC021_BLE_H
#define WRC021_BLE_H

#include "vs_wrc021_memmap.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>


#define VIA_BLE 1

// UUIDの生成については以下のURLを参照してください:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" 
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"



namespace ble {

  extern BLEServer *megarover_server;
  extern BLECharacteristic *megarover_tx_characteristic;

  extern bool device_connected;
  extern bool old_device_connected;
  extern int  rx_count;
  extern char rcv_msg[];
  extern bool get_msg;


  class MegaroverBleServerCallbacks: public BLEServerCallbacks {
    private:
    void onConnect(BLEServer* pServer);
    void onDisconnect(BLEServer* pServer);
  };

  class MegaroverBleCallbacks: public BLECharacteristicCallbacks {
    private:
    void onWrite(BLECharacteristic *pCharacteristic);


  };


  void bleInit();
  void bleInit(char name[]);
  int sendMultiByte(char msg[], int length );

}


#endif
