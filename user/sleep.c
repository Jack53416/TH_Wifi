/*
 * sleep.c
 *
 *  Created on: 24.09.2016
 *      Author: Jacek
 */
#include "sleep.h"

uint32_t sleepTime=2; // deir


void ICACHE_FLASH_ATTR fallAsleep(modes wakupType)
{
	if(wifi_station_get_connect_status() == STATION_GOT_IP)
	{
		wifi_station_disconnect();
	}
	system_deep_sleep_set_option(wakupType);
	if(wakupType == NO_RF_CALIBRATION)
	{
		if(wifi_station_dhcpc_status()!=DHCP_STOPPED)
				wifi_station_dhcpc_stop();
		if(wifi_get_opmode_default() != NULL_MODE)
			wifi_set_opmode(NULL_MODE);
		/*if(wifi_fpm_get_sleep_type()!= NONE_SLEEP_T)
		{
			wifi_fpm_set_sleep_type (NONE_SLEEP_T);
			wifi_fpm_auto_sleep_set_in_null_mode(0);
		}*/
		if(readParams()->flags.setupWifi == true)
		{
			copyParams();
			getCurrParPtr()->flags.setupWifi = false;
			storeParams();
		}
	}


	if(readParams()->flags.configMode || readParams()->flags.sendNow)
	{
		copyParams();
		getCurrParPtr()->flags.sendNow=false;
		getCurrParPtr()->flags.configMode=false;
		storeParams();
	}
	SET_PERI_REG_MASK(UART_CONF0(0), UART_TXFIFO_RST);//RESET FIFO
	CLEAR_PERI_REG_MASK(UART_CONF0(0), UART_TXFIFO_RST);

	if(!rtcSetTimer(readParams()->sensorData.sleepTime_s/60) || readParams()->sensorData.sleepTime_s < 60)
	{
		ets_uart_printf("RTC SLEEP FAILED! Falling asleep using standard rtc\r\n");
		system_deep_sleep_instant(readParams()->sensorData.sleepTime_s*1000000);
	}
	else
	{
		ets_uart_printf("Sleeping using RTC\r\n");
		system_deep_sleep_instant(0); // in seconds now
	}


}

uint32_t ICACHE_FLASH_ATTR getSleepTime(){
	return readParams()->sensorData.sleepTime_s;
}

void ICACHE_FLASH_ATTR setSleepTime(uint32_t newTime)
{
	sleepTime=newTime;
	copyParams();
	getCurrParPtr()->sensorData.sleepTime_s=sleepTime;
	storeParams();
}
