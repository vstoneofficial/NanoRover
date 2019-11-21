/**
 * スケッチ名：nanorover_common
 * バージョン：1.01
 * 概要：
 * 　ナノローバーを外部のデバイスから制御する際に汎用的に使用可能なスケッチです。
 * 　
 */


/*******************************************************************
 * 本プログラムではライブラリのサイズの関係で、複数の無線通信規格を同時に使用することはできません。
 * WRC021_WIRELESS_MODEに使用する規格を定義してください。
 * 
 * 〇WiFi使用時
 * WRC021_WIRELESS_MODE を USE_WIFI で定義してください。
 * また、アクセスポイントのSSIDとパスワードも設定してください(L67,68)。
 * 
 * 〇BLE使用時
 * WRC021_WIRELESS_MODE を USE_BLE で定義してください。
 * 
 * 〇Bluetooth clasic使用時
 * WRC021_WIRELESS_MODE を USE_BTC で定義してください。
 * 
 * 〇無線通信を使用しないとき
 * WRC021_WIRELESS_MODE を WIRELESS_OFF で定義してください。
 */ 
#define WIRELESS_OFF 0
#define USE_WIFI     1
#define USE_BLE      2
#define USE_BTC      3
#define WRC021_WIRELESS_MODE   WIRELESS_OFF

/*******************************************************************
 * 本製品はROSから制御することが可能です。
 * WiFi接続と有線シリアル接続を選ぶことができます。両方同時には使えません。
 * WRC021_ROS_SERIAL_MODEに使用する規格を定義してください。
 * 
 * 〇WiFi接続時
 * WRC021_ROS_SERIAL_MODE を MODE_WIFI で定義してください。
 * また、WRC021_WIRELESS_MODE を USE_WIFI で定義してください。
 * 
 * 〇有線シリアル使用時
 * WRC021_ROS_SERIAL_MODE を MODE_SERIAL で定義してください。
 * 
 * 〇ROSを使用しないとき
 * WRC021_ROS_SERIAL_MODE を MODE_OFF で定義してください。
 */
#define MODE_OFF    0
#define MODE_WIFI   1
#define MODE_SERIAL 2
#define WRC021_ROS_SERIAL_MODE MODE_OFF


#include <WiFi.h>
#include <vs_wrc021_nanorover.h>
#include <vs_wrc021_ros.h>
#include <std_msgs/String.h>
#include <geometry_msgs/Twist.h>

/*******************************************
 * プロトタイプ宣言
 */
void twistCallBack(const geometry_msgs::Twist& );
void setRoverOdo();
void LED(int cmd);



/*******************************************
 * WiFi関連の設定
 */
const char* ui_path = "/index.html";  //HTMLコントローラのパス
const char* ssid     = "";            //WiFi使用時に接続するアクセスポイントのSSIDを設定してください
const char* password = "";            //WiFi使用時に接続するSSIDのパスワードを設定してください

/*******************************************
 * ROS接続設定
 */
IPAddress ROSserver(192,168,1,1);        //rosserial socket serverのIPアドレスを設定してください。
const uint16_t serverPort = 11411;       //rosserial socket serverのポートを設定してください。

/*******************************************
 *  ROS用の各種定義
 */
ros::NodeHandle nh;     //ノードハンドル

geometry_msgs::Twist rover_odo;
ros::Publisher pub_odo("rover_odo", &rover_odo);
ros::Subscriber<geometry_msgs::Twist> sub_twist("rover_twist", &twistCallBack);

int publishInterval = millis();   //ROSパブリッシュのタイミング制御
int spinOnceInterval = millis();  //ROS spinOnceのタイミング制御


/*******************************************
 * セットアップ
 */
void setup()
{

  //モータ制御関連パラメータの設定(適切な値を設定してください)
  std_motor_param[MAX_SPEED] = 0.1;     //最大速度[m/s]
  std_motor_param[MAX_RAD]   = 2.51;    //最大旋回速度[rad/s]
  std_motor_param[K_P]       = 1.4;     //Pゲイン
  std_motor_param[K_I]       = 0.2;     //Iゲイン
  std_motor_param[K_D]       = 1.2;     //Dゲイン
  std_motor_param[PAD_DEAD]  = 30.0;    //コントローラデッドゾーン（絶対値）
  std_motor_param[PAD_MAX]   = 127.0;   //コントローラ入力最大値（絶対値)

  //標準LED色の設定
  stdLedColor.number = 0x00;
  stdLedColor.r      = 23;
  stdLedColor.g      = 23;
  stdLedColor.b      = 23;
  
  

  //[ROS]接続モード別設定値の設定
  #if (WRC021_ROS_SERIAL_MODE == MODE_WIFI)
    // using ROS over Wi-Fi
    nh.getHardware()->setConnection(ROSserver, serverPort);
    Serial.begin(115200);
  #else
    // using ROS via USB-Serial or do not use ROS
    nh.getHardware()->setBaud(115200);
    nh.getHardware()->setRead(readMsg4ROS); 
  #endif

  //各無線通信機能のセットアップ
  #if (WRC021_WIRELESS_MODE == USE_WIFI)
    wifiInit((char* )ssid, (char* )password);
    serverInit((char* )ui_path);
  #elif (WRC021_WIRELESS_MODE == USE_BLE)
    ble::bleInit("Nanorover BLE");          //BLEのデバイス名を引数に設定
  #elif (WRC021_WIRELESS_MODE == USE_BTC)
    btc::btcInit("Nanorover BTC");          //Bluetooth classicのデバイス名を引数に設定
  #endif

  delay(50);

  //[ROS]ros serial初期設定
  nh.initNode();  //MODE_SERIAL or MODE_OFFの場合、この内部でSerail.begin()が行われます

  //[ROS]publish & subscribeの設定
  nh.advertise(pub_odo);
  nh.subscribe(sub_twist);

  delay(10);


  //I2Cセットアップ
  i2cMasterInit();

  //メモリマップ初期化
  wrc021.initMemmap(8.0);

  wrc033LedInit();

  //Syncタスクセットアップ
  delay(200);

  setupInterruptTimer(); 
  //setUpSync();
  
  delay(100);

  //エンコーダの初期化
  clearEnc();
  delay(50);


}


/*******************************************
 * メインループ
 */
void loop(){

  int code = NO_INPUT;    //状態判定のためのコード


  if(Serial.available()){                                                                //シリアル入力があった場合、
    code = SERIAL_ACCES;                                                                //状態コードをSERIAL_ACCESとする。
    
    if(!persMsgViaSerial()){                                                          //シリアル入力を解釈する。
      //無効なシリアルアクセス                                                          //
      code = NO_INPUT;                                                                //無効なシリアルアクセスであった場合は状態コードをNO_INPUTとする
    }
    
  }else if(WRC021_ROS_SERIAL_MODE == MODE_WIFI && WiFi.status() == WL_CONNECTED){       //Wi-Fi接続でROSを使用し、Wi-Fiアクセスポイントとの接続が確立していて、
    if(millis() - spinOnceInterval > 10){                                               //かつ前回の処理から10ms以上経過していた場合、
      spinOnceInterval = millis();                                                      //
      if(nh.spinOnce() == 0){                                                          //ros_spinOnce()を実行し
        //ROSからの指令値取得                                                             //ROSからの指令値であれば
        code = ROS_CTRL;                                                                //状態コードをROS_CTRLとする
      }
    }

  }else if(WRC021_WIRELESS_MODE == USE_BLE){                                            //BLE使用設定で、
    if(ble::get_msg){                                                                   //BLEで取得した入力があれば、
      if(checkI2cAddrOfMsg(ble::rcv_msg, ble::rx_count, VIA_BLE)){                      //有効なアクセスかどうかを判定し処理する
        code = BLE_ACCES;                                                               //状態コードをBLE_ACCESとする
      }else{                                                                            //
        char err_msg[] = "ERR";                                                         //無効なアクセスであれば
        ble::sendMultiByte(err_msg, 3);                                                 //エラーメッセージを送信する
      }                                                                                 //
      ble::get_msg = false;                                                             //フラグのリセット
    }
  
  }else if(WRC021_WIRELESS_MODE == USE_BTC){                                            //Bluetooth Classic使用設定で
    if(btc::readMsg()){                                                                 //Bluettoth Classicでの有効なアクセスがあれば処理をして
      code = BTC_ACCES;                                                                 //状態コードをBTC_ACCESとする
    }

  }else{
    if(WiFi.status() == WL_CONNECTED){                                                  //Wi-Fiアクセスポイントとの接続が確立していていれば
      code = checkClient();                                                             //HTTPアクセスの確認および処理を行う

    }
  }

  if(WRC021_ROS_SERIAL_MODE == MODE_SERIAL /*&& !rcvMsg4ROS.empty()*/){                     //rosserialを有線シリアル接続で使用するなら
    spinOnceInterval = millis();                                                      //
    if(nh.spinOnce() == 0){                                                          //spinOnceを実行する
      //ROSからの指令値取得                                                             //ROSからの指令値であれば
      code = ROS_CTRL;                                                                //状態コードをROS_CTRLとする。
    }
  }


  //スタートスイッチが押されていたら
  static uint32_t pushSwitchTime = millis();
  static uint32_t upSwitchTime   = millis();
    
    if(wrc021.u8Map(0x7f) == 0x01){

      //チャタリング対策(前回離されてから300ms以内なら押しっぱなしと判定)
      if(millis() - upSwitchTime > 300){

        int blinkCount = 0;
        for(blinkCount = 0; blinkCount < 5; blinkCount++){
          delay(20);
          setLedColor(blackTop);
          wrc033.sendWriteMap();
          delay(20);
          setLedColor(orangeTop);
          wrc033.sendWriteMap();
        }

        pushSwitchTime = millis();

      }

      //3秒以上長押しで電源OFF
      while(wrc021.u8Map(0x7f) == 0x01){
        wrc021.readMemmap(0x7f, 1);
        if(millis() - pushSwitchTime >= 3000 && !((millis() - pushSwitchTime)%5)){
          changeLed2TurnOff();
        }
      }

      upSwitchTime = millis();
    
    }
    

    if(isInterrupt){
      isInterrupt = false;

      wrc021.readMemmap(0x7f, 1);

      resetOdom();

      checkPenUpDown();

      memCom2V();
      posControl();

      //ESP32のメモリマップのうち、書込み要求のあったものをSTM32のメモリマップへ送信する
      wrc021.sendWriteMap();                //メモリマップの送信を実行する  

      //ESP32のLED用メモリマップのうち、書込み要求のあったものをWRC033に送信する
      wrc033.sendWriteMap();

    }

  //ROSメッセージの送信
  if(WRC021_ROS_SERIAL_MODE != MODE_OFF && nh.connected()){
    if(millis() - publishInterval > 10){
      publishInterval = millis();
      setRoverOdo();
      pub_odo.publish( &rover_odo);
    }
  }

  delay(1);

}

/*******************************************
 * ROSメッセージ rover_twist のコールバック
 */ 
void twistCallBack(const geometry_msgs::Twist& msg){
  wrc021.s16Map(MS16_S_XS, (int16_t)(msg.linear.x*1000));
  wrc021.s16Map(MS16_S_ZS, (int16_t)(msg.angular.z*1000));

  return;
}

/*******************************************
 * ROSメッセージ rover_odo へのオドメトリ情報の入力
 */
void setRoverOdo(){
  rover_odo.linear.x  = ((-1.0*avr_v[M_R] + (avr_v[M_L]))/2.0);
  rover_odo.angular.z = ((-1.0*avr_v[M_R] - (avr_v[M_L]))/(2.0*ROVER_D));
  return;
}





