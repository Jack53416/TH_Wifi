/*
 * data_storage.c
 *
 *  Created on: 24.09.2016
 *      Author: Jacek
 */
#include "data_storage.h"

Measurement currentMes;
Measurement readMes;
Params currentPar;
Params readPar;

/******************************************************************************
 * FunctionName : saveTemperature, saveHumidity, saveOffsetTime, getTemperature,
 * 				  getOffsetTime, getHumidity
 * Description  : getters and setters for the currentMes global variable defined
 * 				  in this file
 * Parameters   : none
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR saveTemperature(int16_t temp)
{
	currentMes.temperature=temp;
}

void ICACHE_FLASH_ATTR saveHumidity(uint16_t hum)
{
	currentMes.humidity=hum;
}

void ICACHE_FLASH_ATTR saveOffsetTime(uint32_t offset)
{
	currentMes.offset_time=offset;
}

int16_t ICACHE_FLASH_ATTR getTemperature()
{
	return currentMes.temperature;
}

uint16_t ICACHE_FLASH_ATTR getHumidity()
{
	return currentMes.humidity;
}

/******************************************************************************
 * FunctionName : storeMeasurement
 * Description  : Stores the measurement - currentMes in the memory(flash or RTC)
 * Parameters   : none
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR storeMeasurement()
{
	uint8_t saveAddress;
	uint32_t measurementNr;
	uint32_t saveSector;
	uint32_t overflowDataCount=0;

	measurementNr=readMeasurementCount(ONLY_RTC);
	measurementNr++;
	if(measurementNr>RTC_MES_CAPACITY)
	{
		measurementNr=1;
		storeInFlash();
		overflowDataCount=readOverflowSectorCount();

		if(overflowDataCount < MAX_OVERFLOW_NR)
		{
			overflowDataCount++;
			saveOverflowSectorCount(overflowDataCount);
		}

	}
		saveAddress=MEASUREMENTS_START_INDEX+MES_SIZE*(measurementNr-1);
		system_rtc_mem_write(saveAddress,&currentMes,BLOCK_SIZE*MES_SIZE);
		system_rtc_mem_write(DATA_COUNT_INDEX,&measurementNr,sizeof(measurementNr));

}

/******************************************************************************
 * FunctionName : storeMeasurement
 * Description  : Reads the measurement to readMes variable
 * Parameters   : mesNr - index of desired measurement, 0 - newest
 * Returns      : pointer to the readMes variable
*******************************************************************************/

Measurement* ICACHE_FLASH_ATTR readMeasurement(uint16_t mesNr)
{
	uint32_t readIndex;
	uint32_t mesCount=readMeasurementCount(ALL);
	uint32_t mesRTC_Count=readMeasurementCount(ONLY_RTC);
	uint32_t readSector;
	if(mesRTC_Count>mesNr)
	{
		readIndex=translateIndex(mesNr, &readSector);

		//ets_uart_printf("attempt to read mes nr:%d with readIndex:%d\r\n",mesNr,readIndex);//debug
		system_rtc_mem_read(MEASUREMENTS_START_INDEX+MES_SIZE*readIndex,&readMes,BLOCK_SIZE*MES_SIZE*sizeof(char));
		return &readMes;
	}
	else if(mesCount>mesNr)
	{
		readIndex = translateIndex(mesNr, &readSector);
		//ets_uart_printf("attempt to read from sec:%x with readIndex:%d\r\n",readSector,readIndex);
		readFromFlash(readSector,readIndex,&readMes,sizeof(Measurement));
		return &readMes;
	}
	return NULL;
}

/******************************************************************************
 * FunctionName : readMeasurementCount
 * Description  : Counts the measurement number stored in the memory
 * Parameters   : countType -- counting type, it can either count measuerments
 * 				  only in RTC memory(ONLY_RTC) or in memory as a whole(ALL) -
 * 				  RTC + FLASH
 * Returns      : measurement count
*******************************************************************************/

int ICACHE_FLASH_ATTR readMeasurementCount(McountType countType)
{
	uint32_t mesCount=0;
	uint32 overflowMes=0;
	system_rtc_mem_read(DATA_COUNT_INDEX,&mesCount,sizeof(mesCount));

	if(countType == ALL)
	{
		overflowMes=readOverflowSectorCount();
		mesCount=mesCount+RTC_MES_CAPACITY*overflowMes;
	}
	return mesCount;
}

void ICACHE_FLASH_ATTR saveOverflowSectorCount(uint32_t overflowDataCount)
{
	system_rtc_mem_write(OVERFLOW_SEC_INDEX,&overflowDataCount,sizeof(uint32_t));
}

uint32_t ICACHE_FLASH_ATTR readOverflowSectorCount()
{
	uint32_t result=0;
	system_rtc_mem_read(OVERFLOW_SEC_INDEX,&result,sizeof(result));
	return result;
}

/******************************************************************************
 * FunctionName : readFromFlash
 * Description  : Reads data chunk from Flash memory
 * Parameters   : sector -- Index of sector to read
 * 				  indx 	 --
 * Returns      : pointer to the readMes variable
*******************************************************************************/

void ICACHE_FLASH_ATTR readFromFlash(uint16 sector,uint32 indx,void* destination, uint32 size)
{
	if(readOverflowSectorCount()>0)
	{
		spi_flash_read(sector*4*1024+indx*sizeof(Measurement),(uint32*)destination,size);
		os_delay_us(3000);
#ifdef DEBUG
		ets_uart_printf("RF:T:%d H:%d O: %d with sc:%d,inx:%d\r\n",readMes.temperature,readMes.temperature,readMes.offset_time,
				sector,indx);
#endif
	}

}

/******************************************************************************
 * FunctionName : initMemory
 * Description  : initially writes the memory(flash + RTC), intended to occur on power on,
 * 				  it is checked by comparing RTC of memory index specified by INIT_NONCE_INDEX
 * 				  defined value with INIT_NONCE defined value
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR initMemory()
{
	uint32_t rtcNull=0;
	time_t nullDate=1000000000;
	uint32_t initVal=0;
	system_rtc_mem_read(INIT_NONCE_INDEX,&initVal,sizeof(uint32_t));
	if(initVal != INIT_NONCE)
		{
			system_rtc_mem_write(DATA_COUNT_INDEX,&rtcNull,sizeof(uint32_t));
			system_rtc_mem_write(OVERFLOW_SEC_INDEX,&rtcNull,sizeof(uint32_t));
			currentPar.flags.setupWifi=DEF_SETUP_WIFI;
			strcpy(currentPar.connectionData.pass,DEF_WIFI_PASSWORD);
			currentPar.connectionData.remoteTcpPort=DEF_REMOTE_TCP_PORT;
			currentPar.sensorData.sendingInterval=DEF_SENDING_INTERVAL;
			currentPar.sensorData.sleepTime_s=DEF_SLEEP_TIME_S;
			strcpy(currentPar.connectionData.ssID,DEF_WIFI_SSID);
			strcpy(currentPar.connectionData.ServerAddress,DEF_SERV_ADDR);
			strcpy(currentPar.sensorData.sensorID,DEF_SENSOR_ID);
			currentPar.flags.configMode=false;
			currentPar.flags.sendNow=false;
			currentPar.tresholds.humMaxTreshold=1000;
			currentPar.tresholds.humMinTreshold=0;
			currentPar.tresholds.tempMaxTreshold=999;
			currentPar.tresholds.tempMinTreshold=-999;
			storeParams();
			rtcSaveUnixTime(&nullDate);
			initVal=INIT_NONCE;
			system_rtc_mem_write(INIT_NONCE_INDEX,&initVal,sizeof(uint32_t));
			initFlash();

			/*int i;
				for(i = 0 ; i<690 ; i++)
				{
					saveTemperature(23);
					saveHumidity(43);
					saveOffsetTime(1477147673 + i);
					storeMeasurement();
				}*/
		}
}

/******************************************************************************
 * FunctionName : getCurrParPtr
 * Description  : returns the pointer to the currentPar global variable
 * Parameters   : none
 * Returns      : pointer to the currentPar
*******************************************************************************/

Params* getCurrParPtr()
{
	return &currentPar;
}

/******************************************************************************
 * FunctionName : storeParams
 * Description  : stores Param type structure currentPar in the flash memory
 * 				  exact sector is described by PARAM_START_SEC defined value
 * Parameters   : none
 * Returns      : none
*******************************************************************************/

void storeParams()
{
	bool res;
	res=system_param_save_with_protect(PARAM_START_SEC,&currentPar,sizeof(Params));
	if(!res)
		ets_uart_printf("Param Saving ERROR! \r\n");
	ets_uart_printf("ParamsSaving... Succesful ...\r\n");
}

/******************************************************************************
 * FunctionName : readParams
 * Description  : reads Param type structure from flash and stores it in readPar
 * 				  variable
 * Parameters   : none
 * Returns      : pointer to readPar global variable
*******************************************************************************/

Params* readParams()
{
	bool res;
	res=system_param_load(PARAM_START_SEC,0,&readPar,sizeof(Params));
	if(!res)
	{
		ets_uart_printf("system_param_load FAILED! /r/n");
	}
	return &readPar;
}


/******************************************************************************
 * FunctionName : copyParams
 * Description  : copies values of Params structure from readPar to currentPar
 * 				  global variables
 * Parameters   : none
 * Returns      : none
*******************************************************************************/

void copyParams()
{
	readParams();
	currentPar.flags.setupWifi=readPar.flags.setupWifi;
	strcpy(currentPar.connectionData.pass,readPar.connectionData.pass);
	currentPar.connectionData.remoteTcpPort=readPar.connectionData.remoteTcpPort;
	currentPar.sensorData.sendingInterval=readPar.sensorData.sendingInterval;
	currentPar.sensorData.sleepTime_s=readPar.sensorData.sleepTime_s;
	strcpy(currentPar.connectionData.ssID,readPar.connectionData.ssID);
	strcpy(currentPar.connectionData.ServerAddress,readPar.connectionData.ServerAddress);
	strcpy(currentPar.sensorData.sensorID, readPar.sensorData.sensorID);

	currentPar.flags.sendNow=readPar.flags.sendNow;
	currentPar.flags.configMode=readPar.flags.configMode;
	currentPar.flags.tresholdsExceeded=readPar.flags.tresholdsExceeded;

	currentPar.tresholds.tempMaxTreshold=readPar.tresholds.tempMaxTreshold;
	currentPar.tresholds.tempMinTreshold=readPar.tresholds.tempMinTreshold;
	currentPar.tresholds.humMaxTreshold=readPar.tresholds.humMaxTreshold;
	currentPar.tresholds.humMinTreshold=readPar.tresholds.humMinTreshold;
}


/******************************************************************************
 * FunctionName : storeDate
 * Description  : It stores the date in unix format in the RTC memory at address
 * 				  defined by the RECENT_DATE_INDEX defined value
 * Parameters   : new_date -- date to be stored in unix format
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR storeDate(uint32_t new_date)
{
	time_t unixTime=new_date;
	system_rtc_mem_write(RECENT_DATE_INDEX,&unixTime,sizeof(time_t));
}

/******************************************************************************
 * FunctionName : readDate
 * Description  : It reads the date in unix format from the RTC memory at address
 * 				  defined by the RECENT_DATE_INDEX defined value
 * Parameters   : none
 * Returns      : date in unix format
*******************************************************************************/

uint32_t ICACHE_FLASH_ATTR readDate()
{
	time_t currentDate;
	system_rtc_mem_read(RECENT_DATE_INDEX,&currentDate,sizeof(time_t));
	return currentDate;
}

/******************************************************************************
 * FunctionName : storeInFlash
 * Description  : appends MesBlock type structure taken from RTC to the DataSec
 * 				  structure stored in the flash memory. Along with that the
 * 				  metadata are stored containing the number of the Mesblocks on the
 * 				  given sector.
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR storeInFlash()
{
	uint32_t saveSector = START_SEC+(uint8_t)((readMeasurementCount(ALL) - RTC_MES_CAPACITY)/SEC_CAPACITY);
	MesBlock rtc;
	DataSec sector;
	if(saveSector > END_SEC)
	{
		saveSector = END_SEC;
	}
	system_rtc_mem_read(MEASUREMENTS_START_INDEX,&rtc, sizeof(MesBlock));
	spi_flash_read(saveSector*4*1024, (uint32*)&sector,sizeof(DataSec));
	sector.mBlocks[sector.secData.mBlockCount] = rtc;
	sector.secData.mBlockCount++;

	//ets_uart_printf("Writing to flash, sector:%x , block count:%d\r\n", saveSector, sector.secData.mBlockCount);
	spi_flash_erase_sector(saveSector);
	spi_flash_write(saveSector*4*1024,(uint32*)&sector,sizeof(DataSec));

}

/******************************************************************************
 * FunctionName : initFlash
 * Description  : Initializes the flash memory, by writing metadata of each sector
 * 				  designated for storage. Initialized metada say that there are no
 * 				  measuremets blocks on the given sector
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR initFlash()
{
	uint32_t saveSector = START_SEC;
	DataSec clearSector;
	unsigned int i;
	clearSector.secData.mBlockCount=0;

	while(saveSector <= END_SEC)
	{
		spi_flash_erase_sector(saveSector);
		spi_flash_write(saveSector*4*1024,(uint32*)&clearSector,sizeof(DataSec));
		saveSector++;
	}
}


/******************************************************************************
 * FunctionName : translateIndex
 * Description  : Function that maps the indexes for readMeasurement function in
 * 				  such a way that they are in descending order sorted by date, where
 * 				  index = 0 is the newest measurement
 * Parameters   : indx -- index to be mapped
 * 				  sector -- pointer to sector variable, the fuction also changes that
 * 				  variable to indicate in which sector the desired data are located
 * Returns      : real index in the flash memory where the data are stored
*******************************************************************************/

uint16_t ICACHE_FLASH_ATTR translateIndex(uint16_t indx, uint32_t * sector)
{
	uint32_t readSector;
	Metadata metaData;
	uint16_t result;
	if(indx < readMeasurementCount(ONLY_RTC))
	{
		return readMeasurementCount(ONLY_RTC) - indx - 1;
	}

	indx-= readMeasurementCount(ONLY_RTC);
	readSector = START_SEC+(uint8_t)((readMeasurementCount(ALL) - readMeasurementCount(ONLY_RTC) - 1)/SEC_CAPACITY);

	metaData = readSectorMetadata(readSector);
	while(indx >= metaData.mBlockCount * RTC_MES_CAPACITY)
	{
		indx -= metaData.mBlockCount * RTC_MES_CAPACITY;
		readSector--;
		if(readSector < START_SEC)
			return -1;
		metaData = readSectorMetadata(readSector);
	}

	*sector = readSector;
	return metaData.mBlockCount*RTC_MES_CAPACITY - indx -1;

}

/******************************************************************************
 * FunctionName : readSectorMetadata
 * Description  : Function reads only metadata part of the sector
 * Parameters   : sector -- index of the sector, which metada are to be returned
 * Returns      : Metada type structur coresponding to given sector
*******************************************************************************/
Metadata ICACHE_FLASH_ATTR readSectorMetadata (uint32_t sector)
{
	Metadata result;
	spi_flash_read(sector*4*1024+SEC_CAPACITY*sizeof(Measurement)
			, (uint32*)&result,sizeof(Metadata));
	return result;
}

/******************************************************************************
 * FunctionName : readSectorMeasurement
 * Description  : Function reads only single measurement within the sector
 * Parameters   : sector -- index of the sector, which measurement is to be returned
 * 				  indx -- address offset alligned to measurement structure
 * Returns      : Measurement type structur coresponding to given sector and given offset
*******************************************************************************/

Measurement ICACHE_FLASH_ATTR readSectorMeasurement (uint32_t sector, uint16_t indx)
{
	Measurement result;
	spi_flash_read(sector*4*1024+indx*sizeof(Measurement)
			, (uint32*)&result,sizeof(Metadata));
	return result;
}

/******************************************************************************
 * FunctionName : deleteMeasurements
 * Description  : Function deletes all measurements from RTC and a number of RTC
 * 				  blocks from flash starting from newest ones.
 * Parameters   : blocks to be delete -- number of RTC blocks to delete
 * Returns      : true on success, false on failure
*******************************************************************************/

bool deleteMeasurements(uint16_t blocksToDelete)
{
	uint32_t deleteSector;
	uint32_t mesCount = 0;
	uint32_t overflowCount;
	DataSec delSector;
	system_rtc_mem_write(DATA_COUNT_INDEX,&mesCount,sizeof(mesCount));
	if(blocksToDelete == 0)
		return true;

	overflowCount = readOverflowSectorCount();

	if(blocksToDelete > overflowCount)
		blocksToDelete = overflowCount;

	overflowCount -= blocksToDelete;
	deleteSector = START_SEC+(uint8_t)((readMeasurementCount(ALL) - readMeasurementCount(ONLY_RTC) - 1)/SEC_CAPACITY);
	while(blocksToDelete > 0)
	{
		spi_flash_read(deleteSector*4*1024, (uint32*)&delSector,sizeof(DataSec));
		//ets_uart_printf("Rmeta: %d \r\n",delSector.secData);
		if(delSector.secData.mBlockCount < blocksToDelete)
		{
			blocksToDelete -= delSector.secData.mBlockCount;
			delSector.secData.mBlockCount = 0;
			spi_flash_erase_sector(deleteSector);
			spi_flash_write(deleteSector*4*1024,(uint32*)&delSector,sizeof(DataSec));
			deleteSector--;
		}
		else
		{
			delSector.secData.mBlockCount -= blocksToDelete;
			blocksToDelete = 0;
			spi_flash_erase_sector(deleteSector);
			spi_flash_write(deleteSector*4*1024,(uint32*)&delSector,sizeof(DataSec));
		}
		//ets_uart_printf("RECENT SECTOR : %x, recentMetadata:%d\r\n" , deleteSector, delSector.secData.mBlockCount);
		system_soft_wdt_feed();
	}
	//ets_uart_printf("saving overflow : %d\r\n", overflowCount);
	system_rtc_mem_write(OVERFLOW_SEC_INDEX,&overflowCount,sizeof(uint32_t));
	return true;
}

/******************************************************************************
 * FunctionName : MurmurHash3_32
 * Description  : Hash function implementing MurmurHash3 algoryth for x86
 * 				  architecture computing 32 bit hash. Implementation comes
 * 				  from https://github.com/PeterScott/murmur3
 * Parameters   : key -- pointer to data destined to be hashed
 * 				  len -- length of the data in bytes
 * 				  seed -- arbitrary seed number
 * Returns      : 32bit hash value
*******************************************************************************/

uint32_t ICACHE_FLASH_ATTR MurmurHash3_32 ( const void * key, int len,
                          uint32_t seed)
{
  const uint8_t * data = (const uint8_t*)key;
  const int nblocks = len / 4;
  int i;

  uint32_t h1 = seed;

  uint32_t c1 = 0xcc9e2d51;
  uint32_t c2 = 0x1b873593;

  //----------
  // body

  const uint32_t * blocks = (const uint32_t *)(data + nblocks*4);

  for(i = -nblocks; i; i++)
  {
    uint32_t k1 = blocks[i];

    k1 *= c1;
    k1 = rotl32(k1,15);
    k1 *= c2;

    h1 ^= k1;
    h1 = rotl32(h1,13);
    h1 = h1*5+0xe6546b64;
  }

  //----------
  // tail

  const uint8_t * tail = (const uint8_t*)(data + nblocks*4);

  uint32_t k1 = 0;

  switch(len & 3)
  {
  case 3: k1 ^= tail[2] << 16;
  case 2: k1 ^= tail[1] << 8;
  case 1: k1 ^= tail[0];
          k1 *= c1; k1 = rotl32(k1,15); k1 *= c2; h1 ^= k1;
  };

  h1 ^= len;

  h1 ^= h1 >> 16;
  h1 *= 0x85ebca6b;
  h1 ^= h1 >> 13;
  h1 *= 0xc2b2ae35;
  h1 ^= h1 >> 16;

  return h1;
}

static inline ICACHE_FLASH_ATTR uint32_t rotl32 ( uint32_t x, int8_t r )
{
  return (x << r) | (x >> (32 - r));
}
