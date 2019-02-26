

#include <mp.h>
#include "py/objstr.h"
#include "video.h"

typedef struct {
    mp_obj_base_t base;
    avi_t         obj;
} py_video_avi_obj_t;


static void py_video_avi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_video_avi_obj_t *self = (py_video_avi_obj_t*)self_in;
    avi_t* avi = &self->obj;

    mp_printf(print, "[MaixPy] video_avi:\n[video] w:%d, h:%d, t:%dus, fps:%.2f, total_frame:%d, status:%d\n"
                "[audio] type:%d, channel:%d, sample_rate:%d",
                avi->width, avi->height, avi->sec_per_frame, 1000.0/(avi->sec_per_frame/1000.0), 
                avi->total_frame, avi->status,
                avi->audio_type, avi->audio_channels, avi->audio_sample_rate);
}


STATIC mp_obj_t py_video_play(uint n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    py_video_avi_obj_t* arg_avi = (py_video_avi_obj_t*)args[0];
    int status = video_play_avi(&arg_avi->obj);
    if(status > 0)
        mp_raise_OSError(status);
    return mp_obj_new_int(-status);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_video_play_obj, 0, py_video_play);

static const mp_rom_map_elem_t locals_dict_table[] = {
    {MP_OBJ_NEW_QSTR(MP_QSTR_play),  (&py_video_play_obj)}
};

STATIC MP_DEFINE_CONST_DICT(locals_dict, locals_dict_table);

static const mp_obj_type_t py_video_avi_type = {
    { &mp_type_type },
    .name  = MP_QSTR_video_avi,
    .print = py_video_avi_print,
    .locals_dict = (mp_obj_t) &locals_dict
};


mp_obj_t py_video_open(uint n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    const char *path = mp_obj_str_get_str(args[0]);
    GET_STR_LEN(args[0], len);
    if(path[len-1]=='i' && path[len-2]=='v' && path[len-3]=='a' && path[len-4]=='.')
    {
        py_video_avi_obj_t *o = m_new_obj(py_video_avi_obj_t);
        o->base.type = &py_video_avi_type;
        avi_t* avi = &o->obj;
        int err = video_play_avi_init(path, avi);
        if(err != 0)
        {
            m_del_obj(py_video_avi_obj_t, o);
            mp_raise_OSError(err);
        }
        return MP_OBJ_FROM_PTR(o);

    }
    else
    {
        mp_raise_NotImplementedError("[MaixPy] video: format not support");
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_video_open_obj, 1, py_video_open);


static const mp_map_elem_t globals_dict_table[] = {
    {MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_video)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_open), (mp_obj_t)&py_video_open_obj},
};

STATIC MP_DEFINE_CONST_DICT(globals_dict, globals_dict_table);

const mp_obj_module_t video_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_t)&globals_dict,
};
