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
#include "py/objarray.h"
#include "py/binary.h"
#include "mphalport.h"
#include "py_audio.h"
#include "Maix_i2s.h"
#include "modMaix.h"
#include "wav.h"
#include "vfs_internal.h"
#define MAX_SAMPLE_RATE 65535
#define MAX_SAMPLE_POINTS 1024

const mp_obj_type_t Maix_audio_type;


STATIC void Maix_audio_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    Maix_audio_obj_t *self = MP_OBJ_TO_PTR(self_in);
    audio_t* audio_obj = &self->audio;
    mp_printf(print, "[MAIXPY]audio:(points=%u, buffer addr=%p)",
        audio_obj->points,audio_obj->buf);
}

STATIC mp_obj_t Maix_audio_init_helper(Maix_audio_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    //parse paremeter
    enum {ARG_array,
          ARG_path,
          ARG_points};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_array, MP_ARG_OBJ , {.u_obj = mp_const_none} },
        { MP_QSTR_path,  MP_ARG_OBJ  , {.u_obj = mp_const_none} },
        { MP_QSTR_points, MP_ARG_INT | MP_ARG_KW_ONLY , {.u_int = MAX_SAMPLE_POINTS} },
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
    else if(args[ARG_path].u_obj == mp_const_none)
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
    }else if(args[ARG_path].u_obj != mp_const_none)
    {
        int err = 0;
        char* path_str = (char*)mp_obj_str_get_str(args[ARG_path].u_obj);
        mp_obj_t fp = vfs_internal_open(path_str,"rb",&err);
        if( err != 0)
            mp_raise_OSError(err);
        audio_obj->fp = fp;
        audio_obj->type = FILE_AUDIO;
        //We can find the format of audio by path_str,but now just support wav
        int16_t index_format = strlen(path_str)-4;
        index_format = (index_format>0)?index_format:0;
        if(NULL != strstr(path_str+index_format,".wav"))
            audio_obj->format = AUDIO_WAV_FMT;
        //else if(NULL != strstr(path_str,"mp3"))
        //    audio_obj->format = AUDIO_MP3_FMT;
    }
    else
    {
        return mp_const_false;
    }
    return mp_const_true;
}
STATIC mp_obj_t Maix_audio_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    //mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    // create instance
    Maix_audio_obj_t *self = m_new_obj(Maix_audio_obj_t);
    memset(self,0,sizeof(Maix_audio_obj_t));
    self->base.type = &Maix_audio_type;
    self->audio.type = EXT_AUDIO;
    self->audio.volume = 40;  // volume default 40%

    // init instance
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    if(mp_const_false == Maix_audio_init_helper(self, n_args, args, &kw_args))
        return mp_const_false;
    return MP_OBJ_FROM_PTR(self);
}

//----------------init------------------------

STATIC mp_obj_t Maix_audio_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return Maix_audio_init_helper(args[0], n_args -1 , args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(Maix_audio_init_obj,0 ,Maix_audio_init);

//----------------bo byte ------------------------
STATIC mp_obj_t Maix_audio_to_bytes(void* self) {
    audio_t* audio = &((Maix_audio_obj_t*)self)->audio; 
    if(audio->buf == NULL || audio->points == 0)
        mp_raise_msg(&mp_type_AttributeError,"empty Audio");
    mp_obj_array_t* audio_array = m_new_obj(mp_obj_array_t);
    audio_array->base.type = &mp_type_bytearray;
    audio_array->typecode = BYTEARRAY_TYPECODE;
    audio_array->free = 0;
    audio_array->len = audio->points * 4;
    audio_array->items = audio->buf;
    return audio_array;
}

MP_DEFINE_CONST_FUN_OBJ_1(Maix_audio_to_bytes_obj, Maix_audio_to_bytes);

//----------------play_process ------------------------
STATIC mp_obj_t Maix_audio_play_process(mp_obj_t self_in,mp_obj_t I2S_dev) {
    Maix_audio_obj_t* self = MP_OBJ_TO_PTR(self_in);
    audio_t* audio = &self->audio; 
    if(&Maix_i2s_type != mp_obj_get_type(I2S_dev))
        mp_raise_ValueError("Invaild I2S device");
    Maix_i2s_obj_t* i2s_dev = MP_OBJ_TO_PTR(I2S_dev);
    audio->dev = i2s_dev;
    audio->points = i2s_dev->points_num;//max points
    audio->buf = i2s_dev->buf;//buf addr
    uint32_t file_size = vfs_internal_size(audio->fp);
    if(0 == file_size)
    {
        mp_printf(&mp_plat_print, "[MAIXPY]: file length is 0\n");
        return mp_const_false;
    }
    switch(audio->format)
    {
        case AUDIO_WAV_FMT:
            return wav_play_process(audio,file_size);
            break;
        default:
            break;
    }
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(Maix_audio_play_process_obj, Maix_audio_play_process);


STATIC mp_obj_t Maix_audio_volume(size_t n_args, const mp_obj_t *args)
{
    Maix_audio_obj_t* self = MP_OBJ_TO_PTR(args[0]);
    if(n_args == 1)
        return mp_obj_new_float(self->audio.volume);
    float v = mp_obj_get_float(args[1]);
    if(v<0 || v>100)
        mp_raise_ValueError("value:[0,100]");
    self->audio.volume = v;
    return mp_obj_new_float(self->audio.volume);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(Maix_audio_volume_obj, 1, 2, Maix_audio_volume);

//----------------play ------------------------

STATIC mp_obj_t Maix_audio_play(mp_obj_t self_in) {
    Maix_audio_obj_t *self = MP_OBJ_TO_PTR(self_in);//get auduio obj
    audio_t* audio = &self->audio; 
    switch(audio->format)
    {
        case AUDIO_WAV_FMT:
            return wav_play(audio);
            break;
        default:
            break;
    }
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(Maix_audio_play_obj,Maix_audio_play);

//----------------record_process ------------------------
STATIC mp_obj_t Maix_audio_record_process(size_t n_args, const mp_obj_t * pos_args, mp_map_t *kw_args)
{
    Maix_audio_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);//get py_audio
    //parse parameter
    enum{ARG_I2S_dev,
         ARG_channels, 
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_dev, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_channels, MP_ARG_INT, {.u_int = 1} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if(&Maix_i2s_type != mp_obj_get_type(args[ARG_I2S_dev].u_obj))
        mp_raise_ValueError("Invaild I2S device");

    Maix_i2s_obj_t* i2s_dev = MP_OBJ_TO_PTR(args[ARG_I2S_dev].u_obj);//get i2s device
    audio_t* audio = &self->audio; //get audio
    audio->dev = i2s_dev;//set dev
    audio->points = i2s_dev->points_num;//max points
    audio->buf = i2s_dev->buf;//buf addr
    uint32_t file_size = vfs_internal_size(audio->fp);
    if(0 != file_size)
    {
        mp_printf(&mp_plat_print, "[MAIXPY]: file length isn't empty\n");
        return mp_const_false;
    }
    switch(audio->format)
    {
        case AUDIO_WAV_FMT:
            if(args[ARG_channels].u_int > 2){//get channel num
                mp_printf(&mp_plat_print, "[MAIXPY]: The number of channels must be less than or equal to 2\n");
                return mp_const_false;
            }
            return wav_record_process(audio,args[ARG_channels].u_int);
            break;
        default:
            break;
    }
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(Maix_audio_record_process_obj,2, Maix_audio_record_process);

//----------------record ------------------------

STATIC mp_obj_t Maix_audio_record(mp_obj_t self_in) {
    Maix_audio_obj_t *self = MP_OBJ_TO_PTR(self_in);//get auduio obj
    audio_t* audio = &self->audio; 
    switch(audio->format)
    {
        case AUDIO_WAV_FMT:
            wav_record(audio,DMAC_CHANNEL5);
            break;
        default:
            break;
    }
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(Maix_audio_record_obj,Maix_audio_record);

//----------------finish ------------------------

STATIC mp_obj_t Maix_audio_finish(mp_obj_t self_in) {
    Maix_audio_obj_t *self = MP_OBJ_TO_PTR(self_in);//get auduio obj
    audio_t* audio = &self->audio; 

    switch(audio->format)
    {
        case AUDIO_WAV_FMT:
            wav_finish(audio);
            break;
        default:
            break;
    }
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(Maix_audio_finish_obj,Maix_audio_finish);

//----------------deinit ------------------------

STATIC mp_obj_t Maix_audio_deinit(mp_obj_t self_in)
{
    Maix_audio_obj_t* self = MP_OBJ_TO_PTR(self_in);
    audio_t* audio = &self->audio; 
    if(audio->type == EXT_AUDIO)
        m_del(uint32_t, audio->buf, audio->points);
    m_del_obj(Maix_audio_obj_t,self);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(Maix_audio_deinit_obj, Maix_audio_deinit);

STATIC const mp_rom_map_elem_t Maix_audio_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&Maix_audio_init_obj) },
    { MP_ROM_QSTR(MP_QSTR___deinit__), MP_ROM_PTR(&Maix_audio_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_to_bytes), MP_ROM_PTR(&Maix_audio_to_bytes_obj) },  
    { MP_ROM_QSTR(MP_QSTR_play_process), MP_ROM_PTR(&Maix_audio_play_process_obj) }, 
    { MP_ROM_QSTR(MP_QSTR_volume), MP_ROM_PTR(&Maix_audio_volume_obj) }, 
    { MP_ROM_QSTR(MP_QSTR_play), MP_ROM_PTR(&Maix_audio_play_obj) }, 
    { MP_ROM_QSTR(MP_QSTR_record_process), MP_ROM_PTR(&Maix_audio_record_process_obj) }, 
    { MP_ROM_QSTR(MP_QSTR_record), MP_ROM_PTR(&Maix_audio_record_obj) },
    { MP_ROM_QSTR(MP_QSTR_finish), MP_ROM_PTR(&Maix_audio_finish_obj) }, 
};

STATIC MP_DEFINE_CONST_DICT(Maix_audio_dict, Maix_audio_locals_dict_table);

const mp_obj_type_t Maix_audio_type = {
    { &mp_type_type },
    .print = Maix_audio_print,
    .name = MP_QSTR_Audio,
    .make_new = Maix_audio_make_new,
    .locals_dict = (mp_obj_dict_t*)&Maix_audio_dict,
};

static const mp_rom_map_elem_t globals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__),            MP_OBJ_NEW_QSTR(MP_QSTR_audio)},
    { MP_ROM_QSTR(MP_QSTR_Audio),  MP_ROM_PTR(&Maix_audio_type) },
};


STATIC MP_DEFINE_CONST_DICT(globals_dict, globals_dict_table);
const mp_obj_module_t audio_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_t) &globals_dict,
};
