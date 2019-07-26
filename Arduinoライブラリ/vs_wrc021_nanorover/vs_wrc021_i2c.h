/**
 * VS-WRC021上のESP32とSTM32間のi2c通信に関するライブラリ
 */
#ifndef WRC021_I2C_H
#define WRC021_I2C_H

#include <Arduino.h>
#include <Wire.h>
#include "vs_wrc021_memmap.h"

void i2cMasterInit();
void i2cSlaveInit(uint8_t addr);





#endif /* I2C_H */
