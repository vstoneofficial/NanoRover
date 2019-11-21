#include "vs_wrc021_motor.h"
#include "vs_wrc021_memmap.h"
//#include "vs_wrc021_spi.h"
#include "vs_wrc031_spiffs.h"
#include <Arduino.h>
#include <math.h>
#include <vector>
#include <string>
#include <cstdint>

double  ctl_v_com[2] = {0.0, 0.0};//制御器からの速度指令値
double  v_com[2];    //左右輪の速度指令値
int16_t m_com[2];    //左右モータへの出力指令値

int32_t enc[2] = {0, 0};
int32_t old_enc[2] = {0, 0};


double sum_v[2][5] = {{0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}};
double avr_v[2] = {0.0, 0.0};
uint32_t old_micros = 0;


double v_enc[2]  = {0.0, 0.0};
double v_diff[2] = {0.0, 0.0};

uint32_t pos_time = 0;
uint32_t pen_moving_time = 0;



float std_motor_param[] = { 0.1,  //最大並進速度[m/s]
                            3.14, //最大旋回量[rad]
                            520, //速度からモータ出力値への変換係数
                            1.6,  //Pゲイン
                            0.2,  //Iゲイン
                            1.6,  //Dゲイン
                            30,   //コントローラデッドゾーン(絶対値)
                            127,  //コントローラ入力最大値(絶対値)
                            1.5   //タイヤ回転速度に対する最大加速度[m/s^2]
                          };

float motor_param[] = { 0.1,  //最大並進速度[m/s]
                        3.14, //最大旋回量[rad]
                        520, //速度からモータ出力値への変換係数
                        1.6,  //Pゲイン
                        0.2,  //Iゲイン
                        1.6,  //Dゲイン
                        30,   //コントローラデッドゾーン(絶対値)
                        127,  //コントローラ入力最大値(絶対値)
                        1.5   //タイヤ回転速度に対する最大加速度[m/s^2]
                      };


//出力ON/OFFを設定： 0->両方OFF 1->CH0のみON 2->CH1のみON 3->両方ON
int setO_EN(int data){
  setO_EN(data, &wrc021);

  return data;
}

int setO_EN(int data, Wrc021* memmap){

  //memmap->write1Byte(MU8_O_EN, data);
  memmap->u8Map(MU8_O_EN, data);

  return data;
}

//出力ON/OFFの設定を確認
int getO_EN(){
  int return_value = 0;
  return_value += getO_EN(&wrc021);

  return return_value;
}

int getO_EN(Wrc021* memmap){
  //memmap->readMemmap(MU8_O_EN, 1);

  return memmap->u8Map(MU8_O_EN);
}

//直接制御モードと位置制御モードを切り替え：　1->直接制御 0->位置制御
int setCtrlMode(int ctrl_mode){
  setCtrlMode(ctrl_mode, &wrc021);

  return ctrl_mode;
}

int setCtrlMode(int ctrl_mode, Wrc021* memmap){

  //現在の制御モードを確認
  if(getCtrlMode(memmap)){
    //現在はPWM制御
    if(ctrl_mode == MODE_POS){ //位置制御に変更なら
      //出力をOFFに設定
      setO_EN(OFF_OFF, memmap);

      //位置制御に設定
      //memmap->write4Byte(MS16_FB_PG0, KP_NORMAL);
      //memmap->write1Byte(MU8_TRIG, OFF_OFF);

      memmap->u32Map(MS16_FB_PG0, KP_NORMAL);
    }
  }else{
    //現在は位置制御
    if(ctrl_mode == MODE_PWM){ //PWM制御に変更なら
      //出力をOFFに設定
      setO_EN(OFF_OFF, memmap);

      //直接制御に設定
      //memmap->write4Byte(MS16_FB_PG0, KP_ZERO);
      //memmap->write1Byte(MU8_TRIG, ENC_RESET);

      memmap->u32Map(MS16_FB_PG0, KP_ZERO);
    } 
  }
  
  return ctrl_mode;
}

//現在の制御モードを取得
int getCtrlMode(){
  int return_value = 0;;
  return_value += getCtrlMode(&wrc021);

  return return_value;
}

int getCtrlMode(Wrc021* memmap){
  //memmap->readMemmap(MS16_FB_PG0, 4);
  if(memmap->u32Map(MS16_FB_PG0)){
    return MODE_POS;
  }else{
    return MODE_PWM;
  }
}


//メモリマップ記載の速度値から、各輪の速度を算出する
void memCom2V(){
  double mem_com_x = wrc021.s16Map(MS16_S_XS)/1000.0;  
  double mem_com_z = wrc021.s16Map(MS16_S_ZS)/1000.0;  //[m/s]

  setMortorParam2Std();

  ctl_v_com[M_L] = (mem_com_x - ROVER_D*mem_com_z);
  ctl_v_com[M_R] = -1.0*(mem_com_x + ROVER_D*mem_com_z);

  if(fabs(ctl_v_com[M_L]) > motor_param[MAX_SPEED] || fabs(ctl_v_com[M_R]) > motor_param[MAX_SPEED]){
    double ctl_v_com_max = fabs(ctl_v_com[0]);
    int i;
    for(i = 1; i < 2; i++){
      if(ctl_v_com_max < fabs(ctl_v_com[i])){
        ctl_v_com_max = fabs(ctl_v_com[i]);
      }
    }

    for(i = 0; i < 2; i++){
       ctl_v_com[i] /= (ctl_v_com_max/motor_param[MAX_SPEED]);
    }
  }


}


//コントローラからの速度指令値に対して加速度制限を実施し、PID制御器への速度指令値を算出する
void ctl2Vcom(){

  //各車輪のv_comとctl_v_comの差を求める
  int i;
  double v_diff[2];
  for(i = 0; i < 2; i++){
    v_diff[i] = ctl_v_com[i] - v_com[i];
  }

  //現在のv_comとctl_v_comの差が最も大きい車輪を探す
  double max_v_diff = fabs(v_diff[M_L]);
  for(i = 1; i < 2; i++){
    if(fabs(v_diff[i]) > max_v_diff){
      max_v_diff = fabs(v_diff[i]);
    }
  }

  //前回から今回の処理までの経過時間
  static double prev_micros = micros();
  double new_micros = micros(); //[μsec]
  double elapsed_time = (new_micros - prev_micros); //[μsec]
  if(elapsed_time < 0){
    elapsed_time = (new_micros + (UINT64_MAX - prev_micros)); //[μsec]
  }

  //差が最も大きい車輪の加速度がMAX_ACCとなるように計算する
  double add_v = 0.0;
  if(max_v_diff != 0.0){
    for(i = 0; i < 2; i++){
      add_v = ((motor_param[MAX_ACC] * (v_diff[i]/max_v_diff)*elapsed_time)/1000000.0);
      if(fabs(v_diff[i]) >= fabs(add_v)){
        v_com[i] = v_com[i] + add_v;
      }else{
        v_com[i] = ctl_v_com[i];
      }      
    }
  }


  
  for(i = 0; i < 2; i++){
    //ctl_v_com = 0.0 かつ v_com < 0.02 なら v_com = 0.0
    if(ctl_v_com[i] == 0.0 && fabs(v_com[i]) <= 0.001){
      v_com[i] = 0.0;
    }
  }  

  prev_micros = new_micros;

}

//エンコーダ値を取得する
void getEncoderValue(){

  wrc021.readMemmap(MS32_M_POS0, 8);
  //wrc021.readMemmap(MS32_M_POS1, 4);
}

//エンコーダ値から移動速度を求める
void getRoverV(){
    
  getEncoderValue();
  enc[M_L] = (int32_t)wrc021.s32Map(MS32_M_POS0);
  enc[M_R] = (int32_t)wrc021.s32Map(MS32_M_POS1);

  uint32_t new_micros;
  new_micros = micros();

  int i;
  for(i = 0; i < 2; i++){
    v_enc[i] = (((double)(enc[i]-old_enc[i])/ENC_COUNTS_PER_TURN)*TIRE_CIRCUMFERENCE)/(double)((new_micros-old_micros)/1000.0);//[m/s]
    old_enc[i] = enc[i];
  }

  old_micros = new_micros;

  for(int i = 0; i < 2; i++){
    //エンコーダが異常に大きな値を吐いた場合は速度変化なしとして処理する
    if(fabs(v_enc[i]) > 2*motor_param[MAX_SPEED]){
      v_enc[i] = sum_v[i][0];
    }
    
    for(int j = 4; j > 0; j--){
      sum_v[i][4] += sum_v[i][j-1];
      sum_v[i][j] =  sum_v[i][j-1];
    }

    sum_v[i][4] += v_enc[i];
    avr_v[i]    =  sum_v[i][4]/5.0;
    sum_v[i][0] =  v_enc[i];
  }
}



double buf_enc_com[2];  //エンコーダ逐次目標値のストック
//速度指令値をベースにモータを位置制御
void posControl(){

  setCtrlMode(MODE_POS);
  setO_EN(ON_ON);
  ctl2Vcom();
  getRoverV();

  //Serial.println(v_com[M_L]);
  double e_v_com[2];  //速度指令値
  e_v_com[M_L] = v_com[M_L]*ENC_PER_MM*1000.0;
  e_v_com[M_R] = v_com[M_R]*ENC_PER_MM*1000.0;

  

  //前回から今回の処理までの経過時間
  static double prev_micros = micros();
  double new_micros = micros(); //[μsec]
  double elapsed_time = (new_micros - prev_micros); //[μsec]
  if(elapsed_time < 0){
    elapsed_time = (new_micros + (UINT32_MAX - prev_micros)); //[μsec]
  }

  //逐次速度指令値から逐次加算値を求める
  buf_enc_com[M_L] += (e_v_com[M_L]*elapsed_time)/1000000.0;
  buf_enc_com[M_R] += (e_v_com[M_R]*elapsed_time)/1000000.0;


  //逐次加算値の整数部をA_POSにセット   
  if(wrc021.checkWriteFlag(MS32_A_POS0)){
    //前回処理時の指令値がまだ書き込まれていないので
    wrc021.s32Map(MS32_A_POS0, (int32_t)wrc021.s32Map(MS32_A_POS0)+(int32_t)buf_enc_com[M_L]);
  }else{
    wrc021.s32Map(MS32_A_POS0, (int32_t)buf_enc_com[M_L]);
  }
  if(wrc021.checkWriteFlag(MS32_A_POS1)){
    //前回処理時の指令値がまだ書き込まれていないので
    wrc021.s32Map(MS32_A_POS1, (int32_t)wrc021.s32Map(MS32_A_POS1)+(int32_t)buf_enc_com[M_R]);
  }else{
    wrc021.s32Map(MS32_A_POS1, (int32_t)buf_enc_com[M_R]);
  }


  //出力方向（正回転/逆回転）に合わせて、出力オフセット
  if((int32_t)buf_enc_com[M_L] > 0){
    if(wrc021.s16Map(MS16_T_OUT0) <= 0){
      wrc021.s16Map(MS16_T_OUT0, 1500); //
    }
  }else if((int32_t)buf_enc_com[M_L] < 0){
    if(wrc021.s16Map(MS16_T_OUT0) >= 0){
      wrc021.s16Map(MS16_T_OUT0, -1500); //
    }
  }
  if((int32_t)buf_enc_com[M_R] > 0){
    if(wrc021.s16Map(MS16_T_OUT1) <= 0){
      wrc021.s16Map(MS16_T_OUT1, 1500); //
    }
  }else if((int32_t)buf_enc_com[M_R] < 0){
    if(wrc021.s16Map(MS16_T_OUT1) >= 0){
      wrc021.s16Map(MS16_T_OUT1, -1500); //
    }
  }

  buf_enc_com[M_L] -=  (int32_t)buf_enc_com[M_L];
  buf_enc_com[M_R] -=  (int32_t)buf_enc_com[M_R];

  wrc021.u8Map(MU8_TRIG, (wrc021.u8Map(MU8_TRIG) | 0x03));

  prev_micros = new_micros;

}

//MU8_TRIG b7: 1でWP_PX-WP_THとM_POS,T_POSをクリア
void resetOdom(){
  //MU8_TRIG b7 LOWならリターン
  if(!(wrc021.u8Map(MU8_TRIG) & 0x80)){
    return;
  }

  wrc021.s32Map(MS32_WP_PX, 0x00000000);
  wrc021.s32Map(MS32_WP_PY, 0x00000000);
  wrc021.s16Map(MS16_WP_TH, 0x00000000);

  old_enc[M_L] = 0;
  old_enc[M_R] = 0;
  enc[M_L] = 0;
  enc[M_R] = 0;

  //b7クリアフラグを折り、M_POS,T_POSのクリアフラグを建てる
  wrc021.write1Byte(MU8_TRIG, 0x0c);
  wrc021.u8Map(MU8_TRIG, wrc021.u8Map(MU8_TRIG) & 0x7F);

  return;

}

//バンパーor電源スイッチが反応していた場合、速度指令値を0にする
uint16_t chkBumper(Wrc021* memmap){

  //バンパーand電源スイッチの状態確認
  memmap->readMemmap(MU16_M_DI, 2);
  return memmap->u16Map(MU16_M_DI);
}


//std_motor_paramの値をmortor_paramに反映する
void setMortorParam2Std(){

  int i;
  for(i = 0; i < 7; i++){
    motor_param[i] = std_motor_param[i];
  }

  return;
}


uint8_t pen_status = 0x00; //ペンのアップダウンステータス 0->ダウン 1->アップ 2->遷移中
//ペンアップ
void penUp(){
  if(pen_status == 0x01 || pen_status > 0x02 ){
    return;
  }

  static int16_t penPower = -2200;

  if(pen_status != 0x02){
    penPower = -2200;
    wrc021.write1Byte(MU8_O_EN, 0x07);
    wrc021.write2Byte(MS16_T_OUT2, penPower);

    pen_status = 0x02;
    pen_moving_time = millis();
  }

  if(millis() - pen_moving_time < 200){//10//240//500
    penPower = -2200 - ((millis()-pen_moving_time)*5);
    wrc021.write2Byte(MS16_T_OUT2, penPower);
    //delay(2);
    return;
  }
  if(penPower < -150){
    penPower += 100;
    wrc021.write2Byte(MS16_T_OUT2, penPower);
    delay(2);
    return;
  }

  wrc021.write2Byte(MS16_T_OUT2, 0x0000);
  setO_EN(ON_ON, &wrc021);
  wrc021.u8Map(MU8_PEN_SW, 0x01);
  pen_status = 0x01;
  
  //delay(10);

  return;
}

//ペンダウン
void penDawn(){
  if(pen_status == 0x00 || pen_status > 0x02){
    return;
  }

  static int16_t penPower = 2200;

  if(pen_status != 0x02){
    penPower = 2200;
    wrc021.write1Byte(MU8_O_EN, 0x07);
    wrc021.write2Byte(MS16_T_OUT2, penPower);

    pen_status = 0x02;
    pen_moving_time = millis();
  }

  if(millis() - pen_moving_time < 200){//10//240//500
    penPower = 2200 + ((millis()-pen_moving_time)*5);
    wrc021.write2Byte(MS16_T_OUT2, penPower);
    //delay(2);
    return;
  }
  if(penPower > 150){
    penPower -= 100;
    wrc021.write2Byte(MS16_T_OUT2, penPower);
    delay(2);
    return;
  }

  wrc021.write2Byte(MS16_T_OUT2, 0x0000);
  setO_EN(ON_ON, &wrc021);
  wrc021.u8Map(MU8_PEN_SW, 0x00);
  pen_status = 0x00;

  //delay(10);

  return;
}



