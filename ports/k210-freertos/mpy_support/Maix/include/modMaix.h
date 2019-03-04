/*
* Copyright 2019 Sipeed Co.,Ltd.

* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef MICROPY_INCLUDED_MAIX_MAIX_H
#define MICROPY_INCLUDED_MAIX_MAIX_H

#include "py/obj.h"
#include "i2s.h"

typedef enum _audio_buf_type
{
    IS2_AUDIO,
    EXT_AUDIO,
} __attribute__((aligned(8))) audio_buf_type;

typedef struct _audio_t{
    uint32_t points;
    uint8_t* buf;
} __attribute__((aligned(8))) audio_t;

typedef struct _Maix_audio_obj_t {
    mp_obj_base_t base;
    audio_t audio;
    audio_buf_type type;
} __attribute__((aligned(8))) Maix_audio_obj_t;

typedef struct _i2s_channle_t{
    i2s_word_length_t resolution;
    i2s_word_select_cycles_t cycles;
    i2s_work_mode_t align_mode;
    i2s_transmit_t mode;
}__attribute__((aligned(8))) i2s_channle_t ;

typedef struct _Maix_i2s_obj_t {
    mp_obj_base_t base;
    i2s_device_number_t i2s_num;
    i2s_channle_t channel[4];
    uint32_t sample_rate;
    uint32_t points_num;
    uint32_t* buf;
    i2s_word_select_cycles_t cycles;
}__attribute__((aligned(8))) Maix_i2s_obj_t ;

extern const mp_obj_type_t Maix_fpioa_type;
extern const mp_obj_type_t Maix_gpio_type;
extern const mp_obj_type_t Maix_i2s_type;
extern const mp_obj_type_t Maix_audio_type;
extern const mp_obj_type_t Maix_fft_type;
#endif // MICROPY_INCLUDED_MAIX_MAIX_H