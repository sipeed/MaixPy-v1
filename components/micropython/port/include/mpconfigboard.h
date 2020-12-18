#ifndef __MPCONFIGBOARD_MAIX_H
#define __MPCONFIGBOARD_MAIX_H

#include "py/stream.h"
#include "py/runtime.h"
#include "py/misc.h"
#include "py/mphal.h"
#include "uart.h"
#include "uarths.h"
#include "global_config.h"

/***********Uart module**************/
#define MAIX_UART_BUF 2048
#define MICROPY_UARTHS_DEVICE 4
/***********net mod**************/
#define ESP8285_BUF_SIZE 40960
#define MICROPY_UART_NIC 1
/***********freq mod******************/
#define FREQ_STORE_FILE_NAME "/freq.conf" //must start with '/'
#define FREQ_PLL0_MAX        1200000000UL //1800MHz max
#define FREQ_PLL0_DEFAULT    (CONFIG_CPU_DEFAULT_FREQ*2)
#define FREQ_PLL0_MIN        52000000UL
#define FREQ_PLL1_MAX        1200000000UL //1800MHz max
#define FREQ_PLL1_DEFAULT    400000000UL
#define FREQ_PLL1_MIN        26000000UL
#define FREQ_PLL2_DEFAULT    45158400UL


#define FREQ_CPU_MAX     600000000UL
#define FREQ_CPU_DEFAULT (CONFIG_CPU_DEFAULT_FREQ)
#define FREQ_CPU_MIN     (FREQ_PLL0_MIN/2)
#define FREQ_KPU_MAX     600000000UL
#define FREQ_KPU_DEFAULT FREQ_PLL1_DEFAULT
#define FREQ_KPU_MIN     FREQ_PLL1_MIN
// #define I2S_MAX_FREQ 0

// change REPL baud rate to 9600 when freq < REPL_BAUDRATE_9600_FREQ_THRESHOLD
#define REPL_BAUDRATE_9600_FREQ_THRESHOLD  60000000UL


/////////////////////////////////////////////////////////////////////////
typedef struct{
	uint32_t freq_cpu;
    uint32_t freq_pll1;
    uint8_t  kpu_div;
    uint32_t gc_heap_size;
} config_data_t;

void load_config_from_spiffs(config_data_t* config);
bool save_config_to_spiffs(config_data_t* config);


#endif//__MPCONFIGBOARD_MAIX_H
