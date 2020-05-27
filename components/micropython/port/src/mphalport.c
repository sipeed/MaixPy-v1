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
#include "printf.h"

#if  MICROPY_PY_THREAD
	#include "FreeRTOS.h"
#endif 

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
		// MICROPY_EVENT_POLL_HOOK
		// #if MICROPY_PY_THREAD
		// 		ulTaskNotifyTake(pdFALSE, 1);
		// #endif
		extern void mp_handle_pending(void);
        mp_handle_pending();
        MICROPY_PY_USOCKET_EVENTS_HANDLER
        MP_THREAD_GIL_EXIT();
	#if MICROPY_PY_THREAD
		ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
	#endif
        MP_THREAD_GIL_ENTER();
	}

}
void mp_hal_debug_tx_strn_cooked(void *env, const char *str, size_t len);

const mp_print_t mp_debug_print = {NULL, mp_hal_debug_tx_strn_cooked};

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {

	bool release_gil = len > 20 ? true : false;
	if(release_gil)
		MP_THREAD_GIL_EXIT();
    if (MP_STATE_PORT(Maix_stdio_uart) != NULL) {
        uart_tx_strn(MP_STATE_PORT(Maix_stdio_uart), str, len);
    }
	if(release_gil)
		MP_THREAD_GIL_ENTER();
   	mp_uos_dupterm_tx_strn(str, len);
}

void mp_hal_debug_tx_strn_cooked(void *env, const char *str, size_t len) {
    (void)env;
    while (len--) {
        if (*str == '\n') {
            printk("\r\n");
        }
       printk("%c", *str);
	   str++;
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
	#if  MICROPY_PY_THREAD
		if( xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED )
		{
			unsigned int current_ms = (unsigned int)(mp_hal_ticks_ms());
    		while((mp_hal_ticks_ms() - current_ms) < ms){}
		}
		else
		{
			mp_uint_t us = ms * 1000;
			mp_uint_t dt;
			mp_uint_t t0 = mp_hal_ticks_us();
			for (;;) {
				extern void mp_handle_pending();
				mp_handle_pending();
				MICROPY_PY_USOCKET_EVENTS_HANDLER
				MP_THREAD_GIL_EXIT();
				// ulTaskNotifyTake(pdFALSE, 1);
				portYIELD();
				MP_THREAD_GIL_ENTER();
				mp_uint_t t1 = mp_hal_ticks_us();
				dt = t1 - t0;
				if (dt + portTICK_PERIOD_MS * 1000 >= us) {
					// doing a vTaskDelay would take us beyond requested delay time
					break;
				}
			}
			if (dt < us) {
				// do the remaining delay accurately
				mp_hal_delay_us(us - dt);
			}
		}
	#else
		unsigned int current_ms = (unsigned int)(mp_hal_ticks_ms());
		while((mp_hal_ticks_ms() - current_ms) < ms){}
	#endif
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
			portYIELD();
		}
	#endif
}
