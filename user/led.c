/*
 * led.c
 *
 *  Created on: 21.11.2016
 *      Author: Jacek
 */
#include "led.h"

LOCAL os_timer_t ledTimer;
uint8_t activeLed=GREEN;


void ICACHE_FLASH_ATTR initLed()
{
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
}

void ICACHE_FLASH_ATTR lightGreen()
{
	GPIO_OUTPUT_SET(GREEN, HIGH);
	GPIO_OUTPUT_SET(RED, LOW);
}
void ICACHE_FLASH_ATTR lightRed()
{
	GPIO_OUTPUT_SET(RED, HIGH);
	GPIO_OUTPUT_SET(GREEN, LOW);
}
void ICACHE_FLASH_ATTR LedTurnOff()
{
	//os_timer_disarm(&ledTimer);
	GPIO_OUTPUT_SET(RED, LOW);
	GPIO_OUTPUT_SET(GREEN, LOW);
}

void ICACHE_FLASH_ATTR blinkRed(uint16_t period_ms, uint8_t pulseWidth)
{
	LedTurnOff();
	activeLed=RED;
}

void ICACHE_FLASH_ATTR blinkGreen(uint16_t period_ms, uint8_t pulseWidth)
{
	LedTurnOff();
	activeLed=GREEN;
}

void ICACHE_FLASH_ATTR blinkOnce(uint8_t blinkDuration_ms)
{
	if(activeLed == RED)
	{
		GPIO_OUTPUT_SET(RED, HIGH);
		os_delay_us(blinkDuration_ms*1000);
		GPIO_OUTPUT_SET(RED, LOW);
	}
	else
	{
		GPIO_OUTPUT_SET(GREEN, HIGH);
		os_delay_us(blinkDuration_ms*1000);
		GPIO_OUTPUT_SET(GREEN, LOW);
	}
}
void ICACHE_FLASH_ATTR blinkerCB(void *arg)
{
	blinkOnce(BLINK_PW);
}

void ICACHE_FLASH_ATTR signalizeStatus(STATUS status)
{
	Params* parPtr=NULL;
	parPtr=readParams();
	switch(status)
	{
		case GOOD:
			if(parPtr->flags.sendNow)
				blinkGreen(BLINK_PERIOD,BLINK_PW);
			else if(parPtr->flags.configMode)
				lightGreen();
			break;
		case ERROR:
			if(parPtr->flags.sendNow)
				blinkRed(BLINK_PERIOD,BLINK_PW);
			else if(parPtr->flags.configMode)
				lightRed();
			break;
		default:
			if(parPtr->flags.configMode || parPtr->flags.sendNow)
			{
				initLed();
				lightGreen();
			}
			if(parPtr->flags.sendNow)
			{
				blinkOnce(BLINK_PW);
				os_timer_disarm(&ledTimer);
				os_timer_setfn(&ledTimer, (os_timer_func_t *)blinkerCB, (void *) 0);
				os_timer_arm(&ledTimer, BLINK_PERIOD, 1);
			}
			break;

	}
}

