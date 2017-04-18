/*
 * tcp.h
 *
 *  Created on: 14.09.2016
 *      Author: Jacek
 */

#ifndef INCLUDE_TCP_H_
#define INCLUDE_TCP_H_
#include "user_config.h"
#include "wifi.h"
#include "sensor.h"
#include "sleep.h"
#include "data_storage.h"
#include "jsmn.h"
#include "led.h"

#define SENDING_INTERVAL 240
#define WIFI_TIMEOUT_INTERVAL 3000
#define TCP_PORT 48042
#define HEAD_SIZE 3
#define SND_STR "0x%02d %d %d %02d/%02d/%d %02d:%02d:%02d\n"
#define RTC_BLOCK_CHUNK 5 // RTC blocks in one data chunk to send
#define SEND_CHUNK RTC_BLOCK_CHUNK * RTC_MES_CAPACITY + readMeasurementCount(true)

#define MES_TEMPLATE "{\"mac\":\"%s\",\"volt\":\"%03d\",\"m\":["
#define MES_VAL_TEMPLATE "{\"tm\":\"%04d\",\"h\":\"%03d\",\"ts\":\"%010d\"}"
#define PAR_TEMPLATE "{\"Settings\":"
#define PAR_VALUE_TEMPLATE "{\"ServerAddress\":\"%s\",\"SendingInterval\":%d,\"Port\":%d,\"SleepTime_s\":%d,\"SS_ID\":\"%s\",\"Password\":\"%s\",\"SensorID\":\"%s\"}"
#define BASIC_LENGTH strlen(MES_TEMPLATE)-2+strlen(MAC)-4+3
#define MES_LENGTH strlen("{\"tm\":\"0000\",\"h\":\"000\",\"ts\":\"1477147673\"}")
#define PAR_VAL_LENGTH strlen(PAR_VALUE_TEMPLATE)-7*2
#define MAC "5c:cf:7f:0f:59:28"


void ICACHE_FLASH_ATTR setupTCP(const char* rIP, int rPort,struct espconn* connPtr,esp_tcp* tcpPtr);
void ICACHE_FLASH_ATTR connectCB(void *arg );
void ICACHE_FLASH_ATTR reconnectCB(void* arg, sint8 err);
void ICACHE_FLASH_ATTR disconnectCB(void* arg);
void ICACHE_FLASH_ATTR recvCB(void* arg, char* pData, unsigned short len);
void ICACHE_FLASH_ATTR sentCB(void* arg);

bool ICACHE_FLASH_ATTR resolveMode(short mode, char* data, struct espconn *pEspConn);
short ICACHE_FLASH_ATTR anylyzeReceived( char* pRec, char**pEnd, unsigned short len,struct espconn *pEspConn);

void ICACHE_FLASH_ATTR sendMeasurements(int totalMeasurements, int interval);
void ICACHE_FLASH_ATTR sendMeasurements_cb(void* arg);
bool ICACHE_FLASH_ATTR isStillSending();
char* ICACHE_FLASH_ATTR toSendFormat(Measurement* data, bool IsLast);
char* ICACHE_FLASH_ATTR toOneString(int measurementCount);
char* ICACHE_FLASH_ATTR paramsToString();
bool ICACHE_FLASH_ATTR  parseAnswer(char* dataString, int size);
static int ICACHE_FLASH_ATTR jsoneq(const char *json, jsmntok_t *tok, const char *s);

/*Prototype*/
bool ICACHE_FLASH_ATTR checkWifi(os_timer_t* timer);
void ICACHE_FLASH_ATTR http_cb(char * response_body, int http_status, char * response_headers, int body_size);
void ICACHE_FLASH_ATTR saveVoltage(uint16_t voltage);
bool ICACHE_FLASH_ATTR searchForNewParams(Params* newPar);
#endif /* INCLUDE_TCP_H_ */
