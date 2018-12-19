
#ifndef INCLUDED_MPHALPORT_H
#define INCLUDED_MPHALPORT_H

#include "py/ringbuf.h"
#include "lib/utils/interrupt_char.h"

static inline mp_uint_t mp_hal_ticks_ms(void) { return 0; }
//static inline void mp_hal_set_interrupt_char(int c) {}
void mp_hal_set_interrupt_char(int c);
void mp_hal_delay_us(mp_uint_t us);
void mp_hal_delay_ms(mp_uint_t ms);

#endif

