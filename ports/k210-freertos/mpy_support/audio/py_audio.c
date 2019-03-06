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
    enum {ARG_array,
          ARG_path,
          ARG_points};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_array, MP_ARG_OBJ , {.u_obj = mp_const_none} },
        { MP_QSTR_path,  MP_ARG_OBJ  , {.u_int = mp_const_none} },
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
        printf("[MAIXPY] : audiof file init\n");    
        int err = 0;
        char* path_str = mp_obj_str_get_str(args[ARG_path].u_obj);
        mp_obj_t fp = vfs_internal_open(path_str,"+b",&err);
        if(err != 0)
        {
            fp = vfs_internal_open(path_str,"+ab",&err);
        }
        if( err != 0)
            mp_raise_OSError(err);
        audio_obj->fp = fp;
        audio_obj->type = FILE_AUDIO;
        //We can find the format of audio by path_str,but now just support wav
        //if(-1 != strstr(path_str,"wav"))
            audio_obj->format = AUDIO_WAV_FMT;
        //else if(-1 != strstr(path_str,"mp3"))
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
STATIC mp_obj_t Maix_audio_to_byte(Maix_audio_obj_t* self) {
    audio_t* audio = &self->audio; 
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

MP_DEFINE_CONST_FUN_OBJ_1(Maix_audio_to_byte_obj, Maix_audio_to_byte);

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
        printf("[MAIXPY]: file length is 0\n");
        return mp_const_false;
    }
    switch(audio->format)
    {
        case AUDIO_WAV_FMT:
            return wav_play_process(audio);
            break;
        default:
            break;
    }
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(Maix_audio_play_process_obj, Maix_audio_play_process);

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
STATIC mp_obj_t Maix_audio_record_process(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
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
        printf("[MAIXPY]: file length isn't empty\n");
        return mp_const_false;
    }
    switch(audio->format)
    {
        case AUDIO_WAV_FMT:
            if(args[ARG_channels].u_int > 2){//get channel num
                printf("[MAIXPY]: The number of channels must be less than or equal to 2\n");
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
    uint32_t err_code = 0;
    uint32_t close_code = 0;
    switch(audio->format)
    {
        case AUDIO_WAV_FMT:
            if(audio->decode_obj != NULL)
            {
                m_del(wav_decode_t,audio->decode_obj,1);
                audio->decode_obj = NULL;
            }
            if(audio->encode_obj != NULL)
            {
                //write head data
                vfs_internal_seek(audio->fp,0,VFS_SEEK_SET,&err_code);//
                wav_encode_t* wav_encode = audio->encode_obj;
                wav_encode->file.file_size = 44 - 8 + wav_encode->data.chunk_size;
                vfs_internal_write(audio->fp, &wav_encode->file, 12, &err_code);//write file chunk
                if(err_code != 0)
                {
                    printf("[MAIXPY]: write file chunk error  close file\n");
                    m_del(wav_encode_t,audio->encode_obj,1);
                    vfs_internal_close(audio->fp,&close_code);
                    mp_raise_OSError(err_code);
                }
                vfs_internal_write(audio->fp, &wav_encode->format, 24, &err_code);//write fromate chunk
                if(err_code != 0)
                {
                    printf("[MAIXPY]: write formate chunk error  close file\n");
                    m_del(wav_encode_t,audio->encode_obj,1);
                    vfs_internal_close(audio->fp,&close_code);
                    mp_raise_OSError(err_code);
                }
                vfs_internal_write(audio->fp, &wav_encode->data, 8 ,&err_code);//write data chunk
                if(err_code != 0)
                {
                    printf("[MAIXPY]: write data chunk error  close file\n");
                    m_del(wav_encode_t,audio->encode_obj,1);
                    vfs_internal_close(audio->fp,&close_code);
                    mp_raise_OSError(err_code);
                }
                m_del(wav_encode_t,audio->encode_obj,1);
            }
            break;
        default:
            break;
    }
    vfs_internal_close(audio->fp,&close_code);
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
    { MP_ROM_QSTR(MP_QSTR_tobyte), MP_ROM_PTR(&Maix_audio_to_byte_obj) },  
    { MP_ROM_QSTR(MP_QSTR_play_process), MP_ROM_PTR(&Maix_audio_play_process_obj) }, 
    { MP_ROM_QSTR(MP_QSTR_play), MP_ROM_PTR(&Maix_audio_play_obj) }, 
    { MP_ROM_QSTR(MP_QSTR_record_process), MP_ROM_PTR(&Maix_audio_record_process_obj) }, 
    { MP_ROM_QSTR(MP_QSTR_record), MP_ROM_PTR(&Maix_audio_record_obj) },
    { MP_ROM_QSTR(MP_QSTR_finish), MP_ROM_PTR(&Maix_audio_finish_obj) }, 
};

STATIC MP_DEFINE_CONST_DICT(Maix_audio_dict, Maix_audio_locals_dict_table);

const mp_obj_type_t Maix_audio_type = {
    { &mp_type_type },
    .print = Maix_audio_print,
    .name = MP_QSTR_AUDIO,
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
