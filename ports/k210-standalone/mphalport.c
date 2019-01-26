#include <string.h>

#include "py/obj.h"
#include "py/mpstate.h"
#include "py/mphal.h"
#include "extmod/misc.h"
#include "lib/utils/pyexec.h"
#include "mphalport.h"

#include "sleep.h"
#include "sysctl.h"
#include "encoding.h"

void mp_hal_delay_ms(mp_uint_t ms) {
	msleep(ms);
}

void mp_hal_delay_us(mp_uint_t us) {
	usleep(us);
}

mp_uint_t mp_hal_ticks_us(void) {
    return (read_cycle()) / (sysctl_clock_get_freq(SYSCTL_CLOCK_CPU) / 1000000UL);
}

mp_uint_t mp_hal_ticks_ms(void) {
    return (read_cycle()) / (sysctl_clock_get_freq(SYSCTL_CLOCK_CPU) / 1000UL);
}

mp_uint_t mp_hal_ticks_cpu(void) {
    return (read_cycle());
}


