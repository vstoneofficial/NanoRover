/**
 * SPIFFS関連の関数
 * 
 */
#ifndef WRC031_SPIFFS_H
#define WRC031_SPIFFS_H


#include <vector>
#include <string>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>


#define BUF_SIZE 10240
extern uint8_t buf[];
extern std::vector<std::string> userProgram;
extern int numCommand;

//車両パラメータ
extern const size_t paramCapacity;
extern DynamicJsonDocument roverParam;



const char userComPath[] = "/user.txt";  //ユーザープログラムのパス
const char roverParamPath[] = "/param.json";    //車両パラメータのパス

uint8_t loadFile(const char* path, uint8_t tmp[]);
uint8_t writeFile(const char* path, String data);
uint8_t appendFile(const char* path, String data);
uint8_t deleteFile(const char* path);
uint8_t loadUserProgram();
uint8_t loadUserProgramOnWakeUp();

uint8_t saveRoverParam();
uint8_t loadRoverParam();


#endif /* SPIFFS_H */