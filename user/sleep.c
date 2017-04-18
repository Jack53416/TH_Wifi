/*
 * sleep.c
 *
 *  Created on: 24.09.2016
 *      Author: Jacek
 */
#include "sleep.h"

/******************************************************************************
 * FunctionName : fallAsleep
 * Description  : Function puts the device into sleep mode, clears all the flags
 * 				  ,closes any wifi connection and clears UART buffers
 * Parameters   : wakupType -- indicates what RF circuit will do on the device
 * 				  wakup
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR fallAsleep(modes wakupType)
{
	if(wifi_station_get_connect_status() == STATION_GOT_IP)
	{
		wifi_station_disconnect();
	}
	if(wifi_station_dhcpc_status()!=DHCP_STOPPED)
			wifi_station_dhcpc_stop();
	if(wifi_get_opmode_default() != NULL_MODE)
		wifi_set_opmode(NULL_MODE);

	if(readParams()->flags.configMode || readParams()->flags.sendNow)
	{
		copyParams();
		getCurrParPtr()->flags.sendNow=false;
		getCurrParPtr()->flags.configMode=false;
		storeParams();
	}

	if(wakupType == RF_DISABLED)
	{
		if(readParams()->flags.setupWifi == true)
		{
			copyParams();
			getCurrParPtr()->flags.setupWifi = false;
			storeParams();
		}
	}

	SET_PERI_REG_MASK(UART_CONF0(0), UART_TXFIFO_RST);//RESET FIFO
	CLEAR_PERI_REG_MASK(UART_CONF0(0), UART_TXFIFO_RST);

	system_deep_sleep_set_option(wakupType);

	if(!rtcSetTimer(readParams()->sensorData.sleepTime_s/60) || readParams()->sensorData.sleepTime_s < 60)
	{
		ets_uart_printf("RTC SLEEP FAILED! Falling asleep using standard rtc\r\n");
		system_deep_sleep(readParams()->sensorData.sleepTime_s*1000000);
	}
	else
	{
		ets_uart_printf("Sleeping using RTC\r\n");
		system_deep_sleep(0); // in seconds now
	}


}

/******************************************************************************
 * FunctionName : getSleepTime
 * Description  : Gets currently stored sleep time in seconds
 * Parameters   : none
 * Returns      : sleep time in seconds
*******************************************************************************/

uint32_t ICACHE_FLASH_ATTR getSleepTime(){
	return readParams()->sensorData.sleepTime_s;
}

/******************************************************************************
 * FunctionName : setSleepTime
 * Description  : Sets the new sleep time and stores it in the memory
 * Parameters   : newTime -- new sleep time in seconds
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR setSleepTime(uint32_t newTime)
{
	uint32_t sleepTime=2;
	sleepTime=newTime;
	copyParams();
	getCurrParPtr()->sensorData.sleepTime_s=sleepTime;
	storeParams();
}
