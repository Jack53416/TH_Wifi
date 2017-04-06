/*
 * rtc_i2c.c
 *
 *  Created on: 26.11.2016
 *      Author: Jacek
 */

#include "driver/rtc_i2c.h"

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

bool ICACHE_FLASH_ATTR waitForAck()
{
	  if (!i2c_check_ack())
	  {
	    i2c_stop();
	    return false;
	  }
	  return true;
}

uint8_t ICACHE_FLASH_ATTR bcdToDec(uint8_t val)
{
  return ( (val/16*10) + (val%16) );
}

uint8_t ICACHE_FLASH_ATTR decToBcd(uint8_t val)
{
  return ( (val/10*16) + (val%10) );
}

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

bool ICACHE_FLASH_ATTR rtcSaveUnixTime(time_t* rawTime)
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
