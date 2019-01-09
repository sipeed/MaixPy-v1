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
typedef struct _machine_uart_obj_t {
    mp_obj_base_t base;
	uint8_t uart_num;
    uint32_t baudrate;
    uint8_t bitwidth;
    uart_parity_t parity;
    uart_stopbit_t stop;
	bool active : 1;
	bool attached_to_repl: 1;
	bool rx_int_flag: 1;
	uint16_t read_buf_len;             
    uint16_t read_buf_head;    
    uint16_t read_buf_tail;
	uint16_t timeout;
	uint16_t timeout_char;
	uint16_t data_len;
    uint8_t *read_buf;
} machine_uart_obj_t;
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