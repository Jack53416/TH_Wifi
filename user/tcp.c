/*
 * tcp.c
 *
 *  Created on: 14.09.2016
 *      Author: Jacek
 */
/**
 * File responsible for TCP communication and sending measurments to the server
 */
#include "tcp.h"

struct espconn conn1;
uint8_t connErr=0;
esp_tcp tcp1;
os_timer_t sendingTimer;//Timer ktory odpowiada za wysylke badz za oczekiwanie na poprawne polaczenie tcp/wifi
int measurementsToSend=-1;
bool stillSending=false;
bool sendingSuccesfull=false;
char* ptrToSndData=NULL;
uint32_t recentDate=0;
uint16_t batteryVoltage=0;

/******************************************************************************
 * FunctionName : setupTCP
 * Description  : Preapares the TCP connection without the http protocol with
 * 				  local ip address
 * Parameters   : rIP -- remote ip address (for local use only! no dns support)
 * 				  rPort -- remote port
 * 				  connPtr -- pointer to espconn structure which stores the tcpPtr
 * 				  tcpPtr -- pointer to esp_tcp which stores the tcp connection
 * 				  			settings
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR setupTCP(const char* rIP, int rPort, struct espconn* connPtr,esp_tcp* tcpPtr )
{
	uint32_t ip = ipaddr_addr(rIP);
	struct ip_info ipConfig;

	tcpPtr->remote_port=rPort;
	os_memcpy(tcpPtr->remote_ip, &ip, 4);
	tcpPtr->local_port=TCP_PORT;//espconn_port();

	wifi_get_ip_info(STATION_IF,&ipConfig);
	os_memcpy(tcpPtr->local_ip,&ipConfig.ip,4);

	connPtr->type=	ESPCONN_TCP;
	connPtr->state = ESPCONN_NONE;
	connPtr->proto.tcp=tcpPtr;

	espconn_regist_disconcb(connPtr,disconnectCB);
	espconn_regist_connectcb(connPtr,connectCB);
	espconn_regist_recvcb(connPtr, recvCB);
	espconn_regist_reconcb(connPtr,reconnectCB);
	espconn_regist_sentcb(connPtr,sentCB);
}

/******************************************************************************
 * FunctionName : connectCB, reconnectCB, disconnectCb
 * Description  : callback functions for events related to local TCP connection
 * Parameters   : arg -- event related data
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR connectCB(void *arg )
{
	struct espconn *pespconn = arg;

}

void ICACHE_FLASH_ATTR reconnectCB(void* arg, sint8 err)
{

}
void ICACHE_FLASH_ATTR disconnectCB(void *arg)
{

}

void ICACHE_FLASH_ATTR sentCB(void* arg)
{
	sendingSuccesfull=true;
}

/******************************************************************************
 * FunctionName : recvCB
 * Description  : Handles the data received during local TCP connection.
 * 				  Mainly used for config mode purposes
 * Parameters   : arg -- pointer to pEspConn struct of current connection
 * 				  pData -- pointer to received data
 * 				  len -- length of received data
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR recvCB(void* arg, char* pData, unsigned short len)
{
	///Byly watpliwosci co tu sie dzieje to tlumacze:
	// format danych otrzymywanych przez esp wyobrazam sobie jako naglowek+dane wlasciwe, na naglowek przyjalem 4
	// znaki char.Dane wlasciwe moga miec dowolna dlugosc. Ponizej najpierw sprawdzam jaki jest naglowek, a przy okazji
	// zapisuje tez dane, nastepenie funkcja Resolve_mode stosuje odpowiednie czynnosci dla danych w zaleznosci od tego
	//jaki jest naglowek. Dla jasnosci naglowek w praktyce wyglada tak: 0x + 2 cyfry np 0x01
	struct espconn *pEspConn = (struct espconn *)arg;
	bool history_enable = false;
	char* pEnd=pData;
	unsigned short rest=len;
	while(rest>0)
	{
		rest=anylyzeReceived(pEnd,&pEnd,rest,pEspConn);
	}

}

/******************************************************************************
 * FunctionName : analyzeReceived
 * Description  : General function for hanling console config mode. It breaks the received string
 * 				  into n- number of smaller insturctions. Each instruction is separated with space
 * 				  and comprises of header(0x+digit) and value. Ex.(0x156)
 * Parameters   : pRec -- pointer to received data in string format
 * 				  pEnd -- end pointer, stores the place where the function ended
 * 				  		  after its iteration
 * 				  len -- length of data string (pRec)
 * 				  pEspConn -- pointer to esponn structure( for response puprposes)
 * Returns      : remaining length of the received message to analyze
*******************************************************************************/

short ICACHE_FLASH_ATTR anylyzeReceived(char* pRec, char**pEnd, unsigned short len,struct espconn *pEspConn)
{
    int i=0;
    char header[HEAD_SIZE];
    char* data=NULL;
    short mode;

    if(len>HEAD_SIZE)
		data= (char*)os_malloc((len-HEAD_SIZE+1)*(sizeof(char)));
    for(i=0;i<len;i++)
    {
		if(i<HEAD_SIZE)
        {
			header[i]=pRec[i];
		}
		else if(data)
		{

			if(pRec[i]==' ')
            {
                *pEnd=pRec+i+1;
                break;
            }
			else if(pRec[i] != '\r')
				data[i-HEAD_SIZE]=pRec[i];
		}
    }
    header[HEAD_SIZE]='\0';
    if(data)
        data[i-HEAD_SIZE]='\0';
    if(len!=i)
    {
        data=(char*)os_realloc(data,i*sizeof(char));
    }
    mode=strtol(header,NULL,16);
#ifdef DEBUG
	ets_uart_printf("header: %s \r\ndata:%s\r\n",header,data);
	ets_uart_printf("mode :%d \r\n",mode);
#endif
	resolveMode(mode, data,pEspConn);
	if(data)
		os_free(data);
    return len-i;
}

/******************************************************************************
 * FunctionName : resolveMode
 * Description  : It resolves the header of each instruction and takes the action
 * 				  accordingly
 * Parameters   : mode -- header stripped from 0x indentifier
 * 				  data -- value of the instruction
 * 				  pEspConn -- pointer to connection settings
 * Returns      : true on succesfull parameter update, false on failure
*******************************************************************************/

bool ICACHE_FLASH_ATTR resolveMode(short mode, char* data, struct espconn *pEspConn)
{
	//zwykly case dotyczacy co robic w zaleznosci od moda, nie potrzeba przesylac dlugosci char* data, bo jest to
	//juz pelnoprawny string i ma na koncu \0
	char* tmp;
	uint16_t intPar=0;
	int k=0;
	switch(mode)
	{
	case SERVER_IP:
#ifdef DEBUG
		ets_uart_printf("mode resolved: SERVER_IP \r\n");
#endif
#ifdef DEBUG
		else

			ets_uart_printf("IP NOT VALID \r\n");
#endif
		break;
	case SENDING_INT:
		if(!data)
			return false;
		intPar=strtol(data,NULL,10);
		if(intPar>0 && intPar < MAX_SEND_INT)
		{
			copyParams();
			getCurrParPtr()->sensorData.sendingInterval=intPar;
			storeParams();
			if(pEspConn)
				espconn_sent(pEspConn,"Interval OK\r\n",13);
		}
		break;
	case REM_TCP_PORT:
		if(!data)
			return false;
		intPar=strtol(data,NULL,10);
		if(intPar>1024)
		{
			copyParams();
			getCurrParPtr()->connectionData.remoteTcpPort=intPar;
			storeParams();
			if(pEspConn)
							espconn_sent(pEspConn,"PORT OK\r\n",9);
		}
		break;
	case SS_ID:
		if(!data)
			return false;
		copyParams();
		strncpy(getCurrParPtr()->connectionData.ssID,data,32);
		//strcpy(getCurrParPtr()->ssID,data);
		storeParams();
		if(pEspConn)
						espconn_sent(pEspConn,"SS_ID OK\r\n",10);
		break;
	case TIME_SET:
#ifdef DEBUG
		ets_uart_printf("mode resolved: TIME_REQUEST \r\n");
#endif
		storeDate(strtol(data,NULL,10));
		if(pEspConn)
						espconn_sent(pEspConn,"TIME OK\r\n",9);
		break;
	case SLEEP_TIME:
		if(!data)
			return false;
		intPar=strtol(data,NULL,10);
		if(intPar>0 && intPar<MAX_SLEEP_TIME_S)
		{
			copyParams();
			getCurrParPtr()->sensorData.sleepTime_s=intPar;
			storeParams();
			if(pEspConn)
							espconn_sent(pEspConn,"SLEEP TIME OK\r\n",15);
		}
		break;
	case WIFI_PASS:
		if(!data)
			return false;
		copyParams();
		strncpy(getCurrParPtr()->connectionData.pass,data,64);
		//strcpy(getCurrParPtr()->pass,data);
		storeParams();
		if(pEspConn)
						espconn_sent(pEspConn,"WIFI PASS OK\r\n",14);
		break;
	case SERVER_URL:
		if(!data)
			return false;
		copyParams();
		strncpy(getCurrParPtr()->connectionData.ServerAddress,data,64);
		storeParams();
		if(pEspConn)
						espconn_sent(pEspConn,"URL OK\r\n",8);
		break;
	case SENSOR_ID:
		if(!data)
			return false;
		copyParams();
		strncpy(getCurrParPtr()->sensorData.sensorID,data,64);
		storeParams();
		if(pEspConn)
						espconn_sent(pEspConn,"SENSOR ID OK\r\n",14);
		break;
	case PARAMS_REQ:
		tmp = paramsToString();
		espconn_sent(pEspConn,tmp,strlen(tmp));
		if(tmp)
			os_free(tmp);
		break;
	case CONFIG_END:
		if(readParams()->flags.configMode || readParams()->flags.sendNow)
			{
				copyParams();
				getCurrParPtr()->flags.sendNow=false;
				getCurrParPtr()->flags.configMode=false;
				storeParams();
			}
		system_deep_sleep(100);
		break;

	default:
#define WRONG_COMMAND "Wrong Command!\r\nAvalaible Commands:\r\n0x2 - Set sending interval\r\n0x4 - set wifi SS_ID\r\n0x6 - set wifi password\r\n0x7 - set measurement interval\r\n0x9 - set server URL\r\n0xA - set server ID\r\n0xB - get current parameter values\r\n0xC - get out of config mode, restart sensor\r\n"
		if(pEspConn)
		{
			espconn_sent(pEspConn,WRONG_COMMAND,strlen(WRONG_COMMAND));

		}

		break;
	}
	return true;
}

/******************************************************************************
 * FunctionName : sendMeasurements
 * Description  : Arms the sending timer if total measurments stored exceeds set
 * 				  interval
 * Parameters   : totalMeasurements -- number of measurements stored
 * 				  interval -- set interval
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR sendMeasurements(int totalMeasurements, int interval) //po co przekazywac skoro mozna zczytac bezposrednio?
{
	measurementsToSend=totalMeasurements;
	ets_uart_printf("Measurements To Send:%d\r\n",measurementsToSend);
	if(totalMeasurements>= interval)
	{
		recentDate=readDate();
		os_timer_disarm(&sendingTimer);
		os_timer_setfn(&sendingTimer, (os_timer_func_t *)sendMeasurements_cb, (void *)0);
		os_timer_arm(&sendingTimer, SENDING_INTERVAL, 1);
	}

}

/******************************************************************************
 * FunctionName : sendMeasurements_cb
 * Description  : Actual function that realizes the sending procedure. It chunks the
 * 				  data and initializes sending operations untill all data are sent.
 * 				  After each sucessfully sent chunk it is deleted and when there are no
 * 				  measurments to send it puts the device into sleep mode.
 * Parameters   : none (arg = NULL)
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR sendMeasurements_cb(void* arg)
{
	if(checkWifi(&sendingTimer))
	{
		if(sendingSuccesfull) //jesli udalo sie wyslac poprzedni pakiet przechodzimy dalej, jak nie to poprawka
			{
				sendingSuccesfull=false;
				ets_uart_printf("Measurements to send : %d\r\n", measurementsToSend);
			}
		else if(stillSending==false && !sendingSuccesfull)
		{
			ets_uart_printf("SERWER ADRESSS: %s\r\n", readParams()->connectionData.ServerAddress);
			stillSending=true;
			if(measurementsToSend > SEND_CHUNK)
			{
				if(!ptrToSndData)
				{
					ptrToSndData=toOneString(SEND_CHUNK);
				}
			}
			else
			{
				if(!ptrToSndData)
					ptrToSndData=toOneString(measurementsToSend);
			}
			http_post(readParams()->connectionData.ServerAddress,&ptrToSndData,"Accept: application/json\r\nContent-Type: application/json\r\n", http_cb);

		}

		if(measurementsToSend <= 0) //jak nie ma juz nic do wyslania wychodzimy z timera
		{
			storeDate(recentDate);
			rtcSaveUnixTime((time_t *)&recentDate);

			os_timer_disarm(&sendingTimer);
			if(readParams()->sensorData.sendingInterval != 1)
				fallAsleep(RF_DISABLED);
			else
				fallAsleep(RF_CALIBRATION);
		}
	}

}

bool ICACHE_FLASH_ATTR isStillSending()
{
	return stillSending;
}

/******************************************************************************
 * FunctionName : toSendFormat
 * Description  : Wrapps the one measurement instance into JSON array
 * Parameters   : data -- measurment to wrapp into JSON string
 * 				  isLast -- specified if the measurment is last in the JSON array(
 * 							JSON format requires proper array ending)
 * Returns      : measurement wrapped into specified JSON string
*******************************************************************************/

char* ICACHE_FLASH_ATTR toSendFormat(Measurement* data, bool IsLast)
{
	char* result=NULL;
	uint32_t timestamp=data->timestamp;

	result=(char*)os_malloc(MES_LENGTH+1);
	if(result)
	{
		os_sprintf(result,MES_VAL_TEMPLATE,data->temperature,data->humidity,timestamp);

	if(IsLast)
        result[MES_LENGTH]=']';
    else
        result[MES_LENGTH]=',';
	}

	return result;
}

/******************************************************************************
 * FunctionName : toOneString
 * Description  : Computes whole JSON string with all measurements, MAC address and
 * 				  battery voltage
 * Parameters   : measurementCount
 * Returns      : full JSON response, containing specified number of measurements,
 * 				  sensor MAC address and battery voltage
*******************************************************************************/

char* ICACHE_FLASH_ATTR toOneString(int measurementCount)
{
	int i;
	char macAddr[6];
	char macStr[sizeof(MAC)+1];

	uint16_t rSize=BASIC_LENGTH+(MES_LENGTH+1)*(measurementCount)+3;

	ets_uart_printf("MEs count %d, rSize %d bas len %d mes len %d\r\n",measurementCount,rSize,BASIC_LENGTH,MES_LENGTH);

	char* result= (char*)os_malloc(rSize+1);
	char* tmp=NULL;
	if(result)
	{
		wifi_get_macaddr(STATION_IF, macAddr);
		os_sprintf(macStr,MACSTR,MAC2STR(macAddr));
	    os_sprintf(result,MES_TEMPLATE,macStr,batteryVoltage);
		for(i=0;i<measurementCount;i++)
		{
		    if(i!= measurementCount-1)
                tmp=toSendFormat(readMeasurement(i),false);
            else
            	tmp=toSendFormat(readMeasurement(i),true);
			if(tmp)
			{
				os_memcpy(result+BASIC_LENGTH+i*(MES_LENGTH+1),tmp,MES_LENGTH+1);
				os_free(tmp);
			}
			system_soft_wdt_feed();
		}
		result[rSize-3]='}';
		result[rSize-2]='\r';
		result[rSize-1]='\n';
		result[rSize]='\0';
	}
	ets_uart_printf("strlen:%d",strlen(result));
	//ets_uart_printf("%s\r\n",result);
	return result;
}

/******************************************************************************
 * FunctionName : paramsToString
 * Description  : Computes JSON string containing all stored parameters
 * Parameters   : none
 * Returns      : JSON string containg all crucial parameters that are stored in
 * 				  flash memory
*******************************************************************************/

char* ICACHE_FLASH_ATTR paramsToString()
{
    char* result = NULL;
    int len;
    Params* readPar= readParams();
    uint16_t rSize=strlen(PAR_TEMPLATE)+PAR_VAL_LENGTH+strlen(readPar->connectionData.pass) +
    		strlen(readPar->sensorData.sensorID)+strlen(readPar->connectionData.ServerAddress) +
			strlen(readPar->connectionData.ssID)+ 15+3;

    result= (char*) os_malloc(rSize);

    if(!result)
    	return NULL;
    os_sprintf(result,"%s" PAR_VALUE_TEMPLATE,PAR_TEMPLATE,readPar->connectionData.ServerAddress,
    		readPar->sensorData.sendingInterval,readPar->connectionData.remoteTcpPort,
			readPar->sensorData.sleepTime_s, readPar->connectionData.ssID,readPar->connectionData.pass,
			readPar->sensorData.sensorID);

    len=strlen(result);
    result[len]='}';
    result[len+1]='\r';
    result[len+2]='\n';
    result[len+3]='\0';

   // ets_uart_printf("Params strlen:%d vs size:%d\r\n", strlen(result),rSize);

    return result;
}

/******************************************************************************
 * FunctionName : checkWifi
 * Description  : REWORK NEEDED !
 * Parameters   : timer --
 * Returns      : true if WiFi connection is established, false otherwise
*******************************************************************************/

bool ICACHE_FLASH_ATTR checkWifi(os_timer_t* timer)
{
	if(wifi_station_get_connect_status()!=STATION_GOT_IP)
	{
		ets_uart_printf("WIFI CHECKED NO CONNECTION! \r\n");
		if(timer)
		{
			os_timer_disarm(timer);
			os_timer_arm(timer,WIFI_TIMEOUT_INTERVAL,1); //jesli nie ma wifi, poczekaj
		}
		return false;
	}
	return true;
}

/******************************************************************************
 * FunctionName : searchForNewParams
 * Description  : checks if the parameters values specified in the argument structure
 * 				  are different than those stored in flash memory
 * Parameters   : newPar -- parameters to be checked
 * Returns      : true if specified parameters are different than stored ones,
 * 				  false otherwise
*******************************************************************************/

bool ICACHE_FLASH_ATTR searchForNewParams(Params* newPar)
{
	Params* oldPar=readParams();
	if(oldPar->sensorData.sendingInterval != newPar->sensorData.sendingInterval ||
		oldPar->tresholds.humMaxTreshold != newPar->tresholds.humMaxTreshold ||
		oldPar->tresholds.humMinTreshold != newPar->tresholds.humMinTreshold ||
		oldPar->tresholds.tempMaxTreshold != newPar->tresholds.tempMaxTreshold ||
		oldPar->tresholds.tempMinTreshold != newPar->tresholds.tempMinTreshold ||
		oldPar->sensorData.sleepTime_s != newPar->sensorData.sleepTime_s)
	{
		return true;
	}
	if(strcmp(oldPar->sensorData.sensorID, newPar->sensorData.sensorID) != 0)
		return true;

	return false;
}

/******************************************************************************
 * FunctionName : jsoneq
 * Description  : Compares the specified strings in JSON structure with given keys
 * Parameters   : json -- Original JSON string that is parsed
 * 				  tok -- token table(token is a word in JSON string)
 * 				  s -- searched key
 * Returns      : 0 if key was found, -1 otherwise
*******************************************************************************/

static int ICACHE_FLASH_ATTR jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

/******************************************************************************
 * FunctionName : parseAnswer
 * Description  : parses the http response from the server
 * Parameters   : dataString -- server response
 * 				  size -- size of the response
 * Returns      : true if JSON structure was semantically correct, false otherwise
*******************************************************************************/

bool ICACHE_FLASH_ATTR  parseAnswer(char* dataString, int size)
{
    int jSize,i;
	jsmn_parser p;
	jsmntok_t t[20];
	Params* parPtr=NULL;
	uint32_t temp;
    jsmn_init(&p);
    jSize = jsmn_parse(&p, dataString, size, t, sizeof(t)/sizeof(t[0]));
    ets_uart_printf("jSize:%d\r\n",jSize);
    if (jSize < 0)
    {
		return false;
    }

    readParams();
    copyParams();
    parPtr=getCurrParPtr();

    for (i = 1; i < jSize; i++)
    {
        if (jsoneq(dataString, &t[i], "info") == 0)
        {

			if(!(strncmp("success",dataString+t[i+1].start,t[i+1].end-t[i+1].start)==0))
            {
				ets_uart_printf("Not success?\r\n");
                return false;
            }
            i++;

        }
        else if (jsoneq(dataString, &t[i], "timestamp") == 0)
        {

			recentDate=strtol(dataString + t[i+1].start,NULL,0);
			i++;
        }
        else if (jsoneq(dataString, &t[i], "settings") == 0)
        {
			i++;
        }
        else if (jsoneq(dataString, &t[i], "name") == 0)
        {
            strncpy(parPtr->sensorData.sensorID,dataString + t[i+1].start,t[i+1].end-t[i+1].start);
            parPtr->sensorData.sensorID[t[i+1].end-t[i+1].start]='\0';
            ets_uart_printf("Sensor ID:%s\n",parPtr->sensorData.sensorID);
			i++;
		}
        else if (jsoneq(dataString, &t[i], "sendingInterval") == 0)
        {
            temp = strtol(dataString + t[i+1].start,NULL,10);
            if(temp <= MAX_SEND_INT)
            	parPtr->sensorData.sendingInterval=temp;
            ets_uart_printf("Sending interval :%d\n",parPtr->sensorData.sendingInterval);
			i++;
        }
        else if (jsoneq(dataString, &t[i], "measurementInterval") == 0)
        {
            temp=strtol(dataString + t[i+1].start,NULL,10);
            if(temp <= MAX_SLEEP_TIME_S)
            	parPtr->sensorData.sleepTime_s=temp;
            ets_uart_printf("sleep Time: %d\n",parPtr->sensorData.sleepTime_s);
			i++;

        }
        else if(jsoneq(dataString,&t[i], "tMaxTreshold")==0)
        {
        	parPtr->tresholds.tempMaxTreshold=strtol(dataString+t[i+1].start,NULL,10);
        	i++;
        }
        else if(jsoneq(dataString,&t[i], "tMinTreshold")==0)
		{
			parPtr->tresholds.tempMinTreshold=strtol(dataString+t[i+1].start,NULL,10);
			i++;
		}
        else if(jsoneq(dataString,&t[i], "hMaxTreshold")==0)
		{
			parPtr->tresholds.humMaxTreshold=strtol(dataString+t[i+1].start,NULL,10);
			i++;
		}
        else if(jsoneq(dataString,&t[i], "tMaxTreshold")==0)
		{
			parPtr->tresholds.humMinTreshold=strtol(dataString+t[i+1].start,NULL,10);
			i++;
		}
    }
    if(searchForNewParams(parPtr))
    {
    	storeParams();
    }
    return true;
}

/******************************************************************************
 * FunctionName : http_cb
 * Description  : HTTP callback function it handles the received response for the HTTP
 * 				  request
 * Parameters   : response_body -- response text
 * 				  http_status -- overall http status sent with the response
 * 				  response_headers -- HTTP headers of the response
 * 				  body_size -- size of the response
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR http_cb(char * response_body, int http_status, char * response_headers, int body_size)
{
	if (http_status != HTTP_STATUS_GENERIC_ERROR)
	{
		//ets_uart_printf("%s\n",response_body);
			sendingSuccesfull=true;
			signalizeStatus(OK);
			if(ptrToSndData)
			{
				os_free(ptrToSndData);
				ptrToSndData=NULL;
			}
			else
				ets_uart_printf("ptrToSnd is NULL! \r\n");
		//ets_uart_printf("END C: %x\r\n",response_body[body_size+2]);
		if(!parseAnswer(response_body,body_size+2))
			ets_uart_printf("PARSOWANIE ZJEBANE \n");
		measurementsToSend -= SEND_CHUNK;
		deleteMeasurements(RTC_BLOCK_CHUNK);
		ets_uart_printf("deleted one chunk! now on RTC :%d and in total :%d\r\n",
				readMeasurementCount(ONLY_RTC), readMeasurementCount(ALL));
	}
		else
		{
			sendingSuccesfull=false;
			connErr++;
			signalizeStatus(FAIL);
			if(connErr>= TIMEOUT)
			{
				if(ptrToSndData)
					os_free(ptrToSndData);
				else
					ets_uart_printf("ptrToSnd is NULL! \r\n");
				os_timer_disarm(&sendingTimer);
				fallAsleep(RF_CALIBRATION);
			}
		}

	stillSending=false;


}

void ICACHE_FLASH_ATTR saveVoltage(uint16_t voltage)
{
	batteryVoltage=voltage;
}
