#include "vs_wrc021_ble.h"
#include "vs_wrc021_memmap.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>




//BLE名前空間
namespace ble {

  BLEServer *megarover_server = NULL;
  BLECharacteristic *megarover_tx_characteristic;
  
  bool device_connected = false;
  bool old_device_connected = false;
  std::string rx_value = "";
  int  rx_count;
  char rcv_msg[MAP_SIZE];
  bool get_msg = false;




  void MegaroverBleServerCallbacks::onConnect(BLEServer* pServer) {
    Serial.println("connect");
    device_connected = true;
  };

  void MegaroverBleServerCallbacks::onDisconnect(BLEServer* pServer) {
    Serial.println("disconnect");
    device_connected = false;
  }



  void MegaroverBleCallbacks::onWrite(BLECharacteristic *p_characteristic) {
  
    rx_value += p_characteristic->getValue();
    rx_count = rx_value.length() -1;

    if(rx_value[rx_count] != '\n'){
      get_msg = false;
      return;
    }

    const char* msg_head = rx_value.c_str();

    int i;
    for(i = 0; i <= rx_count; i++){
      rcv_msg[i] = *(msg_head+i); 
    }

    rx_value.erase(rx_value.begin(), rx_value.begin() + rx_count+1);

    get_msg = true;
    
    

  }

  void bleInit(){
    BLEDevice::init("Nanorover BLE UART");
  }

  void bleInit(char name[]){
    // Create the BLE Device
    BLEDevice::init(name);

    // Create the BLE Server
    megarover_server = BLEDevice::createServer();
    megarover_server->setCallbacks(new MegaroverBleServerCallbacks());

    // Create the BLE Service
    BLEService *megarover_service = megarover_server->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    megarover_tx_characteristic = megarover_service->createCharacteristic(
		  								CHARACTERISTIC_UUID_TX,
			  							BLECharacteristic::PROPERTY_NOTIFY
				  					);
                      
    megarover_tx_characteristic->addDescriptor(new BLE2902());

    BLECharacteristic * megarover_rx_characteristic = megarover_service->createCharacteristic(
		  									CHARACTERISTIC_UUID_RX,
			  								BLECharacteristic::PROPERTY_WRITE
		  								);

    megarover_rx_characteristic->setCallbacks(new MegaroverBleCallbacks());

    // Start the service
    megarover_service->start();

    // Start advertising
    megarover_server->getAdvertising()->start();
    Serial.println("Waiting a client connection to notify...");
  }

  int sendMultiByte(char msg[], int length ){
    int return_value = 0;
    
    if (device_connected) {
      msg[length] = '\n';
      
      megarover_tx_characteristic->setValue((uint8_t*)msg, length+1);
      megarover_tx_characteristic->notify();

      return_value =  1;  
	  }

    // disconnecting
    if (!device_connected && old_device_connected) {
        delay(50); // give the bluetooth stack the chance to get things ready
        megarover_server->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        old_device_connected = device_connected;
    }
    // connecting
    if (device_connected && !old_device_connected) {
		// do stuff here on connecting
        old_device_connected = device_connected;
    }

    return return_value;;
  }

  
}
