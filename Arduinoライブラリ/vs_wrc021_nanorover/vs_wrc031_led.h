/**
 * LED関連の関数
 * 
 */
#ifndef WRC031_LED_H
#define WRC031_LED_H

#include "vs_wrc021_memmap.h"
#include <Arduino.h>
#include <Wire.h>


#define I2C_LED_AD 0b01111000

#define LED_I_4    0b00000110
#define LED_I_3    0b00000100
#define LED_I_2    0b00000010
#define LED_I_MAX  0b00000000

#define LED_ON     0x01
#define LED_OFF    0x00

struct ledColor{
  uint8_t number; //topから反時計回りに0-8
  uint8_t r;      //r成分 0-255
  uint8_t g;      //g成分 0-255
  uint8_t b;      //b成分 0-255
};

extern const ledColor blackTop;
extern const ledColor orangeTop;


enum ledBordRegister{
  U8_SHUTDOWN = 0x00,  /* シャットダウン */
  U8_PWM1     = 0x05,  /* LED1のPWM値   PWM値は32step */
  U8_PWM2     = 0x06,  /* LED2のPWM値   */
  U8_PWM3     = 0x07,  /* LED3のPWM値   */
  U8_PWM4     = 0x08,  /* LED4のPWM値   */
  U8_PWM5     = 0x09,  /* LED5のPWM値   */
  U8_PWM6     = 0x0a,  /* LED6のPWM値   */
  U8_PWM7     = 0x0b,  /* LED7のPWM値   */
  U8_PWM8     = 0x0c,  /* LED8のPWM値   */
  U8_PWM9     = 0x0d,  /* LED9のPWM値   */
  U8_PWM10    = 0x0e,  /* LED10のPWM値   */
  U8_PWM11    = 0x0f,  /* LED11のPWM値   */
  U8_PWM12    = 0x10,  /* LED12のPWM値   */
  U8_PWM13    = 0x11,  /* LED13のPWM値   */
  U8_PWM14    = 0x12,  /* LED14のPWM値   */
  U8_PWM15    = 0x13,  /* LED15のPWM値   */
  U8_PWM16    = 0x14,  /* LED16のPWM値   */
  U8_PWM17    = 0x15,  /* LED17のPWM値   */
  U8_PWM18    = 0x16,  /* LED18のPWM値   */
  U8_PWM19    = 0x17,  /* LED19のPWM値   */
  U8_PWM20    = 0x18,  /* LED20のPWM値   */
  U8_PWM21    = 0x19,  /* LED21のPWM値   */
  U8_PWM22    = 0x1a,  /* LED22のPWM値   */
  U8_PWM23    = 0x1b,  /* LED23のPWM値   */
  U8_PWM24    = 0x1c,  /* LED24のPWM値   */
  U8_PWM25    = 0x1d,  /* LED25のPWM値   */
  U8_PWM26    = 0x1e,  /* LED26のPWM値   */
  U8_PWM27    = 0x1f,  /* LED27のPWM値   */
  U8_PWM28    = 0x20,  /* LED28のPWM値   */
  U8_UPDATE   = 0x25,  /* PWM/CTLレジスタの値を制御に反映する */
  U8_LED1_CTL = 0x2a,  /* LED1出力コントロールレジスタ */
                       /* b0: 0->LED OFF, 1->LED ON */
                       /* b1-2: 00->Imax, 01->Imax/2, 10->Imax/3, 11->Imax/4 */
                       /* b3-7: 0000 0 stable */
  U8_LED2_CTL = 0x2b,  /* LED2出力コントロールレジスタ */
  U8_LED3_CTL = 0x2c,  /* LED3出力コントロールレジスタ */
  U8_LED4_CTL = 0x2d,  /* LED4出力コントロールレジスタ */
  U8_LED5_CTL = 0x2e,  /* LED5出力コントロールレジスタ */
  U8_LED6_CTL = 0x2f,  /* LED6出力コントロールレジスタ */
  U8_LED7_CTL = 0x30,  /* LED7出力コントロールレジスタ */
  U8_LED8_CTL = 0x31,  /* LED8出力コントロールレジスタ */
  U8_LED9_CTL = 0x32,  /* LED9出力コントロールレジスタ */
  U8_LED10_CTL= 0x33,  /* LED10出力コントロールレジスタ */
  U8_LED11_CTL= 0x34,  /* LED11出力コントロールレジスタ */
  U8_LED12_CTL= 0x35,  /* LED12出力コントロールレジスタ */
  U8_LED13_CTL= 0x36,  /* LED13出力コントロールレジスタ */
  U8_LED14_CTL= 0x37,  /* LED14出力コントロールレジスタ */
  U8_LED15_CTL= 0x38,  /* LED15出力コントロールレジスタ */
  U8_LED16_CTL= 0x39,  /* LED16出力コントロールレジスタ */
  U8_LED17_CTL= 0x3a,  /* LED17出力コントロールレジスタ */
  U8_LED18_CTL= 0x3b,  /* LED18出力コントロールレジスタ */
  U8_LED19_CTL= 0x3c,  /* LED19出力コントロールレジスタ */
  U8_LED20_CTL= 0x3d,  /* LED20出力コントロールレジスタ */
  U8_LED21_CTL= 0x3e,  /* LED21出力コントロールレジスタ */
  U8_LED22_CTL= 0x3f,  /* LED22出力コントロールレジスタ */
  U8_LED23_CTL= 0x40,  /* LED23出力コントロールレジスタ */
  U8_LED24_CTL= 0x41,  /* LED24出力コントロールレジスタ */
  U8_LED25_CTL= 0x42,  /* LED25出力コントロールレジスタ */
  U8_LED26_CTL= 0x43,  /* LED26出力コントロールレジスタ */
  U8_LED27_CTL= 0x44,  /* LED27出力コントロールレジスタ */
  U8_LED28_CTL= 0x45,  /* LED28出力コントロールレジスタ */
  U8_ALL_EN   = 0x4a,  /* 全LEDの出力許可/不許可 */
  U8_FREQ     = 0x4b,  /* PWMの周波数 */
                       /* b0: 0->3kHz 1->22kHz */
  U8_RESET    = 0x4f   /* レジスタをリセット */
      
};


void wrc033LedInit();

void setLedColor(struct ledColor color);

void changeLed2TurnOff();

#endif /* SPIFFS_H */