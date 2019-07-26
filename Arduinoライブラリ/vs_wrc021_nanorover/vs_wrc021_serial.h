/**
 * PCとのシリアル通信に関連するライブラリ
 * 
 * [通信コマンド]
 * ・メモリマップへの書込み
 * w[][アドレスH][アドレスL][][データ1][データ2][データ3][データ4][改行]
 * 
 * ・メモリマップからの読み込み
 * r[][アドレスH][アドレスL][][バイト数H][バイト数L][改行]
 * 
 */
#ifndef WRC021_SERIAL_H
#define WRC021_SERIAL_H

#include "vs_wrc021_memmap.h"
#include <iostream>
#include <vector>
#include <algorithm>    
#include <iterator>  

#define SERIAL_BUF_SIZE 512

extern std::string rcvMsgViaSerial;
extern std::string rcvMsg4ROS;

void serialInit();
int persMsgViaSerial();
int readMsgViaSerial();
int readMsg4ROS();

#endif /* SERIAL_H */
