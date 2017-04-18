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

/******************************************************************************
 * FunctionName : user_init
 * Description  : Entry function for the application. It initializes memory, GPIO button PIN,
 * 				  checks if the button was pressed
 * Parameters   : none
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR user_init(void)
{
	Params* readPar;

	initMemory();
	readPar=readParams();
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
	GPIO_DIS_OUTPUT(4);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO4_U);
	//sprawdzac czy nie ma juz jakiegos trybu jak byl to reset techniczny

	if(!readPar->flags.sendNow && !readPar->flags.configMode)
		checkButton();
	if(readParams()->flags.configMode==true)
	{
		setConfig(true);
		if(readPar->flags.setupWifi==false)
		{
			restartToRF();
		}
		signalizeStatus(PENDING);

	}
	else
	{
		readPar=readParams();
		if(readPar->flags.sendNow ==true)
		{
			if(readPar->flags.setupWifi==false)
			{
				restartToRF();
			}
			signalizeStatus(PENDING);
			copyParams();
			getCurrParPtr()->sensorData.sendingInterval=1;
			storeParams();
		}

		sensorInit();
	}

	system_init_done_cb(initDone);


}

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : required by SDK 2.0
 * Parameters   : none
 * Returns      : none
*******************************************************************************/

uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void)
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

/******************************************************************************
 * FunctionName : user_rf_pre_init
 * Description  : Function that is executed before main application
 * Parameters   : none
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR user_rf_pre_init(void)
{
	system_set_os_print(0);
}

/******************************************************************************
 * FunctionName : initDone
 * Description  : Executed right after user_init, used for calling wifi related
 * 				  methods
 * Parameters   : none
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR initDone()
{
	Params* par=readParams();
	if(par->flags.setupWifi)
	{
		ets_uart_printf("Configuring wifi \r\n");
		if(par->flags.configMode != true)
		{
			setupWifi(par->connectionData.ssID,par->connectionData.pass);

			if(!rtcDisableTimer())
				ets_uart_printf("RTC disable failed!\r\n");
		}

		else
		{
			setupWifi(DEF_CONFIG_WIFI_SSID,DEF_CONFIG_WIFI_PASSWORD);
		}

	}


}

/******************************************************************************
 * FunctionName : isButtonPressed
 * Description  : chekcs if button tied to GPIO4 is currently pressed
 * Parameters   : none
 * Returns      : true if its pressed, false otherwise
*******************************************************************************/

bool ICACHE_FLASH_ATTR isButtonPressed()
{
	if(GPIO_INPUT_GET(4)==1)
		return false;
	return true;
}

/******************************************************************************
 * FunctionName : checkButton
 * Description  : checks the button and determines if theere was a long or short
 * 				  press. Then it sets appropriate flag accordingly
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
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
				config=readParams()->flags.configMode;
				copyParams();
				getCurrParPtr()->flags.configMode=!config;
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
			getCurrParPtr()->flags.sendNow=true;
			storeParams();
		}
}

/******************************************************************************
 * FunctionName : restartToRF
 * Description  : restarts the device and sets RF callibration and WiFi connection
 * 				  at wake-up
 * Parameters   : none
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR restartToRF()
{
	copyParams();
	getCurrParPtr()->flags.setupWifi=true;
	storeParams();
	system_deep_sleep_set_option(1);
	system_deep_sleep_instant(100);
}









