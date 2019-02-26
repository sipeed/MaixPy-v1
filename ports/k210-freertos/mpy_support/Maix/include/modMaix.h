#ifndef MICROPY_INCLUDED_MAIX_MAIX_H
#define MICROPY_INCLUDED_MAIX_MAIX_H

#include "py/obj.h"
#include "i2s.h"

typedef struct _audio_t{
    uint32_t buf_len;
    uint8_t* buf;
}audio_t;

typedef struct _Maix_audio_obj_t {
    mp_obj_base_t base;
    audio_t audio;
} Maix_audio_obj_t;

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
    uint32_t buf_len;
    uint8_t* buf;
    i2s_word_select_cycles_t cycles;
} Maix_i2s_obj_t;

extern const mp_obj_type_t Maix_fpioa_type;
extern const mp_obj_type_t Maix_gpio_type;
extern const mp_obj_type_t Maix_i2s_type;
extern const mp_obj_type_t Maix_audio_type;
extern const mp_obj_type_t Maix_fft_type;
#endif // MICROPY_INCLUDED_MAIX_MAIX_H