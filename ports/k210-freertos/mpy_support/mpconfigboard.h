#ifndef __MPCONFIGBOARD_MAIX_H
#define __MPCONFIGBOARD_MAIX_H

#include "py/stream.h"
#include "py/runtime.h"
#include "py/misc.h"
#include "py/mphal.h"
#include "uart.h"
#include "uarths.h"

/***********Uart module**************/
#define MAIX_UART_BUF 2048
#define MICROPY_UARTHS_DEVICE 4
/***********net mod**************/
#define ESP8285_BUF_SIZE 2048 
#define MICROPY_UART_NIC 1

#endif//__MPCONFIGBOARD_MAIX_H