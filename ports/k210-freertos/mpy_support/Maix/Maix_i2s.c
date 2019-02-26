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
#include "dmac.h"

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "mphalport.h"
#include "modMaix.h"

#define MAX_SAMPLE_RATE (60*1024)
#define MAX_BUFFER_LEN 60*1024


const mp_obj_type_t Maix_i2s_type;

STATIC void Maix_i2s_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    Maix_i2s_obj_t *self = MP_OBJ_TO_PTR(self_in);
    i2s_channle_t* channel_iter = &self->channel[0];
    mp_printf(print, "[MAIXPY]i2s%d:(sample rate=%u, buffer length=%u)\n",
        self->i2s_num,self->sample_rate,self->buf_len);
    for(int channel_iter = 0; channel_iter < 4; channel_iter++)
    {
        self->channel[channel_iter];
        mp_printf(print, "[MAIXPY]channle%d:(resolution=%u, cycles=%u, align_mode=%u, mode=%u)\n",
                  channel_iter,
                  self->channel[channel_iter].resolution,
                  self->channel[channel_iter].cycles,
                  self->channel[channel_iter].align_mode,
                  self->channel[channel_iter].mode);
    }
}
    uint32_t buf_len;
    uint8_t* buf;
STATIC mp_obj_t Maix_i2s_init_helper(Maix_i2s_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum {ARG_buf_len,
         };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_buf_len, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 4096} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    //set buffer len
    if(args[ARG_buf_len].u_int > MAX_BUFFER_LEN)
    {
        mp_raise_ValueError("[MAIXPY]I2S:invalid buffer length");
    }
    self->buf_len = args[ARG_buf_len].u_int;
    //self->buf = m_new(uint8_t,self->buf_len);
    self->buf = (uint8_t*)malloc(sizeof(uint8_t) * self->buf_len);//m_new
    return mp_const_true;
}
STATIC mp_obj_t Maix_i2s_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    // get uart id
    mp_int_t i2s_num = mp_obj_get_int(args[0]);
    if (i2s_num >= I2S_DEVICE_MAX) {
    	nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "[MAIXPY]I2S%d:does not exist", i2s_num));
    }
    // create instance
    Maix_i2s_obj_t *self = m_new_obj(Maix_i2s_obj_t);
    self->base.type = &Maix_i2s_type;

    self->i2s_num = i2s_num;
    self->sample_rate = 0;
    memset(&self->channel,0,4 * sizeof(i2s_channle_t));
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    Maix_i2s_init_helper(self, n_args - 1, args + 1, &kw_args);
    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t Maix_i2s_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return Maix_i2s_init_helper(args[0], n_args -1 , args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(Maix_i2s_init_obj, 0, Maix_i2s_init);

STATIC mp_obj_t Maix_i2s_channel_config(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    Maix_i2s_obj_t *self = pos_args[0];//get i2s obj
    enum{ARG_channel, 
         ARG_mode,
         ARG_resolution,
         ARG_cycles,
         ARG_align_mode,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_channel, MP_ARG_INT, {.u_int = I2S_CHANNEL_0} },
        { MP_QSTR_mode, MP_ARG_INT, {.u_int = I2S_RECEIVER} },
        { MP_QSTR_resolution, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = RESOLUTION_16_BIT} },
        { MP_QSTR_cycles, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = SCLK_CYCLES_32} },
        { MP_QSTR_align_mode, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = STANDARD_MODE} },
    };
    //parse parameter
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    //set channel
    if(args[ARG_channel].u_int > I2S_CHANNEL_3)
    {
        mp_raise_ValueError("[MAIXPY]I2S:invalid i2s channle");
    }
    i2s_channel_num_t channel_num = args[ARG_channel].u_int;
    i2s_channle_t* channle = &self->channel[channel_num];
    
    //set resolution
    if(args[ARG_resolution].u_int >  RESOLUTION_32_BIT )
    {
        mp_raise_ValueError("[MAIXPY]I2S:invalid resolution");
    }
    channle->resolution = args[ARG_resolution].u_int;
    if(args[ARG_cycles].u_int >   SCLK_CYCLES_32 )
    {
        mp_raise_ValueError("[MAIXPY]I2S:invalid cycles");
    }
    channle->cycles = args[ARG_cycles].u_int;
    self->cycles = args[ARG_cycles].u_int;

    //set align mode
    if(args[ARG_align_mode].u_int != STANDARD_MODE && args[ARG_align_mode].u_int != RIGHT_JUSTIFYING_MODE && args[ARG_align_mode].u_int != LEFT_JUSTIFYING_MODE)
    {
        mp_raise_ValueError("[MAIXPY]I2S:invalid align mode");
    }
    channle->align_mode = args[ARG_align_mode].u_int;
 
    //set mode
    if(args[ARG_mode].u_int >  I2S_RECEIVER )
    {
        mp_raise_ValueError("[MAIXPY]I2S:invalid cycles");
    }
    channle->mode = args[ARG_mode].u_int;

    //running config
    if(channle->mode == I2S_RECEIVER)
    {
        uint32_t chn_mask = 0x3 << (channel_num * 2);
        i2s_init(self->i2s_num, I2S_RECEIVER, chn_mask);
        i2s_rx_channel_config(self->i2s_num,
                            channel_num,
                            channle->resolution,
                            channle->cycles,
                            TRIGGER_LEVEL_4,
                            channle->align_mode);
    }
    else
    {
        uint32_t chn_mask = 0x3 << (channel_num * 2);
        i2s_init(self->i2s_num, I2S_TRANSMITTER,chn_mask);
        i2s_tx_channel_config(self->i2s_num,
                            channel_num,
                            channle->resolution,
                            channle->cycles,
                            TRIGGER_LEVEL_4,
                            channle->align_mode);
    }                    
    return mp_const_true;
}
MP_DEFINE_CONST_FUN_OBJ_KW(Maix_i2s_channel_config_obj, 2, Maix_i2s_channel_config);

STATIC mp_obj_t Maix_i2s_set_sample_rate(Maix_i2s_obj_t *self, mp_obj_t sample_rate)
{
    if(sample_rate > MAX_SAMPLE_RATE)
    {
        mp_raise_ValueError("[MAIXPY]I2S:invalid sample rate");
    }
    uint32_t smp_rate = mp_obj_get_int(sample_rate);
    int res = i2s_set_sample_rate(self->i2s_num,smp_rate);
    //judege cycles,which channel should we select ?
    if(self->cycles == SCLK_CYCLES_16)
    {
        self->sample_rate = res / 32;
    }
    else if(self->cycles == SCLK_CYCLES_24)
    {
        self->sample_rate = res / 48;
    }
    else if(self->cycles == SCLK_CYCLES_32)
    {
        self->sample_rate = res / 64;
    }
    return mp_const_true;
}
MP_DEFINE_CONST_FUN_OBJ_2(Maix_i2s_set_sample_rate_obj,Maix_i2s_set_sample_rate);


STATIC mp_obj_t Maix_i2s_record(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)//point_nums,time
{
    
    Maix_i2s_obj_t *self = pos_args[0];//get i2s obj
    Maix_audio_obj_t *audio_obj = m_new_obj(Maix_audio_obj_t);
    enum{ARG_points, 
         ARG_time,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_points, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_time, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = 0} },
    };
    //parse parameter
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    audio_obj->base.type = &Maix_audio_type;
    //compute buffer length
    if(args[ARG_points].u_int > 0)
    {
        audio_obj->audio.buf_len = args[ARG_points].u_int * sizeof(uint32_t);
        audio_obj->audio.buf = self->buf;
        if(audio_obj->audio.buf_len > self->buf_len)
        {
            mp_raise_ValueError("[MAIXPY]I2S:Too many points");
        }
    }
    else if(args[ARG_time].u_int > 0)
    {
        uint32_t times = args[ARG_time].u_int;
        if(self->sample_rate <= 0)
             mp_raise_ValueError("[MAIXPY]I2S:please set sample rate");
        uint32_t sample_rate = self->sample_rate;
        uint32_t points_sum = sample_rate * times;
        //TODO:Determine if the length is too large
        audio_obj->audio.buf_len = points_sum * sizeof(uint32_t);
        audio_obj->audio.buf = self->buf;
    }
    i2s_receive_data_dma(self->i2s_num, audio_obj->audio.buf, audio_obj->audio.buf_len, DMAC_CHANNEL5);
    //dmac_wait_idle(DMAC_CHANNEL5);//wait to finish recv
    return MP_OBJ_FROM_PTR(audio_obj);
}
MP_DEFINE_CONST_FUN_OBJ_KW(Maix_i2s_record_obj,1,Maix_i2s_record);

STATIC mp_obj_t Maix_i2s_play(Maix_i2s_obj_t *self,mp_obj_t audio_obj)
{
    Maix_audio_obj_t *audio_p = MP_OBJ_TO_PTR(audio_obj);
    i2s_send_data_dma(self->i2s_num, audio_p->audio.buf, audio_p->audio.buf_len, DMAC_CHANNEL4);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(Maix_i2s_play_obj,Maix_i2s_play);

STATIC mp_obj_t Maix_i2s_play_PCM(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    Maix_i2s_obj_t *self = pos_args[0];//get i2s obj
    enum{ARG_frame_len,
         ARG_bps,
         ARG_track_num,
         ARG_audio,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_frame_len, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 128} },
        { MP_QSTR_bps, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 16} },
        { MP_QSTR_track_num, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_audio, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };
    //parse parameter
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    //get frame_len
    uint32_t frame_len = args[ARG_frame_len].u_int;
    if(frame_len > 1024)
    {
        mp_raise_ValueError("[MAIXPY]I2S:invalid i2s frame_len");
    }
    //get bps
    uint32_t bps = args[ARG_bps].u_int;
    if(bps > 32)
    {
        mp_raise_ValueError("[MAIXPY]I2S:invalid i2s bits/per_sample");
    }    
    //get track num
    uint32_t track_num = args[ARG_track_num].u_int;
    if(bps > 32)
    {
        mp_raise_ValueError("[MAIXPY]I2S:invalid i2s track num");
    } 
    //get audio
    Maix_audio_obj_t *audio_obj = args[ARG_audio].u_obj;
    i2s_play(self->i2s_num,DMAC_CHANNEL4,audio_obj->audio.buf,audio_obj->audio.buf_len,frame_len,bps,track_num);
}

MP_DEFINE_CONST_FUN_OBJ_KW(Maix_i2s_play_PCM_obj, 0, Maix_i2s_play_PCM);



// STATIC  MP_DEFINE_CONST_FUN_OBJ_KW(Maix_i2s_set_dma_divede_16_obj,1,);
// STATIC  MP_DEFINE_CONST_FUN_OBJ_KW(Maix_i2s_set_dma_divede_16_obj,1,);


STATIC const mp_rom_map_elem_t Maix_i2s_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&Maix_i2s_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_config), MP_ROM_PTR(&Maix_i2s_channel_config_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_sample_rate), MP_ROM_PTR(&Maix_i2s_set_sample_rate_obj) },
    { MP_ROM_QSTR(MP_QSTR_record), MP_ROM_PTR(&Maix_i2s_record_obj) },
    { MP_ROM_QSTR(MP_QSTR_play), MP_ROM_PTR(&Maix_i2s_play_obj) },
    { MP_ROM_QSTR(MP_QSTR_play_PCM), MP_ROM_PTR(&Maix_i2s_play_PCM_obj) },
    //advance interface , some user don't use it
    // { MP_ROM_QSTR(MP_QSTR_set_dma_divede_16), MP_ROM_PTR(&Maix_i2s_set_dma_divede_16_obj) },
    // { MP_ROM_QSTR(MP_QSTR_set_dma_divede_16), MP_ROM_PTR(&Maix_i2s_get_dma_divede_16_obj) },
    { MP_ROM_QSTR(MP_QSTR_DEVICE_0), MP_ROM_INT(I2S_DEVICE_0) },
    { MP_ROM_QSTR(MP_QSTR_DEVICE_1), MP_ROM_INT(I2S_DEVICE_1) },
    { MP_ROM_QSTR(MP_QSTR_DEVICE_2), MP_ROM_INT(I2S_DEVICE_2) },

    { MP_ROM_QSTR(MP_QSTR_CHANNEL_0), MP_ROM_INT(I2S_CHANNEL_0) },
    { MP_ROM_QSTR(MP_QSTR_CHANNEL_1), MP_ROM_INT(I2S_CHANNEL_1) },
    { MP_ROM_QSTR(MP_QSTR_CHANNEL_2), MP_ROM_INT(I2S_CHANNEL_2) },
    { MP_ROM_QSTR(MP_QSTR_CHANNEL_3), MP_ROM_INT(I2S_CHANNEL_3) },

    { MP_ROM_QSTR(MP_QSTR_SCLK_CYCLES_16), MP_ROM_INT(SCLK_CYCLES_16) },
    { MP_ROM_QSTR(MP_QSTR_SCLK_CYCLES_24), MP_ROM_INT(SCLK_CYCLES_24) },
    { MP_ROM_QSTR(MP_QSTR_SCLK_CYCLES_32), MP_ROM_INT(SCLK_CYCLES_32) },

    

    { MP_ROM_QSTR(MP_QSTR_TRANSMITTER), MP_ROM_INT(I2S_TRANSMITTER) },
    { MP_ROM_QSTR(MP_QSTR_RECEIVER), MP_ROM_INT(I2S_RECEIVER) },

    { MP_ROM_QSTR(MP_QSTR_STANDARD_MODE), MP_ROM_INT(STANDARD_MODE) },
    { MP_ROM_QSTR(MP_QSTR_RIGHT_JUSTIFYING_MODE), MP_ROM_INT(RIGHT_JUSTIFYING_MODE) },
    { MP_ROM_QSTR(MP_QSTR_LEFT_JUSTIFYING_MODE), MP_ROM_INT(LEFT_JUSTIFYING_MODE) },
    
};

STATIC MP_DEFINE_CONST_DICT(Maix_i2s_dict, Maix_i2s_locals_dict_table);

const mp_obj_type_t Maix_i2s_type = {
    { &mp_type_type },
    .name = MP_QSTR_I2S,
    .print = Maix_i2s_print,
    .make_new = Maix_i2s_make_new,
    .locals_dict = (mp_obj_dict_t*)&Maix_i2s_dict,
};
