#ifndef __MPCONFIGBOARD_MAIX_H
#define __MPCONFIGBOARD_MAIX_H

#include "py/stream.h"
#include "py/runtime.h"
#include "py/misc.h"
#include "py/mphal.h"
#include "uart.h"
#include "uarths.h"

#define MAIX_UART_BUF 2048
#define ESP8285_BUF_SIZE 2048 
#define MICROPY_UARTHS_DEVICE 4
#define MICROPY_UART_NIC 1

typedef struct _ipconfig_obj
{
	mp_obj_t ip;
	mp_obj_t gateway;
	mp_obj_t netmask;
	mp_obj_t ssid;
	mp_obj_t MAC;
}ipconfig_obj;

typedef struct _esp8285_obj
{
	mp_obj_t uart_obj;
	uint8_t buffer[ESP8285_BUF_SIZE];
	
}esp8285_obj;


#endif//__MPCONFIGBOARD_MAIX_H