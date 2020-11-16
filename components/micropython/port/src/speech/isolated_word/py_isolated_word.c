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
#define mfcc_dats_mask 12345
// #include "voice_model.h"

#include "printf.h"

// extern const mp_obj_type_t Maix_i2s_type;
const mp_obj_type_t speech_isolated_word_type;

typedef struct _isolated_word_obj_t
{
    mp_obj_base_t base;

    uint32_t min_dis;
    int16_t min_comm;
    uint16_t min_frm;
    uint16_t cur_frm;
    uint16_t size:14;
    uint16_t shift:2;
    i2s_device_number_t device_num;
    dmac_channel_number_t channel_num;
    v_ftr_tag *mfcc_dats;
} isolated_word_obj_t;

void speech_set_word(v_ftr_tag *mfcc_dats, uint8_t model_num, const int16_t *voice_model, uint16_t frame_num)
{
    mfcc_dats[model_num].save_sign = mfcc_dats_mask;
    mfcc_dats[model_num].frm_num = frame_num;
    memcpy(mfcc_dats[model_num].mfcc_dat, voice_model, sizeof(mfcc_dats[model_num].mfcc_dat));
}

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
        ARG_shift,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_size, MP_ARG_INT, {.u_int = 10}},
        { MP_QSTR_i2s, MP_ARG_INT, {.u_int = I2S_DEVICE_0}},
        { MP_QSTR_dmac, MP_ARG_INT, {.u_int = DMAC_CHANNEL2}},
        { MP_QSTR_priority, MP_ARG_INT, {.u_int = 3}},
        { MP_QSTR_shift, MP_ARG_INT, {.u_int = 0}},
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
    self->shift = args[ARG_shift].u_int;
    iw_run(self->device_num, self->channel_num, self->shift, self->size);

    // speech_set_word(self->mfcc_dats, 0, hey_friday_0, fram_num_hey_friday_0);
    // speech_set_word(self->mfcc_dats, 1, hey_friday_1, fram_num_hey_friday_1);
    // speech_set_word(self->mfcc_dats, 2, hey_friday_2, fram_num_hey_friday_2);
    // speech_set_word(self->mfcc_dats, 3, hey_friday_3, fram_num_hey_friday_3);
    // speech_set_word(self->mfcc_dats, 4, hey_jarvis_0, fram_num_hey_jarvis_0);
    // speech_set_word(self->mfcc_dats, 5, hey_jarvis_1, fram_num_hey_jarvis_1);
    // speech_set_word(self->mfcc_dats, 6, hey_jarvis_2, fram_num_hey_jarvis_2);
    // speech_set_word(self->mfcc_dats, 7, hey_jarvis_3, fram_num_hey_jarvis_3);

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
            iw_stop();
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

STATIC mp_obj_t speech_isolated_word_set(mp_obj_t self_in, mp_obj_t pos_in, mp_obj_t tuple_in)
{
    isolated_word_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int pos = mp_obj_get_int(pos_in);

    size_t len;
    mp_obj_t *elem;

    mp_obj_get_array(tuple_in, &len, &elem);

    if (len != 2) {
        mp_raise_ValueError("isolated_word error, is not (frm_num, mfcc_dat[vv_frm_max*mfcc_num*sizeof(uint16_t)])");
    }
    
    int frm_num = mp_obj_get_int(elem[0]);

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(elem[1], &bufinfo, MP_BUFFER_READ);

    if (bufinfo.len != vv_frm_max*mfcc_num*sizeof(uint16_t)) {
        mp_raise_ValueError("bufinfo.len != vv_frm_max*mfcc_num");
    }
    
    speech_set_word(self->mfcc_dats, pos, bufinfo.buf, frm_num);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(speech_isolated_word_set_obj, speech_isolated_word_set);

STATIC mp_obj_t speech_isolated_word_get(mp_obj_t self_in, mp_obj_t pos_in) {
    isolated_word_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int pos = mp_obj_get_int(pos_in);
    if (pos < self->size && self->mfcc_dats[pos].save_sign == mfcc_dats_mask) {
        mp_obj_t tuple[2] = {
            mp_obj_new_int(self->mfcc_dats[pos].frm_num),
            mp_obj_new_bytes(self->mfcc_dats[pos].mfcc_dat, sizeof(self->mfcc_dats[pos].mfcc_dat)),
        };
        return mp_obj_new_tuple(2, tuple);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(speech_isolated_word_get_obj, speech_isolated_word_get);

STATIC mp_obj_t speech_isolated_word_result(mp_obj_t self_in) {
    isolated_word_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->min_comm != -1) {
        mp_obj_t tuple[4] = {
            mp_obj_new_int(self->min_comm),
            mp_obj_new_int(self->min_dis),
            mp_obj_new_int(self->cur_frm),
            mp_obj_new_int(self->min_frm),
        };
        return mp_obj_new_tuple(4, tuple);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(speech_isolated_word_result_obj, speech_isolated_word_result);

STATIC mp_obj_t speech_isolated_word_reset(mp_obj_t self_in) {
    iw_set_state(Idle);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(speech_isolated_word_reset_obj, speech_isolated_word_reset);

STATIC mp_obj_t speech_isolated_word_run(mp_obj_t self_in) {
    isolated_word_obj_t *self = MP_OBJ_TO_PTR(self_in);
    iw_run(self->device_num, self->channel_num, self->shift, self->size);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(speech_isolated_word_run_obj, speech_isolated_word_run);

STATIC mp_obj_t speech_isolated_word_stop(mp_obj_t self_in) {
    iw_stop();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(speech_isolated_word_stop_obj, speech_isolated_word_stop);

STATIC mp_obj_t speech_isolated_word_dtw(mp_obj_t self_in, mp_obj_t tuple_in)
{
    isolated_word_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t result = 0;
    if (iw_get_state() == Done)
    {
        size_t len;
        mp_obj_t *elem;

        mp_obj_get_array(tuple_in, &len, &elem);

        if (len != 2) {
            mp_raise_ValueError("isolated_word error, is not (frm_num, mfcc_dat[vv_frm_max*mfcc_num])");
        }
        
        int frm_num = mp_obj_get_int(elem[0]);

        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(elem[1], &bufinfo, MP_BUFFER_READ);
        if (bufinfo.len != vv_frm_max*mfcc_num*sizeof(uint16_t)) {
            mp_raise_ValueError("bufinfo.len != vv_frm_max*mfcc_num");
        }
        
        v_ftr_tag ftr_mdl;
        ftr_mdl.frm_num = frm_num;
        memcpy(ftr_mdl.mfcc_dat, bufinfo.buf, sizeof(ftr_mdl.mfcc_dat));
        uint32_t tmp = dtw(&ftr_mdl, &ftr_curr);
        if (tmp != -1) {
            result = tmp;
        }
    }
    return mp_obj_new_int(result);
}
MP_DEFINE_CONST_FUN_OBJ_2(speech_isolated_word_dtw_obj, speech_isolated_word_dtw);

STATIC mp_obj_t speech_isolated_word_recognize(mp_obj_t self_in) {
    isolated_word_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (iw_get_state() == Done)
    {
        self->min_comm = -1;
        self->min_dis = dis_max;
        // uint32_t cycle0 = read_csr(mcycle);
        for (uint32_t ftr_num = 0; ftr_num < self->size; ftr_num += 1)
        {
            //  ftr_mdl=(v_ftr_tag*)ftr_num;
            v_ftr_tag *ftr_mdl = (v_ftr_tag *)(&self->mfcc_dats[ftr_num]);
            if ((ftr_mdl->save_sign) == mfcc_dats_mask)
            {
                printk("NO. %d, ftr_mdl->frm_num %d, ", ftr_num, ftr_mdl->frm_num);
                
                uint32_t cur_dis = dtw(ftr_mdl, &ftr_curr);
                printk("cur_dis %d, ftr_curr.frm_num %d\r\n", cur_dis, ftr_curr.frm_num);
            
                if (cur_dis < self->min_dis)
                {
                    self->min_comm = ftr_num;
                    self->min_dis = cur_dis;
                    self->cur_frm = ftr_curr.frm_num;
                    self->min_frm = ftr_mdl->frm_num;
                    printk("min_comm: %d \r\n", self->min_comm);
                }
            }
        }
        // uint32_t cycle1 = read_csr(mcycle) - cycle0;
        // printk("[INFO] recg cycle = 0x%08x\n", cycle1);
        //printk("recg end ");
        // printk("min_comm: %d >\r\n", self->min_comm);
        iw_set_state(Idle);
        return mp_obj_new_int(Done);
    }
    return mp_obj_new_int(iw_get_state());
}
MP_DEFINE_CONST_FUN_OBJ_1(speech_isolated_word_recognize_obj, speech_isolated_word_recognize);

STATIC mp_obj_t speech_isolated_word_record(mp_obj_t self_in, mp_obj_t pos_in) {
    isolated_word_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int pos = mp_obj_get_int(pos_in);
    if (iw_get_state() == Done)
    {
        v_ftr_tag *tmp = iw_get_ftr();
        if (pos < self->size)
        {
            speech_set_word(self->mfcc_dats, pos, tmp->mfcc_dat, tmp->frm_num);
            iw_set_state(Idle);
            return mp_obj_new_int(Done);
        }
    }
    return mp_obj_new_int(iw_get_state());
}
MP_DEFINE_CONST_FUN_OBJ_2(speech_isolated_word_record_obj, speech_isolated_word_record);

STATIC const mp_rom_map_elem_t mp_module_isolated_word_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),        MP_OBJ_NEW_QSTR(MP_QSTR_isolated_word) },

    { MP_ROM_QSTR(MP_QSTR_set_threshold), MP_ROM_PTR(&speech_isolated_word_set_threshold_obj)},
    { MP_ROM_QSTR(MP_QSTR_size), MP_ROM_PTR(&speech_isolated_word_size_obj)},
    { MP_ROM_QSTR(MP_QSTR_set), MP_ROM_PTR(&speech_isolated_word_set_obj)},
    { MP_ROM_QSTR(MP_QSTR_get), MP_ROM_PTR(&speech_isolated_word_get_obj)},
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&speech_isolated_word_del_obj)},

    { MP_ROM_QSTR(MP_QSTR_run), MP_ROM_PTR(&speech_isolated_word_run_obj)},
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&speech_isolated_word_stop_obj)},
    { MP_ROM_QSTR(MP_QSTR_dtw), MP_ROM_PTR(&speech_isolated_word_dtw_obj)},
    { MP_ROM_QSTR(MP_QSTR_record), MP_ROM_PTR(&speech_isolated_word_record_obj)},
    { MP_ROM_QSTR(MP_QSTR_recognize), MP_ROM_PTR(&speech_isolated_word_recognize_obj)},
    { MP_ROM_QSTR(MP_QSTR_state), MP_ROM_PTR(&speech_isolated_word_state_obj)},
    { MP_ROM_QSTR(MP_QSTR_result), MP_ROM_PTR(&speech_isolated_word_result_obj)},
    { MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&speech_isolated_word_reset_obj)},

    { MP_ROM_QSTR(MP_QSTR_Init), MP_ROM_INT(Init) },
    { MP_ROM_QSTR(MP_QSTR_Idle), MP_ROM_INT(Idle) },
    { MP_ROM_QSTR(MP_QSTR_Ready), MP_ROM_INT(Ready) },
    { MP_ROM_QSTR(MP_QSTR_MaybeNoise), MP_ROM_INT(MaybeNoise) },
    { MP_ROM_QSTR(MP_QSTR_Restrain), MP_ROM_INT(Restrain) },
    { MP_ROM_QSTR(MP_QSTR_Speak), MP_ROM_INT(Speak) },
    { MP_ROM_QSTR(MP_QSTR_Done), MP_ROM_INT(Done) },
};

MP_DEFINE_CONST_DICT(mp_module_isolated_word_locals_dict, mp_module_isolated_word_locals_dict_table);

const mp_obj_type_t speech_isolated_word_type = {
    .base = {&mp_type_type},
    .name = MP_QSTR_isolated_word,
    .print = speech_isolated_word_print,
    .make_new = speech_isolated_word_make_new,
    .locals_dict = (mp_obj_dict_t *)&mp_module_isolated_word_locals_dict,
};
