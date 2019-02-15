#ifndef _SIPEED_CONV_H
#define _SIPEED_CONV_H
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "dvp.h"
#include "fpioa.h"
#include "plic.h"
#include "sysctl.h"
#include "uarths.h"
#include "utils.h"
#include "kpu.h"
void layer_conv_init(kpu_task_t* task, uint16_t w, uint16_t h, uint8_t ch_in, uint8_t ch_out, float* conv_data);
void layer_conv_run(kpu_task_t* task, uint8_t* img_src, uint8_t* img_dst, plic_irq_callback_t callback);

#endif
