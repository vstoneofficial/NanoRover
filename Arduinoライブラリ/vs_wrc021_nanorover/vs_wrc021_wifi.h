/**
 * vs-wrc021メガローバーで使用するwifi関連の定義
 * 
*/
#ifndef WRC021_WIFI_H
#define WRC021_WIFI_H

#include "vs_wrc021_memmap.h"
#include "vs_wrc021_spi.h"
#include "vs_wrc021_motor.h"
#include "vs_wrc031_spiffs.h"
#include <Arduino.h>
#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>

enum responses{
    INDEX       = 0,
    WIFI_WRITE  = 1,
    WIFI_READ   = 2,
    GET_BUTTON  = 3,
    WIFI_VIN    = 4,
    BAD_REQUEST = 5,
    GET_JSON    = 6,
    GET_JSON_L  = 7,
    NOT_FOUND   = 99
};



extern WiFiServer server;

void wifiInit(char* ssid, char* password);
void serverInit(char* ui_path);

int getJsonLength(String currentLine);
int getJsonBody(WiFiClient* client, int jsonLength);
int sendJsonCommandToCheckMsg(WiFiClient* client);
int runECom(WiFiClient* client);
int runCCom(WiFiClient* client);
int runSCom(WiFiClient* client);
int runPCom(WiFiClient* client);
int runLCom(WiFiClient* client);

int returnSuccess(WiFiClient* clieb);
int returnSuccess(WiFiClient* client, String msg);
void badRequest(WiFiClient* client);

int loadUI(char* ui_path);
int checkClient();

#endif /* WIFI_H */