/*
 * sensor.c
 *
 *  Created on: 15.09.2016
 *      Author: Jacek
 */
/*TO DO
 * 2)Kod dla wykrycia poprawnego dzialania sensora
 * 3) Clean up
 */
#include "sensor.h"
LOCAL os_timer_t main_timer; //timer w ramach którego wykonywane s¹ pomiary
uint32_t measurement_interval = 5;
uint8_t error_measurement = 0;
uint8_t user_reg_content;
bool sensor_is_broken = false;
int sendingInterval=100; //co ile pomiarow wysylka danych;

bool ICACHE_FLASH_ATTR first_SHT21_Init(void)
{
	  i2c_init();
	  os_delay_us(15000); //time for sensor power-up, according to 5.1 chapter in Datasheet SHT21

	  //setting the resolution of the measurements
	  uint8_t command_resolution  = 0x80; //T=13 bit RH=10 bit

	  uint8_t default_config = read_user_register();
	  if (default_config == 0xF)
		  return false; //We must read the content of the User Register due to protect the reserved bits of this
	  command_resolution=command_resolution|default_config;

	  if (write_user_register(command_resolution) == false)
		  return false; //zapisujemy do User Register wartosc okreslajaca dokladnosc pomiarow

	  return true;
}

void ICACHE_FLASH_ATTR  sensor_timerfunc(void *arg)
{
	int mesCount=0;
	uint32_t new_offset=0;
	uint32_t time=0;
	params* currPar;
	params* readPar;
	readPar=readParams();
	copyParams();
	currPar=getCurrParPtr();
	sendingInterval=currPar->sendingInterval;

    saveTemperature(SHT21_GetVal(GET_SHT_TEMPERATURE));
    saveHumidity(SHT21_GetVal(GET_SHT_HUMIDITY));    /*);*/
    saveOffsetTime(rtcGetUnixTime());
    if(areTresholdsExceeded(getTemperature(),getHumidity()) && !currPar->SetupWifi)
    {
    	ets_uart_printf("Tresholds Exceeded\r\n");
    	//currPar->sendNow=true;
    	currPar->SetupWifi=true;
    	currPar->sendingInterval=1;
    	currPar->sleepTime_s=60*20;
    	storeParams();
    	system_deep_sleep_set_option(1);
    	system_deep_sleep_instant(100);
    }
    mesCount=readMeasurementCount(false);
    if(mesCount<= MAX_MES_TOTAL)
    	storeMeasurement();

	if(readMeasurementCount(false)<sendingInterval-1)
	{
		if(readPar->SetupWifi !=false)
		{
			copyParams();
			currPar->SetupWifi=false;
			storeParams();
		}

		fallAsleep(NO_RF_CALIBRATION);
	}

	else if(readMeasurementCount(false)==sendingInterval-1)
	{
		if(readPar->SetupWifi != true)
		{
			copyParams();
			currPar->SetupWifi=true;
			storeParams();
		}

		fallAsleep(RF_CALIBRATION);
	}
	else if(!isStillSending())
	{
		saveVoltage(readAdc());
		sendMeasurements(readMeasurementCount(false),sendingInterval);
	}


}

uint8_t ICACHE_FLASH_ATTR read_user_register (void)
{
	  i2c_start();
	  i2c_writeByte(SHT21_ADDRESS);
	  if (!i2c_check_ack())
	  {
	    i2c_stop();
	    return 0xF;
	  }
	  i2c_writeByte(READ_USER_REG);
	  if (!i2c_check_ack())
	  {
	    i2c_stop();
	    return 0xF;
	  }
	  i2c_stop();
	  i2c_start();
	  i2c_writeByte(SHT21_ADDRESS+1);
	  if (!i2c_check_ack())
	  {
	    i2c_stop();
	    return 0xF;
	  }
	  user_reg_content = i2c_readByte();
	  i2c_send_ack(0);
	  i2c_stop();
	  return user_reg_content;
}

bool ICACHE_FLASH_ATTR write_user_register (uint8_t command_resolution)
{
	  i2c_start();
	  i2c_writeByte(SHT21_ADDRESS);
	  if (!i2c_check_ack())
	  {
	    i2c_stop();
	    return(0);
	  }
	  i2c_writeByte(WRITE_USER_REG);
	  if (!i2c_check_ack())
	  {
	    i2c_stop();
	    return(0);
	  }
	  i2c_writeByte(command_resolution);
	  if (!i2c_check_ack())
	  {
	    i2c_stop();
	    return(0);
	  }
	  i2c_stop();
	  return 1;
}

bool ICACHE_FLASH_ATTR areTresholdsExceeded(int16_t cTemp, uint16_t cHum)
{
	params* parameters = NULL;
	parameters=readParams();
	if(parameters)
	{
		if(cTemp > parameters->tempMaxTreshold || cTemp < parameters->tempMinTreshold)
		{
			return true;
		}
		if(cHum > parameters->humMaxTreshold || cHum < parameters->humMinTreshold)
		{
			return true;
		}
	}
	return false;
}

void ICACHE_FLASH_ATTR sensorInit()
{
		i2c_init();
	    first_SHT21_Init();
		os_timer_disarm(&main_timer);
		os_timer_setfn(&main_timer, (os_timer_func_t *)sensor_timerfunc, (void *)0);
		os_timer_arm(&main_timer, measurement_interval, 0);

}

uint16_t ICACHE_FLASH_ATTR readAdc()
{
	uint16_t adcVal;
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
	GPIO_OUTPUT_SET(ADC_PIN,1);
	adcVal=system_adc_read();
	adcVal= (float)adcVal/1024 *100*4.0;
	ets_uart_printf("ADC value :%d\r\n",adcVal);
	GPIO_OUTPUT_SET(ADC_PIN,0);
	return adcVal;
}
