/*
 * sensor.h
 *
 *  Created on: 15.09.2016
 *      Author: Jacek
 */

#ifndef INCLUDE_DRIVER_SENSOR_H_
#define INCLUDE_DRIVER_SENSOR_H_
#include "user_config.h"
#include "driver/i2c.h"
#include "driver/i2c_sht21.h"
#include "tcp.h"
#include "data_storage.h"
#include "sleep.h"
#include "driver/rtc_i2c.h"

#define SHT_POWER_UP_TIME_US 15000
#define ALERT_MES_INTERVAL 60*20

void ICACHE_FLASH_ATTR sensorTimerFunc(void *arg);
bool ICACHE_FLASH_ATTR SHT21FirstInit(void);
uint8_t ICACHE_FLASH_ATTR SHTreadUserRegister (void);
bool ICACHE_FLASH_ATTR SHTwriteUserRegister(uint8_t commandResolution);
void ICACHE_FLASH_ATTR sensorInit();
uint16_t ICACHE_FLASH_ATTR readAdc();
bool ICACHE_FLASH_ATTR areTresholdsExceeded(int16_t cTemp, uint16_t cHum);

#endif /* INCLUDE_DRIVER_SENSOR_H_ */
