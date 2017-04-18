/*
 * sleep.h
 *
 *  Created on: 24.09.2016
 *      Author: Jacek
 */

#ifndef INCLUDE_SLEEP_H_
#define INCLUDE_SLEEP_H_
#include "user_config.h"
#include "data_storage.h"
#include "driver/rtc_i2c.h"
typedef enum{
	RF_DEFAULT=0,
	RF_CALIBRATION=1,
	RF_NO_CALIBRATION=2,
	RF_DISABLED=4 // RF OFF actually
}modes;

void ICACHE_FLASH_ATTR fallAsleep(modes wakupType);
uint32_t ICACHE_FLASH_ATTR getSleepTime();
void ICACHE_FLASH_ATTR setSleepTime(uint32_t newTime);

#endif /* INCLUDE_SLEEP_H_ */
