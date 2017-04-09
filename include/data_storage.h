/*
 * data_storage.h
 *
 *  Created on: 24.09.2016
 *      Author: Jacek
 */

#ifndef INCLUDE_DATA_STORAGE_H_
#define INCLUDE_DATA_STORAGE_H_

#include "user_config.h"
#include "driver/rtc_i2c.h"

#define MIN_RTC_MEMORY_INDEX 64 // Wczesniejsze sa dla systemu
#define INIT_NONCE_INDEX 64 //tu init magic
#define DATA_COUNT_INDEX 65 // w tym bloku przechowuje ogolna zawartosc pomiarow
#define OVERFLOW_SEC_INDEX 66 //ilosc przekrecen licznika w tym bloku
#define WIFI_FLAG 67 //tu flage od ktorej zalezy czy bedzie setup wifi
#define RECENT_DATA_INDEX 68 // A tu date
#define MEASUREMENTS_START_INDEX 70
#define MAX_MEMORY_INDEX 192

#define INIT_NONCE 0xDAADBEEF
#define MES_SIZE sizeof(measurement)/4
#define BLOCK_SIZE 4
#define RECENT_DATA_SIZE 20 //bytes
#define RTC_MES_CAPACITY 56
/*FLASH*/
#define START_SEC 0x59
#define PARAM_START_SEC 0x7D
#define END_SEC 0x7C
#define SEC_CAPACITY 504

#define MAX_MES_TOTAL 18200//(END_SEC-START_SEC+1)*SEC_CAPACITY + RTC_MES_CAPACITY
#define MAX_OVERFLOW_NR (MAX_MES_TOTAL-RTC_MES_CAPACITY)/RTC_MES_CAPACITY

typedef struct
{
	uint16_t remoteTcpPort;
	char serverIp[16];
	uint8 ServerAddress[64];
	uint8 ssID[32];
	uint8 pass[64];
	bool SetupWifi;
	bool configMode;
	bool sendNow;
	bool tresholdsExceeded;
	int16_t tempMaxTreshold;
	int16_t tempMinTreshold;
	uint16_t humMaxTreshold;
	uint16_t humMinTreshold;
	uint16_t sendingInterval;
	uint32_t sleepTime_s;
	char SensorID[64];
}params;
typedef struct
{
	int16_t  temperature;
	uint16_t  humidity;
	uint32_t  offset_time;
}measurement;


typedef struct
{
	uint8_t mBlockCount;
}Metadata;

typedef struct
{
	measurement measurements[RTC_MES_CAPACITY];
}MesBlock;

typedef struct
{
	MesBlock mBlocks[SEC_CAPACITY/RTC_MES_CAPACITY];
	Metadata secData;
}DataSec;

enum Flags
{
	WIFI,
	TODOs

}Flags;

void ICACHE_FLASH_ATTR saveTemperature(int16_t temp);
void ICACHE_FLASH_ATTR saveHumidity(uint16_t hum);
void ICACHE_FLASH_ATTR saveOffsetTime(uint32_t offset);
uint16_t ICACHE_FLASH_ATTR getHumidity();
int16_t ICACHE_FLASH_ATTR getTemperature();
void ICACHE_FLASH_ATTR storeMeasurement();
measurement* ICACHE_FLASH_ATTR readMeasurement(uint16_t mesNr);
void ICACHE_FLASH_ATTR delMeasurement(bool All);
int ICACHE_FLASH_ATTR readMeasurementCount(bool OnlyRTC);
uint32_t ICACHE_FLASH_ATTR getFlagValue(enum Flags flag);
void ICACHE_FLASH_ATTR setFlagValue(enum Flags flag, uint32_t value);
void ICACHE_FLASH_ATTR saveOverflowSectorCount();
uint32_t ICACHE_FLASH_ATTR readOverflowSectorCount();

void ICACHE_FLASH_ATTR storeDate(uint32_t new_date);
uint32_t ICACHE_FLASH_ATTR readDate();


void ICACHE_FLASH_ATTR storeToFlash(uint16 sector);
void ICACHE_FLASH_ATTR storeInFlash();
void ICACHE_FLASH_ATTR initFlash();
uint16_t ICACHE_FLASH_ATTR translateIndex(uint16_t indx, uint32_t * sector);
Metadata ICACHE_FLASH_ATTR readSectorMetadata (uint32_t sector);
measurement ICACHE_FLASH_ATTR readSectorMeasurement (uint32_t sector, uint16_t indx);
void ICACHE_FLASH_ATTR readFromFlash(uint16 sector,uint32 indx,void* destination, uint32 size);
/*TO DO*/
uint16_t ICACHE_FLASH_ATTR calculateHash();
/*****************************/
void ICACHE_FLASH_ATTR storeParams();
params* ICACHE_FLASH_ATTR readParams();
void ICACHE_FLASH_ATTR copyParams();
params* ICACHE_FLASH_ATTR getCurrParPtr();
void ICACHE_FLASH_ATTR initRTC_memory();


#endif /* INCLUDE_DATA_STORAGE_H_ */
