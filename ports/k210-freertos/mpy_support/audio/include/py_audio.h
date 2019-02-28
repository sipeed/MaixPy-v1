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
#ifndef MICROPY_AUDIO_H
#define MICROPY_AUDIO_H

#include "py/obj.h"
#include "i2s.h"
#include "Maix_i2s.h"
typedef enum _audio_type
{
    I2S_AUDIO,
    EXT_AUDIO,
    FILE_AUDIO,
}audio_type;

typedef enum _audio_fmt
{
    AUDIO_WAV_FMT,
}audio_fmt;

typedef struct _audio_t{
    uint32_t points;
    uint32_t* buf;
    audio_type type;
    mp_obj_t fp;
    audio_fmt format;
    void* fmt_obj;
    Maix_i2s_obj_t* dev;
}audio_t;

typedef struct _Maix_audio_obj_t {
    mp_obj_base_t base;
    audio_t audio;
} Maix_audio_obj_t;
#endif // MICROPY_INCLUDED_MAIX_MAIX_H