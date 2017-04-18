/**
 * rtc_i2c.h
 *
 *  Created on: 26.11.2016
 *      Author: Jacek
 */

/**
 *  File is a collection of function that handles the PCF8653 RTC module from NXP company.
 *  For detalis see : http://www.nxp.com/documents/data_sheet/PCF8563.pdf
 */

#ifndef INCLUDE_DRIVER_RTC_I2C_H_
#define INCLUDE_DRIVER_RTC_I2C_H_

#include "driver/i2c.h"
#include <time.h>

#define RTC_WRITE_ADDRESS 0xA2
#define RTC_READ_ADDRESS 0xA3
#define SECONDS_REG_ADDR 0x02
#define MINUTE_ALARM_REG_ADDR 0x09
#define HOUR_ALARM_REG_ADDR 0xA
#define CONTROL_REG2_ADDR 0x01
#define TIMER_CONTROL_REG_ADDR 0x0E


bool ICACHE_FLASH_ATTR rtcSetTime(struct tm* timeInfo);
bool ICACHE_FLASH_ATTR rtcGetTime(struct tm* timePtr);
time_t ICACHE_FLASH_ATTR rtcGetUnixTime();
bool ICACHE_FLASH_ATTR rtcSaveUnixTime(const time_t* rawTime);
bool ICACHE_FLASH_ATTR rtc_init();
bool ICACHE_FLASH_ATTR rtcSetTimer(uint8_t time_m); // time in minutes!
bool ICACHE_FLASH_ATTR rtcDisableTimer();
bool ICACHE_FLASH_ATTR waitForAck();
uint8_t ICACHE_FLASH_ATTR decToBcd(uint8_t val);
uint8_t ICACHE_FLASH_ATTR bcdToDec(uint8_t val);

#endif /* INCLUDE_DRIVER_RTC_I2C_H_ */
