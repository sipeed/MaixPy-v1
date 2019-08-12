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

#include "dmac.h"
#include "fft.h"
#include "i2s.h"
#include "math.h"

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "py/objarray.h"
#include "py/binary.h"
#include "mphalport.h"
#include "modMaix.h"

#define MAX_SAMPLE_RATE 65535
#define MAX_BUFFER_LEN 1024

const mp_obj_type_t Maix_fft_type;

STATIC mp_obj_t Maix_fft_run(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
 {
    //----------parse parameter---------------
    enum{ARG_byte,
         ARG_points,
         ARG_shift,
         ARG_direction,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_byte, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_points, MP_ARG_INT, {.u_int = 64} },
        { MP_QSTR_shift, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_direction, MP_ARG_INT, {.u_int = FFT_DIR_FORWARD} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    uint32_t points = args[ARG_points].u_int;
    uint32_t shift = args[ARG_shift].u_int;
    uint32_t direction = args[ARG_direction].u_int;
    
    if(points != 64 && points != 128 && points != 256 && points != 512)
    {
        mp_raise_ValueError("[MAIXPY]FFT:invalid points");
    }

    uint32_t byte_len = 0;
    uint32_t* byte_addr = NULL;

    if( args[ARG_byte].u_obj != mp_const_none)
    {
        mp_obj_t byte = args[ARG_byte].u_obj;
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(byte, &bufinfo, MP_BUFFER_READ);
        byte_len = bufinfo.len;
        byte_addr = (uint32_t*)bufinfo.buf;
    }
    else
    {
        mp_raise_ValueError("[MAIXPY]FFT:invalid byte");
    }
    if(byte_len % 4 != 0)
    {
        mp_raise_ValueError("[MAIXPY]FFT:Buffer length must be a multiple of 4");
    }
    // how to get the length of i2s buffer?
    if(byte_len < points * 4)
    {
        mp_printf(&mp_plat_print, "[MAIXPY]FFT:Zero padding\n");
        memset(byte_addr+byte_len, 0, points * 4 - byte_len );//Zero padding
    }

    //------------------get data----------------------
    uint64_t* buffer_input = (uint64_t*)m_new(uint64_t, points);//m_new
    uint64_t* buffer_output = (uint64_t*)m_new(uint64_t ,points);//m_new
    fft_data_t * input_data = NULL;
    fft_data_t * output_data = NULL;
    for(int i = 0; i < points / 2; ++i)
    {
        input_data = (fft_data_t *)&buffer_input[i];
        input_data->R1 = byte_addr[2*i];    
        input_data->I1 = 0;                  
        input_data->R2 = byte_addr[2*i+1];  
        input_data->I2 = 0;
    }
    //run fft
    fft_complex_uint16_dma(DMAC_CHANNEL3, DMAC_CHANNEL4,shift,direction,buffer_input,points,buffer_output);
    //return a list
    mp_obj_list_t* ret_list = (mp_obj_list_t*)m_new(mp_obj_list_t,sizeof(mp_obj_list_t));//m_new
    mp_obj_list_init(ret_list, 0);
    mp_obj_t tuple_1[2];
    mp_obj_t tuple_2[2];
    for (int i = 0; i < points / 2; i++)
    {
        output_data = (fft_data_t*)&buffer_output[i];
        tuple_1[0] = mp_obj_new_int(output_data->R1);
        tuple_1[1] = mp_obj_new_int(output_data->I1);
        mp_obj_list_append(ret_list, mp_obj_new_tuple(MP_ARRAY_SIZE(tuple_1), tuple_1));

        tuple_2[0] = mp_obj_new_int(output_data->R2);
        tuple_2[1] = mp_obj_new_int(output_data->I2);
        mp_obj_list_append(ret_list, mp_obj_new_tuple(MP_ARRAY_SIZE(tuple_2), tuple_2));
    }
    return MP_OBJ_FROM_PTR(ret_list);
}

MP_DEFINE_CONST_FUN_OBJ_KW(Maix_fft_run_obj,1, Maix_fft_run);

STATIC mp_obj_t Maix_fft_freq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
 {
    //----------parse parameter---------------
    enum{ARG_points,
         ARG_sample_rate,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_points, MP_ARG_INT, {.u_int = 64} },
        { MP_QSTR_sample_rate, MP_ARG_INT, {.u_int = 16000} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    uint32_t sample_rate = args[ARG_sample_rate].u_int;
    uint32_t points = args[ARG_points].u_int;

    uint32_t step = sample_rate/points;
    mp_obj_list_t* ret_list = (mp_obj_list_t*)m_new(mp_obj_list_t,sizeof(mp_obj_list_t));//m_new
    mp_obj_list_init(ret_list, 0);
    for(int i = 0; i < points; i++)
    {
        mp_obj_list_append(ret_list, mp_obj_new_int(step * i));
    }
    return MP_OBJ_FROM_PTR(ret_list);
}

MP_DEFINE_CONST_FUN_OBJ_KW(Maix_fft_freq_obj,1, Maix_fft_freq);


STATIC mp_obj_t Maix_fft_amplitude(const mp_obj_t list_obj)
 {

    if(&mp_type_list != mp_obj_get_type(list_obj))
    {
        mp_raise_ValueError("[MAIXPY]FFT:obj is not a list");
    }
    mp_obj_list_t* ret_list = (mp_obj_list_t*)m_new(mp_obj_list_t,sizeof(mp_obj_list_t));//m_new
    mp_obj_list_init(ret_list, 0);
    //----------------------------------
    mp_obj_list_t* list = MP_OBJ_TO_PTR(list_obj);
    uint32_t index = 0;
    mp_obj_t list_iter;
    mp_obj_tuple_t* tuple;
    for(index = 0; index < list->len; index++)
    {
        list_iter = list->items[index];
        tuple = MP_OBJ_FROM_PTR(list_iter);
        uint32_t r_val = MP_OBJ_SMALL_INT_VALUE(tuple->items[0]);
        uint32_t i_val = MP_OBJ_SMALL_INT_VALUE(tuple->items[1]);
        uint32_t amplitude = sqrt(r_val * r_val + i_val * i_val);
        //Convert to power
        uint32_t hard_power = 2*amplitude/list->len;
        mp_obj_list_append(ret_list,mp_obj_new_int(hard_power));
    }
    return MP_OBJ_FROM_PTR(ret_list);
}
MP_DEFINE_CONST_FUN_OBJ_1(Maix_fft_amplitude_obj, Maix_fft_amplitude);

STATIC const mp_rom_map_elem_t Maix_fft_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_run), MP_ROM_PTR(&Maix_fft_run_obj) },
    { MP_ROM_QSTR(MP_QSTR_freq), MP_ROM_PTR(&Maix_fft_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_amplitude), MP_ROM_PTR(&Maix_fft_amplitude_obj) },
    
};

STATIC MP_DEFINE_CONST_DICT(Maix_fft_dict, Maix_fft_locals_dict_table);

const mp_obj_type_t Maix_fft_type = {
    { &mp_type_type },
    .name = MP_QSTR_FFT,
    .locals_dict = (mp_obj_dict_t*)&Maix_fft_dict,
};
