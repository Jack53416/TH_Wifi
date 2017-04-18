/*
 * led.c
 *
 *  Created on: 21.11.2016
 *      Author: Jacek
 */
#include "led.h"

LOCAL os_timer_t ledTimer;
uint8_t activeLed=GREEN;

/******************************************************************************
 * FunctionName : initLed
 * Description  : Maps the GPIO 12 and GPIO13 as the output
 * Parameters   : none
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR initLed()
{
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
}
/******************************************************************************
 * FunctionName : lightGreen, lightRed
 * Description  : lights the diode continuously in desired color
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
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
/******************************************************************************
 * FunctionName : LedTurnOff
 * Description  : Sets both GPIOs connected to the LED to low
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR LedTurnOff()
{
	//os_timer_disarm(&ledTimer);
	GPIO_OUTPUT_SET(RED, LOW);
	GPIO_OUTPUT_SET(GREEN, LOW);
}
/******************************************************************************
 * FunctionName : blinkRed, blinkGreen
 * Description  : functions that changes the color of the diode or in other words
 * 				  which GPIO pin shold be high and which low
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR blinkRed()
{
	LedTurnOff();
	activeLed=RED;
}

void ICACHE_FLASH_ATTR blinkGreen()
{
	LedTurnOff();
	activeLed=GREEN;
}

/******************************************************************************
 * FunctionName : blinkOnce
 * Description  : Function that drives the diode in such a way that it is only
 * 				  active for the duration of blinkDuration_ms. In combination with
 * 				  timer it can effectively be used to set pulse width of blinking
 * 				  diode
 * Parameters   : blinkDuration_ms -- time during the LED remains active in
 * 				  miliseconds
 * Returns      : none
*******************************************************************************/
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

/******************************************************************************
 * FunctionName : signalizeStatus
 * Description  : Function that manages the LED mounted to the device.
 * 				  It distinguish between two modes of the device: conig and send now
 * 				  where it manages the diode silightly differently. By default (anything
 * 				  larger than 1 in status) it initializes the diode
 * Parameters   : enum status -- device status to be signalized by the diode
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR signalizeStatus(STATUS status)
{
	Params* parPtr=NULL;
	parPtr=readParams();
	switch(status)
	{
		case OK:
			if(parPtr->flags.sendNow)
				blinkGreen(BLINK_PERIOD,BLINK_PW);
			else if(parPtr->flags.configMode)
				lightGreen();
			break;
		case FAIL:
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

void ICACHE_FLASH_ATTR blinkerCB(void *arg)
{
	blinkOnce(BLINK_PW);
}


