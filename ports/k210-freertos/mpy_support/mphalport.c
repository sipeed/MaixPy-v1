#include <string.h>

#include "py/obj.h"
#include "py/mpstate.h"
#include "py/mphal.h"
#include "extmod/misc.h"
#include "lib/utils/pyexec.h"
#include "mphalport.h"
#include "uarths.h"
#
#include "encoding.h"
#include "sysctl.h"
#include "sleep.h"


int mp_hal_stdin_rx_chr(void) {
	char c = 0;
    for (;;) 
	{
        if (MP_STATE_PORT(Maix_stdio_uart) != NULL && uart_rx_any(MP_STATE_PORT(Maix_stdio_uart))) 
		{
            return uart_rx_char(MP_STATE_PORT(Maix_stdio_uart));
        }
		for (size_t idx = 0; idx < MICROPY_PY_OS_DUPTERM; ++idx)
		{
			if( MP_STATE_VM(dupterm_objs[idx]) != NULL && uart_rx_any(MP_STATE_VM(dupterm_objs[idx])))
			{
				int dupterm_c = mp_uos_dupterm_rx_chr();
				 if (dupterm_c >= 0) 
				{
            		return dupterm_c;
        		}
			}
		}
   }

}
void mp_hal_debug_tx_strn_cooked(void *env, const char *str, uint32_t len);

const mp_print_t mp_debug_print = {NULL, mp_hal_debug_tx_strn_cooked};

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
//    uarths_send_data(str,len);

    if (MP_STATE_PORT(Maix_stdio_uart) != NULL) {
        uart_tx_strn(MP_STATE_PORT(Maix_stdio_uart), str, len);
    }
   mp_uos_dupterm_tx_strn(str, len);
}

void mp_hal_debug_tx_strn_cooked(void *env, const char *str, uint32_t len) {
    (void)env;
    while (len--) {
        if (*str == '\n') {
            mp_hal_stdout_tx_strn("\r", 1);
        }
       mp_hal_stdout_tx_strn(str++, 1);
    }
}


mp_uint_t inline mp_hal_ticks_cpu(void)
{
	return (unsigned int)(read_csr(mcycle));
}

mp_uint_t inline mp_hal_ticks_us(void)
{
	return (unsigned long)(read_csr(mcycle)/(sysctl_clock_get_freq(SYSCTL_CLOCK_CPU)/1000000));
}

mp_uint_t inline mp_hal_ticks_ms(void)
{
    return (unsigned int)(read_csr(mcycle)/(sysctl_clock_get_freq(SYSCTL_CLOCK_CPU)/1000));
}

void mp_hal_delay_ms(mp_uint_t ms) 
{
    unsigned int current_ms = (unsigned int)(mp_hal_ticks_ms());
    while((mp_hal_ticks_ms() - current_ms) < ms){}
/*
    uint64_t us = ms * 1000;
    uint64_t dt;
    uint64_t t0 = esp_timer_get_time();
    for (;;) {
        uint64_t t1 = esp_timer_get_time();
        dt = t1 - t0;
        if (dt + portTICK_PERIOD_MS * 1000 >= us) {
            // doing a vTaskDelay would take us beyond requested delay time
            break;
        }
        MICROPY_EVENT_POLL_HOOK
        ulTaskNotifyTake(pdFALSE, 1);
    }
    if (dt < us) {
        // do the remaining delay accurately
        mp_hal_delay_us(us - dt);
    }
*/
}

void mp_hal_delay_us(mp_uint_t us) {
    unsigned long current_us = (unsigned int)(mp_hal_ticks_us());
    while((mp_hal_ticks_us() - current_us) < us){}
/*	Maix implement
	const uint32_t overhead = 8;
	const uint32_t pend_overhead = 240;//390Mhz
	uint64_t dt;
    unsigned long current_us = mp_hal_ticks_us();
    if (us < overhead) {
		
        mp_raise_ValueError("us must > 8");
    }
    us -= overhead;
	uint64_t t0 = mp_hal_ticks_us();
	for (;;)
	{
        uint64_t dt = mp_hal_ticks_us() - t0;
        if (dt >= us) {
            return;
        }
        if (dt + pend_overhead < us) {
            //mp_handle_pending();
        }
    }
*/
/*  esp32 implement
    // these constants are tested for a 240MHz clock
    const uint32_t this_overhead = 5;
    const uint32_t pend_overhead = 150;

    // return if requested delay is less than calling overhead
    if (us < this_overhead) {
        return;
    }
    us -= this_overhead;

    uint64_t t0 = esp_timer_get_time();
    for (;;) {
        uint64_t dt = esp_timer_get_time() - t0;
        if (dt >= us) {
            return;
        }
        if (dt + pend_overhead < us) {
            // we have enough time to service pending events
            // (don't use MICROPY_EVENT_POLL_HOOK because it also yields)
            mp_handle_pending();
        }
    }
*/
}



