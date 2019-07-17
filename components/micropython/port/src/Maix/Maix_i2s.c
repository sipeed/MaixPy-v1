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
#include "dmac.h"

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"

#include "modMaix.h"
#include "py_audio.h"
#include "Maix_i2s.h"
#define MAX_SAMPLE_RATE (4*1024*1024)
#define MAX_SAMPLE_POINTS (64*1024)

const mp_obj_type_t Maix_i2s_type;


STATIC void Maix_i2s_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    Maix_i2s_obj_t* self = MP_OBJ_TO_PTR(self_in);
    // i2s_channle_t*  channel_iter = &self->channel[0];
    mp_printf(print, "[MAIXPY]i2s%d:(sampling rate=%u, sampling points=%u)\n",
        self->i2s_num,self->sample_rate,self->points_num);
    for(int channel_iter = 0; channel_iter < 4; channel_iter++)
    {
        mp_printf(print, "[MAIXPY]channle%d:(resolution=%u, cycles=%u, align_mode=%u, mode=%u)\n",
                  channel_iter,
                  self->channel[channel_iter].resolution,
                  self->channel[channel_iter].cycles,
                  self->channel[channel_iter].align_mode,
                  self->channel[channel_iter].mode);
    }
}
STATIC mp_obj_t Maix_i2s_init_helper(Maix_i2s_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum 
    {
        ARG_sample_points,
    };
    static const mp_arg_t allowed_args[] = 
    {
        { MP_QSTR_sample_points, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1024} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    //set buffer len
    if(args[ARG_sample_points].u_int > MAX_SAMPLE_POINTS)
    {
        mp_raise_ValueError("[MAIXPY]I2S:invalid buffer length");
    }
    self->points_num = args[ARG_sample_points].u_int;
    self->buf = m_new(uint32_t,self->points_num);

    //set i2s channel mask
    self->chn_mask = 0;

    return mp_const_true;
}
STATIC mp_obj_t Maix_i2s_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get i2s num
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

    // init instance
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
    //get i2s obj
    Maix_i2s_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    //parse parameter
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
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    //set channel
    if(args[ARG_channel].u_int > I2S_CHANNEL_3)
    {
        mp_raise_ValueError("[MAIXPY]I2S:invalid i2s channle");
    }
    i2s_channel_num_t channel_num = args[ARG_channel].u_int;
    i2s_channle_t*    channle = &self->channel[channel_num];
    
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
        self->chn_mask |= 0x3 << (channel_num * 2);
        i2s_init(self->i2s_num, I2S_RECEIVER, self->chn_mask);
        i2s_rx_channel_config(self->i2s_num,
                            channel_num,
                            channle->resolution,
                            channle->cycles,
                            TRIGGER_LEVEL_4,
                            channle->align_mode);
    }
    else
    {
        self->chn_mask |= 0x3 << (channel_num * 2);
        i2s_init(self->i2s_num, I2S_TRANSMITTER,self->chn_mask);
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

STATIC mp_obj_t Maix_i2s_set_sample_rate(void* self_, mp_obj_t sample_rate)
{
    Maix_i2s_obj_t* self = (Maix_i2s_obj_t*)self_;
    uint32_t smp_rate = mp_obj_get_int(sample_rate);
    if(smp_rate > MAX_SAMPLE_RATE)
    {
        mp_raise_ValueError("[MAIXPY]I2S:invalid sample rate");
    }
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
    //get i2s obj
    Maix_i2s_obj_t *self = pos_args[0];
    Maix_audio_obj_t *audio_obj = m_new_obj(Maix_audio_obj_t);
    audio_obj->audio.type = I2S_AUDIO;
    //parse parameter
    enum{ARG_points, 
         ARG_time,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_points, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_time, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = 0} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    audio_obj->base.type = &Maix_audio_type;

    //compute buffer length
    if(args[ARG_points].u_int > 0)
    {
        if(audio_obj->audio.points > self->points_num)
        {
            mp_raise_ValueError("[MAIXPY]I2S:Too many points");
        }
        audio_obj->audio.points = args[ARG_points].u_int;
        audio_obj->audio.buf = self->buf;
    }
    else if(args[ARG_time].u_int > 0)
    {
        if(self->sample_rate <= 0)
            mp_raise_ValueError("[MAIXPY]I2S:please set sample rate");
        uint32_t record_sec = args[ARG_time].u_int;
        uint32_t smp_points = self->sample_rate * record_sec;
        if(smp_points > self->points_num)
            mp_raise_ValueError("[MAIXPY]I2S:sampling size is out of bounds");
        audio_obj->audio.points = smp_points;
        audio_obj->audio.buf = self->buf;
    }else 
    {
        mp_raise_ValueError("[MAIXPY]I2S:please input recording points or time");
    }

    //record 
    i2s_receive_data_dma(self->i2s_num, audio_obj->audio.buf, audio_obj->audio.points , DMAC_CHANNEL5);
    dmac_wait_idle(DMAC_CHANNEL5);//wait to finish recv
    return MP_OBJ_FROM_PTR(audio_obj);
}
MP_DEFINE_CONST_FUN_OBJ_KW(Maix_i2s_record_obj,1,Maix_i2s_record);

STATIC mp_obj_t Maix_i2s_play(void*self_, mp_obj_t audio_obj)
{
    Maix_i2s_obj_t* self = (Maix_i2s_obj_t*)self_;
    Maix_audio_obj_t *audio_p = MP_OBJ_TO_PTR(audio_obj);
    i2s_send_data_dma(self->i2s_num, audio_p->audio.buf, audio_p->audio.points, DMAC_CHANNEL4);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(Maix_i2s_play_obj,Maix_i2s_play);

STATIC mp_obj_t Maix_i2s_deinit(void*self_)
{
    Maix_i2s_obj_t* self = (Maix_i2s_obj_t*)self_;
    m_del(uint32_t,self->buf,self->points_num);
    m_del_obj(Maix_i2s_obj_t,self);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(Maix_i2s_deinit_obj,Maix_i2s_deinit);

// STATIC  MP_DEFINE_CONST_FUN_OBJ_KW(Maix_i2s_set_dma_divede_16_obj,1,);
// STATIC  MP_DEFINE_CONST_FUN_OBJ_KW(Maix_i2s_set_dma_divede_16_obj,1,);


STATIC const mp_rom_map_elem_t Maix_i2s_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___deinit__),      MP_ROM_PTR(&Maix_i2s_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_init),            MP_ROM_PTR(&Maix_i2s_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel_config),  MP_ROM_PTR(&Maix_i2s_channel_config_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_sample_rate), MP_ROM_PTR(&Maix_i2s_set_sample_rate_obj) },
    { MP_ROM_QSTR(MP_QSTR_record),          MP_ROM_PTR(&Maix_i2s_record_obj) },
    { MP_ROM_QSTR(MP_QSTR_play),            MP_ROM_PTR(&Maix_i2s_play_obj) },
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

    { MP_ROM_QSTR(MP_QSTR_IGNORE_WORD_LENGTH), MP_ROM_INT(IGNORE_WORD_LENGTH) },
    { MP_ROM_QSTR(MP_QSTR_RESOLUTION_12_BIT),  MP_ROM_INT(RESOLUTION_12_BIT) },
    { MP_ROM_QSTR(MP_QSTR_RESOLUTION_16_BIT),  MP_ROM_INT(RESOLUTION_16_BIT) },
    { MP_ROM_QSTR(MP_QSTR_RESOLUTION_20_BIT),  MP_ROM_INT(RESOLUTION_20_BIT) },
    { MP_ROM_QSTR(MP_QSTR_RESOLUTION_24_BIT),  MP_ROM_INT(RESOLUTION_24_BIT) },
    { MP_ROM_QSTR(MP_QSTR_RESOLUTION_32_BIT),  MP_ROM_INT(RESOLUTION_32_BIT) },

    { MP_ROM_QSTR(MP_QSTR_SCLK_CYCLES_16), MP_ROM_INT(SCLK_CYCLES_16) },
    { MP_ROM_QSTR(MP_QSTR_SCLK_CYCLES_24), MP_ROM_INT(SCLK_CYCLES_24) },
    { MP_ROM_QSTR(MP_QSTR_SCLK_CYCLES_32), MP_ROM_INT(SCLK_CYCLES_32) },

    { MP_ROM_QSTR(MP_QSTR_TRANSMITTER), MP_ROM_INT(I2S_TRANSMITTER) },
    { MP_ROM_QSTR(MP_QSTR_RECEIVER),    MP_ROM_INT(I2S_RECEIVER) },

    { MP_ROM_QSTR(MP_QSTR_STANDARD_MODE),         MP_ROM_INT(STANDARD_MODE) },
    { MP_ROM_QSTR(MP_QSTR_RIGHT_JUSTIFYING_MODE), MP_ROM_INT(RIGHT_JUSTIFYING_MODE) },
    { MP_ROM_QSTR(MP_QSTR_LEFT_JUSTIFYING_MODE),  MP_ROM_INT(LEFT_JUSTIFYING_MODE) },
    
};

STATIC MP_DEFINE_CONST_DICT(Maix_i2s_dict, Maix_i2s_locals_dict_table);

const mp_obj_type_t Maix_i2s_type = {
    { &mp_type_type },
    .name = MP_QSTR_I2S,
    .print = Maix_i2s_print,
    .make_new = Maix_i2s_make_new,
    .locals_dict = (mp_obj_dict_t*)&Maix_i2s_dict,
};
