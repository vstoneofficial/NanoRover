#include "vs_wrc021_serial.h"
#include "vs_wrc021_memmap.h"
#include <iostream>
#include <vector>
#include <algorithm>    
#include <iterator>  


//シリアル通信初期化
void serialInit(){
  Serial.begin(115200);
  return;
}

//Serialで受信したメッセージを2次バッファにため、コマンドとして成立し次第
//メモリマップread/writeまたはROSに流す
std::string rcvMsgViaSerial(SERIAL_BUF_SIZE, '\0');
std::string rcvMsg4ROS(SERIAL_BUF_SIZE, '\0');
int persMsgViaSerial(){
  int return_val = 0;

  static int16_t rosMsgBytes = 0; //ROSコマンドのbyte数　msg_length+8byte
  char tmp;

  //while(Serial.available()){

  //バッファ空＝新規コマンドなら先頭文字を取得
  if(rcvMsgViaSerial.empty()){
    tmp = Serial.read();
    rcvMsgViaSerial.push_back(tmp);
  }

  //先頭文字r/R/w/Wならメモリマップ操作，ffならROS．それ以外は破棄．
  if(rcvMsgViaSerial[0] == 'r' || rcvMsgViaSerial[0] == 'R' || rcvMsgViaSerial[0] == 'w' || rcvMsgViaSerial[0] == 'W'
    || rcvMsgViaSerial[0] == 'E' ||rcvMsgViaSerial[0] == 'C' || rcvMsgViaSerial[0] == 'S' || rcvMsgViaSerial[0] == 'P'|| rcvMsgViaSerial[0] == 'L'){
    //メモリマップ操作
    while(Serial.available()){
      tmp = Serial.read();

      if(tmp == 'r' || tmp == 'R' || tmp == 'w' || tmp == 'W'
        /*|| tmp == 'E' || tmp == 'C' */|| tmp == 'S' || tmp == 'P' || tmp == 'L'){
        rcvMsgViaSerial.clear();
      }

      if(rcvMsgViaSerial.length() >= SERIAL_BUF_SIZE){
        rcvMsgViaSerial.clear();
      }

      rcvMsgViaSerial.push_back(tmp);

      //改行コード取得でコマンド成立
      if(tmp == '\n'){
        //Serial.print(rcvMsgViaSerial.c_str());
        return_val += checkI2cAddrOfMsg(rcvMsgViaSerial.c_str(), rcvMsgViaSerial.length()-1);
        
        rcvMsgViaSerial.clear();
        break;
      }
    }

    

  }else if(rcvMsgViaSerial[0] == 0xff){
    //ROSコマンド

    //rosMsgBytes == 0なら、4byte目までを取得して全体のbyte数を算出
    while(rosMsgBytes==0){
      if(!Serial.available()){
        return return_val;
      }

      tmp = Serial.read();
      rcvMsgViaSerial.push_back(tmp);

      if(rcvMsgViaSerial.length() >= 4){
        rosMsgBytes = (int)rcvMsgViaSerial[2];
        rosMsgBytes += (int)rcvMsgViaSerial[3] << 8;
        rosMsgBytes += 8;

        if(rosMsgBytes > SERIAL_BUF_SIZE){
          rosMsgBytes = SERIAL_BUF_SIZE;
        }

        Serial.println(rosMsgBytes);
      }
    }

    //bytes取得済みなら、指定のメッセージ長になるまでread
    while(rcvMsgViaSerial.length() < rosMsgBytes){
      if(!Serial.available()){
        return return_val;
      }

      tmp = Serial.read();
      rcvMsgViaSerial.push_back(tmp);

    }

    //3次バッファが空ならコピーしてクリア
    if(rcvMsg4ROS.empty()){
      std::reverse_copy(rcvMsgViaSerial.begin(), rcvMsgViaSerial.end(), std::back_inserter(rcvMsg4ROS));
      rcvMsgViaSerial.clear();
      rosMsgBytes = 0;
    }

    return return_val;
    

  }else{
    rcvMsgViaSerial.clear();
  }


  //}

  return return_val;

}

//下方置換
int readMsgViaSerial(){
  return persMsgViaSerial();
}

//ROS用3次バッファのreadコマンド
int readMsg4ROS(){

  //rcvMsg4ROSが空なら、-1を返す
  if(rcvMsg4ROS.empty()){
    return -1;
  }

  int return_val = rcvMsg4ROS[rcvMsg4ROS.length()-1];

  rcvMsg4ROS.pop_back();

  return return_val;


}


