#include <string.h>

#include "py/obj.h"
#include "py/mpstate.h"
#include "py/mphal.h"
#include "extmod/misc.h"
#include "lib/utils/pyexec.h"
#include "mphalport.h"
#include "uarths.h"
#include "encoding.h"
#include "sysctl.h"
#include "sleep.h"
#include "machine_uart.h"


int mp_hal_stdin_rx_chr(void) {
	int c = 0;
	for (;;)
	{
		if (MP_STATE_PORT(Maix_stdio_uart) != NULL ) 
		{
			c = uart_rx_char(MP_STATE_PORT(Maix_stdio_uart));
			if (c != -1) {
				return c;
			}
			MICROPY_EVENT_POLL_HOOK
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

	MP_THREAD_GIL_EXIT();
    if (MP_STATE_PORT(Maix_stdio_uart) != NULL) {
        uart_tx_strn(MP_STATE_PORT(Maix_stdio_uart), str, len);
    }
	MP_THREAD_GIL_ENTER();
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
	return (unsigned long)(read_csr(mcycle));
}

mp_uint_t inline mp_hal_ticks_us(void)
{
	return (unsigned long)(read_csr(mcycle)/(sysctl_clock_get_freq(SYSCTL_CLOCK_CPU)/1000000));
}

mp_uint_t inline mp_hal_ticks_ms(void)
{
    return (unsigned long)(read_csr(mcycle)/(sysctl_clock_get_freq(SYSCTL_CLOCK_CPU)/1000));
}

void mp_hal_delay_ms(mp_uint_t ms) 
{
    unsigned int current_ms = (unsigned int)(mp_hal_ticks_ms());
    while((mp_hal_ticks_ms() - current_ms) < ms){}
}

void mp_hal_delay_us(mp_uint_t us) {
    unsigned long current_us = (unsigned int)(mp_hal_ticks_us());
    while((mp_hal_ticks_us() - current_us) < us){};
}


mp_uint_t systick_current_millis(void) __attribute__((weak, alias("mp_hal_ticks_ms")));

// Wake up the main task if it is sleeping
void mp_hal_wake_main_task_from_isr(void) {
	#if MICROPY_PY_THREAD 
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		vTaskNotifyGiveFromISR(mp_main_task_handle, &xHigherPriorityTaskWoken);
		if (xHigherPriorityTaskWoken == pdTRUE) {
			//TODO: not implement yet
			// portYIELD_FROM_ISR();
		}
	#endif
}
