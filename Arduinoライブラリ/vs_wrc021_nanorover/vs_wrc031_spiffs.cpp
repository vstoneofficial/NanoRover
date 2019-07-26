#include "vs_wrc031_spiffs.h"
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

/*
void spiffsInit(){ 
  SPIFFS.begin(true);
}
*/



uint8_t buf[BUF_SIZE];
std::vector<std::string> userProgram;
int numCommand;

const size_t paramCapacity = JSON_OBJECT_SIZE(3) + 128;
DynamicJsonDocument roverParam(paramCapacity);
uint8_t bufParam[paramCapacity];



//readする
uint8_t loadFile(const char* path, uint8_t tmp[]){

  if(!SPIFFS.begin()){
    Serial.println("SPIFFS maunt failed");
    while(!SPIFFS.begin(true)){
      Serial.println("SPIFFS maunt failed");
    }
    return 0;
  }
  File fi = SPIFFS.open(path);
  if (!fi) {
    Serial.println("File does not exist.");
    return 0;
  }else if(fi.size() >= BUF_SIZE){
    Serial.print("Could not load file. Make file smaller.");
    return 0;
  }else{
    //buf = (uint8_t*)malloc(fi.size()+1);
    fi.read(tmp,fi.size());
    fi.close();
    SPIFFS.end();
    //Serial.println("loaded");
    return 1;
  }
}

//writeする
uint8_t writeFile(const char* path, String data){
  Serial.printf("Writing file: %s\r\n", path);

  if(!SPIFFS.begin()){
    Serial.println("SPIFFS maunt failed");
    while(!SPIFFS.begin(true)){
      Serial.println("SPIFFS maunt failed");
    }
    return 0;
  }
  //deleteFile(path);

  File fi = SPIFFS.open(path, FILE_WRITE);
  if (!fi) {
    Serial.println("File does not exist.");
    return 0;
  }

  if(fi.print(data.c_str())){
    fi.close();
    Serial.println("written");
    SPIFFS.end();
    return 1;
  }else{
    fi.close();
    Serial.println("write failed");
    SPIFFS.end();
    return 0;
  }

}

//appendする
uint8_t appendFile(const char* path, String data){

  if(!SPIFFS.begin()){
    Serial.println("SPIFFS maunt failed");
    while(!SPIFFS.begin(true)){
      Serial.println("SPIFFS maunt failed");
    }
    return 0;
  }
  File fi = SPIFFS.open(path, FILE_APPEND);
  if (!fi) {
    Serial.println("File does not exist.");
    return 0;
  }

  if(fi.print(data.c_str())){
    fi.close();
    Serial.println("appended");
    SPIFFS.end();
    return 1;
  }else{
    fi.close();
    Serial.println("append failed");
    SPIFFS.end();
    return 0;
  }

}

//deleteする
uint8_t deleteFile(const char* path){
  if(!SPIFFS.begin()){
    Serial.println("SPIFFS maunt failed");
    while(!SPIFFS.begin(true)){
      Serial.println("SPIFFS maunt failed");
    }
    return 0;
  }
  if(SPIFFS.remove(path)){
    Serial.println("deleted");
    SPIFFS.end();
    return 1;
  }else{
    Serial.println("delete failed");
    SPIFFS.end();
    return 0;
  }
}

//ユーザープログラムを読み出してコマンド毎にstringに格納
uint8_t loadUserProgram(){
  
  int i;
  for( i = 0; i < BUF_SIZE; i++){
    buf[i] = 0;
  }
  loadFile(userComPath, buf);

  userProgram.clear();
  userProgram.shrink_to_fit();
  //userProgram.reserve(BUF_SIZE);



  i = 0;
  int commandHead = 0;
  int commandLength = 0;
  int commandNo = 0;
  while(buf[i] != 'Q' && i < BUF_SIZE){ //ユーザープログラム終了コードQ
    commandHead = i;
    commandLength = 0;
    do{
      i++;
      commandLength++;
    }while(buf[i] != 'L' && buf[i] != 'Q');
    

    std::string command(&buf[commandHead], &buf[commandHead] + commandLength);

    if(buf[i] == 'Q'){
      command += 'Q';
    }else{
      i++;
    }
    command += '\n';
    userProgram.push_back(command);
    
    printf("%s", userProgram[commandNo].c_str());
    
    commandNo++;
    
  }

  //userProgram.shrink_to_fit();
  numCommand = userProgram.size() -1;

  return 1;
}

//起動直後にユーザープログラムを読み出してコマンド毎にstringに格納
uint8_t loadUserProgramOnWakeUp(){
  int i;
  for( i = 0; i < BUF_SIZE; i++){
    buf[i] = 0;
  }
  loadFile(userComPath, buf);

  userProgram.clear();
  userProgram.shrink_to_fit();
  //userProgram.reserve(BUF_SIZE);

  i = 0;
  int commandHead = 0;
  int commandLength = 0;
  int commandNo = 0;
  while(buf[i] != 'Q' && i < BUF_SIZE){ //ユーザープログラム終了コードQ
    commandHead = i;
    commandLength = 0;
    do{
      i++;
      commandLength++;
    }while(buf[i] != 'L' && buf[i] != 'Q');
    

    std::string command(&buf[commandHead], &buf[commandHead] + commandLength);

    if(buf[i] == 'Q'){
      command += 'Q';
    }else{
      i++;
    }
    command += '\n';
    userProgram.push_back(command);
    
    //printf("%s", userProgram[commandNo].c_str());
    
    commandNo++;
    
  }

  //userProgram.shrink_to_fit();
  numCommand = userProgram.size() -1;
  

  return 1;
}

uint8_t saveRoverParam(){
  char output[paramCapacity];

  serializeJson(roverParam, output);

  String outputStr = output;

  writeFile(roverParamPath, outputStr);

  return 1;
}

uint8_t loadRoverParam(){
  int i;
  
  for( i = 0; i < paramCapacity; i++){
    bufParam[i] = 0;
  }

  uint8_t isLoadParam = 0;
  isLoadParam = loadFile(roverParamPath, bufParam);

  if(isLoadParam){
    String bufParamStr = (char *)bufParam;
    deserializeJson(roverParam, bufParamStr);

    return 1;
  }
  
  return 0;
}

