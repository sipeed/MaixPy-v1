#ifndef __LIB_MIC_H
#define __LIB_MIC_H

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <plic.h>
#include <i2s.h>
#include <sysctl.h>
#include <dmac.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//uint8_t data[256];
typedef void (*get_thermal_map_cb)(uint8_t *data);

int lib_mic_init(uint8_t dma_ch, get_thermal_map_cb cb);
int lib_mic_deinit(void);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
