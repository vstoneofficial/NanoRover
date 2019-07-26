#include "vs_wrc031_led.h"
#include "vs_wrc021_memmap.h"
#include <Arduino.h>
#include <Wire.h>

const ledColor blackTop = {0x00, 0x00, 0x00, 0x00};
const ledColor orangeTop = {0x00, 0xff, 0x9f, 0x05};

void wrc033LedInit(){

  //Serial.println("WRC033 LED Setup Start");

  //ノーマルオペレーションモードに設定
  wrc033.u8Map(U8_SHUTDOWN, 0x01);

  //全LEDをオフに
  wrc033.u8Map(U8_ALL_EN, 0x01);

  wrc033.sendWriteMap();

  //PWM周波数を3kHzに設定
  wrc033.u8Map(U8_FREQ, 0x00);

  //全LEDの出力デューティー比を0に
  int i;
  for(i = 0; i < 27; i++){
    wrc033.u8Map(U8_PWM1+(uint8_t)i, 0x00);
  }

  //全LEDの出力をON、電流値を1/4に
  for(i = 0; i < 27; i++){
    wrc033.u8Map(U8_LED1_CTL+(uint8_t)i, LED_I_4 | LED_ON);
  }

  //TOPのLEDのみ電流値を最大に
  wrc033.u8Map(U8_LED1_CTL, LED_I_MAX | LED_ON);
  wrc033.u8Map(U8_LED2_CTL, LED_I_MAX | LED_ON);
  wrc033.u8Map(U8_LED3_CTL, LED_I_MAX | LED_ON);

  //全LEDをONに
  wrc033.u8Map(U8_ALL_EN, 0x00);

  wrc033.sendWriteMap();

  //書き換えた値を反映
  wrc033.u8Map(U8_UPDATE, 0x00);
  delay(2);
  wrc033.sendWriteMap();

  //Serial.println("set preset");

  //約0.8秒かけてゆるやかに白く点灯
  int j;
  for(j = 0; j < 23; j++){
    for(i = 0; i < 27; i ++){
      wrc033.u8Map(U8_PWM1+(uint8_t)i, j);
    }
    //書き換えた値を反映
    wrc033.u8Map(U8_UPDATE, 0x00);
    wrc033.sendWriteMap();

    delay(50);
  }


  return;
}

//LEDの色を指定する
void setLedColor(ledColor color){
  //指定されているLEDの番号が適切か確認
  if(color.number > 8){
    return;
  }

  //RGB値をpwm値に変換
  ledColor pwm;
  pwm.number = color.number;
  pwm.r      = (int)(((double)color.r));//*0.25);
  pwm.g      = (int)(((double)color.g));//*0.5*0.25);
  pwm.b      = (int)(((double)color.b));//*0.25);

  //pwm値をメモリマップにセット
  wrc033.u8Map(U8_PWM1+(pwm.number*3), pwm.r);
  wrc033.u8Map(U8_PWM2+(pwm.number*3), pwm.g);
  wrc033.u8Map(U8_PWM3+(pwm.number*3), pwm.b);

  //書き換えた値を反映
  wrc033.u8Map(U8_UPDATE, 0x00);

  return;

}

//全てのLEDを緩やかに消灯する
void changeLed2TurnOff(){
  int i;
  for(i = 0; i < 27; i++){
    if(wrc033.u8Map(U8_PWM1+(uint8_t)i) > 0){
      wrc033.u8Map(U8_PWM1+(uint8_t)i, wrc033.u8Map(U8_PWM1+(uint8_t)i) - 1);
    }
  }

  wrc033.u8Map(U8_UPDATE, 0x00);
  wrc033.sendWriteMap();
}


