#ifndef MICROPY_MAIX_I2S_H
#define MICROPY_MAIX_I2S_H

#include "i2s.h"
typedef struct _i2s_channle_t{
    i2s_word_length_t resolution;
    i2s_word_select_cycles_t cycles;
    i2s_work_mode_t align_mode;
    i2s_transmit_t mode;
}i2s_channle_t;

typedef struct _Maix_i2s_obj_t {
    mp_obj_base_t base;
    i2s_device_number_t i2s_num;
    i2s_channle_t channel[4];
    uint32_t sample_rate;
    uint32_t points_num;
    uint32_t* buf;
    i2s_word_select_cycles_t cycles;
    uint32_t chn_mask;
} Maix_i2s_obj_t;
#endif