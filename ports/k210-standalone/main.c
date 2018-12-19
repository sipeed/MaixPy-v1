/* Copyright 2018 Sipeed Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
//#include <stdint.h>
#include <stdio.h>
//#include <string.h>

#include "sleep.h"
#include "encoding.h"

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "lib/utils/pyexec.h"
#include "fpioa.h"
#include "gpio.h"
#include "lib/mp-readline/readline.h"
#include "lib/utils/interrupt_char.h"

#include "timer.h"
#include "sysctl.h"
#include "w25qxx.h"
#include "plic.h"
#include "uarths.h"
#include "lcd.h"
#include "spiffs-port.h"
#include <malloc.h>
#define UART_BUF_LENGTH_MAX 269
#define MPY_HEAP_SIZE 1 * 1024 * 1024
extern int mp_hal_stdin_rx_chr(void);


static char *stack_top;
#if MICROPY_ENABLE_GC
static char heap[MPY_HEAP_SIZE];
#endif

void do_str(const char *src, mp_parse_input_kind_t input_kind);
const uint8_t Banner[] = {"\n __  __              _____  __   __  _____   __     __ \n\
|  \\/  |     /\\     |_   _| \\ \\ / / |  __ \\  \\ \\   / /\n\
| \\  / |    /  \\      | |    \\ V /  | |__) |  \\ \\_/ / \n\
| |\\/| |   / /\\ \\     | |     > <   |  ___/    \\   /  \n\
| |  | |  / ____ \\   _| |_   / . \\  | |         | |   \n\
|_|  |_| /_/    \\_\\ |_____| /_/ \\_\\ |_|         |_|\n\
Official Site:http://www.sipeed.com/\n\
Wiki:http://maixpy.sipeed.com/\n"};
int main()
{
    uint64_t core_id = current_coreid();
    plic_init();
	set_csr(mie, MIP_MEIP);
	set_csr(mstatus, MSTATUS_MIE);
    if (core_id == 0)
    {
        sysctl_pll_set_freq(SYSCTL_PLL0,320000000);
		sysctl_pll_enable(SYSCTL_PLL1);
		sysctl_pll_set_freq(SYSCTL_PLL1,160000000);
		uarths_init();
        printf(Banner);
		printf("[MAIXPY]Pll0:freq:%d\r\n",sysctl_clock_get_freq(SYSCTL_CLOCK_PLL0));
		printf("[MAIXPY]Pll1:freq:%d\r\n",sysctl_clock_get_freq(SYSCTL_CLOCK_PLL1));
		sysctl->power_sel.power_mode_sel6 = 1;
		sysctl->power_sel.power_mode_sel7 = 1;
		uarths_set_irq(UARTHS_RECEIVE,on_irq_uarths_recv,NULL,1);
		uarths_config(115200,UARTHS_STOP_1);
		uarths_init();
        uint8_t manuf_id, device_id;
		while (1) {
			w25qxx_init(3);
			w25qxx_read_id(&manuf_id, &device_id);
			if (manuf_id != 0xFF && manuf_id != 0x00 && device_id != 0xFF && device_id != 0x00)
			    break;
		}
		w25qxx_enable_quad_mode();
        printf("[MAIXPY]Flash:0x%02x:0x%02x\n", manuf_id, device_id);
		my_spiffs_init();
	    int stack_dummy;
	    stack_top = (char*)&stack_dummy;
	    #if MICROPY_ENABLE_GC
	    gc_init(heap, heap + sizeof(heap));
	    #endif
	    mp_init();
	    readline_init0();
	    readline_process_char(27);
	    pyexec_frozen_module("boot.py");
	    #if MICROPY_REPL_EVENT_DRIVEN
            pyexec_event_repl_init();
			mp_hal_set_interrupt_char(CHAR_CTRL_C);
            char c = 0;
            for (;;) {
                int cnt = read_ringbuff(&c,1);
                if(cnt==0){continue;}
                if(pyexec_event_repl_process_char(c)) {
                    break;
                }
            }
	    #else
	        pyexec_friendly_repl();
	    #endif
	    mp_deinit();
	    msleep(1);
	    printf("prower off\n");
	    return 0;
    }
    while (1);
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

void gc_collect(void) {
    // WARNING: This gc_collect implementation doesn't try to get root
    // pointers from CPU registers, and thus may function incorrectly.
    void *dummy;
    gc_collect_start();
    gc_collect_root(&dummy, ((mp_uint_t)stack_top - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
    gc_collect_end();
    gc_dump_info();
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


