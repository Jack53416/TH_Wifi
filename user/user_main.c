/*
	RTC nie trzeba eneablowac timera caly czas, raz ustawiony chodzi na stalym interwale
*/

#include "user_config.h"
#include "wifi.h"
#include "tcp.h"
#include "sensor.h"
#include "data_storage.h"
#include "led.h"

extern int ets_uart_printf(const char *fmt, ...);
void user_rf_pre_init(void);
void ICACHE_FLASH_ATTR initDone();
bool ICACHE_FLASH_ATTR isButtonPressed();
void ICACHE_FLASH_ATTR checkButton();
void ICACHE_FLASH_ATTR restartToRF();


void ICACHE_FLASH_ATTR user_init(void)
{
#ifdef DEBUG
	uart_init(BIT_RATE_74880, BIT_RATE_74880);
#endif
	params* readPar;

	initRTC_memory();
	readPar=readParams();
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
	GPIO_DIS_OUTPUT(4);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO4_U);
	/*sprawdzac czy nie ma juz jakiegos trybu jak byl to reset techniczny*/
	if(!readPar->sendNow && !readPar->configMode)
		checkButton();
	ets_uart_printf("Flash ID: %d\r\n", spi_flash_get_id());
	ets_uart_printf("Sleep type: %d\r\n", wifi_fpm_get_sleep_type());
	if(readParams()->configMode==true)
	{
		setConfig(true);
		if(readPar->SetupWifi==false)
		{
			restartToRF();
		}
		signalizeStatus(NONE);

	}
	else
	{
		readPar=readParams();
		if(readPar->sendNow ==true)
		{
			if(readPar->SetupWifi==false)
			{
				restartToRF();
			}
			signalizeStatus(NONE);
			copyParams();
			getCurrParPtr()->sendingInterval=1;
			storeParams();
		}

		sensorInit();
	}

	system_init_done_cb(initDone);


}

uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

void user_rf_pre_init(void)
{

}

void ICACHE_FLASH_ATTR initDone()
{
	params* par=readParams();
	if(par->SetupWifi)
	{
		ets_uart_printf("Configuring wifi \r\n");
		if(par->configMode != true)
		{
			setupWifi(par->ssID,par->pass);

			if(!rtcDisableTimer())
				ets_uart_printf("RTC disable failed!\r\n");
		}

		else
		{
			setupWifi(DEF_CONFIG_WIFI_SSID,DEF_CONFIG_WIFI_PASSWORD);
		}

	}


}

bool ICACHE_FLASH_ATTR isButtonPressed()
{
	if(GPIO_INPUT_GET(4)==1)
		return false;
	return true;
}

void ICACHE_FLASH_ATTR checkButton()
{
#define CHECK_INTERVAL 10
	bool config;
	uint16_t pressInt=0;
	while(isButtonPressed())
		{
			pressInt+=CHECK_INTERVAL;
			os_delay_us(CHECK_INTERVAL*1000);
			if(pressInt> LONG_PRESS)
			{
				config=readParams()->configMode;
				copyParams();
				getCurrParPtr()->configMode=!config;
				storeParams();
				break;
			}
			system_soft_wdt_feed();
		}
		ets_uart_printf("pressed for: %d ms\n", pressInt);

		if(pressInt>=CHECK_INTERVAL && pressInt < LONG_PRESS)
		{
			readParams();
			copyParams();
			getCurrParPtr()->sendNow=true;
			storeParams();
		}
}

void ICACHE_FLASH_ATTR restartToRF()
{
	copyParams();
	getCurrParPtr()->SetupWifi=true;
	storeParams();
	system_deep_sleep_set_option(1);
	system_deep_sleep_instant(100);
}

//wzor opisu funkcji...., moze kiedys :D

/******************************************************************************
 * FunctionName : user_devicefind_recv
 * Description  : Processing the received data from the host
 * Parameters   : arg -- Additional argument to pass to the callback function
 *                pusrdata -- The received data (or NULL when the connection has been closed!)
 *                length -- The length of received data
 * Returns      : none
*******************************************************************************/









