/*****std lib****/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
/*****mpy****/
#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/stackctrl.h"
#include "lib/utils/pyexec.h"
#include "lib/mp-readline/readline.h"
#include "gccollect.h"
#if MICROPY_PY_THREAD
#include "mpthreadport.h"
#include "py/mpthread.h"
#endif
/*****bsp****/
#include "sleep.h"
#include "encoding.h"
#include "sysctl.h"
#include "plic.h"
//#include <devices.h>
/*****peripheral****/
#include "fpioa.h"
#include "gpio.h"
#include "timer.h"
#include "uarths.h"
//#include "spiffs-port.h"
/*****freeRTOS****/
#include "FreeRTOS.h"
#include "task.h"


//************************************************************************************************
//temp ops
mp_import_stat_t mp_vfs_import_stat(const char *path) {

    //if (st_mode & MP_S_IFDIR) {
    //    return MP_IMPORT_STAT_DIR;
    //} else {
    //    return MP_IMPORT_STAT_FILE;
    //}

    //if (SPIFFS_stat(&fs,path, &st) == 0) {
        return MP_IMPORT_STAT_FILE;
    //}else{
        //return MP_IMPORT_STAT_NO_EXIST;
    //}
    
}
//************************************************************************************************

#define UART_BUF_LENGTH_MAX 269
#define MPY_HEAP_SIZE 1 * 1024 * 1024
extern int mp_hal_stdin_rx_chr(void);

static char *stack_top;
#if MICROPY_ENABLE_GC
static char heap[MPY_HEAP_SIZE];
#endif

#define MP_TASK_PRIORITY        4
#define MP_TASK_STACK_SIZE      (16 * 1024)
#define MP_TASK_STACK_LEN       (MP_TASK_STACK_SIZE / sizeof(StackType_t))

STATIC StaticTask_t mp_task_tcb;
STATIC StackType_t mp_task_stack[MP_TASK_STACK_LEN] __attribute__((aligned (8)));
TaskHandle_t mp_main_task_handle;

void do_str(const char *src, mp_parse_input_kind_t input_kind);

const uint8_t Banner[] = {"\n __  __              _____  __   __  _____   __     __ \n\
|  \\/  |     /\\     |_   _| \\ \\ / / |  __ \\  \\ \\   / /\n\
| \\  / |    /  \\      | |    \\ V /  | |__) |  \\ \\_/ / \n\
| |\\/| |   / /\\ \\     | |     > <   |  ___/    \\   /  \n\
| |  | |  / ____ \\   _| |_   / . \\  | |         | |   \n\
|_|  |_| /_/    \\_\\ |_____| /_/ \\_\\ |_|         |_|\n\
Official Site:http://www.sipeed.com/\n\
Wiki:http://maixpy.sipeed.com/\n"};


void mp_task(
	#if MICROPY_PY_THREAD 
	void *pvParameter
	#endif
	) {
		volatile uint32_t sp = (uint32_t)get_sp();
#if MICROPY_PY_THREAD
		mp_thread_init(&mp_task_stack[0], MP_TASK_STACK_LEN);
#endif
soft_reset:
		// initialise the stack pointer for the main thread
		mp_stack_set_top((void *)sp);
		mp_stack_set_limit(MP_TASK_STACK_SIZE - 1024);
#if MICROPY_ENABLE_GC
		gc_init(heap, heap + sizeof(heap));
#endif
		mp_init();
    #if MICROPY_REPL_EVENT_DRIVEN
	    	readline_init0();
            readline_process_char(27);
			pyexec_event_repl_init();
			//pyexec_frozen_module("boot.py");
			char c = 0;
   			MP_THREAD_GIL_EXIT();//given gil
			for (;;) {
				int cnt = uarths_receive_data(&c,1);
				if(cnt==0){continue;}
				if(pyexec_event_repl_process_char(c)) {
					break;
				}
			}
    #else
			pyexec_friendly_repl();
    #endif
		mp_deinit();
		// msleep(1);
		printf("prower off\n");

		return 0;
}

int main()
{		
	/*todo interrupt init*/
	printf(Banner);

	printf("[MAIXPY]Pll0:freq:%d\r\n",sysctl_clock_get_freq(SYSCTL_CLOCK_PLL0));
	printf("[MAIXPY]Pll1:freq:%d\r\n",sysctl_clock_get_freq(SYSCTL_CLOCK_PLL1));
	sysctl_set_power_mode(SYSCTL_POWER_BANK6,SYSCTL_POWER_V33);
	sysctl_set_power_mode(SYSCTL_POWER_BANK7,SYSCTL_POWER_V33);
	
	uint8_t manuf_id, device_id;
	w25qxx_init_dma(3, 0);
	w25qxx_enable_quad_mode_dma();
	w25qxx_read_id_dma(&manuf_id, &device_id);
	w25qxx_sector_erase_dma(0x600000);
	printf("[MAIXPY]Flash:0x%02x:0x%02x\n", manuf_id, device_id);
	my_spiffs_init();
	/*
	xTaskCreateAtProcessor(0, // processor
					     mp_task, // function entry
					     "mp_task", //task name
					     MP_TASK_STACK_LEN, //stack_deepth
					     NULL, //function arg
					     MP_TASK_PRIORITY, //task priority
					     &mp_main_task_handle);//task handl
	*/
	mp_task();
}
void do_str(const char *src, mp_parse_input_kind_t input_kind) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, MP_EMIT_OPT_NONE, true);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
}

void nlr_jump_fail(void *val) {
    while (1);
}

#if !MICROPY_DEBUG_PRINTERS
// With MICROPY_DEBUG_PRINTERS disabled DEBUG_printf is not defined but it
// is still needed by esp-open-lwip for debugging output, so define it here.
#include <stdarg.h>
int mp_vprintf(const mp_print_t *print, const char *fmt, va_list args);
int DEBUG_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = mp_vprintf(MICROPY_DEBUG_PRINTER, fmt, ap);
    va_end(ap);
    return ret;
}
#endif


