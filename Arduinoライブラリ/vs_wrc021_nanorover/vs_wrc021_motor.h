/**
 * モータ出力制御用ライブラリ
 * ROS, V-CON用
 * 
 * 左前進->正
 * 右前進->負
 */
#ifndef WRC021_MOTOR_H
#define WRC021_MOTOR_H

#include "vs_wrc021_memmap.h"
#include "vs_wrc021_spi.h"
#include "vs_wrc031_spiffs.h"
#include <vector>
#include <string>

#define MODE_PWM      1
#define MODE_POS      0
/*
#define OFF_OFF    0
#define ON_OFF     1
#define OFF_ON     2
#define ON_ON      3
*/
#define OFF_OFF    0
#define ON_OFF     5
#define OFF_ON     6
#define ON_ON      7
#define PEN_ON     4
#define ENC_RESET  0x0c

#define KP_NORMAL  0x08000800   //STM32の比例制御ゲイン
#define KP_ZERO    0x00000000

#define M_L        0 
#define M_R        1  
 

const double ROVER_D = 0.0600/2;  //0.0607 //0.0581 //0.06
const double TIRE_CIRCUMFERENCE = 128.8;  //128.8 //129.6
const double ENC_COUNTS_PER_TURN = 2376;//2360;//2376.048 2（外ギア）*3pulse*4逓倍*99.002（内ギア）;
const double ENC_PER_MM = ENC_COUNTS_PER_TURN/TIRE_CIRCUMFERENCE; //単位距離当たりのエンコーダ値

//走行系のパラメータ。関数pidControl()で走行する際にのみ作用します。
extern float std_motor_param[]; //ユーザ設定の初期値
extern float motor_param[];     //発揮値

enum MotorList{
  F_L = 0,
  F_R = 1,
  R_L = 2,
  R_R = 3
};

enum IndexOfMotorParam{
  MAX_SPEED = 0,
  MAX_RAD   = 1,
  K_V2MP    = 2,
  K_P       = 3,
  K_I       = 4,
  K_D       = 5,
  PAD_DEAD  = 6,
  PAD_MAX   = 7,
  MAX_ACC   = 8
};

extern double  ctl_v_com[2];
extern double v_com[2];
extern int16_t m_com[2];
extern int16_t prev_m[2];
extern double v_enc[2];
extern double v_diff[2];
extern double avr_v[2];

extern uint8_t pen_status;

int setO_EN(int data);
int setO_EN(int data, Wrc021* memmap);
int getO_EN();
int getO_EN(Wrc021* memmap);
int setCtrlMode(int ctrl_mode);
int setCtrlMode(int ctrl_mode, Wrc021* memmap);
int getCtrlMode();
int getCtrlMode(Wrc021* memmap);
void memCom2V();
void ctl2Vcom();
void getRoverV();
void posControl();
void resetOdom();
uint16_t chkBumper(Wrc021* memmap);
void setMortorParam2Std();
void penUp();
void penDawn();

#endif
