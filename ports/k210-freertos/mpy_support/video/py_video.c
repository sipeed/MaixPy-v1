

#include <mp.h>
#include "py/objstr.h"
#include "video.h"

#include "py_image.h"

typedef struct {
    mp_obj_base_t base;
    avi_t         obj;
} py_video_avi_obj_t;


static void py_video_avi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_video_avi_obj_t *self = (py_video_avi_obj_t*)self_in;
    avi_t* avi = &self->obj;

    mp_printf(print, "[MaixPy] avi:\n[video] w:%d, h:%d, t:%dus, fps:%.2f, total_frame:%d, status:%d\n"
                "[audio] format:%d, channel:%d, sample_rate:%d",
                avi->width, avi->height, avi->usec_per_frame, 1000.0/(avi->usec_per_frame/1000.0), 
                avi->total_frame, -avi->status,
                avi->audio_format, avi->audio_channels, avi->audio_sample_rate);
}


STATIC mp_obj_t py_video_play(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    py_video_avi_obj_t* arg_avi = (py_video_avi_obj_t*)args[0];
    if( arg_avi->obj.record )
        mp_raise_OSError(MP_EPERM);
    int status = video_play_avi(&arg_avi->obj);
    if(status > 0)
        mp_raise_OSError(status);
    return mp_obj_new_int(-status);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_video_play_obj, 0, py_video_play);

STATIC mp_obj_t py_video_volume(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    py_video_avi_obj_t* arg_avi = (py_video_avi_obj_t*)args[0];
    avi_t* avi = &arg_avi->obj;
    int volume = 100;
    if( n_args > 1)
    {
        avi->volume = mp_obj_get_int(args[1]);
    }
    volume = avi->volume;
    return mp_obj_new_int(volume);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_video_volume_obj, 0, py_video_volume);

static const mp_obj_type_t py_video_avi_type;
STATIC mp_obj_t py_video_record(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    printf("--%p\n", args[0]);
    py_video_avi_obj_t* arg_avi = (py_video_avi_obj_t*)args[0];
    printf("--\n");
    avi_t* avi = &arg_avi->obj;
    if( !avi->record )
        mp_raise_OSError(MP_EPERM);
    printf("--\n");
    //TODO:resolve align error
    // if( !py_image_obj_is_image(args[1]) )
    //     mp_raise_ValueError("param must be image obj");
    image_t* img = (image_t*)py_image_cobj(args[1]);
    printf("--\n");
    int ret = avi_record_append_video(avi, img);
    printf("--\n");
    if( ret == 0)
        ret = -MP_EIO;
    if( ret <= 0 )
        mp_raise_OSError(-ret);
    return mp_obj_new_int(ret);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_video_record_obj, 2, py_video_record);

STATIC mp_obj_t py_video_record_finish(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    py_video_avi_obj_t* arg_avi = (py_video_avi_obj_t*)args[0];
    avi_t* avi = &arg_avi->obj;
    if( !arg_avi->obj.record )
        mp_raise_OSError(MP_EPERM);
    avi_record_finish(avi);
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_video_record_finish_obj, 0, py_video_record_finish);

STATIC mp_obj_t py_video_del(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    py_video_avi_obj_t* arg_avi = (py_video_avi_obj_t*)args[0];
    avi_t* avi = &arg_avi->obj;
    if( !avi->record ) // play
    {
        video_hal_audio_deinit(avi);
    }
    else // record
    {

    }
    memset(avi, 0, sizeof(avi_t));
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_video_del_obj, 0, py_video_del);

static const mp_rom_map_elem_t locals_dict_table[] = {
    {MP_OBJ_NEW_QSTR(MP_QSTR___del__),  (&py_video_del_obj)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_play),  (&py_video_play_obj)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_volume),  (&py_video_volume_obj)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_record),  (&py_video_record_obj)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_record_finish),  (&py_video_record_finish_obj)},
};

STATIC MP_DEFINE_CONST_DICT(locals_dict, locals_dict_table);

static const mp_obj_type_t py_video_avi_type = {
    { &mp_type_type },
    .name  = MP_QSTR_avi,
    .print = py_video_avi_print,
    .locals_dict = (mp_obj_t) &locals_dict
};


mp_obj_t py_video_open(uint n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    const char *path = mp_obj_str_get_str(args[0]);
    GET_STR_LEN(args[0], len);
    enum { ARG_record,
           ARG_quality
        };
    const mp_arg_t machine_video_open_allowed_args[] = {
        { MP_QSTR_record,    MP_ARG_BOOL|MP_ARG_KW_ONLY, {.u_bool = false} },
        { MP_QSTR_quality,    MP_ARG_INT|MP_ARG_KW_ONLY, {.u_int = 50} }
    };
    mp_arg_val_t args_parsed[MP_ARRAY_SIZE(machine_video_open_allowed_args)];
    mp_arg_parse_all(n_args - 1, args + 1, kw_args,
        MP_ARRAY_SIZE(machine_video_open_allowed_args), machine_video_open_allowed_args, args_parsed);
    if(path[len-1]=='i' && path[len-2]=='v' && path[len-3]=='a' && path[len-4]=='.')
    {
        py_video_avi_obj_t *o = m_new_obj(py_video_avi_obj_t);
        o->base.type = &py_video_avi_type;
        avi_t* avi = &o->obj;
        memset(avi, 0, sizeof(avi_t));
        if(args_parsed[ARG_record].u_bool)// record avi
        {
            avi->record = true;
            avi->usec_per_frame = 200000; // 5fps
            avi->max_byte_sec   = 220525; //TODO:
            avi->width = 320;
            avi->height = 240;
            avi->mjpeg_quality = args_parsed[ARG_quality].u_int;
            avi->audio_sample_rate = 44100;
            avi->audio_channels = 1;
            avi->audio_format = AVI_AUDIO_FORMAT_PCM;
            int err = avi_record_header_init(path, avi);
            if( err != 0)
                mp_raise_OSError(err);
        }
        else//play avi
        {
            avi->record = false;
            int err = video_play_avi_init(path, avi);
            if(err != 0)
            {
                m_del_obj(py_video_avi_obj_t, o);
                mp_raise_OSError(err);
            }
        }
        return MP_OBJ_FROM_PTR(o);

    }
    else
    {
        mp_raise_NotImplementedError("[MaixPy] video: format not support");
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_video_open_obj, 2, py_video_open);


static const mp_map_elem_t globals_dict_table[] = {
    {MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_video)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_open), (mp_obj_t)&py_video_open_obj},
};

STATIC MP_DEFINE_CONST_DICT(globals_dict, globals_dict_table);

const mp_obj_module_t video_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_t)&globals_dict,
};
