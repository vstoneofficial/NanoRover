#include "vs_wrc021_memmap.h"
#include "vs_wrc021_btc.h"
#include "BluetoothSerial.h"

namespace btc {

  BluetoothSerial SerialBT;

  void btcInit(){
    btcInit("Nanorover BTC");
  }

  void btcInit(char name[]){
    SerialBT.begin(name); //Bluetooth device name
    Serial.println("The device started, now you can pair it with bluetooth!");
  }

  int readMsg(){
    int rcvMsgCount = 0;
    if(SerialBT.available()){
      //rcvMsgに改行コードまでのメッセージを取得
      char tmp;
      static String rcvMsg;

      
      while(SerialBT.available()){
        tmp = SerialBT.read();
        rcvMsg.concat(String(tmp));
      
        if(tmp == '\n'){
          //Serial.print(rcvMsg.c_str());
          checkI2cAddrOfMsg( rcvMsg, rcvMsg.length()-1, VIA_BTC);
          rcvMsg = "";
        }

        if(rcvMsg.length() > 256){
          rcvMsg = "";
          rcvMsgCount = 0;
        }
        rcvMsgCount++;
      }

    }

    return rcvMsgCount;
  }

  void sendMsg(const char msg[]){
    SerialBT.println(msg);
  }
}
