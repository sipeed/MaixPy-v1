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

#include <stdio.h>
#include <string.h>

#include "i2s.h"
#include "../platform/sdk/kendryte-standalone-sdk/lib/drivers/include/fft.h"

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "py/objarray.h"
#include "py/binary.h"
#include "mphalport.h"
#include "modMaix.h"

#define MAX_SAMPLE_RATE 65535
#define MAX_BUFFER_LEN 1024

const mp_obj_type_t Maix_audio_type;


STATIC void Maix_audio_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    Maix_audio_obj_t *self = MP_OBJ_TO_PTR(self_in);
    audio_t* audio_obj = &self->audio;
    mp_printf(print, "[MAIXPY]audio:(points=%u, buffer addr=%p)",
        audio_obj->points,audio_obj->buf);
}

STATIC mp_obj_t Maix_audio_init_helper(Maix_audio_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum {ARG_array,ARG_points};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_array, MP_ARG_OBJ , {.u_obj = mp_const_none} },
        { MP_QSTR_points, MP_ARG_INT | MP_ARG_KW_ONLY , {.u_int = MAX_BUFFER_LEN} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    audio_t* audio_obj = &self->audio;
    //Use arrays first
    if(args[ARG_array].u_obj != mp_const_none)
    {
        mp_obj_t audio_array = args[ARG_array].u_obj;
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(audio_array, &bufinfo, MP_BUFFER_READ);
        audio_obj->points = bufinfo.len / sizeof(uint32_t);
        audio_obj->buf = bufinfo.buf;
    }
    else
    {
        //runing init
        m_del(uint32_t, audio_obj->buf, audio_obj->points);
        audio_obj->points = args[ARG_points].u_int;
        if(0 == audio_obj->points)//
        {
            audio_obj->buf = NULL;
        }
        else
        {
            audio_obj->buf = m_new(uint32_t,audio_obj->points);//here can not work,so don't use buf_len to make a new obj
            memset(audio_obj->buf, 0, audio_obj->points * sizeof(uint32_t));
        }
    }
    return mp_const_true;
}
STATIC mp_obj_t Maix_audio_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    //mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    // create instance
    Maix_audio_obj_t *self = m_new_obj(Maix_audio_obj_t);
    self->base.type = &Maix_audio_type;
    self->type = EXT_AUDIO;
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    Maix_audio_init_helper(self, n_args, args, &kw_args);
    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t Maix_audio_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return Maix_audio_init_helper(args[0], n_args -1 , args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(Maix_audio_init_obj,0 ,Maix_audio_init);

STATIC mp_obj_t Maix_audio_to_byte(Maix_audio_obj_t* self) {
    audio_t* audio = &self->audio; 
    mp_obj_array_t* audio_array = m_new_obj(mp_obj_array_t);
    audio_array->base.type = &mp_type_bytearray;
    audio_array->typecode = BYTEARRAY_TYPECODE;
    audio_array->free = 0;
    audio_array->len = audio->points * 4;
    audio_array->items = audio->buf;
    return audio_array;
}

MP_DEFINE_CONST_FUN_OBJ_1(Maix_audio_to_byte_obj, Maix_audio_to_byte);


STATIC mp_obj_t Maix_audio_deinit(Maix_audio_obj_t* self) {
    audio_t* audio = &self->audio; 
    if(self->type == EXT_AUDIO)
        m_del(uint32_t, audio->buf, audio->points);
    m_del_obj(Maix_audio_obj_t,self);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(Maix_audio_deinit_obj, Maix_audio_deinit);

STATIC const mp_rom_map_elem_t Maix_audio_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&Maix_audio_init_obj) },
    { MP_ROM_QSTR(MP_QSTR___deinit__), MP_ROM_PTR(&Maix_audio_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_tobyte), MP_ROM_PTR(&Maix_audio_to_byte_obj) },
    
};

STATIC MP_DEFINE_CONST_DICT(Maix_audio_dict, Maix_audio_locals_dict_table);

const mp_obj_type_t Maix_audio_type = {
    { &mp_type_type },
    .print = Maix_audio_print,
    .name = MP_QSTR_AUDIO,
    .make_new = Maix_audio_make_new,
    .locals_dict = (mp_obj_dict_t*)&Maix_audio_dict,
};
