#include "platform.h"
#include "sysctl.h"
#include "utils.h"
#include "math.h"
#include "sleep.h"
#include "sipeed_sys.h"

#define OSC_FREQ (26000000UL)

unsigned long  systick_current_us(void)
{
	return (uint32_t)(read_cycle()/(OSC_FREQ/1000000));
}

uint32_t systick_current_millis(void)
{
	return (uint32_t)(read_cycle()/(OSC_FREQ/1000));
}

uint32_t systick_current_sec(void)
{
	return (uint32_t)(read_cycle()/(OSC_FREQ));
}
