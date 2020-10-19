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

#include "isolated_word.h"

// extern const mp_obj_type_t Maix_i2s_type;
const mp_obj_type_t speech_isolated_word_type;

typedef struct _isolated_word_obj_t
{
    mp_obj_base_t base;

    uint16_t size;
    i2s_device_number_t device_num;
    dmac_channel_number_t channel_num;
    v_ftr_tag *mfcc_dats;
} isolated_word_obj_t;

STATIC mp_obj_t speech_isolated_word_init_helper(isolated_word_obj_t *self_in, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    isolated_word_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    //parse paremeter
    enum
    {
        ARG_size,
        ARG_i2s,
        ARG_dmac,
        ARG_priority,
    };
    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_size, MP_ARG_INT, {.u_int = 10}},
        {MP_QSTR_i2s, MP_ARG_INT, {.u_int = I2S_DEVICE_0}},
        {MP_QSTR_dmac, MP_ARG_INT, {.u_int = DMAC_CHANNEL2}},
        {MP_QSTR_priority, MP_ARG_INT, {.u_int = 3}},
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    self->mfcc_dats = m_new(v_ftr_tag, args[ARG_size].u_int);
    if (self->mfcc_dats == NULL) {
        mp_raise_ValueError("[MAIXPY] mfcc_dats malloc fail (not enough memory)");
    }
    self->size = args[ARG_size].u_int;
    self->device_num = args[ARG_i2s].u_int;
    self->channel_num = args[ARG_dmac].u_int;
    iw_load(args[ARG_i2s].u_int, args[ARG_dmac].u_int, args[ARG_priority].u_int);

    return mp_const_none;
}

STATIC mp_obj_t speech_isolated_word_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    mp_arg_check_num(n_args, n_kw, 0, 5, true);

    isolated_word_obj_t *self = m_new_obj_with_finaliser(isolated_word_obj_t);
    self->base.type = &speech_isolated_word_type;

    // init instance
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    if (mp_const_false == speech_isolated_word_init_helper(self, n_args, args, &kw_args))
        return mp_const_false;

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t speech_isolated_word_make_del(mp_obj_t self_in){
    if(mp_obj_get_type(self_in) == &speech_isolated_word_type) {
        isolated_word_obj_t *self = MP_OBJ_TO_PTR(self_in);
        // mp_printf(&mp_plat_print, "%s __del__\r\n", __func__);
        if (self->mfcc_dats != NULL) {
            iw_free(self->channel_num);
            m_del(v_ftr_tag, self->mfcc_dats, self->size);
            self->mfcc_dats = NULL;
        }
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(speech_isolated_word_del_obj, speech_isolated_word_make_del);

STATIC mp_obj_t speech_isolated_word_size(mp_obj_t self_in) {
    isolated_word_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(self->size);
}
MP_DEFINE_CONST_FUN_OBJ_1(speech_isolated_word_size_obj, speech_isolated_word_size);

STATIC mp_obj_t speech_isolated_word_state(mp_obj_t self) {
    return mp_obj_new_int(iw_get_state());
}
MP_DEFINE_CONST_FUN_OBJ_1(speech_isolated_word_state_obj, speech_isolated_word_state);

STATIC void speech_isolated_word_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    isolated_word_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "[MAIXPY] isolated_word:(%p)\r\n mfcc_dats=%p\r\n size=%d\r\n i2s_device_number_t=%d\r\n dmac_channel_number_t=%d\r\n", self, self->mfcc_dats, self->size, self->device_num, self->channel_num);
}

STATIC mp_obj_t speech_isolated_word_set_threshold(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    isolated_word_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    //parse parameter
    enum{ARG_n_thl, 
         ARG_z_thl,
         ARG_s_thl,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_n_thl, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_z_thl, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_s_thl, MP_ARG_INT, {.u_int = 10000} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    iw_atap_tag(args[ARG_n_thl].u_int, args[ARG_z_thl].u_int, args[ARG_s_thl].u_int);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(speech_isolated_word_set_threshold_obj, 3, speech_isolated_word_set_threshold);

STATIC mp_obj_t speech_isolated_word_set(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    isolated_word_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    //parse parameter
    enum{ARG_pos, 
         ARG_frames,
         ARG_mfcc_dat,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_n_thl, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_frames, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_mfcc_dat, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // isolated_word_set_Threshold(args[ARG_n_thl].u_int, args[ARG_z_thl].u_int, args[ARG_s_thl].u_int);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(speech_isolated_word_set_obj, 3, speech_isolated_word_set);

STATIC mp_obj_t speech_isolated_word_get(mp_obj_t self, mp_obj_t pos_in) {
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(speech_isolated_word_get_obj, speech_isolated_word_get);

STATIC mp_obj_t speech_isolated_word_result(mp_obj_t self) {
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(speech_isolated_word_result_obj, speech_isolated_word_result);

STATIC mp_obj_t speech_isolated_word_recognize(mp_obj_t self) {
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(speech_isolated_word_recognize_obj, speech_isolated_word_recognize);

STATIC mp_obj_t speech_isolated_word_record(mp_obj_t self, mp_obj_t pos_in) {
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(speech_isolated_word_record_obj, speech_isolated_word_record);

STATIC const mp_rom_map_elem_t mp_module_isolated_word_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),        MP_OBJ_NEW_QSTR(MP_QSTR_isolated_word) },

    {MP_ROM_QSTR(MP_QSTR_set_threshold), MP_ROM_PTR(&speech_isolated_word_set_threshold_obj)},
    {MP_ROM_QSTR(MP_QSTR_size), MP_ROM_PTR(&speech_isolated_word_size_obj)},
    {MP_ROM_QSTR(MP_QSTR_set), MP_ROM_PTR(&speech_isolated_word_set_obj)},
    {MP_ROM_QSTR(MP_QSTR_get), MP_ROM_PTR(&speech_isolated_word_get_obj)},
    {MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&speech_isolated_word_del_obj)},

    {MP_ROM_QSTR(MP_QSTR_record), MP_ROM_PTR(&speech_isolated_word_record_obj)},
    {MP_ROM_QSTR(MP_QSTR_recognize), MP_ROM_PTR(&speech_isolated_word_recognize_obj)},
    {MP_ROM_QSTR(MP_QSTR_state), MP_ROM_PTR(&speech_isolated_word_state_obj)},
    {MP_ROM_QSTR(MP_QSTR_result), MP_ROM_PTR(&speech_isolated_word_result_obj)},
};

MP_DEFINE_CONST_DICT(mp_module_isolated_word_locals_dict, mp_module_isolated_word_locals_dict_table);

const mp_obj_type_t speech_isolated_word_type = {
    .base = {&mp_type_type},
    .name = MP_QSTR_SpeechRecognizer,
    .print = speech_isolated_word_print,
    .make_new = speech_isolated_word_make_new,
    .locals_dict = (mp_obj_dict_t *)&mp_module_isolated_word_locals_dict,
};


// STATIC mp_obj_t speech_isolated_word_record(size_t n_args, const mp_obj_t *args)
// {
//     mp_printf(&mp_plat_print, "speech_isolated_word_record\r\n");

//     mp_int_t keyword_num = mp_obj_get_int(args[1]);
//     mp_int_t model_num = mp_obj_get_int(args[2]);
//     if (keyword_num > 10)
//     {
//         mp_printf(&mp_plat_print, "[MaixPy] keyword_num>10\n");
//         return mp_const_false;
//     }
//     if (model_num > 4)
//     {
//         mp_printf(&mp_plat_print, "[MaixPy] keyword_num>4\n");
//         return mp_const_false;
//     }
//     mp_printf(&mp_plat_print, "[MAIXPY]: record[%d:%d]\n", keyword_num, model_num);
//     // isolated_word_record(keyword_num, model_num);
//     return mp_const_true;
// }
// MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(speech_isolated_word_record_obj, 3, 3, speech_isolated_word_record);

// STATIC mp_obj_t speech_isolated_word_get_model_data_info(size_t n_args, const mp_obj_t *args)
// {
//     isolated_word_obj_t *self = MP_OBJ_TO_PTR(args[0]);
//     mp_int_t keyword_num = mp_obj_get_int(args[1]);
//     mp_int_t model_num = mp_obj_get_int(args[2]);

//     mp_obj_t list = mp_obj_new_list(0, NULL);
//     mp_obj_list_append(list, MP_OBJ_NEW_SMALL_INT(self->frm_num));
//     return list;
// }
// MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(speech_isolated_word_get_model_data_info_obj, 3, 3, speech_isolated_word_get_model_data_info);

// STATIC mp_obj_t speech_isolated_word_print_model(size_t n_args, const mp_obj_t *args)
// {
//     isolated_word_obj_t *self = MP_OBJ_TO_PTR(args[0]);
//     mp_int_t keyword_num = mp_obj_get_int(args[1]);
//     mp_int_t model_num = mp_obj_get_int(args[2]);
//     if (keyword_num > 10)
//     {
//         mp_printf(&mp_plat_print, "[MaixPy] keyword_num>10\n");
//         return mp_const_false;
//     }
//     if (model_num > 4)
//     {
//         mp_printf(&mp_plat_print, "[MaixPy] keyword_num>4\n");
//         return mp_const_false;
//     }

//     mp_obj_array_t *sr_array = m_new_obj(mp_obj_array_t);
//     sr_array->base.type = &mp_type_bytearray;
//     sr_array->typecode = BYTEARRAY_TYPECODE;
//     sr_array->free = 0;
//     sr_array->len = self->voice_model_len;
//     sr_array->items = self->p_mfcc_data;

//     mp_printf(&mp_plat_print, "[MaixPy] [(%d,%d)|frm_num:%d]\n", keyword_num, model_num, self->frm_num);

//     printf("\r\n[%s]-----------------\r\n", __FUNCTION__);
//     int16_t *pbuf_16 = (int16_t *)sr_array->items;
//     for (int i = 0; i < (sr_array->len); i++)
//     {
//         if (((i + 1) % 20) == 0)
//         {
//             printf("%4d  \n", pbuf_16[i]);
//             msleep(2);
//         }
//         else
//             printf("%4d  ", pbuf_16[i]);
//     }
//     printf("\r\n-----------------#\r\n");

//     return mp_const_true;
// }
// MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(speech_isolated_word_print_model_obj, 3, 3, speech_isolated_word_print_model);

// // -----------------------------------------------------------------------------
// STATIC mp_obj_t speech_isolated_word_get_model_data(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
// {
//     isolated_word_obj_t *self = MP_OBJ_TO_PTR(args[0]);
//     mp_int_t keyword_num = mp_obj_get_int(args[1]);
//     mp_int_t model_num = mp_obj_get_int(args[2]);
//     if (keyword_num > 10)
//     {
//         mp_printf(&mp_plat_print, "[MaixPy] keyword_num>10\n");
//         return mp_const_false;
//     }
//     if (model_num > 4)
//     {
//         mp_printf(&mp_plat_print, "[MaixPy] model_num>4\n");
//         return mp_const_false;
//     }

//     mp_printf(&mp_plat_print, "[MaixPy] [(%d,%d)|frm_num:%d]\n", keyword_num, model_num, self->frm_num);

//     mp_obj_array_t *sr_array = m_new_obj(mp_obj_array_t);
//     sr_array->base.type = &mp_type_bytearray;
//     sr_array->typecode = BYTEARRAY_TYPECODE;
//     sr_array->free = 0;
//     sr_array->len = self->voice_model_len;
//     sr_array->items = self->p_mfcc_data;

//     return sr_array;
// }
// MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(speech_isolated_word_get_model_data_obj, 3, 3, speech_isolated_word_get_model_data);

// STATIC mp_obj_t speech_isolated_word_set_threshold(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
// {
//     isolated_word_obj_t *self = MP_OBJ_TO_PTR(args[0]);
//     mp_int_t n_thl = mp_obj_get_int(args[1]);
//     mp_int_t z_thl = mp_obj_get_int(args[2]);
//     mp_int_t s_thl = mp_obj_get_int(args[3]);

//     // isolated_word_set_Threshold(n_thl, z_thl, s_thl);
//     return mp_const_true;
// }
// MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(speech_isolated_word_set_threshold_obj, 2, 4, speech_isolated_word_set_threshold);

// STATIC mp_obj_t speech_isolated_word_add_voice_model(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
// {
//     isolated_word_obj_t *self = MP_OBJ_TO_PTR(args[0]);
//     mp_int_t keyword_num = mp_obj_get_int(args[1]);
//     mp_int_t model_num = mp_obj_get_int(args[2]);
//     mp_int_t frm_num = mp_obj_get_int(args[3]);
//     mp_obj_array_t *sr_array = MP_OBJ_TO_PTR(args[4]);
//     self->p_mfcc_data = (int16_t *)sr_array->items;
//     self->voice_model_len = sr_array->len;
//     self->frm_num = frm_num;
//     if (keyword_num > 10)
//     {
//         mp_printf(&mp_plat_print, "[MaixPy] keyword_num>10\n");
//         return mp_const_false;
//     }
//     if (model_num > 4)
//     {
//         mp_printf(&mp_plat_print, "[MaixPy] keyword_num>4\n");
//         return mp_const_false;
//     }
//     mp_printf(&mp_plat_print, "[MaixPy] add_voice_model[%d:%d]\n", keyword_num, model_num);
//     mp_printf(&mp_plat_print, "[MaixPy] model[frm_num:%d:len:%d]\n", self->frm_num, sr_array->len);
//     // mp_printf(&mp_plat_print, "[MaixPy] self[p:0x%X:len:%d]\n", self->p_mfcc_data, self->voice_model_len);
//     // mp_printf(&mp_plat_print, "[MaixPy] sr_array[p:0x%X:len:%d]\n", sr_array->items, sr_array->len);

//     return mp_const_true;
// }
// MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(speech_isolated_word_add_voice_model_obj, 5, 5, speech_isolated_word_add_voice_model);

// STATIC mp_obj_t speech_isolated_word_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
// {
//     return speech_isolated_word_init_helper(args[0], n_args - 1, args + 1, kw_args);
// }
// MP_DEFINE_CONST_FUN_OBJ_KW(speech_isolated_word_init_obj, 1, speech_isolated_word_init);

// STATIC mp_obj_t speech_isolated_word_get_result(mp_obj_t self_in)
// {
//     int res = 0;
//     return mp_obj_new_int(res);
// }
// MP_DEFINE_CONST_FUN_OBJ_KW(speech_isolated_word_get_result_obj, 1, speech_isolated_word_get_result);

// STATIC mp_obj_t speech_isolated_word_finish(mp_obj_t self_in)
// {
//     return mp_const_true;
// }
// MP_DEFINE_CONST_FUN_OBJ_KW(speech_isolated_word_finish_obj, 1, speech_isolated_word_finish);

// STATIC mp_obj_t speech_isolated_word_recognize(mp_obj_t self_in)
// {
//     // mp_printf(&mp_plat_print, "[MaixPy] recognize...\r\n");
//     isolated_word_obj_t *self = MP_OBJ_TO_PTR(self_in);
//     mp_int_t rev_val = 0;
//     return mp_obj_new_int(rev_val);
// }
// MP_DEFINE_CONST_FUN_OBJ_1(speech_isolated_word_recognize_obj, speech_isolated_word_recognize);


// STATIC mp_obj_t speech_isolated_word_get_status(mp_obj_t self_in)
// {
//     mp_int_t status_val = 0;
//     return mp_obj_new_int(status_val);
// }
// MP_DEFINE_CONST_FUN_OBJ_KW(speech_isolated_word_get_status_obj, 1, speech_isolated_word_get_status);




// STATIC mp_obj_t speech_isolated_word_set_threshold(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
// {
//     isolated_word_obj_t *self = MP_OBJ_TO_PTR(args[0]);
//     mp_int_t n_thl = mp_obj_get_int(args[1]);
//     mp_int_t z_thl = mp_obj_get_int(args[2]);
//     mp_int_t s_thl = mp_obj_get_int(args[3]);

//     // isolated_word_set_Threshold(n_thl, z_thl, s_thl);
//     return mp_const_none;
// }
// MP_DEFINE_CONST_FUN_OBJ_KW(speech_isolated_word_set_threshold_obj, 3, speech_isolated_word_set_threshold);
