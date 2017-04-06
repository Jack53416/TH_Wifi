/*
 * udp.c
 *
 *  Created on: 14.09.2016
 *      Author: Jacek
 */
#include "udp.h"

struct espconn UDP_send;
esp_udp udp;



char* ICACHE_FLASH_ATTR get_broadcast_address()
{
	char* broadcast=(char*)os_malloc(16*sizeof(char));
	int i=0;
	uint16 ipAddress[4];
	uint16 netmask[4];
	uint16 result[4];
	struct ip_info ip;
	wifi_get_ip_info(STATION_IF, &ip);
	for(i=0; i < 4; i++)
	{
		ipAddress[i]=((uint16)(((uint8*)(&(ip.ip.addr)))[i]));
		netmask[i]=((uint16)(((uint8*)(&(ip.netmask.addr)))[i]));
		result[i] = ipAddress[i] & netmask[i];
		if(result[i]==0 && netmask[i]==0)
			result[i]=255;
	}
	os_sprintf(broadcast,IPSTR, result[0],result[1],result[2],result[3]);

	return broadcast;
}

sint8 ICACHE_FLASH_ATTR UDP_sendData(char* datagram, uint16 size, char* ipAddress)
{
	sint8 err;
	uint32_t ip = ipaddr_addr(ipAddress);
	UDP_send.type=ESPCONN_UDP;
	UDP_send.state = ESPCONN_NONE;
	UDP_send.proto.udp= &udp;
	UDP_send.proto.udp->remote_port=UDP_PORT;
	os_memcpy(UDP_send.proto.udp->remote_ip, &ip, 4);

	err = espconn_create(&UDP_send);
	err = espconn_send(&UDP_send, datagram, size);
	err = espconn_delete(&UDP_send);

	return err;
}



