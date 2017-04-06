/*
 * tcp.c
 *
 *  Created on: 14.09.2016
 *      Author: Jacek
 */

/*
 * To Do:
 * 3)Dopracowac odbior ip serwera
 * 4)Czysczenie kodu
 *
 */
#include "tcp.h"

struct espconn conn1;
struct espconn connOut;
struct espconn* currentConnection;
uint8_t connErr=0;
uint8_t sendErr=0;
esp_tcp tcp1;
esp_tcp tcp2;
os_timer_t sendingTimer;//Timer ktory odpowiada za wysylke badz za oczekiwanie na poprawne polaczenie tcp/wifi
os_timer_t sendingTimer2;// za inne dane do wysylki
int measurementsToSend=-1;
bool connection_established=false;
bool stillSending=false;
bool sendingSuccesfull=false;
char* ptrToSndData=NULL;
uint16_t dataSize=0;
uint16_t dataOffset=0;
uint32_t recentDate=0;
uint16_t batteryVoltage=0;
void ICACHE_FLASH_ATTR SetupTCP(char* rIP, int rPort, struct espconn* connPtr,esp_tcp* tcpPtr )
{
	char temp[50];
	tcpPtr->remote_port=rPort;
	uint32_t ip = ipaddr_addr(rIP);
#ifdef DEBUG
	os_sprintf(temp, "SERVER ip:" IPSTR "\r\n", IP2STR(&(ip)));
	ets_uart_printf(temp);
#endif
	os_memcpy(tcpPtr->remote_ip, &ip, 4);
	tcpPtr->local_port=TCP_PORT;//espconn_port();
#ifdef DEBUG
	ets_uart_printf("Port: %d \r\n",tcpPtr->local_port);
#endif
	struct ip_info ipConfig;
	wifi_get_ip_info(STATION_IF,&ipConfig);
	os_memcpy(tcpPtr->local_ip,&ipConfig.ip,4);

	connPtr->type=	ESPCONN_TCP;
	connPtr->state = ESPCONN_NONE;
	connPtr->proto.tcp=tcpPtr;
	espconn_regist_disconcb(connPtr,DisconnectCB);
	espconn_regist_connectcb(connPtr,ConnectCB);
	espconn_regist_recvcb(connPtr, RecvCB);
	espconn_regist_reconcb(connPtr,ReconnectCB);
	espconn_regist_sentcb(connPtr,SentCB);
}


void ICACHE_FLASH_ATTR ConnectCB(void *arg )
{
	struct espconn *pespconn = arg;
	currentConnection=pespconn;
	sendingSuccesfull=false;
	if(measurementsToSend>=0)
	{
		ptrToSndData=ToOneString(measurementsToSend, dataOffset);//ToSendFormat(readMeasurement(measurementsToSend));
		espconn_sent(pespconn,ptrToSndData,strlen(ptrToSndData));

	}
	else if(ptrToSndData)
	{
		espconn_sent(pespconn,ptrToSndData,(HEAD_SIZE+dataSize));
	}
	os_delay_us(500000);
}

void ICACHE_FLASH_ATTR ReconnectCB(void* arg, sint8 err)
{
#ifdef DEBUG
	ets_uart_printf("ReconnectCB\r\n");
#endif
	connection_established=false;
	connErr++;


}
void ICACHE_FLASH_ATTR DisconnectCB(void *arg)
{
#ifdef DEBUG
	ets_uart_printf("DisconnectCB\r\n");
#endif
	connection_established=false;
}

void ICACHE_FLASH_ATTR SentCB(void* arg)
{
	sendingSuccesfull=true;
	ets_uart_printf("WYSLANO Z SUKCESEM\r\n");
	sendErr=0;
}

void ICACHE_FLASH_ATTR RecvCB(void* arg, char* pData, unsigned short len)
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
		rest=AnylyzeReceived(pEnd,&pEnd,rest,pEspConn);
	}

}

short ICACHE_FLASH_ATTR AnylyzeReceived(char* pRec, char**pEnd, unsigned short len,struct espconn *pEspConn)
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
                //i++;
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
	ResolveMode(mode, data,pEspConn);
	if(data)
		os_free(data);
    return len-i;
}

bool ICACHE_FLASH_ATTR ResolveMode(short mode, char* data, struct espconn *pEspConn)
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
		if(!data)
			return false;
		if(isValidIp(data))
		{
			copyParams();
			strcpy(getCurrParPtr()->serverIp,data);
			storeParams();

		}
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
			getCurrParPtr()->sendingInterval=intPar;
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
			getCurrParPtr()->remoteTcpPort=intPar;
			storeParams();
			if(pEspConn)
							espconn_sent(pEspConn,"PORT OK\r\n",9);
		}
		break;
	case SS_ID:
		if(!data)
			return false;
		copyParams();
		strncpy(getCurrParPtr()->ssID,data,32);
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
			getCurrParPtr()->sleepTime_s=intPar;
			storeParams();
			if(pEspConn)
							espconn_sent(pEspConn,"SLEEP TIME OK\r\n",15);
		}
		break;
	case WIFI_PASS:
		if(!data)
			return false;
		copyParams();
		strncpy(getCurrParPtr()->pass,data,64);
		//strcpy(getCurrParPtr()->pass,data);
		storeParams();
		if(pEspConn)
						espconn_sent(pEspConn,"WIFI PASS OK\r\n",14);
		break;
	case SERVER_URL:
		if(!data)
			return false;
		copyParams();
		strncpy(getCurrParPtr()->ServerAddress,data,64);
		storeParams();
		if(pEspConn)
						espconn_sent(pEspConn,"URL OK\r\n",8);
		break;
	case SENSOR_ID:
		if(!data)
			return false;
		copyParams();
		strncpy(getCurrParPtr()->SensorID,data,64);
		storeParams();
		if(pEspConn)
						espconn_sent(pEspConn,"SENSOR ID OK\r\n",14);
		break;
	case PARAMS_REQ:
		tmp = ParamsToString();
		espconn_sent(pEspConn,tmp,strlen(tmp));
		if(tmp)
			os_free(tmp);
		break;
	case CONFIG_END:
		if(readParams()->configMode || readParams()->sendNow)
			{
				copyParams();
				getCurrParPtr()->sendNow=false;
				getCurrParPtr()->configMode=false;
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
bool ICACHE_FLASH_ATTR isValidIp(char* ipString)
{
	// Sprawdzam czy ip jest poprawne na zasadzie czy zawiera tylko  cyfry i spacje i czy nie jest za dlugie
	int i=0;
	if(!ipString)
		return false;
	while(ipString[i]!='\0')
	{
		if((ipString[i]<'0' || ipString[i]>'9') && ipString[i]!='.')
			return false;
		i++;
		if(i>15)
			return false;
	}
	return true;
}


void ICACHE_FLASH_ATTR sendMeasurements(int totalMeasurements, int interval)
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

void ICACHE_FLASH_ATTR sendMeasurements_cb(void* arg)
{
	//char* tmp;
	//os_timer_disarm(&sendingTimer);
	//os_timer_arm(&sendingTimer, SENDING_INTERVAL, 1);
	if(checkWifi(&sendingTimer))
	{
		//EstablishConnection(&sendingTimer);
		if(sendingSuccesfull) //jesli udalo sie wyslac poprzedni pakiet przechodzimy dalej, jak nie to poprawka
			{
				sendingSuccesfull=false;
				if(measurementsToSend > SEND_CHUNK)
					measurementsToSend-=SEND_CHUNK;
				else
					measurementsToSend=-1;
				 //mamy tablice stala wiec po prostu zmieniamy obecny index i dane sie same nadpisuja

			}
		else if(stillSending==false && !sendingSuccesfull)
		{
			ets_uart_printf("SERWER ADRESSS: %s\r\n", readParams()->ServerAddress);
			stillSending=true;
			if(measurementsToSend > SEND_CHUNK)
			{
				if(!ptrToSndData)
				{
					ptrToSndData=ToOneString(SEND_CHUNK,dataOffset);
					dataOffset+=SEND_CHUNK;
				}
			}
			else
			{
				if(!ptrToSndData)
					ptrToSndData=ToOneString(measurementsToSend,dataOffset);
			}
			http_post(readParams()->ServerAddress,ptrToSndData,"Accept: application/json\r\nContent-Type: application/json\r\n", http_cb);

		}

		if(measurementsToSend<0) //jak nie ma juz nic do wyslania wychodzimy z timera
		{
			//espconn_disconnect(&connOut);
			//espconn_delete(&connOut);
			storeDate(recentDate);
			time_t a= recentDate;
			rtcSaveUnixTime(&a);
			delMeasurement(true);
			os_timer_disarm(&sendingTimer);
			if(readParams()->sendingInterval != 1)
				fallAsleep(NO_RF_CALIBRATION);
			else
				fallAsleep(RF_CALIBRATION);
		}
	}

}

bool ICACHE_FLASH_ATTR isStillSending()
{
	return stillSending;
}

char* ICACHE_FLASH_ATTR ToSendFormat(measurement* data, bool IsLast)
{
	char* result=NULL;
	uint32_t offset_s=data->offset_time;

	result=(char*)os_malloc(MES_LENGTH+1);
	if(result)
	{
		os_sprintf(result,MES_VAL_TEMPLATE,data->temperature,data->humidity,offset_s);

	if(IsLast)
        result[MES_LENGTH]=']';
    else
        result[MES_LENGTH]=',';
	}

	return result;
}

char* ICACHE_FLASH_ATTR ToOneString(int measurementCount, uint16_t offset)
{
	int i;
	char macAddr[6];
	char macStr[sizeof(MAC)+1];
	//measurementCount++;
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
                tmp=ToSendFormat(readMeasurement(i+offset),false);
            else
            	tmp=ToSendFormat(readMeasurement(i+offset),true);
			if(tmp)
			{
				//strncat(result,tmp,MES_LENGTH+1);
				os_memcpy(result+BASIC_LENGTH+i*(MES_LENGTH+1),tmp,MES_LENGTH+1);
				os_free(tmp);
			}
		}
		result[rSize-3]='}';
		result[rSize-2]='\r';
		result[rSize-1]='\n';
		result[rSize]='\0';
	}
	ets_uart_printf("strlen:%d",strlen(result));
	ets_uart_printf("%s\r\n",result);
	return result;
}

char* ICACHE_FLASH_ATTR ParamsToString()
{
    char* result = NULL;
    int len;
    params* readPar= readParams();
    uint16_t rSize=strlen(PAR_TEMPLATE)+PAR_VAL_LENGTH+strlen(readPar->pass)+strlen(readPar->SensorID)+strlen(readPar->ServerAddress)+strlen(readPar->ssID)+ 15+3;
    result= (char*) os_malloc(rSize);
    if(!result)
    	return NULL;
    os_sprintf(result,"%s" PAR_VALUE_TEMPLATE,PAR_TEMPLATE,readPar->ServerAddress,readPar->sendingInterval,readPar->remoteTcpPort,readPar->sleepTime_s,
               readPar->ssID,readPar->pass,readPar->SensorID);
    len=strlen(result);
    result[len]='}';
    result[len+1]='\r';
    result[len+2]='\n';
    result[len+3]='\0';

   // ets_uart_printf("Params strlen:%d vs size:%d\r\n", strlen(result),rSize);

    return result;
}

/*Proptotyp*/
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

void ICACHE_FLASH_ATTR EstablishConnection(os_timer_t* timer)
{
		if(!connection_established) //Jesli juz jest polaczenie to po prostu wyslij//
		{
			sendErr=0;
			if(espconn_connect(&connOut)!=0)
			{
				os_timer_disarm(timer);
				os_timer_arm(timer,WIFI_TIMEOUT_INTERVAL,1); //Jak cos nie zadziala poczekaj 5 razy i odpusc
				connErr++;
#ifdef DEBUG
				ets_uart_printf("Espconn_connect error! connErr:%d\r\n",connErr);
#endif
			}
			else{connection_established=true;}

			if(connErr>TIMEOUT)
			{
				os_timer_disarm(timer);
				fallAsleep(RF_CALIBRATION);
			}
		}

}

void ICACHE_FLASH_ATTR SendData(enum RX_INFOS mode, char* data, uint16_t len)
{
	ptrToSndData=(char*)os_malloc(len*sizeof(char));
	dataSize=len;
	os_sprintf(ptrToSndData,"%d%s\n",mode,data);
	os_timer_disarm(&sendingTimer2);
	os_timer_setfn(&sendingTimer2, (os_timer_func_t *)SendData_cb, (void *)0);
	os_timer_arm(&sendingTimer2, SENDING_INTERVAL, 0);

}
void ICACHE_FLASH_ATTR SendData_cb(void* arg)
{
	checkWifi(&sendingTimer2);
	EstablishConnection(&sendingTimer2);
	if(sendingSuccesfull)
	{
		if(ptrToSndData)
		{
			os_free(ptrToSndData);
			sendingSuccesfull=false;
			ptrToSndData=NULL;
			dataSize=0;
		}
	}
}

bool ICACHE_FLASH_ATTR searchForNewParams(params* newPar)
{
	params* oldPar=readParams();
	if(oldPar->sendingInterval != newPar->sendingInterval ||
		oldPar->humMaxTreshold != newPar->humMaxTreshold ||
		oldPar->humMinTreshold != newPar->humMinTreshold ||
		oldPar->tempMaxTreshold != newPar->tempMaxTreshold ||
		oldPar->tempMinTreshold != newPar->tempMinTreshold ||
		oldPar->sleepTime_s != newPar->sleepTime_s)
	{
		return true;
	}
	if(strcmp(oldPar->SensorID, newPar->SensorID) != 0)
		return true;

	return false;
}

static int ICACHE_FLASH_ATTR jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

bool ICACHE_FLASH_ATTR  parseAnswer(char* dataString, int size)
{
    int jSize,i;
	jsmn_parser p;
	jsmntok_t t[20];
	params* parPtr=NULL;
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
            strncpy(parPtr->SensorID,dataString + t[i+1].start,t[i+1].end-t[i+1].start);
            parPtr->SensorID[t[i+1].end-t[i+1].start]='\0';
            ets_uart_printf("Sensor ID:%s\n",parPtr->SensorID);
			i++;
		}
        else if (jsoneq(dataString, &t[i], "sendingInterval") == 0)
        {
            temp = strtol(dataString + t[i+1].start,NULL,10);
            if(temp <= MAX_SEND_INT)
            	parPtr->sendingInterval=temp;
            ets_uart_printf("Sending interval :%d\n",parPtr->sendingInterval);
			i++;
        }
        else if (jsoneq(dataString, &t[i], "measurementInterval") == 0)
        {
            temp=strtol(dataString + t[i+1].start,NULL,10);
            if(temp <= MAX_SLEEP_TIME_S)
            	parPtr->sleepTime_s=temp;
            ets_uart_printf("sleep Time: %d\n",parPtr->sleepTime_s);
			i++;

        }
        else if(jsoneq(dataString,&t[i], "tMaxTreshold")==0)
        {
        	parPtr->tempMaxTreshold=strtol(dataString+t[i+1].start,NULL,10);
        	i++;
        }
        else if(jsoneq(dataString,&t[i], "tMinTreshold")==0)
		{
			parPtr->tempMinTreshold=strtol(dataString+t[i+1].start,NULL,10);
			i++;
		}
        else if(jsoneq(dataString,&t[i], "hMaxTreshold")==0)
		{
			parPtr->humMaxTreshold=strtol(dataString+t[i+1].start,NULL,10);
			i++;
		}
        else if(jsoneq(dataString,&t[i], "tMaxTreshold")==0)
		{
			parPtr->humMinTreshold=strtol(dataString+t[i+1].start,NULL,10);
			i++;
		}
    }
    if(searchForNewParams(parPtr))
    {
    	storeParams();
    }
    return true;
}

void ICACHE_FLASH_ATTR http_cb(char * response_body, int http_status, char * response_headers, int body_size)
{
	if (http_status != HTTP_STATUS_GENERIC_ERROR)
	{
		//ets_uart_printf("%s\n",response_body);
			sendingSuccesfull=true;
			signalizeStatus(GOOD);
			if(ptrToSndData)
			{
				os_free(ptrToSndData);
				ptrToSndData=NULL;
			}
		//ets_uart_printf("END C: %x\r\n",response_body[body_size+2]);
		if(!parseAnswer(response_body,body_size+2))
			ets_uart_printf("PARSOWANIE ZJEBANE \n");
	}
		else
		{
			sendingSuccesfull=false;
			connErr++;
			signalizeStatus(ERROR);
			if(connErr>= TIMEOUT)
			{
				if(ptrToSndData)
					os_free(ptrToSndData);
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
