/*
 * wifi.c
 *
 *  Created on: 14.09.2016
 *      Author: Jacek
 */
#include "wifi.h"
#include "led.h"

extern struct espconn conn1;
extern esp_tcp tcp1;

int wifi_err=0;
uint8_t retrysNr=0;
LOCAL os_timer_t retryTimer;
bool CONFIG=false;
uint16_t configActiveTime=0;

/******************************************************************************
 * FunctionName : setupWifi
 * Description  : It setups the wifi connection by inputing the ssID and password
 * 				  to the SDK defined structure. It also registers the handler function
 * 				  for wifi events, attempts to connect to the access point, starts dhcp
 * 				  and initializes wifiGuardian.
 * Parameters   : ssId -- wifi network name
 * 				  password -- wifi network password
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR setupWifi(const char* ssId, const char* password)
{
	struct station_config stationConfig;

	wifi_set_event_handler_cb(wifiEventHandler);
	wifi_set_opmode_current(STATION_MODE);

	if(wifi_station_dhcpc_status()!=DHCP_STOPPED)
		wifi_station_dhcpc_stop();

	wifi_station_disconnect();

	strncpy(stationConfig.ssid,ssId ,32);/*"piproject"*/
	strncpy(stationConfig.password,password , 64); /*"314Project"*/
	wifi_station_set_config_current(&stationConfig);

	wifi_station_connect();
	wifi_station_dhcpc_start();

	os_timer_disarm(&retryTimer);
	os_timer_setfn(&retryTimer, (os_timer_func_t *)wifiGuardian, (void *)0);
	os_timer_arm(&retryTimer, RETRY_DELAY, 1);
}

/******************************************************************************
 * FunctionName : wifiGuardian
 * Description  : Periodic function that checks the wifi connection status and
 * 				  puts the device into sleep if wifi connection has been lost for
 * 				  excessive amount of time
 * Parameters   : arg = NULL
 * Returns      : none
*******************************************************************************/

static void ICACHE_FLASH_ATTR wifiGuardian(void *arg)
{
	if(CONFIG)
	{
		configActiveTime+=RETRY_DELAY/1000;
		ets_uart_printf("ConfigActiveTime:%d\r\n",configActiveTime);
		if(configActiveTime > CONFIG_TIMEOUT)
		{
			fallAsleep(RF_CALIBRATION);
		}
	}
	if(wifi_station_get_connect_status()==STATION_GOT_IP)
		retrysNr=0;

	else if(retrysNr<MAX_RETRYS_IN_AWAKE)
	{
		retrysNr++;
		signalizeStatus(FAIL);
	}

	if(retrysNr>MAX_RETRYS_IN_AWAKE)
	{
		os_timer_disarm(&retryTimer);
		fallAsleep(RF_CALIBRATION);
	}
}

/******************************************************************************
 * FunctionName : wifiEventHandler
 * Description  : Handler function for wifi related events
 * Parameters   : event -- system wifi events
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR wifiEventHandler(System_Event_t* event)
{
	struct ip_info ip;
    char temp[30];
    char* broad;

	switch(event->event)
	{
	case EVENT_STAMODE_CONNECTED:
#ifdef DEBUG
		ets_uart_printf("connect to ssid: %s, channel: %d \r\n",
		event->event_info.connected.ssid,
		event->event_info.connected.channel);
#endif
		signalizeStatus(OK);
		break;
	case EVENT_STAMODE_DISCONNECTED:
#ifdef DEBUG
		ets_uart_printf("disconnect from: ssid %s, reason: %d\r\n",
		event->event_info.disconnected.ssid,
		event->event_info.disconnected.reason);
#endif
		wifi_err++;
		signalizeStatus(FAIL);

		if(wifi_err>TIMEOUT)
		{
			wifi_station_disconnect();
			fallAsleep(RF_CALIBRATION);
		}
		break;
	case EVENT_STAMODE_AUTHMODE_CHANGE:
#ifdef DEBUG
		ets_uart_printf("AUTHMODE_CHANGE...\r\n");
#endif
		break;
	case EVENT_STAMODE_GOT_IP:
		wifi_get_ip_info(STATION_IF, &ip);
#ifdef DEBUG
		ets_uart_printf("Got IP! \r\n");
	    os_sprintf(temp, "Station ip:" IPSTR "\r\n", IP2STR(&(ip.ip.addr)));
	    ets_uart_printf(temp);
	    wifi_get_macaddr(STATION_IF, macaddr);
	    os_sprintf(temp, "Mac address:" MACSTR "\r\n", MAC2STR(macaddr));
	    ets_uart_printf(temp);
#endif
	    if(CONFIG)
	    {
	    	broad=get_broadcast_address();
	    	setupTCP("0.0.0.0", TCP_PORT,&conn1,&tcp1);
	    	os_sprintf(temp, "0x01" IPSTR "\n", IP2STR(&(ip.ip.addr)));
	    	UDP_sendData(temp, strlen(temp),broad);
	    	espconn_accept(&conn1);
	    	espconn_regist_time(&conn1,500,0);
	    }

		break;
	case EVENT_STAMODE_DHCP_TIMEOUT:
#ifdef DEBUG
		ets_uart_printf("DHCP_TIMEOUT...\r\n");
#endif
		break;
	case EVENT_SOFTAPMODE_STACONNECTED:
		break;
	case EVENT_SOFTAPMODE_STADISCONNECTED:
		break;
	case EVENT_SOFTAPMODE_PROBEREQRECVED:
		break;
	default:
		break;
	}
}


void ICACHE_FLASH_ATTR setConfig(bool Conf)
{
	CONFIG=Conf;
}
