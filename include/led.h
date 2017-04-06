/*
 * led.h
 *
 *  Created on: 21.11.2016
 *      Author: Jacek
 */
#include "user_config.h"
#include "data_storage.h"

#ifndef INCLUDE_LED_H_
#define INCLUDE_LED_H_

#define HIGH 1
#define LOW 0
#define RED 13
#define GREEN 12

enum STATUS{
	GOOD,
	ERROR,
	NONE
};

void ICACHE_FLASH_ATTR initLed(); // * in future * if red pin=1 red light, if green=1 green light
void ICACHE_FLASH_ATTR lightGreen();
void ICACHE_FLASH_ATTR lightRed();
void ICACHE_FLASH_ATTR LedTurnOff();
void ICACHE_FLASH_ATTR blinkRed(uint16_t period_ms, uint8_t pulseWidth);
void ICACHE_FLASH_ATTR blinkGreen(uint16_t period_ms, uint8_t pulseWidth);
void ICACHE_FLASH_ATTR blinkOnce(uint8_t blinkDuration_ms);

void blinkerCB();

void ICACHE_FLASH_ATTR signalizeStatus(STATUS status);

#endif /* INCLUDE_LED_H_ */
