/*
 * data_storage.c
 *
 *  Created on: 24.09.2016
 *      Author: Jacek
 */
#include "data_storage.h"
/*TO DO
 * 1)Zapis pomiarów na kolejne sektory
 * 2)Hash do sektorów pomiarowych
 */
measurement current_mes;
measurement readMes;
params currentPar;
params readPar;
date storedDate;
uint32_t overflowDataCount=0; //ile razy sie licznik przekrecil baranie, a nie ile sektorow !!!
int mes_num=0;

void ICACHE_FLASH_ATTR saveTemperature(int16_t temp)
{
	current_mes.temperature=temp;
}

void ICACHE_FLASH_ATTR saveHumidity(uint16_t hum)
{
	current_mes.humidity=hum;
}

void ICACHE_FLASH_ATTR saveOffsetTime(uint32_t offset)
{
	current_mes.offset_time=offset;
}

int16_t ICACHE_FLASH_ATTR getTemperature()
{
	return current_mes.temperature;
}

uint16_t ICACHE_FLASH_ATTR getHumidity()
{
	return current_mes.humidity;
}

void ICACHE_FLASH_ATTR storeMeasurement()
{
	uint8_t saveAddress;
	uint32_t measurementNr;
	measurementNr=readMeasurementCount(true);
	measurementNr++;
	if(measurementNr>MAX_MES_NR)
	{
		measurementNr=1;
		storeToFlash(DATA_SEC1);
		overflowDataCount=readOverflowSectorCount();
		overflowDataCount++;
		saveOverflowSectorCount();

	}
		saveAddress=MEASUREMENTS_START_INDEX+MES_SIZE*(measurementNr-1);
		system_rtc_mem_write(saveAddress,&current_mes,BLOCK_SIZE*MES_SIZE);
		system_rtc_mem_write(DATA_COUNT_INDEX,&measurementNr,sizeof(measurementNr));

}

measurement* ICACHE_FLASH_ATTR readMeasurement(uint16_t mesNr)
{
	uint32_t readIndex;
	uint32_t mesCount=readMeasurementCount(false);
	uint32_t mesRTC_Count=readMeasurementCount(true);
	if(mesRTC_Count>mesNr)
	{
		readIndex=MEASUREMENTS_START_INDEX+MES_SIZE*mesNr;
#ifdef DEBUG
		ets_uart_printf("attempt to read mes nr:%d with readIndex:%d\r\n",mesNr,readIndex);//debug
#endif
		system_rtc_mem_read(readIndex,&readMes,BLOCK_SIZE*MES_SIZE*sizeof(char));
	}
	else if(mesCount>mesNr)
	{
		readFromFlash(DATA_SEC1,mesNr-mesRTC_Count,&readMes,sizeof(measurement));
	}
	return &readMes;
}

int ICACHE_FLASH_ATTR readMeasurementCount(bool OnlyRTC)
{
	uint32_t mesCount=999;
	uint32 overflowMes=0;
	system_rtc_mem_read(DATA_COUNT_INDEX,&mesCount,sizeof(mesCount));

	if(!OnlyRTC)
	{
		overflowMes=readOverflowSectorCount();
		//ets_uart_printf("Overflow data count:%d\r\n",overflowMes);
		mesCount=mesCount+MAX_MES_NR*overflowMes;
	}
	//ets_uart_printf("Mes Count readed : %d\r\n",mesCount);
	return mesCount;
}

void ICACHE_FLASH_ATTR delMeasurement(bool All) //Do przebudowy !!!
{
	uint32_t mesCount = readMeasurementCount(true);
	if(!All)
	{
		if(mesCount != 0)
		{
			mesCount--;
			system_rtc_mem_write(DATA_COUNT_INDEX,&mesCount,sizeof(mesCount));
		}
	}
	else
	{
		mesCount=0;
		overflowDataCount=0;
		saveOverflowSectorCount();
		system_rtc_mem_write(DATA_COUNT_INDEX,&mesCount,sizeof(mesCount));//redudancja tutaj!
		/*copyParams();
		storeParams();*/
	}

}

void ICACHE_FLASH_ATTR saveOverflowSectorCount()
{
	system_rtc_mem_write(OVERFLOW_SEC_INDEX,&overflowDataCount,sizeof(uint32_t));
#ifdef DEBUG
	ets_uart_printf("Saved Overflowsector:%d\r\n",overflowDataCount);
#endif
}

uint32_t ICACHE_FLASH_ATTR readOverflowSectorCount()
{
	uint32_t result=0;
	system_rtc_mem_read(OVERFLOW_SEC_INDEX,&result,sizeof(result));
	return result;
}

void ICACHE_FLASH_ATTR readFromFlash(uint16 sector,uint32 indx,void* destination, uint32 size)
{
	if(readOverflowSectorCount()>0)
	{
		spi_flash_read(sector*4*1024+indx*sizeof(measurement),(uint32*)destination,size);
		os_delay_us(3000);
#ifdef DEBUG
		ets_uart_printf("RF:T:%d H:%d O: %d with sc:%d,inx:%d\r\n",readMes.temperature,readMes.temperature,readMes.offset_time,
				sector,indx);
#endif
	}

}

void ICACHE_FLASH_ATTR storeToFlash(uint16 sector)
{
#ifdef DEBUG
	ets_uart_printf("Flash_saving!!\r\n");
#endif
	uint32 data_size=readMeasurementCount(false);
	uint32_t tmp;
	measurement* mes_table=NULL;
	int i=0;

	mes_table=(measurement*)os_malloc(data_size*sizeof(measurement));
	if(mes_table)
	{
		for(i=0; i<data_size;i++) //chyba lepiej isc w dol, w przypadku jak sie nie zmiesci mozna przeniesc na 2 sektor
		{
			readMeasurement(i);
			mes_table[i].temperature=readMes.temperature;
			mes_table[i].humidity=readMes.humidity;
			mes_table[i].offset_time=readMes.offset_time;
		}

		spi_flash_erase_sector(sector);
		spi_flash_write(sector*4*1024,(uint32*)mes_table,data_size*sizeof(measurement));
		os_free(mes_table);
	}


}

void ICACHE_FLASH_ATTR initRTC_memory()
{
	uint32_t rtcNull=0;
	time_t nullDate=1000000000;
	uint32_t initVal=0;
	system_rtc_mem_read(INIT_NONCE_INDEX,&initVal,sizeof(uint32_t));
	if(initVal != INIT_NONCE)
		{
			system_rtc_mem_write(DATA_COUNT_INDEX,&rtcNull,sizeof(uint32_t));
			system_rtc_mem_write(OVERFLOW_SEC_INDEX,&rtcNull,sizeof(uint32_t));
			system_rtc_mem_write(RECENT_DATA_INDEX,&nullDate,sizeof(date));
			system_rtc_mem_write(WIFI_FLAG,&rtcNull,sizeof(uint32_t));
			currentPar.SetupWifi=DEF_SETUP_WIFI;
			strcpy(currentPar.pass,DEF_WIFI_PASSWORD);
			currentPar.remoteTcpPort=DEF_REMOTE_TCP_PORT;
			currentPar.sendingInterval=DEF_SENDING_INTERVAL;
			strcpy(currentPar.serverIp,DEF_TCP_IP);
			currentPar.sleepTime_s=DEF_SLEEP_TIME_S;
			strcpy(currentPar.ssID,DEF_WIFI_SSID);
			strcpy(currentPar.ServerAddress,DEF_SERV_ADDR);
			strcpy(currentPar.SensorID,DEF_SENSOR_ID);
			currentPar.configMode=false;
			currentPar.sendNow=false;
			currentPar.humMaxTreshold=1000;
			currentPar.humMinTreshold=0;
			currentPar.tempMaxTreshold=999;
			currentPar.tempMinTreshold=-999;
			storeParams();
			rtcSaveUnixTime(&nullDate);
			initVal=INIT_NONCE;
			system_rtc_mem_write(INIT_NONCE_INDEX,&initVal,sizeof(uint32_t));
		}
}

uint32_t ICACHE_FLASH_ATTR getFlagValue(enum Flags flag)
{
	uint32_t result=0;
	switch(flag)
	{
	case WIFI:
		system_rtc_mem_read(WIFI_FLAG,&result,sizeof(uint32_t));
		break;

	}
	return result;
}

void ICACHE_FLASH_ATTR setFlagValue(enum Flags flag, uint32_t value)
{
	uint32_t newVal=value;
	switch(flag)
	{
	case WIFI:
		system_rtc_mem_write(WIFI_FLAG,&newVal,sizeof(uint32_t));
		break;
	}
}

params* ICACHE_FLASH_ATTR getCurrParPtr()
{
	return &currentPar;
}

void ICACHE_FLASH_ATTR storeParams()
{
	bool res;
	res=system_param_save_with_protect(PARAM_START_SEC,&currentPar,sizeof(params));
	if(!res)
		ets_uart_printf("Param Saving ERROR! \r\n");
	ets_uart_printf("ParamsSaving... Succesful ...\r\n");
}

params* ICACHE_FLASH_ATTR readParams()
{
	bool res;
	res=system_param_load(PARAM_START_SEC,0,&readPar,sizeof(params));
	if(!res)
	{
		ets_uart_printf("system_param_load FAILED! /r/n");
	}
	return &readPar;
}

void ICACHE_FLASH_ATTR copyParams()
{
	readParams();
	currentPar.SetupWifi=readPar.SetupWifi;
	strcpy(currentPar.pass,readPar.pass);
	currentPar.remoteTcpPort=readPar.remoteTcpPort;
	currentPar.sendingInterval=readPar.sendingInterval;
	strcpy(currentPar.serverIp,readPar.serverIp);
	currentPar.sleepTime_s=readPar.sleepTime_s;
	strcpy(currentPar.ssID,readPar.ssID);
	strcpy(currentPar.ServerAddress,readPar.ServerAddress);
	strcpy(currentPar.SensorID, readPar.SensorID);

	currentPar.sendNow=readPar.sendNow;
	currentPar.configMode=readPar.configMode;
	currentPar.tresholdsExceeded=readPar.tresholdsExceeded;

	currentPar.tempMaxTreshold=readPar.tempMaxTreshold;
	currentPar.tempMinTreshold=readPar.tempMinTreshold;
	currentPar.humMaxTreshold=readPar.humMaxTreshold;
	currentPar.humMinTreshold=readPar.humMinTreshold;
}

void ICACHE_FLASH_ATTR storeDate(uint32_t new_date)
{
	time_t unixTime=new_date;
	system_rtc_mem_write(RECENT_DATA_INDEX,&unixTime,sizeof(time_t));
}

date* ICACHE_FLASH_ATTR unixToDate(uint32_t Udate)
{
	time_t unixTime=Udate;
	struct tm *accDate;

	accDate=gmtime(&unixTime);
	storedDate.year=accDate->tm_year+1900;
	storedDate.month=accDate->tm_mon+1;
	storedDate.day=accDate->tm_mday;
	storedDate.hour=(accDate->tm_hour+2)%24;
	storedDate.minute=accDate->tm_min;
	storedDate.second=accDate->tm_sec;
	return &storedDate;
}
uint32_t ICACHE_FLASH_ATTR readDate()
{
	time_t currentDate;
	system_rtc_mem_read(RECENT_DATA_INDEX,&currentDate,sizeof(time_t));
	return currentDate;
}
