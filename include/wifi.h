/*
 * wifi.h
 *
 *  Created on: 14.09.2016
 *      Author: Jacek
 */

#ifndef INCLUDE_WIFI_H_
#define INCLUDE_WIFI_H_
#define MAX_RETRYS_IN_AWAKE 5
#define RETRY_DELAY 3000//3sek between reconnect
#define CONFIG_TIMEOUT 180 // 180 sek
#include "user_config.h"
#include "udp.h"
#include "tcp.h"
#include "sleep.h"

void ICACHE_FLASH_ATTR setupWifi(const char* ssId, const char* password);
void ICACHE_FLASH_ATTR wifiEventHandler(System_Event_t* event);
static void ICACHE_FLASH_ATTR wifiGuardian(void *arg);

void ICACHE_FLASH_ATTR setConfig(bool Conf);


#endif /* INCLUDE_WIFI_H_ */
