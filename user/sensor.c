/*
 * sensor.c
 *
 *  Created on: 15.09.2016
 *      Author: Jacek
 */

#include "sensor.h"
/*LOCAL os_timer_t main_timer; //timer w ramach którego wykonywane s¹ pomiary
uint32_t measurement_interval = 5;*/

/******************************************************************************
 * FunctionName : SHT21FirstInit
 * Description  :
 * Parameters   : none
 * Returns      : true on success, false on failure
*******************************************************************************/

bool ICACHE_FLASH_ATTR SHT21FirstInit(void)
{
	  i2c_init();
	  os_delay_us(SHT_POWER_UP_TIME_US); //time for sensor power-up, according to 5.1 chapter in Datasheet SHT21

	  //setting the resolution of the measurements
	  uint8_t command_resolution  = 0x80; //T=13 bit RH=10 bit
	  uint8_t default_config = SHTreadUserRegister();
	  if (default_config == 0xF)
		  return false; //We must read the content of the User Register due to protect the reserved bits of this
	  command_resolution=command_resolution|default_config;

	  if (SHTwriteUserRegister(command_resolution) == false)
		  return false; //zapisujemy do User Register wartosc okreslajaca dokladnosc pomiarow

	  return true;
}

/******************************************************************************
 * FunctionName : sensorTimerFunc
 * Description  : main sensor function during which measurements are performed
 * 				  and if necessary tcp function is called to initialize sending process
 * Parameters   : arg -- originally destined to use with timer. Timers require one
 * 				  argument.
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR  sensorTimerFunc(void *arg)
{
	uint32_t sendingInterval = 0;
	int mesCount=0;
	Params* currPar;
	Params* readPar;

	readPar=readParams();
	copyParams();
	currPar=getCurrParPtr();
	sendingInterval=currPar->sensorData.sendingInterval;

    saveTemperature(SHT21_GetVal(GET_SHT_TEMPERATURE));
    saveHumidity(SHT21_GetVal(GET_SHT_HUMIDITY));
    saveOffsetTime(rtcGetUnixTime());
    if(areTresholdsExceeded(getTemperature(),getHumidity()) && !currPar->flags.setupWifi)
    {
    	ets_uart_printf("Tresholds Exceeded\r\n");
    	currPar->flags.setupWifi=true;
    	currPar->sensorData.sendingInterval=1;
    	if(readPar->sensorData.sleepTime_s > ALERT_MES_INTERVAL)
    		currPar->sensorData.sleepTime_s= ALERT_MES_INTERVAL;
    	storeParams();
    	system_deep_sleep_set_option(RF_CALIBRATION);
    	system_deep_sleep_instant(100);
    }
    mesCount=readMeasurementCount(ALL);
   // if(mesCount<= MAX_MES_TOTAL) do nadpisywania wylaczone!
    	storeMeasurement();

	if(readMeasurementCount(ALL)<sendingInterval-1)
	{
		if(readPar->flags.setupWifi !=false)
		{
			copyParams();
			currPar->flags.setupWifi=false;
			storeParams();
		}

		fallAsleep(RF_DISABLED);
	}

	else if(readMeasurementCount(ALL)==sendingInterval-1)
	{
		if(readPar->flags.setupWifi != true)
		{
			copyParams();
			currPar->flags.setupWifi=true;
			storeParams();
		}

		fallAsleep(RF_CALIBRATION);
	}
	else if(!isStillSending())
	{
		saveVoltage(readAdc());
		sendMeasurements(readMeasurementCount(ALL),sendingInterval);
	}


}

/******************************************************************************
 * FunctionName : SHTreadUserRegister
 * Description  :
 * Parameters   : none
 * Returns      : command resolution on success, 0xF on failure
*******************************************************************************/

uint8_t ICACHE_FLASH_ATTR SHTreadUserRegister (void)
{
	  uint8_t user_reg_content = 0;
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

/******************************************************************************
 * FunctionName : SHTwriteUserRegister
 * Description  :
 * Parameters   : commandResolution --
 * Returns      : true on success, false on failure
*******************************************************************************/

bool ICACHE_FLASH_ATTR SHTwriteUserRegister (uint8_t commandResolution)
{
	  i2c_start();
	  i2c_writeByte(SHT21_ADDRESS);
	  if (!i2c_check_ack())
	  {
	    i2c_stop();
	    return(false);
	  }
	  i2c_writeByte(WRITE_USER_REG);
	  if (!i2c_check_ack())
	  {
	    i2c_stop();
	    return(false);
	  }
	  i2c_writeByte(commandResolution);
	  if (!i2c_check_ack())
	  {
	    i2c_stop();
	    return(false);
	  }
	  i2c_stop();
	  return true;
}

/******************************************************************************
 * FunctionName : areTresholdsExceeded
 * Description  : cheks if measured value exceeds configured tresholds
 * Parameters   : cTemp -- temperature value to be checked
 * 				  cHum -- humidity value to be checked
 * Returns      : true if at least one value exceeds treshold, false if
 * 				  both are in norm
*******************************************************************************/

bool ICACHE_FLASH_ATTR areTresholdsExceeded(int16_t cTemp, uint16_t cHum)
{
	Params* parameters = NULL;
	parameters=readParams();
	if(parameters)
	{
		if(cTemp > parameters->tresholds.tempMaxTreshold || cTemp < parameters->tresholds.tempMinTreshold)
		{
			return true;
		}
		if(cHum > parameters->tresholds.humMaxTreshold || cHum < parameters->tresholds.humMinTreshold)
		{
			return true;
		}
	}
	return false;
}

/******************************************************************************
 * FunctionName : sensorInit
 * Description  : Initializes I2C, SHT21 sensor and starts measurement
 * Parameters   : none
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR sensorInit()
{
		i2c_init();
		SHT21FirstInit();
	    sensorTimerFunc(NULL);
	    /*
		os_timer_disarm(&main_timer);
		os_timer_setfn(&main_timer, (os_timer_func_t *)sensorTimerFunc, (void *)0);
		os_timer_arm(&main_timer, measurement_interval, 0);*/ //Only one measurement in wakup session

}

/******************************************************************************
 * FunctionName : readAdc
 * Description  : Reads the voltage on ADC pin and maps it to value between 0-4V
 * Parameters   : none
 * Returns      : voltage value- 3 digits without coma(388 = 3.88V)
*******************************************************************************/

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
