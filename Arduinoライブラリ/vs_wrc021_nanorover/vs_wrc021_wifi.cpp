#include "vs_wrc021_wifi.h"
#include "vs_wrc021_memmap.h"
#include "vs_wrc021_spi.h"
#include "vs_wrc021_motor.h"
#include "vs_wrc031_spiffs.h"
#include "vs_wrc021_nanorover.h"
#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>

WiFiServer server(80);

WiFiClient client;

const size_t httpCapacity = JSON_ARRAY_SIZE(9) + 2*JSON_OBJECT_SIZE(3) + 128;
//DynamicJsonDocument roverHTTPMsg(paramCapacity);
uint8_t bufHTTPMsg[httpCapacity];
DynamicJsonDocument jsonCommandFromHTTP(httpCapacity);

int responseCode = INDEX;
uint32_t connectTime = 0;

String wifiRead;
String wifiWrite;
uint8_t i2cAddr = 0x00;

String currentLine = "";

// WiFiネットワークに接続
void wifiInit(char* ssid, char* password){
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int printDotTime = millis();
  while (WiFi.status() != WL_CONNECTED) {

    if(millis() - printDotTime > 500){
      Serial.print(".");
      printDotTime = millis();
    }
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// サーバー起動
void serverInit(char* ui_path){
  server.begin();
}

//JSONデータ長の取得
int getJsonLength(String currentLine){
  //先頭の不要な文字列"content-length:　"を削除
  currentLine.remove(0, 16);

  int hexLength = 0;
  while(isHexadecimalDigit(currentLine[hexLength])){
    hexLength++;
  }

  Serial.print("HexLength ---->>>> ");
  Serial.println(hexLength);

  int jsonLength = 0;
  int i;
  for(i = 0; i < hexLength; i++){
    jsonLength += (int)(cToHex(currentLine[i])*pow(10.0,(hexLength-i-1)));
  }

  return jsonLength;

}

//JSONの取得とパース
int getJsonBody(WiFiClient* client, int jsonLength){

  String jsonString = "";

  char tmp;
  while(1){
    tmp = client->read();
    if(tmp == '{'){
      break;
    }
  }

  jsonString += tmp;

  int i;
  for(i = 1; i < jsonLength; i++){
    tmp = client->read();
    jsonString += tmp;
  }

  if(tmp != '}'){
    //Json取得ミス
    Serial.println("bad json");
    return -1;
  }

  while(client->available()){
    tmp = client->read();
  }

  Serial.println(jsonString);

  deserializeJson(jsonCommandFromHTTP, jsonString.c_str());

  return 1;


}


//Jsonコマンドに対するreturnの実行
int sendJsonCommandToCheckMsg(WiFiClient* client){

  if(jsonCommandFromHTTP["command"] == "get_encoder"){
    //エンコーダ値取得コマンド
    if(runECom(client)){
      return 1;
    }else{
      badRequest(client);
      return 0;
    }
    
  }

  if(jsonCommandFromHTTP["command"] == "clear_encoder"){
    return runCCom(client);
  }

  if(jsonCommandFromHTTP["command"] == "set_speed"){
    return runSCom(client);
  }

  if(jsonCommandFromHTTP["command"] == "pen_updown"){
    return runPCom(client);
  }

  if(jsonCommandFromHTTP["command"] == "set_LEDcolor"){
    return runLCom(client);
  }





}

//JsonコマンドによるEコマンドの実行
int runECom(WiFiClient* client){
  
  String encValMsg = "";
  encValMsg  = "{\"left\":";
  encValMsg += String(wrc021.s32Map(MS32_M_POS0),DEC);
  encValMsg += ",\"right\":";
  encValMsg += String(wrc021.s32Map(MS32_M_POS1),DEC);
  encValMsg += "}";

  return returnSuccess(client, encValMsg);

}

//JsonコマンドによるCコマンドの実行
int runCCom(WiFiClient* client){
  wrc021.clearEnc();
  return returnSuccess(client);

}

//JsonコマンドによるSコマンドの実行
int runSCom(WiFiClient* client){
  wrc021.setWheelSpeed(jsonCommandFromHTTP["left"], jsonCommandFromHTTP["right"]);
  return returnSuccess(client);
}

//JsonコマンドによるTコマンドの実行
int runPCom(WiFiClient* client){
  if(jsonCommandFromHTTP["is_up"]){
    penUpdown(1);
  }else{
    penUpdown(0);
  }
  
  return returnSuccess(client);
}

//JsonコマンドによるLコマンドの実行
int runLCom(WiFiClient* client){
  ledColor led;

  led.r = jsonCommandFromHTTP["color"]["r"];
  led.g = jsonCommandFromHTTP["color"]["g"];
  led.b = jsonCommandFromHTTP["color"]["b"];

  //LEDに色情報をセット
  int i;
  for(i = 0; i < 9; i++){
    led.number = jsonCommandFromHTTP["channel"][i] | 255;
    Serial.println(led.number);

    setLedColor(led);

  }

  return returnSuccess(client);
}


//Jsonコマンド実行成功のリターン
int returnSuccess(WiFiClient* client){
  String msg = "";
  return returnSuccess(client, msg);
}

int returnSuccess(WiFiClient* client, String msg){

  client->println("HTTP/1.1 200 OK");

  if(msg.length()){
    client->println("Content-Type: application/json");
    client->print("Content-Length: ");
    client->println(msg.length());
  }else{
    //client->println("Content-Type: text/html");
    client->print("Content-Length: 0");
  }
  client->println();

  if(msg.length()){
    client->println(msg.c_str());
  }
  client->println();

  Serial.println(msg.c_str());

  return 1;

}

//I2Cアドレスの取得
uint8_t getI2CAddr(String currentLine){
  int i,j;
  uint8_t i2cAddr = 0xff;

  i = currentLine.indexOf("i2caddr=");

  if(i == -1){
    return 0x10;
  }

  j=8;
  if(isHexadecimalDigit(currentLine[i+j]) && isHexadecimalDigit(currentLine[i+j+1])){
        i2cAddr = ((cToHex(currentLine[i+j]) << 4) | (cToHex(currentLine[i+j+1])));
  }

  return i2cAddr;
}



//UIページの送信
void handleRoot(WiFiClient* client){
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client->println("HTTP/1.1 200 OK");
  client->println("Content-type:text/html");
  client->println();

  // the content of the HTTP response follows the header:
  //client->print((char *)web_buf);
  //client->print("Please read instructions");

  // The HTTP response ends with another blank line:
  client->println();
}

void Vin(WiFiClient* client){
  if(wrc021.u8Map(MU8_P_STTS)){
    return;
  }

  double voltage = 0.0;
  voltage = wrc021.getVin();
  String res = String(voltage);
  client->println("HTTP/1.1 200 OK");
  client->println("Content-type:text/plain");
  client->println();
  client->print(res);
  client->println();
  Serial.println("Vin");
}

void Go(){
  setCtrlMode(MODE_PWM);
  v_com[M_L] = -0.3;
  v_com[M_R] = 0.3;
  setO_EN(ON_ON, &wrc021);
  Serial.println("Go"); 
}

void Left(){
  setCtrlMode(MODE_PWM);
  v_com[M_L] = 0.3;
  v_com[M_R] = 0.3;
  setO_EN(ON_ON, &wrc021);
  Serial.println("Left");
}

void Right(){
  setCtrlMode(MODE_PWM);
  v_com[M_L] = -0.3;
  v_com[M_R] = -0.3;
  setO_EN(ON_ON, &wrc021);
  Serial.println("Right");
}

void Back(){
  setCtrlMode(MODE_PWM);
  v_com[M_L] = 0.3;
  v_com[M_R] = -0.3;
  setO_EN(ON_ON, &wrc021);
  Serial.println("Back");
}

void Stop(){
  setCtrlMode(MODE_PWM);
  v_com[M_L] = 0.0;
  v_com[M_R] = 0.0;
  setO_EN(ON_ON, &wrc021);
  Serial.println("Stop");
}

void badRequest(WiFiClient* client){
  client->println("HTTP/1.1 400 BadRequest");
  client->println("Content-Length: 0");
  client->println();
  client->println();
  Serial.println("BadRequest");
}

void notFound(WiFiClient* client){
  client->println("HTTP/1.1 404 NotFound");
  client->println("Content-Length: 0");
  client->println();
  client->println();
  Serial.println("NotFound");
}

// クライアント接続時の処理
int checkClient(){
  
  int i, j;
  bool getCom = false;
  int jsonLength = 0;

  if(!client){
    client = server.available();
    responseCode = NOT_FOUND;
  }

  if (client) {
    connectTime = millis();                 // recode the time of connection from client                   
    while (client.connected()) {            // loop while the client's connected
      if(client.available()){               // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.print(c);                    // print it out the serial monitor
        if(c=='\n'){                        // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request header, so send a response or get a body:
          if(currentLine.length() == 0){

            if(responseCode == WIFI_READ){
              if(i2cAddr == 0x10){
                wrc021.checkMsg(wifiRead, &client);
                Serial.println("WIFI_READ");
              }else if(i2cAddr == 0x1F){
                //wrc022.checkMsg(wifiRead, &client);
              }else{
                badRequest(&client);
              }
              wifiRead = "";
              
            }else if(responseCode == INDEX || responseCode == GET_BUTTON){
              handleRoot(&client);
              Serial.println("index or get button");
            }else if(responseCode == WIFI_WRITE){
              if(i2cAddr == 0x10){
                wrc021.checkMsg(wifiWrite, &client);
                client.println("HTTP/1.1 200 OK");
                client.println("Content-type:text/html");
                client.println();
                client.println();
                Serial.println("WIFI_WRITE");
              }else if(i2cAddr == 0x1F){
                //wrc022.checkMsg(wifiWrite, &client);
                client.println("HTTP/1.1 200 OK");
                client.println("Content-type:text/html");
                client.println();
                client.println();
              }else{
                badRequest(&client);
              }
              wifiWrite = "";   
              
            }else if(responseCode == WIFI_VIN){
              Vin(&client);
            }else if(responseCode == NOT_FOUND){
              notFound(&client);
            }else{
              badRequest(&client);
            }
            currentLine = "";
            
            // break out of the while loop:
            break;
            
          }else if(currentLine.startsWith("GET /vin")){
            responseCode = WIFI_VIN;

          }else{
            currentLine.toLowerCase();
          
            if(currentLine.startsWith("content-type: application/json")){
              Serial.println("!!!!! GET JSON !!!!!");
              responseCode = GET_JSON;
              currentLine = "";
            }else if(currentLine.startsWith("content-length: ") && responseCode == GET_JSON){
              jsonLength = getJsonLength(currentLine);
              Serial.print("JSON Length ----->>>>>  ");
              printf("%d\n", jsonLength);
              responseCode = GET_JSON_L;
              currentLine = "";
              if(!getJsonBody(&client, jsonLength)){
                badRequest(&client);
              }

              sendJsonCommandToCheckMsg(&client);

              currentLine = "";

            }else{    // if you got a newline, then clear currentLine:
              //Serial.println(currentLine);
              currentLine = "";
            }
          }
        }else if(c != '\r'){  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
          
          if(currentLine.startsWith("GET /write")){   //GET /writeで始まり
            if(currentLine.endsWith("HTTP")){         //HTTP/1.1で終わった場合は、メモリマップへのwrite命令                      
              if(currentLine.indexOf('?') == 10){     //10文字目'？'でなければフォーマットエラー
                responseCode = WIFI_WRITE;

                //write命令セット
                wifiWrite += "w ";

                //アドレスの取得
                i =  currentLine.indexOf("addr=");
                if(i == -1){
                  responseCode = BAD_REQUEST;
                }
              
                for(j = 5; j <= 6; j++){
                  if(isHexadecimalDigit(currentLine[i+j])){
                    wifiWrite += currentLine[i+j];
                  }else{
                    responseCode = BAD_REQUEST;
                  }
                }
                wifiWrite += ' ';

                //データセット
                i = currentLine.indexOf("data=");
                if(i == -1){
                  responseCode = BAD_REQUEST;
                }

                j = 5;
                while(isHexadecimalDigit(currentLine[i+j])){
                  wifiWrite += currentLine[i+j];
                  j++;
                }
                wifiWrite += '\n';

                //I2Cアドレスの取得
                i2cAddr = getI2CAddr(currentLine);
                if(i2cAddr == 0xff){
                  responseCode = BAD_REQUEST;
                }
              
              }else{
                responseCode = BAD_REQUEST;
              }
            }
          }

          if(currentLine.startsWith("GET /read")){   //GET /readで始まり
            if(currentLine.endsWith("HTTP")){         //HTTP/1.1で終わった場合は、メモリマップへのread命令
              if(currentLine.indexOf('?') == 9){     //9文字目'？'でなければフォーマットエラー
                responseCode = WIFI_READ;
                //write命令セット
                wifiRead += "r ";
                //アドレスの取得
                i =  currentLine.indexOf("addr=");
                if(i == -1){
                  responseCode = BAD_REQUEST;
                }
              
                for(j = 5; j <= 6; j++){
                  if(isHexadecimalDigit(currentLine[i+j])){
                    wifiRead += currentLine[i+j];
                  }else{
                    responseCode = BAD_REQUEST;
                  }
                }
                wifiRead += ' ';

                //読み込みバイト数取得
                i = currentLine.indexOf("length=");
                if(i == -1){
                  responseCode = BAD_REQUEST;
                }

                for(j = 7; j <= 8; j++){
                  if(isHexadecimalDigit(currentLine[i+j])){
                    wifiRead += currentLine[i+j];
                  }else{
                    responseCode = BAD_REQUEST;
                  }
                }
                wifiRead += '\n';

                //I2Cアドレスの取得
                i2cAddr = getI2CAddr(currentLine);
                if(i2cAddr == 0xff){
                  responseCode = BAD_REQUEST;
                }               
              
              }else{
                responseCode = BAD_REQUEST;
              }
            }
          }

      }
    }
    // close the connection:
    client.stop();
  }else{
    client.stop();
  }

  return responseCode;
} 