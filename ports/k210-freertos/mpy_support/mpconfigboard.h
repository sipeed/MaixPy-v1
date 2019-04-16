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
#define ESP8285_BUF_SIZE 4096 
#define MICROPY_UART_NIC 1
/***********freq mod******************/
#define FREQ_STORE_ADDR 0x600000
#define FREQ_READ_NUM 2
#define PLL0_MAX_OUTPUT_FREQ 832000000UL
#define PLL1_MAX_OUTPUT_FREQ 400000000UL
#define PLL2_MAX_OUTPUT_FREQ 45158400UL
#define CPU_MAX_FREQ (PLL0_MAX_OUTPUT_FREQ / 2)
#define KPU_MAX_FREQ PLL1_MAX_OUTPUT_FREQ
#define I2S_MAX_FREQ 0
#endif//__MPCONFIGBOARD_MAIX_H
