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
#define MEASUREMENT_PACKET_SIZE 28



void ICACHE_FLASH_ATTR sensor_timerfunc(void *arg);
bool ICACHE_FLASH_ATTR first_SHT21_Init(void);
uint8_t ICACHE_FLASH_ATTR read_user_register (void);
bool ICACHE_FLASH_ATTR write_user_register (uint8_t command_resolution);
void ICACHE_FLASH_ATTR sensorInit();
uint16_t ICACHE_FLASH_ATTR readAdc();
bool ICACHE_FLASH_ATTR areTresholdsExceeded(int16_t cTemp, uint16_t cHum);

#endif /* INCLUDE_DRIVER_SENSOR_H_ */
