#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#define DEF_WIFI_SSID "th_wifi"
#define DEF_WIFI_PASSWORD "Open34W#Close"
#define DEF_CONFIG_WIFI_SSID "thWifi_cfg"
#define DEF_CONFIG_WIFI_PASSWORD "pBWc05hDM!a"
#define DEF_TCP_IP "192.168.0.100"
#define DEF_SENDING_INTERVAL 2
#define DEF_SLEEP_TIME_S 2
#define DEF_SETUP_WIFI false
#define DEF_REMOTE_TCP_PORT 48042
#define DEF_SERV_ADDR "http://10.24.33.18/push/GVyKRAGt22bwqbFoVRfzbDWg"
#define DEF_SENSOR_ID "NOT NAMED"

#define MAX_SEND_INT 550
#define MAX_SLEEP_TIME_S 255*60

#define BLINK_PERIOD 1000
#define BLINK_PW 40
#define LONG_PRESS 3000 /*ms*/
#define TIMEOUT 3
#define ADC_PIN 5


#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"
#include "driver/uart.h"
#include "espconn.h"

#include "mem.h"
#include "gpio.h"
#include "httpclient.h"


enum RX_INFOS
{
		SERVER_IP = 0x01,
		SENDING_INT,
		REM_TCP_PORT,
		SS_ID,
		TIME_SET,
		WIFI_PASS,
		SLEEP_TIME,
		MEASUREMENT,
		SERVER_URL,
		SENSOR_ID,
		PARAMS_REQ,
		CONFIG_END,
};



#endif
