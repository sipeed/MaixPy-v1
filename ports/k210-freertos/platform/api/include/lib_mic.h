#ifndef __LIB_MIC_H
#define __LIB_MIC_H

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <plic.h>
#include <i2s.h>
#include <sysctl.h>
#include <dmac.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef void (*mic_array_calc_done)(void);

int lib_mic_init(uint8_t dma_ch, mic_array_calc_done cb, uint8_t *thermal_map_data);
int lib_mic_deinit(void);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
