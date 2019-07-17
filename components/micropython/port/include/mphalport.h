
#ifndef INCLUDED_MPHALPORT_H
#define INCLUDED_MPHALPORT_H

#include "py/ringbuf.h"
#include "lib/utils/interrupt_char.h"
#include "mpconfigport.h"

extern ringbuf_t stdin_ringbuf;

#if MICROPY_PY_THREAD 
#include "FreeRTOS.h"
#include "task.h"
extern TaskHandle_t mp_main_task_handle;
#endif

//uint32_t mp_hal_ticks_us(void);
//uint32_t mp_hal_ticks_ms(void);
//uint32_t mp_hal_ticks_cpu(void);
void mp_hal_delay_us(mp_uint_t us);
void mp_hal_delay_ms(mp_uint_t ms);
extern mp_uint_t systick_current_millis(void);
void mp_hal_wake_main_task_from_isr(void);
#endif

