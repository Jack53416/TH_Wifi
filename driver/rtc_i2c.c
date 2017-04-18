/*
 * rtc_i2c.c
 *
 *  Created on: 26.11.2016
 *      Author: Jacek
 */

#include "driver/rtc_i2c.h"

/******************************************************************************
 * FunctionName : rtc_init
 * Description  : Initializes the RTC on first power on
 * Parameters   : none
 * Returns      : true on success, false on failure
*******************************************************************************/

bool ICACHE_FLASH_ATTR rtc_init()
{
	i2c_start();
	i2c_writeByte(RTC_WRITE_ADDRESS);
	if(!waitForAck())
			return false;
	i2c_writeByte(0);
	if(!waitForAck())
			return false;
	i2c_writeByte(0);
	if(!waitForAck())
			return false;
	i2c_writeByte(0);
	if(!waitForAck())
			return false;
	i2c_stop();

	return true;
}

/******************************************************************************
 * FunctionName : rtcSetTime
 * Description  : feeds the specified time to the RTC module
 * Parameters   : timeInfo -- desired time to be written
 * Returns      : true on success, false on failure
*******************************************************************************/

bool ICACHE_FLASH_ATTR rtcSetTime(struct tm* timeInfo)
{
	i2c_start();
	i2c_writeByte(RTC_WRITE_ADDRESS);
	if(!waitForAck())
		return false;
	i2c_writeByte(SECONDS_REG_ADDR);
	if(!waitForAck())
		return false;


	i2c_writeByte(decToBcd(timeInfo->tm_sec)); //sec
	if(!waitForAck())
			return false;
	i2c_writeByte(decToBcd(timeInfo->tm_min)); //min
	if(!waitForAck())
			return false;
	i2c_writeByte(decToBcd(timeInfo->tm_hour)); // hour
	if(!waitForAck())
			return false;
	i2c_writeByte(decToBcd(timeInfo->tm_mday));	 //day
	if(!waitForAck())
			return false;
	i2c_writeByte(decToBcd(timeInfo->tm_wday)); //w day
	if(!waitForAck())
			return false;
	i2c_writeByte(decToBcd(timeInfo->tm_mon+1)); //century months
	if(!waitForAck())
			return false;
	i2c_writeByte(decToBcd(timeInfo->tm_year-100)); //years
	if(!waitForAck())
			return false;

	i2c_stop();
	return true;
}

/******************************************************************************
 * FunctionName : rtcGetTime
 * Description  : gets the current time from the RTC module
 * Parameters   : timePtr -- pointer to tm struct where actual time will be stored
 * Returns      : true on success, false on failure
*******************************************************************************/

bool ICACHE_FLASH_ATTR rtcGetTime(struct tm* timePtr)
{
	uint8_t tmp=0;

	i2c_start();
	i2c_writeByte(RTC_WRITE_ADDRESS);
	if(!waitForAck())
		return false;
	i2c_writeByte(SECONDS_REG_ADDR);
	if(!waitForAck())
		return false;



	i2c_stop(); //restart
	i2c_start();

	i2c_writeByte(RTC_READ_ADDRESS);
	if(!waitForAck())
		return false;

	tmp=i2c_readByte() & 0x7F;
	timePtr->tm_sec=bcdToDec(tmp);
	i2c_send_ack(1);
	tmp=0;

	tmp=i2c_readByte() & 0x7F;
	timePtr->tm_min=bcdToDec(tmp);
	i2c_send_ack(1);
	tmp=0;

	tmp=i2c_readByte() & 0x3F;
	timePtr->tm_hour=bcdToDec(tmp);
	i2c_send_ack(1);
	tmp=0;

	tmp=i2c_readByte() & 0x3F;
	timePtr->tm_mday = bcdToDec(tmp);
	i2c_send_ack(1);
	tmp=0;

	tmp=i2c_readByte() & 0x7;
	timePtr->tm_wday = bcdToDec(tmp);
	i2c_send_ack(1);
	tmp=0;

	tmp=i2c_readByte() & 0x1F;
	timePtr->tm_mon=bcdToDec(tmp)-1;
	i2c_send_ack(1);
	tmp=0;

	tmp=i2c_readByte();
	timePtr->tm_year = bcdToDec(tmp)+100;
	i2c_send_ack(0);
	i2c_stop();
	return true;
}

/******************************************************************************
 * FunctionName : waitForAck
 * Description  : waits for acknowledge response from RTC module
 * Parameters   : none
 * Returns      : true on ACK, false on failure
*******************************************************************************/

bool ICACHE_FLASH_ATTR waitForAck()
{
	  if (!i2c_check_ack())
	  {
	    i2c_stop();
	    return false;
	  }
	  return true;
}

/******************************************************************************
 * FunctionName : bcdToDec
 * Description  : Converts specified value from bcd format to decimal
 * Parameters   : val -- bcd value for conversion
 * Returns      : decimal value
*******************************************************************************/

uint8_t ICACHE_FLASH_ATTR bcdToDec(uint8_t val)
{
  return ( (val/16*10) + (val%16) );
}

/******************************************************************************
 * FunctionName : decToBcd
 * Description  : Converts specified value from decimal to bcd format
 * Parameters   : val -- decimal value for conversion
 * Returns      : Bcd format value
*******************************************************************************/

uint8_t ICACHE_FLASH_ATTR decToBcd(uint8_t val)
{
  return ( (val/10*16) + (val%10) );
}

/******************************************************************************
 * FunctionName : rtcGetUnixTime
 * Description  : gets the actual time from the RTC module in unix format
 * Parameters   : none
 * Returns      : actual unix time(according to RTC)
*******************************************************************************/

time_t ICACHE_FLASH_ATTR rtcGetUnixTime()
{
	struct tm timeRead;
	time_t unix=0;

	if(!rtcGetTime(&timeRead))
		ets_uart_printf("Get time failed\r\n");
	else
		unix=mktime(&timeRead);

	return unix;
}

/******************************************************************************
 * FunctionName : rtcSaveUnixTime
 * Description  : Feeds the RTC with desired time in unix format
 * Parameters   : rawTime -- pointer to desired unix time that is going to be
 * 				  			 stored
 * Returns      : true on success, false on failure
*******************************************************************************/

bool ICACHE_FLASH_ATTR rtcSaveUnixTime(const time_t* rawTime)
{
	struct tm * ptm;
	ptm = gmtime (rawTime);
	i2c_init();
	if(!rtcSetTime(ptm))
	{
		ets_uart_printf("date store failed!\r\n");
		return false;
	}

	return true;

}

/******************************************************************************
 * FunctionName : rtcSetTimer
 * Description  : sets the timer functionality of RTC for a specified time. After
 * 				  specified time RTC will produce square pulse. See PCF documentation
 * 				  for more details
 * Parameters   : time_m -- timer time in minutes
 * Returns      : true on success, false on failure
*******************************************************************************/

bool ICACHE_FLASH_ATTR rtcSetTimer(uint8_t time_m)
{

	i2c_start();
	i2c_writeByte(RTC_WRITE_ADDRESS);
		if(!waitForAck())
			return false;
		i2c_writeByte(CONTROL_REG2_ADDR);
		if(!waitForAck())
			return false;
		i2c_writeByte(0x11);
		if(!waitForAck())
			return false;

	os_delay_us(50);

	i2c_start();
	i2c_writeByte(RTC_WRITE_ADDRESS);
		if(!waitForAck())
			return false;
		i2c_writeByte(TIMER_CONTROL_REG_ADDR);
		if(!waitForAck())
			return false;

	i2c_writeByte(0x83); //timer eneabled + 1/60hz timer frequency
	if(!waitForAck())
				return false;
	i2c_writeByte(time_m);
	if(!waitForAck())
				return false;
	i2c_stop();

	return true;
}

/******************************************************************************
 * FunctionName : rtcDisableTimer
 * Description  : disables RTC timer functionality
 * Parameters   : none
 * Returns      : true on success, false on failure
*******************************************************************************/

bool ICACHE_FLASH_ATTR rtcDisableTimer()
{
	i2c_start();
	i2c_writeByte(RTC_WRITE_ADDRESS);
		if(!waitForAck())
			return false;
		i2c_writeByte(CONTROL_REG2_ADDR);
		if(!waitForAck())
			return false;
		i2c_writeByte(0);
		if(!waitForAck())
			return false;
	i2c_stop();

	return true;

}
