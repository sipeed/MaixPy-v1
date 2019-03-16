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

extern const mp_obj_type_t Maix_fpioa_type;
extern const mp_obj_type_t Maix_gpio_type;
extern const mp_obj_type_t Maix_i2s_type;
extern const mp_obj_type_t Maix_audio_type;
extern const mp_obj_type_t Maix_fft_type;
extern const mp_obj_type_t Maix_mic_array_type;
#endif // MICROPY_INCLUDED_MAIX_MAIX_H