#include <stdio.h>
#include <string.h>

#include "py/obj.h"
#include "py/objarray.h"
#include "py/binary.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "py/mpconfig.h"

#include "i2s.h"
#include "Maix_i2s.h"

#include "speech_recognizer.h"

extern const mp_obj_type_t Maix_i2s_type;
const mp_obj_type_t modules_speech_recognizer_type;

typedef struct _speech_recognizer_obj_t
{
    mp_obj_base_t base;
    Maix_i2s_obj_t *dev;
    uint32_t keyword_num;
    uint32_t model_num;
    uint16_t frm_num;
    uint32_t voice_model_len;
    int16_t *p_mfcc_data;
} speech_recognizer_obj_t;

STATIC mp_obj_t modules_speech_recognizer_record(size_t n_args, const mp_obj_t *args)
{
    mp_printf(&mp_plat_print, "modules_speech_recognizer_record\r\n");

    mp_int_t keyword_num = mp_obj_get_int(args[1]);
    mp_int_t model_num = mp_obj_get_int(args[2]);
    if (keyword_num > 10)
    {
        mp_printf(&mp_plat_print, "[MaixPy] keyword_num>10\n");
        return mp_const_false;
    }
    if (model_num > 4)
    {
        mp_printf(&mp_plat_print, "[MaixPy] keyword_num>4\n");
        return mp_const_false;
    }
    mp_printf(&mp_plat_print, "[MAIXPY]: record[%d:%d]\n", keyword_num, model_num);
    speech_recognizer_record(keyword_num, model_num);
    return mp_const_true;
}

STATIC mp_obj_t modules_speech_recognizer_get_model_data_info(size_t n_args, const mp_obj_t *args)
{
    speech_recognizer_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t keyword_num = mp_obj_get_int(args[1]);
    mp_int_t model_num = mp_obj_get_int(args[2]);

    speech_recognizer_get_data(keyword_num, model_num, &self->frm_num, &self->p_mfcc_data, &self->voice_model_len);
    mp_obj_t list = mp_obj_new_list(0, NULL);
    mp_obj_list_append(list, MP_OBJ_NEW_SMALL_INT(self->frm_num));
    return list;
}

STATIC mp_obj_t modules_speech_recognizer_print_model(size_t n_args, const mp_obj_t *args)
{
    speech_recognizer_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t keyword_num = mp_obj_get_int(args[1]);
    mp_int_t model_num = mp_obj_get_int(args[2]);
    if (keyword_num > 10)
    {
        mp_printf(&mp_plat_print, "[MaixPy] keyword_num>10\n");
        return mp_const_false;
    }
    if (model_num > 4)
    {
        mp_printf(&mp_plat_print, "[MaixPy] keyword_num>4\n");
        return mp_const_false;
    }

    speech_recognizer_get_data(keyword_num, model_num, &self->frm_num, &self->p_mfcc_data, &self->voice_model_len);

    mp_obj_array_t *sr_array = m_new_obj(mp_obj_array_t);
    sr_array->base.type = &mp_type_bytearray;
    sr_array->typecode = BYTEARRAY_TYPECODE;
    sr_array->free = 0;
    sr_array->len = self->voice_model_len;
    sr_array->items = self->p_mfcc_data;

    mp_printf(&mp_plat_print, "[MaixPy] [(%d,%d)|frm_num:%d]\n", keyword_num, model_num, self->frm_num);

    printf("\r\n[%s]-----------------\r\n", __FUNCTION__);
    int16_t *pbuf_16 = (int16_t *)sr_array->items;
    for (int i = 0; i < (sr_array->len); i++)
    {
        if (((i + 1) % 20) == 0)
        {
            printf("%4d  \n", pbuf_16[i]);
            msleep(2);
        }
        else
            printf("%4d  ", pbuf_16[i]);
    }
    printf("\r\n-----------------#\r\n");

    return mp_const_true;
}

// -----------------------------------------------------------------------------
STATIC mp_obj_t modules_speech_recognizer_get_model_data(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    speech_recognizer_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t keyword_num = mp_obj_get_int(args[1]);
    mp_int_t model_num = mp_obj_get_int(args[2]);
    if (keyword_num > 10)
    {
        mp_printf(&mp_plat_print, "[MaixPy] keyword_num>10\n");
        return mp_const_false;
    }
    if (model_num > 4)
    {
        mp_printf(&mp_plat_print, "[MaixPy] model_num>4\n");
        return mp_const_false;
    }

    mp_printf(&mp_plat_print, "[MaixPy] [(%d,%d)|frm_num:%d]\n", keyword_num, model_num, self->frm_num);

    speech_recognizer_get_data(keyword_num, model_num, &self->frm_num, &self->p_mfcc_data, &self->voice_model_len);
    mp_obj_array_t *sr_array = m_new_obj(mp_obj_array_t);
    sr_array->base.type = &mp_type_bytearray;
    sr_array->typecode = BYTEARRAY_TYPECODE;
    sr_array->free = 0;
    sr_array->len = self->voice_model_len;
    sr_array->items = self->p_mfcc_data;

    return sr_array;
}

STATIC mp_obj_t modules_speech_recognizer_set_threshold(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    speech_recognizer_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t n_thl = mp_obj_get_int(args[1]);
    mp_int_t z_thl = mp_obj_get_int(args[2]);
    mp_int_t s_thl = mp_obj_get_int(args[3]);

    speech_recognizer_set_Threshold(n_thl, z_thl, s_thl);
    return mp_const_true;
}
STATIC mp_obj_t modules_speech_recognizer_add_voice_model(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    speech_recognizer_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t keyword_num = mp_obj_get_int(args[1]);
    mp_int_t model_num = mp_obj_get_int(args[2]);
    mp_int_t frm_num = mp_obj_get_int(args[3]);
    mp_obj_array_t *sr_array = MP_OBJ_TO_PTR(args[4]);
    self->p_mfcc_data = (int16_t *)sr_array->items;
    self->voice_model_len = sr_array->len;
    self->frm_num = frm_num;
    if (keyword_num > 10)
    {
        mp_printf(&mp_plat_print, "[MaixPy] keyword_num>10\n");
        return mp_const_false;
    }
    if (model_num > 4)
    {
        mp_printf(&mp_plat_print, "[MaixPy] keyword_num>4\n");
        return mp_const_false;
    }
    mp_printf(&mp_plat_print, "[MaixPy] add_voice_model[%d:%d]\n", keyword_num, model_num);
    mp_printf(&mp_plat_print, "[MaixPy] model[frm_num:%d:len:%d]\n", self->frm_num, sr_array->len);
    // mp_printf(&mp_plat_print, "[MaixPy] self[p:0x%X:len:%d]\n", self->p_mfcc_data, self->voice_model_len);
    // mp_printf(&mp_plat_print, "[MaixPy] sr_array[p:0x%X:len:%d]\n", sr_array->items, sr_array->len);

    speech_recognizer_add_voice_model(keyword_num, model_num, self->p_mfcc_data, self->frm_num);

    return mp_const_true;
}

// -----------------------------------------------------------------------------
STATIC mp_obj_t modules_speech_recognizer_init_helper(speech_recognizer_obj_t *self_in, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    //parse paremeter
    enum
    {
        ARG_dev,
    };
    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_dev, MP_ARG_OBJ, {.u_obj = mp_const_none}},
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    speech_recognizer_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (args[ARG_dev].u_obj != mp_const_none)
    {
        mp_obj_t I2S_dev = args[ARG_dev].u_obj;
        if (&Maix_i2s_type != mp_obj_get_type(I2S_dev))
            mp_raise_ValueError("Invaild I2S device");
        Maix_i2s_obj_t *i2s_dev = MP_OBJ_TO_PTR(I2S_dev);
        self->dev = i2s_dev;
        mp_printf(&mp_plat_print, "[MaixPy] i2s_num:%d\r\n", self->dev->i2s_num);
        speech_recognizer_init(self->dev);
    }
    else
    {
        mp_raise_ValueError("init speech recognizer error\r\n");
        return mp_const_false;
    }
    return mp_const_true;
}

STATIC mp_obj_t modules_speech_recognizer_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    return modules_speech_recognizer_init_helper(args[0], n_args - 1, args + 1, kw_args);
}

STATIC mp_obj_t modules_speech_recognizer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    mp_arg_check_num(n_args, n_kw, 1, 1, true);

    speech_recognizer_obj_t *self = m_new_obj_with_finaliser(speech_recognizer_obj_t);
    self->base.type = &modules_speech_recognizer_type;

    // init instance
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    if (mp_const_false == modules_speech_recognizer_init_helper(self, n_args, args, &kw_args))
        return mp_const_false;

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t modules_speech_recognizer_get_result(mp_obj_t self_in)
{
    int res = speech_recognizer_get_result();
    return mp_obj_new_int(res);
}
STATIC mp_obj_t modules_speech_recognizer_finish(mp_obj_t self_in)
{
    speech_recognizer_finish();
    return mp_const_true;
}
STATIC mp_obj_t modules_speech_recognizer_recognize(mp_obj_t self_in)
{
    // mp_printf(&mp_plat_print, "[MaixPy] recognize...\r\n");
    speech_recognizer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t rev_val = speech_recognizer_recognize();
    return mp_obj_new_int(rev_val);
}

STATIC mp_obj_t modules_speech_recognizer_get_status(mp_obj_t self_in)
{
    mp_int_t status_val = speech_recognizer_get_status();
    return mp_obj_new_int(status_val);
}

MP_DEFINE_CONST_FUN_OBJ_KW(modules_speech_recognizer_init_obj, 1, modules_speech_recognizer_init);
MP_DEFINE_CONST_FUN_OBJ_KW(modules_speech_recognizer_get_status_obj, 1, modules_speech_recognizer_get_status);
MP_DEFINE_CONST_FUN_OBJ_KW(modules_speech_recognizer_get_result_obj, 1, modules_speech_recognizer_get_result);
MP_DEFINE_CONST_FUN_OBJ_KW(modules_speech_recognizer_finish_obj, 1, modules_speech_recognizer_finish);
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modules_speech_recognizer_record_obj, 3, 3, modules_speech_recognizer_record);
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modules_speech_recognizer_print_model_obj, 3, 3, modules_speech_recognizer_print_model);
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modules_speech_recognizer_get_model_data_info_obj, 3, 3, modules_speech_recognizer_get_model_data_info);
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modules_speech_recognizer_get_model_data_obj, 3, 3, modules_speech_recognizer_get_model_data);
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modules_speech_recognizer_add_voice_model_obj, 5, 5, modules_speech_recognizer_add_voice_model);
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modules_speech_recognizer_set_threshold_obj, 2, 4, modules_speech_recognizer_set_threshold);
MP_DEFINE_CONST_FUN_OBJ_1(modules_speech_recognizer_recognize_obj, modules_speech_recognizer_recognize);

STATIC const mp_rom_map_elem_t mp_module_speech_recognizer_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&modules_speech_recognizer_init_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_status), MP_ROM_PTR(&modules_speech_recognizer_get_status_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_result), MP_ROM_PTR(&modules_speech_recognizer_get_result_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_del), MP_ROM_PTR(&modules_speech_recognizer_finish_obj)},

    {MP_ROM_QSTR(MP_QSTR_record), MP_ROM_PTR(&modules_speech_recognizer_record_obj)},

    {MP_ROM_QSTR(MP_QSTR_print_model), MP_ROM_PTR(&modules_speech_recognizer_print_model_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_model_data), MP_ROM_PTR(&modules_speech_recognizer_get_model_data_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_model_info), MP_ROM_PTR(&modules_speech_recognizer_get_model_data_info_obj)},

    {MP_ROM_QSTR(MP_QSTR_add_voice_model), MP_ROM_PTR(&modules_speech_recognizer_add_voice_model_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_threshold), MP_ROM_PTR(&modules_speech_recognizer_set_threshold_obj)},

    {MP_ROM_QSTR(MP_QSTR_recognize), MP_ROM_PTR(&modules_speech_recognizer_recognize_obj)},
};

MP_DEFINE_CONST_DICT(mp_module_speech_recognizer_locals_dict, mp_module_speech_recognizer_locals_dict_table);

const mp_obj_type_t modules_speech_recognizer_type = {
    .base = {&mp_type_type},
    .name = MP_QSTR_SpeechRecognizer,
    // .print = modules_speech_recognizer_print,
    .make_new = modules_speech_recognizer_make_new,
    .locals_dict = (mp_obj_dict_t *)&mp_module_speech_recognizer_locals_dict,
};