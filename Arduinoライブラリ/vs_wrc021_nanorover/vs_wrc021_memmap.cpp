#include "vs_wrc021_memmap.h"
#include "vs_wrc021_ble.h"
#include "vs_wrc021_btc.h"
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <sstream>
#include "vs_wrc031_spiffs.h"
#include "vs_wrc021_nanorover.h"

#define POS_OF_I2CADDR 1

SemaphoreHandle_t xMutexHandle = NULL;
const TickType_t  xTicksToWait = portTICK_RATE_MS; //セマフォ取得待機時間1ms


/**
 * メモリマップ初期値(0x10~0xff) 
 * シャットダウン電圧 11.0V
 */
//ナノローバー用
const uint8_t initialMemmap[MAP_SIZE] = {0x00, 0x00, 0xed, 0x05, 0xff, 0x01, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0x10, 0x00, 0x10,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};




//16進数をアスキー文字に変換するテーブル
//itoa[a] = 'A'
//itoa[3] = '3'
static char hexToA[] = "0123456789ABCDEF";

//アスキー文字を16進数に変換
// '1' => 0x01, 'a' => 0x0a
uint8_t cToHex(uint8_t c){
  
  // '0'～'9'
  if (isDigit(c)) {
    return (c - 0x30);
  }
  // 'A'～'F'
  if (isUpperCase(c)) {
    return (c - 0x37);
  }
  // 'a'～'f'
  if (isLowerCase(c)) {
    return (c - 0x57);
  }
  return 0;
}

String int2HexLittleString(int data, uint8_t length) {
	//int32_t型の数値をリトルエンディアンの16進文字列に変換
  std::stringstream ss;

	ss << std::hex << data;
	std::string bigMes = ss.str();
	std::string littleMes;

	if (bigMes.length() % 2) {
		bigMes.insert(0, "0");
	}

	int i;
	for (i = bigMes.length() / 2; i < length; i++) {
		bigMes.insert(0, "00");
	}
	

	for (i = bigMes.length() - 2; i >= 0; i -= 2) {
		littleMes.append(bigMes, i, 2);
	}
	//printf("%s\n", littleMes.c_str());

  String str = littleMes.c_str();

	return str;
}

String int2HexBigString(int data, uint8_t length) {
	//int32_t型の数値をビッグエンディアンの16進文字列に変換
  std::stringstream ss;

	ss << std::hex << data;
	std::string bigMes = ss.str();

	if (bigMes.length() % 2) {
		bigMes.insert(0, "0");
	}

	int i;
	for (i = bigMes.length() / 2; i < length; i++) {
		bigMes.insert(0, "00");
	}

  String str = bigMes.c_str();
	
	return str;
}

Wrc021::Wrc021(){}
Wrc021::Wrc021(uint8_t addr){
  devAddr = addr;
}

//STM32のメモリマップとmemMapを初期化
void Wrc021::initMemmap(double cutOffLevel){

  //電源カットオフ電圧をメモリマップ用数値に変換
  uint16_t cutOffHex = (uint16_t)((cutOffLevel/29.7)*0xfff);

  //STM32メモリマップ初期化
  Wrc021::writeMemmap(MU8_O_EN, (uint8_t*)&initialMemmap, 0xF0);

  //memMap初期化
  Wrc021::readAll();

  //シャットダウン電圧の設定
  //Wrc021::write2Byte(MU16_SD_VI, cutOffHex);
  Wrc021::u16Map(MU16_SD_VI, cutOffHex); 

  //メモリマップ書込みタイミング制御のための時間初期化
  sendWriteMapTime = millis();

}

//memMapのクリーン
void Wrc021::memMapClean(){  
  int i = 0;
  for(i = 0; i<MAP_SIZE; i++){
    memMap[i] = 0x00; 
  }

  return;
}

//1-127byteをSTM32のメモリマップから読み込む
int Wrc021::readMemmap(uint8_t addr, uint8_t readLength){

  int i = 0;

  //if(xStatus == pdTRUE){

  unsigned char readAddr = 0x00;
  if(addr < 0x01){
    readAddr = 0xFF;
  }else{
    readAddr = addr - 0x01;
  }

  //読み込むアドレスを指定
  Wire.beginTransmission(devAddr);
  Wire.write(readAddr);
  Wire.endTransmission();
  
  //読み込み
  Wire.requestFrom((int)devAddr, (int)(readLength+1));
  while(!Wire.available()){
  }
  Wire.read();
  while (Wire.available()){
    memMap[i+ addr] = Wire.read();
    i++;
  }

  return i;
}

//STM32の全てのメモリマップを読み込み。
int Wrc021::readAll(){

  readMemmap(0x00,64);
  readMemmap(0x40,64);
  readMemmap(0x80,20);

  return 1;
}

//STM32のメモリマップへ書込み。受け取ったデータをそのまま書き込む
int Wrc021::writeMemmap(uint8_t addr, uint8_t data[], uint8_t writeLength){
  
  int i;

  uint8_t writeByte;

  if(writeLength == 0){
    return -1;
  }

  Wire.beginTransmission(devAddr);
  Wire.write(addr);
  for(i = 0; i < writeLength ; i++){
    writeByte = data[i];
    Wire.write(writeByte);
  }
  Wire.endTransmission();
  
  return i;
}

//STM32のメモリマップへ4byte書込み
int Wrc021::write4Byte(uint8_t addr, int32_t data){

  int i;
  uint8_t writeByte;

  Wire.beginTransmission(devAddr);
  Wire.write(addr);
  for(i = 0; i <= 3; i++){
    writeByte = data >> i * 8;
    Wire.write(writeByte);
  }
  Wire.endTransmission();
  
  return 1;
}
//STM32のメモリマップへ2byte書込み
int Wrc021::write2Byte(uint8_t addr, int16_t data){

  uint8_t upperByte = data >> 8;
  uint8_t lowerByte = data;
  Wire.beginTransmission(devAddr);
  Wire.write(addr);
  Wire.write(lowerByte);
  Wire.write(upperByte);
  Wire.endTransmission();

  return 1;
}

//STM32のメモリマップへ1byte書込み
int Wrc021::write1Byte(uint8_t addr, uint8_t data){

  Wire.beginTransmission(devAddr);
  Wire.write(addr);
  Wire.write(data);
  Wire.endTransmission();

  return 1;
}

//デバイスアドレスの取得
uint8_t Wrc021::getAddr(){
  return devAddr;
}

//シリアルからの接続での呼び出し用
void Wrc021::checkMsg(String rcvMsg){
  checkMsg(rcvMsg, NULL, NULL);
}

//WiFiからの接続での呼び出し用
void Wrc021::checkMsg(String rcvMsg, WiFiClient* client){
  checkMsg(rcvMsg, client, NULL);
}

//BLE/BTCからの接続での呼び出し用
void Wrc021::checkMsg(String rcvMsg, uint8_t viaBlt){
  checkMsg(rcvMsg, NULL, viaBlt);
}

void Wrc021::checkMsg(String rcvMsg, WiFiClient* client, uint8_t viaBlt){
  //改行も含めた全体の文字数が奇数ならフォーマットエラー(先頭文字がN/A/C/L/Tを除く)
  if(rcvMsg.length()%2 && rcvMsg[0] != 'E' && rcvMsg[0] != 'e' && rcvMsg[0] != 'C' && rcvMsg[0] != 'c' && rcvMsg[0] != 'S' && rcvMsg[0] != 's' 
    && rcvMsg[0] != 'P' && rcvMsg[0] != 'p' && rcvMsg[0] != 'L' && rcvMsg[0] != 'l'){
    //Serial.println("ERR: The number of characters is odd");
    return;
  }

  //コマンドの種類判定
  if(rcvMsg[0] == 'r' || rcvMsg[0] == 'R' || rcvMsg[0] == 'w' || rcvMsg[0] == 'W'){
    struct cmd rcvCmd;
    
    rcvCmd.cmdType = rcvMsg[0];

    //spaceチェック
    if(!isSpace(rcvMsg[1])){
      ///Serial.println("ERR: Cmd format");
      return;
    }

    //メモリマップのアドレスチェック
    if(isHexadecimalDigit(rcvMsg[2]) && isHexadecimalDigit(rcvMsg[3])){
      rcvCmd.addr = ((cToHex(rcvMsg[2]) << 4) | (cToHex(rcvMsg[3])));
    }

    //spaceチェック
    if(!isSpace(rcvMsg[4])){
      ///Serial.println("ERR: Cmd format");
      return;
    }

    //読み込みと書込みで分岐
    //読み込み
    if(rcvCmd.cmdType == 'r' || rcvCmd.cmdType == 'R'){
      //コマンド長チェック
      if(rcvMsg.length() == 8){
        //読み込みバイト数チェック
        if(isHexadecimalDigit(rcvMsg[5]) && isHexadecimalDigit(rcvMsg[6])){
          rcvCmd.readLength = ((cToHex(rcvMsg[5]) << 4) | (cToHex(rcvMsg[6])));
          sendMap2pc(rcvCmd, client, viaBlt);
          return;
        }
      }else{
        ///Serial.println("ERR: Read format");
        return;
      }

    //書込み
    }else{
      
      int i = 5;
      int j = 0;
      uint8_t value[MAP_SIZE];

      //受信した2文字ずつ一バイトの16進数に変換
      //（例） '2','f' => 0x2f
      while(rcvMsg[i] != '\n'){
        if(isHexadecimalDigit(rcvMsg[i]) && isHexadecimalDigit(rcvMsg[i+1])){
          value[j] = ((cToHex(rcvMsg[i]) << 4) | (cToHex(rcvMsg[i+1])));
          i += 2;
          j++;
                    
        }else{
          rcvCmd.valueCount = 0;
          ///Serial.println("ERR: Write format");
          return;
        }
      }
      rcvCmd.valueCount = j;
      rcvCmd.value = value;

      setWriteMapViaMsg(rcvCmd);
      return;
    }
  
  //【E】エンコーダ読み取り
  }else if(rcvMsg[0] == 'E'/* || rcvMsg[0] == 'e'*/){
    sendEnc2dev(client, viaBlt);
  
  //【C】エンコーダクリア
  }else if(rcvMsg[0] == 'C'){
    clearEnc();

  //【S】スピード設定
  }else if(rcvMsg[0] == 'S'){
    setWheelSpeed(rcvMsg);

  }else if(rcvMsg[0] == 'P'){
    setPenUpDown(rcvMsg);

  }else if(rcvMsg[0] == 'L'){
    setLedColorByMsg(rcvMsg);
  }


  return;
}

//【E】エンコーダ読み取りコマンド
void Wrc021::sendEnc2dev(){
  sendEnc2dev(NULL, NULL);
}

void Wrc021::sendEnc2dev(uint8_t viaBlt){
  sendEnc2dev(NULL, viaBlt);
}

void Wrc021::sendEnc2dev(WiFiClient* client){
  sendEnc2dev(client, NULL);
}

void Wrc021::sendEnc2dev(WiFiClient* client, uint8_t viaBlt){
  int32_t encL;
  int32_t encR;
  String msg = "";

  encL = s32Map(MS32_M_POS0);
  encR = s32Map(MS32_M_POS1);

  msg = int2HexBigString(encL, 4);
  msg += " ";
  msg += int2HexBigString(encR, 4);
  
  Serial.println(msg);

  if(viaBlt == VIA_BLE){
    //ble::sendMultiByte(msg, i*2);
  }else if(viaBlt == VIA_BTC){
    btc::sendMsg(msg.c_str());
  }
  
  return;

}

//【C】エンコーダクリアコマンド
void Wrc021::clearEnc(){
  wrc021.u8Map(MU8_TRIG ,wrc021.u8Map(MU8_TRIG) | 0x80);
  return;
}

//【S】車輪走行
void Wrc021::setWheelSpeed(int32_t left, int32_t right){
  String msg = "S ";

  msg += int2HexBigString(left, 4);
  msg += " ";
  msg += int2HexBigString(right, 4);

  setWheelSpeed(msg);
}

void Wrc021::setWheelSpeed(String spdMsg){
  //先頭のSとSPを削除
  spdMsg.remove(0, 2);

  String spdStrL = spdMsg.substring(0,8);
  String spdStrR = spdMsg.substring(9,17);
  spdStrL += '\n';
  spdStrR += '\n';


  int32_t spdIntL = strtoul(spdStrL.c_str(), NULL, 16);
  int32_t spdIntR = strtoul(spdStrR.c_str(), NULL, 16);


  wheelRun(spdIntL, spdIntR);

  return;
}

//【P】ペンアップダウン
void Wrc021::setPenUpDown(String penMsg){
  //先頭のPとSPを削除
  penMsg.remove(0, 2);

  if(penMsg == "00\n"){
    penUpdown(0);
  }else if(penMsg == "01\n"){
    penUpdown(1);
  }

  return;
}

//【L】LEDカラーセット
void Wrc021::setLedColorByMsg(String ledMsg){
  //先頭のLとSPを削除
  ledMsg.remove(0, 2);

  ledColor led;

  //色情報の取得
  if(isHexadecimalDigit(ledMsg[5]) && isHexadecimalDigit(ledMsg[6])){
      led.r = ((cToHex(ledMsg[5]) << 4) | (cToHex(ledMsg[6])));
  }else{
    return;
  }

  if(isHexadecimalDigit(ledMsg[8]) && isHexadecimalDigit(ledMsg[9])){
      led.g = ((cToHex(ledMsg[8]) << 4) | (cToHex(ledMsg[9])));
  }else{
    return;
  }

  if(isHexadecimalDigit(ledMsg[11]) && isHexadecimalDigit(ledMsg[12])){
      led.b = ((cToHex(ledMsg[11]) << 4) | (cToHex(ledMsg[12])));
  }else{
    return;
  }

  //色指定するLEDの番号取得
  int number = 0;
  if(isHexadecimalDigit(ledMsg[0]) && isHexadecimalDigit(ledMsg[1]) && isHexadecimalDigit(ledMsg[2]) && isHexadecimalDigit(ledMsg[3])){
    number = ((cToHex(ledMsg[0]) << 12) | (cToHex(ledMsg[1]) << 8) | (cToHex(ledMsg[2]) << 4) | (cToHex(ledMsg[3])));
  }else{
    return;
  }

  //LEDに色情報をセット
  int i;
  for(i = 0; i < 9; i++){
    led.number = i;

    if(number & (0x01 << i)){
      setLedColor(led);
    }

  }

  return;

  
}

void Wrc021::sendMap2pc(struct cmd rcvCmd){
  sendMap2pc(rcvCmd, NULL, NULL);
}

void Wrc021::sendMap2pc(struct cmd rcvCmd, WiFiClient* client){
  sendMap2pc(rcvCmd, client, NULL);
}

void Wrc021::sendMap2pc(struct cmd rcvCmd, WiFiClient* client, uint8_t viaBlt){

  //読み込みコマンドか確認
  if(rcvCmd.cmdType == 'r' || rcvCmd.cmdType == 'R'){
    int i = 0;
    int j = 0;
    char msg[MAP_SIZE];

    if(rcvCmd.readLength == 0x00){  //読み込みbyteが0なら、メモリマップ全体を送信
      readAll();

      msg[2] = '\0';

      Serial.println("     0|  1|  2|  3|  4|  5|  6|  7|  8|  9|  A|  B|  C|  D|  E|  F");

      for(i = 0; i < 16; i++){
        Serial.print(i,HEX);
        Serial.print("0");
        Serial.print(": ");

        for(j = 0; j < 16; j++){
          msg[0]= hexToA[memMap[(i*16)+j] >> 4];
          msg[1]= hexToA[memMap[(i*16)+j] & 0x0f];
          Serial.print(msg);
          Serial.print("  ");
        }
        Serial.println();
      }
      Serial.println();
      
    }else{
      //readメモリマップがSTMへのアップロード対象ならreadを実施
      if(rcvCmd.addr < 0x00){
        if(rcvCmd.addr + rcvCmd.readLength > 0x90){
          rcvCmd.readLength -= rcvCmd.addr + rcvCmd.readLength - 0x90;
        }
        //Serial.println(rcvCmd.addr);
        //Serial.println(rcvCmd.readLength);
        readMemmap(rcvCmd.addr, rcvCmd.readLength);
      }
      
      for(i = 0; i < rcvCmd.readLength; i++){
          msg[i*2]= hexToA[memMap[rcvCmd.addr + i] >> 4];
          msg[(i*2)+1]= hexToA[memMap[rcvCmd.addr + i] & 0x0f];
      }
      msg[i*2] = '\0';
      Serial.print(msg);
      Serial.println();
      
      if(client != NULL){
        client->println("HTTP/1.1 200 OK");
        client->println("Content-type:text/html");
        client->println();
        client->print(msg);
        client->println();
      }

      if(viaBlt == VIA_BLE){
        ble::sendMultiByte(msg, i*2);
      }else if(viaBlt == VIA_BTC){
        btc::sendMsg(msg);
      }

      
      
      
    }
  }
}

void Wrc021::setWriteMapViaMsg(struct cmd rcvCmd){

  //書込みコマンドか確認
  if(rcvCmd.cmdType == 'w' || rcvCmd.cmdType == 'W'){
    int i;
    for(i = 0; i < rcvCmd.valueCount; i++){
      u8Map(rcvCmd.addr +i, *(rcvCmd.value +i));
    }
    
  }
}

//memMapのうち、値が新たに書き込まれた部分をSTM32に送信する
void Wrc021::sendWriteMap(){

  uint16_t i;
  uint8_t headAddr;
  uint8_t length;
  for(i = 0x12; i < 0x90; i++){
    headAddr = i; 
    length = 0;
    while(writeFlag[i]){
      writeFlag[i] = 0x00;
      length++;
      i++;
      if(i >= 0x90){
        break;
      }      
    }
    writeMemmap(headAddr,&memMap[headAddr], length);
  }
  for(i = 0xe; i < 0x12; i++){
    headAddr = i; 
    length = 0;
    while(writeFlag[i]){
      writeFlag[i] = 0x00;
      length++;
      i++;
      if(i >= MAP_SIZE){
        break;
      }      
    }
    writeMemmap(headAddr,&memMap[headAddr], length);
  }
}

//符号付き8bitで読み出し
int8_t Wrc021::s8Map(uint8_t addr){
  return (*(int8_t *)(&memMap[addr]));
}
//符号付き8bitで書き込み
int8_t Wrc021::s8Map(uint8_t addr, int8_t data){
  (*(int8_t *)(&memMap[addr])) = data;
  (*(uint8_t *)(&writeFlag[addr])) = 0x01;
  return (*(int8_t *)(&memMap[addr]));
}

//符号なし8bitで読み出し
uint8_t Wrc021::u8Map(uint8_t addr){
  return (*(uint8_t *)(&memMap[addr]));
}
//符号なし8bitで書き込み
uint8_t Wrc021::u8Map(uint8_t addr, uint8_t data){
    (*(uint8_t *)(&memMap[addr])) = data;
    (*(uint8_t *)(&writeFlag[addr])) = 0x01;
  return (*(uint8_t *)(&memMap[addr]));
}

//符号付き16bitで読み出し
int16_t Wrc021::s16Map(uint8_t addr){
  return (*(int16_t *)(&memMap[addr]));
}
//符号付き16bitで書き込み
int16_t Wrc021::s16Map(uint8_t addr, int16_t data){
    (*(int16_t *)(&memMap[addr])) = data;
    (*(uint16_t *)(&writeFlag[addr])) = 0x0101;
  return (*(int16_t *)(&memMap[addr]));
}

//符号なし16bitで読み出し
uint16_t Wrc021::u16Map(uint8_t addr){
  return (*(uint16_t *)(&memMap[addr]));
}
//符号なし16bitで書き込み
uint16_t Wrc021::u16Map(uint8_t addr, uint16_t data){
    (*(uint16_t *)(&memMap[addr])) = data;
    (*(uint16_t *)(&writeFlag[addr])) = 0x0101;
  return (*(uint16_t *)(&memMap[addr]));
}

//符号付き32bitで読み出し
int32_t Wrc021::s32Map(uint8_t addr){
  return (*(int32_t *)(&memMap[addr]));
}
//符号付き32bitで書き込み
int32_t Wrc021::s32Map(uint8_t addr, int32_t data){
    (*(int32_t *)(&memMap[addr])) = data;
    (*(uint32_t *)(&writeFlag[addr])) = 0x01010101;
  return (*(int32_t *)(&memMap[addr]));
}

//符号なし32bitで読み出し
uint32_t Wrc021::u32Map(uint8_t addr){
  return (*(uint32_t *)(&memMap[addr]));
}
//符号なし32bitで書き込み
uint32_t Wrc021::u32Map(uint8_t addr, uint32_t data){
    (*(uint32_t *)(&memMap[addr])) = data;
    (*(uint32_t *)(&writeFlag[addr])) = 0x01010101;
  return (*(uint32_t *)(&memMap[addr]));
}

//writeFlagの状態を確認
uint8_t Wrc021::checkWriteFlag(uint8_t addr){
  return writeFlag[addr];
}

//電圧の取得
double Wrc021::getVin(){
  uint16_t memmapV;
  double vin;
  readMemmap(MU16_M_VI , 0x02);
  memmapV = Wrc021::u16Map(MU16_M_VI);

  vin = ((double)memmapV / 0x0fff)*29.7;

  return vin;
}

int checkI2cAddrOfMsg(String rcvMsg, int rcvMsgCount){
  return checkI2cAddrOfMsg(rcvMsg, rcvMsgCount, NULL);
}

int checkI2cAddrOfMsg(String rcvMsg, int rcvMsgCount, uint8_t viaBlt){
  //正常なコマンドかどうかを確認
  if(rcvMsg[rcvMsgCount] != '\n'){  //rcvMsgの末尾が改行コードかどうかを確認
    //Serial.println("ERR: no \\n");
    return 0;
  
  }else if( rcvMsg[0] == 'E' || rcvMsg[0] == 'e' || rcvMsg[0] == 'C' || rcvMsg[0] == 'c' || rcvMsg[0] == 'S' || rcvMsg[0] == 's' 
          || rcvMsg[0] == 'P' || rcvMsg[0] == 'p' || rcvMsg[0] == 'L' || rcvMsg[0] == 'l' ){
    //Serial.println("get spiffs command");
    wrc021.checkMsg(rcvMsg, viaBlt);
      
  }else if(rcvMsgCount >= 7 ){       //rcvMsgが改行コードを含めて8文字以上あるかどうか(SPIFFS系コマンドを除く)
    //正常なコマンド
    //I2Cアドレスを復号し，文字列から削除
    uint8_t i2cAddr = 0xff;
    if(isHexadecimalDigit(rcvMsg[POS_OF_I2CADDR]) && isHexadecimalDigit(rcvMsg[POS_OF_I2CADDR+1])){
      i2cAddr = ((cToHex(rcvMsg[POS_OF_I2CADDR]) << 4) | (cToHex(rcvMsg[POS_OF_I2CADDR+1])));
      rcvMsg.remove(1, 2);
    }else if(rcvMsg[POS_OF_I2CADDR] == ' '){
      //I2Cアドレスが無いor構文に誤りがある可能性がある場合
      //2文字目が空白であればアドレス指定が無いものとして、i2cアドレスを0x10とする
      i2cAddr = 0x10;        
    }

    //i2cAddrの内容によって呼び出すメモリマップを振り分け
    if(i2cAddr == 0x10){
      wrc021.checkMsg(rcvMsg, viaBlt);
    }else if(i2cAddr == 0x3c){
      //LED基板
      wrc033.checkMsg(rcvMsg, viaBlt);
    }else{
      ///Serial.print("ERR: invalid I2C addr");
      //printf(" 0x%2x \n", i2cAddr);
      return 0;
    }
      
  }else if(rcvMsgCount == 0){       //改行コードのみを受信
    //Serial.println('\n');
    return 0;
      
  }else{                            //コマンドが短い
    //Serial.println("ERR: Msg length too short");
    return 0;
  }

  return 1;    
  
}


//パラメータセット
uint8_t setRoverParam(String rcvMsg){

  int i = 0;
  String name = "";
  String value = "";
  while(!isSpace(rcvMsg[i])){
    name += rcvMsg[i];
    i++;
  }
  i++;
  while(i <= rcvMsg.length()){
    value += rcvMsg[i];
    i++;
  }

  String filePath = "/";
  filePath.concat(name);
  filePath += ".txt";

  value += "\n";

  writeFile(filePath.c_str(), value);


  return 1;
}

uint8_t getRoverParam(String rcvMsg){

  uint8_t tmp[256];
  int i;
  for(i = 0; i < 256; i++){
    tmp[i] = 0;
  }
  tmp[0] = '\n';

  rcvMsg += ".txt";
  String filePath = "/";
  filePath.concat(rcvMsg);
  loadFile(filePath.c_str(), tmp);

  String msg = (char*)tmp;

  if(msg != "null"){
    Serial.println(msg);
    return 1;
  }

  return 0;

}


//LED基板へのread無効化
int Wrc033::readMemmap(uint8_t addr, uint8_t readLength){
  return 1;
}

//LEDmemMapのうち、値が新たに書き込まれた部分をwrc033に送信する
void Wrc033::sendWriteMap(){

  uint16_t i;
  uint8_t headAddr;
  uint8_t length;
  for(i = 0x00; i < 0x21; i++){
    headAddr = i; 
    length = 0;
    while(writeFlag[i]){
      writeFlag[i] = 0x00;
      length++;
      i++;
      if(i >= 0x4C){
        break;
      }      
    }
    writeMemmap(headAddr,&memMap[headAddr], length);
  }
  for(i = 0x2a; i < 0x4C; i++){
    headAddr = i; 
    length = 0;
    while(writeFlag[i]){
      writeFlag[i] = 0x00;
      length++;
      i++;
      if(i >= 0x4C){
        break;
      }      
    }
    writeMemmap(headAddr,&memMap[headAddr], length);
  }
  for(i = 0x25; i < 0x26; i++){
    headAddr = i; 
    length = 0;
    while(writeFlag[i]){
      writeFlag[i] = 0x00;
      length++;
      i++;
      if(i >= 0x4C){
        break;
      }      
    }
    writeMemmap(headAddr,&memMap[headAddr], length);
  }
}


//基板上のSTM32
Wrc021 wrc021(0x10);

//ナノローバーLED基板
Wrc033 wrc033(0x3c);

