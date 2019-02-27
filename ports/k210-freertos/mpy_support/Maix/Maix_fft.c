/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>

#include "i2s.h"
#include "../platform/sdk/kendryte-standalone-sdk/lib/drivers/include/fft.h"
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
    //how to get the length of i2s buffer?
    if(byte_len < points * 4)
    {
        printf("[MAIXPY]FFT:Zero padding\n");
        memset(byte_addr+byte_len, 0, points * 4 - byte_len );//Zero padding
    }

    //------------------get data----------------------
    complex_hard_t* data_hard = (complex_hard_t*)m_new(complex_hard_t,points);//m_new
    uint64_t* buffer_input = (uint64_t*)m_new(uint64_t, points);//m_new
    uint64_t* buffer_output = (uint64_t*)m_new(uint64_t ,points);//m_new
    // complex_hard_t* data_hard = (complex_hard_t*)malloc(fft_points * sizeof(complex_hard_t));//m_new
    // uint64_t* buffer_input = (uint64_t*)malloc(fft_points * sizeof(uint64_t));//m_new
    // uint64_t* buffer_output = (uint64_t*)malloc(fft_points * sizeof(uint64_t));//m_new
    fft_data_t * input_data = NULL;
    fft_data_t * output_data = NULL;
    for(int i = 0; i < points / 2; ++i)
    {
        input_data = (fft_data_t *)&buffer_input[i];
        input_data->R1 = byte_addr[2*i];   // data_hard[2 * i].real;
        input_data->I1 = 0;                 // data_hard[2 * i].imag;
        input_data->R2 = byte_addr[2*i+1]; // data_hard[2 * i + 1].real;
        input_data->I2 = 0;                 // data_hard[2 * i + 1].imag;
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
        data_hard[2 * i].real = output_data->R1 ;
        data_hard[2 * i].imag = output_data->I1 ;
        tuple_1[0] = mp_obj_new_int(data_hard[2 * i].real);
        tuple_1[1] = mp_obj_new_int(data_hard[2 * i].imag);
        mp_obj_list_append(ret_list, mp_obj_new_tuple(MP_ARRAY_SIZE(tuple_1), tuple_1));

        data_hard[2 * i + 1].real = output_data->R2 ;
        data_hard[2 * i + 1].imag = output_data->I2 ;
        tuple_2[0] = mp_obj_new_int(data_hard[2 * i + 1].real);
        tuple_2[1] = mp_obj_new_int(data_hard[2 * i + 1].imag);
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
        { MP_QSTR_points, MP_ARG_INT, {.u_obj = 64} },
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
        //list_iter = list_subscr(list_obj,index,MP_OBJ_SENTINEL);
        tuple = MP_OBJ_FROM_PTR(list_iter);
        uint32_t r_val = MP_OBJ_SMALL_INT_VALUE(tuple->items[0]);
        uint32_t i_val = MP_OBJ_SMALL_INT_VALUE(tuple->items[1]);
        uint32_t amplitude = sqrt(r_val * r_val + i_val * i_val);
        //Convert to dBFS
        uint32_t hard_power = 20*log(2*amplitude/list->len);
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
