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
#define RECENT_DATE_INDEX 68 // A tu date
#define MEASUREMENTS_START_INDEX 70
#define MAX_MEMORY_INDEX 192

#define INIT_NONCE 0xDAADBEEF
#define MES_SIZE sizeof(Measurement)/4
#define BLOCK_SIZE 4
#define RTC_MES_CAPACITY 56
/*FLASH*/
#define START_SEC 0x59
#define PARAM_START_SEC 0x7D
#define END_SEC 0x7C
#define SEC_CAPACITY 504

#define MAX_MES_TOTAL 18200//(END_SEC-START_SEC+1)*SEC_CAPACITY + RTC_MES_CAPACITY
#define MAX_OVERFLOW_NR (MAX_MES_TOTAL-RTC_MES_CAPACITY)/RTC_MES_CAPACITY

typedef struct Flags{
	bool setupWifi;
	bool configMode;
	bool sendNow;
	bool tresholdsExceeded;
}Flags; //4byte alligned

typedef struct ConnectionData{
	uint32_t remoteTcpPort;
	uint8 ServerAddress[64];
	uint8 ssID[32];
	uint8 pass[64];
}ConnectionData; //4 byte alligned

typedef struct Tresholds{
	int16_t tempMaxTreshold;
	int16_t tempMinTreshold;
	uint16_t humMaxTreshold;
	uint16_t humMinTreshold;
}Tresholds; //4 byte alligned

typedef struct SensorData{
	uint32_t sleepTime_s;
	uint32_t sendingInterval;
	char sensorID[64];
}SensorData; // 4 byre alligned

typedef struct
{
	ConnectionData connectionData;
	SensorData sensorData;
	Tresholds tresholds;
	Flags flags;
}Params;

typedef struct
{
	int16_t  temperature;
	uint16_t  humidity;
	uint32_t  offset_time;
}Measurement;


typedef struct
{
	uint8_t mBlockCount;
}Metadata;

typedef struct
{
	Measurement measurements[RTC_MES_CAPACITY];
}MesBlock;

typedef struct
{
	MesBlock mBlocks[SEC_CAPACITY/RTC_MES_CAPACITY];
	Metadata secData;
}DataSec;

typedef enum McountType
{
	ALL,
	ONLY_RTC
}McountType;

void ICACHE_FLASH_ATTR initMemory();

void ICACHE_FLASH_ATTR saveTemperature(int16_t temp);
void ICACHE_FLASH_ATTR saveHumidity(uint16_t hum);
void ICACHE_FLASH_ATTR saveOffsetTime(uint32_t offset);
uint16_t ICACHE_FLASH_ATTR getHumidity();
int16_t ICACHE_FLASH_ATTR getTemperature();
void ICACHE_FLASH_ATTR storeMeasurement();
Measurement* ICACHE_FLASH_ATTR readMeasurement(uint16_t mesNr);
int ICACHE_FLASH_ATTR readMeasurementCount(McountType countType);
void ICACHE_FLASH_ATTR saveOverflowSectorCount(uint32_t overflowDataCount);
uint32_t ICACHE_FLASH_ATTR readOverflowSectorCount();

void ICACHE_FLASH_ATTR storeDate(uint32_t new_date);
uint32_t ICACHE_FLASH_ATTR readDate();


void ICACHE_FLASH_ATTR storeInFlash();
void ICACHE_FLASH_ATTR initFlash();
uint16_t ICACHE_FLASH_ATTR translateIndex(uint16_t indx, uint32_t * sector);
Metadata ICACHE_FLASH_ATTR readSectorMetadata (uint32_t sector);
Measurement ICACHE_FLASH_ATTR readSectorMeasurement (uint32_t sector, uint16_t indx);
void ICACHE_FLASH_ATTR readFromFlash(uint16 sector,uint32 indx,void* destination, uint32 size);

/*TO DO*/
uint16_t ICACHE_FLASH_ATTR calculateHash();
/*****************************/
void storeParams();
Params*  readParams();
void copyParams();
Params* getCurrParPtr();



#endif /* INCLUDE_DATA_STORAGE_H_ */
