/*
 * udp.h
 *
 *  Created on: 14.09.2016
 *      Author: Jacek
 */

#ifndef INCLUDE_UDP_H_
#define INCLUDE_UDP_H_

#define UDP_PORT 8000

#include "user_config.h"


char* ICACHE_FLASH_ATTR get_broadcast_address();
sint8 ICACHE_FLASH_ATTR UDP_sendData(char* datagram, uint16 size, char* ipAddress);




#endif /* INCLUDE_UDP_H_ */
