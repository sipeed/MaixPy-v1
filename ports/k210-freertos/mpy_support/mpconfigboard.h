#ifndef __MPCONFIGBOARD_MAIX_H
#define __MPCONFIGBOARD_MAIX_H

#include "py/stream.h"
#include "py/runtime.h"
#include "py/misc.h"
#include "py/mphal.h"
#include "uart.h"
#include "uarths.h"

#define MICROPY_UARTHS_DEVICE 4
#define MICROPY_UART_NIC 1
typedef struct _ipconfig
{
	mp_obj_t ip;
	mp_obj_t gateway;
	mp_obj_t netmask;
	mp_obj_t ssid;
	mp_obj_t MAC;
}ipconfig;
#define MAIX_UART_BUF 2048
#endif//__MPCONFIGBOARD_MAIX_H