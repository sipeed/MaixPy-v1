#ifndef __SK9822_H
#define __SK9822_H

#include <stdint.h>
#include "fpioa.h"
#include "gpiohs.h"
#include <stdio.h>
#include <string.h>
#include "sysctl.h"
#include "fpioa.h"
#include "sleep.h"
#include "uarths.h"
#include "sysctl.h"
#include "timer.h"
#include "plic.h"

#define SK9822_DAT_GPIONUM 14
#define SK9822_CLK_GPIONUM 15

void init_mic_array_led(void);
void calc_voice_strength(uint8_t voice_data[]);

#endif
