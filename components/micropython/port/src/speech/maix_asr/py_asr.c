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

#include "asr.h"
#include "preprocess.h"
#include "asr_decode.h"


#include "printf.h"

// extern const mp_obj_type_t Maix_i2s_type;
const mp_obj_type_t speech_asr_type;

typedef struct _asr_obj_t
{
    mp_obj_base_t base;

    uint32_t address;
    i2s_device_number_t device_num;
    dmac_channel_number_t channel_num;
    pnyp_t pnyp_history[PNY_LST_LEN][BEAM_CNT];
	pnyp_t pnyp_list[BEAM_CNT*T_CORE];	//记录T_CORE格的拼音解析结果
	volatile int asr_flag;
    int lr_shift;
    int asr_res_cnt;
    int asr_kw_sum;
    asr_res_t* asr_res;
    asr_kw_t *asr_kw_tbl; // 关键词动态表
} asr_obj_t;

STATIC mp_obj_t speech_asr_init_helper(asr_obj_t *self_in, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    asr_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    //parse paremeter
    enum
    {
        ARG_address,
        ARG_i2s,
        ARG_dmac,
        ARG_shift,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_address, MP_ARG_INT, {.u_int = 0x500000}},
        { MP_QSTR_i2s, MP_ARG_INT, {.u_int = I2S_DEVICE_0}},
        { MP_QSTR_dmac, MP_ARG_INT, {.u_int = DMAC_CHANNEL3}},
        { MP_QSTR_shift, MP_ARG_INT, {.u_int = 0}},
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    self->address = args[ARG_address].u_int;
    self->device_num = args[ARG_i2s].u_int;
    self->channel_num = args[ARG_dmac].u_int;
    self->lr_shift = args[ARG_shift].u_int;
    
	pp_init(self->device_num, self->channel_num, self->lr_shift);
	asr_init(self->address);

    self->asr_kw_tbl = NULL;
    asr_reg_kw(NULL, 0);

    return mp_const_none;
}

STATIC mp_obj_t speech_asr_state(mp_obj_t self_in) {
    asr_obj_t *self = MP_OBJ_TO_PTR(self_in);

	int t_cnt = 0;
	uint8_t* mel_data = pp_loop();				// DBG_TIME();	//阻塞等待，获取一帧语音，预处理成mel fbank
					
	if(mel_data == NULL) {
		t_cnt = 0;	//当前时刻不进行处理
		// msleep(100); // micropython 10fps
		// printk("heap:%ld KB\r\n", get_free_heap_size() / 1024);
	} else {
		asr_process(mel_data, &t_cnt, self->pnyp_list); // DBG_TIME();	//声学模型解码出T_CORE格的拼音预测
		// dump_pny(pnyp_list, t_cnt, loop_i);
        memmove(self->pnyp_history, &self->pnyp_history[t_cnt], (PNY_LST_LEN-t_cnt)*BEAM_CNT*sizeof(pnyp_t));
        memcpy(&self->pnyp_history[PNY_LST_LEN-t_cnt], self->pnyp_list, t_cnt*BEAM_CNT*sizeof(pnyp_t));
		// push_pny(pnyp_list, t_cnt);
		decode_ctc_pny_similar((pnyp_t*)self->pnyp_history, &self->asr_res, &self->asr_res_cnt);
		// decode_digit(pnyp_list, t_cnt, &_digit_res, &_orignal_res);	//数字解码使用独立的更长的历史buf
		self->asr_flag = 1;
	}

    return self->asr_flag ? mp_const_true : mp_const_false;
}
MP_DEFINE_CONST_FUN_OBJ_1(speech_asr_state_obj, speech_asr_state);

STATIC mp_obj_t speech_asr_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    mp_arg_check_num(n_args, n_kw, 0, 4, true);

    asr_obj_t *self = m_new_obj_with_finaliser(asr_obj_t);
    self->base.type = &speech_asr_type;

    // init instance
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    if (mp_const_false == speech_asr_init_helper(self, n_args, args, &kw_args))
        return mp_const_false;

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t speech_asr_make_del(mp_obj_t self_in){
    if(mp_obj_get_type(self_in) == &speech_asr_type) {
        asr_obj_t *self = MP_OBJ_TO_PTR(self_in);
        pp_deinit();
        asr_deinit();
        mp_printf(&mp_plat_print, "%s __del__\r\n", __func__);
        if (self->asr_kw_tbl != NULL) free(self->asr_kw_tbl), self->asr_kw_tbl = NULL;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(speech_asr_del_obj, speech_asr_make_del);

STATIC void speech_asr_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    asr_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "[MAIXPY] asr:(%p)\r\n address=%d\r\n i2s_device_number_t=%d\r\n dmac_channel_number_t=%d\r\n", self, self->address, self->device_num, self->channel_num);
}

STATIC mp_obj_t speech_asr_set(mp_obj_t self_in, mp_obj_t tuple_in)
{
    // static asr_kw_t my_asr_kw_tbl[] = {
    //     {{ASR_DICT_XING  ,ASR_DICT_SHI   ,0xffff         ,0xffff ,0xffff ,0xffff} ,2, 0.2},
    //     {{ASR_DICT_QING  ,ASR_DICT_ZHUAN ,ASR_DICT_ZHANG ,0xffff ,0xffff ,0xffff} ,3, 0.2},
    //     {{ASR_DICT_NI    ,ASR_DICT_HAO   ,0xffff         ,0xffff ,0xffff ,0xffff} ,2, 0.2},
    //     {{ASR_DICT_BI    ,ASR_DICT_YUAN  ,0xffff         ,0xffff ,0xffff ,0xffff} ,2, 0.2},
    //     {{ASR_DICT_BI    ,ASR_DICT_YUAN  ,ASR_DICT_LIAN  ,0xffff ,0xffff ,0xffff} ,3, 0.2},
    //     {{ASR_DICT_BI    ,ASR_DICT_TE    ,ASR_DICT_BI    ,0xffff ,0xffff ,0xffff} ,3, 0.2},
    //     {{ASR_DICT_BI    ,ASR_DICT_TI    ,ASR_DICT_SUI   ,0xffff ,0xffff ,0xffff} ,3, 0.2},
    //     {{ASR_DICT_YI    ,ASR_DICT_TAI   ,ASR_DICT_FANG  ,0xffff ,0xffff ,0xffff} ,3, 0.2},
    //     {{ASR_DICT_YI    ,ASR_DICT_TI    ,ASR_DICT_QU    ,0xffff ,0xffff ,0xffff} ,3, 0.2},
    //     {{ASR_DICT_QUE   ,ASR_DICT_REN   ,0xffff         ,0xffff ,0xffff ,0xffff} ,2, 0.2},
    //     {{ASR_DICT_HAO   ,ASR_DICT_DE    ,0xffff         ,0xffff ,0xffff ,0xffff} ,2, 0.2},
    //     {{ASR_DICT_KE    ,ASR_DICT_YI    ,0xffff         ,0xffff ,0xffff ,0xffff} ,2, 0.2},
    //     {{ASR_DICT_QU    ,ASR_DICT_XIAO  ,0xffff         ,0xffff ,0xffff ,0xffff} ,2, 0.2},
    //     {{ASR_DICT_BU    ,ASR_DICT_YONG  ,0xffff         ,0xffff ,0xffff ,0xffff} ,2, 0.2},
    //     {{ASR_DICT_BU    ,ASR_DICT_XU    ,ASR_DICT_YAO   ,0xffff ,0xffff ,0xffff} ,3, 0.2},
    //     {{ASR_DICT_HAI   ,ASR_DICT_HOU   ,ASR_DICT_DE    ,0xffff ,0xffff ,0xffff} ,3, 0.2},
    //     {{ASR_DICT_YI    ,ASR_DICT_MOU   ,0xffff		 ,0xffff ,0xffff ,0xffff} ,2, 0.2},
    //     {{ASR_DICT_HEI   ,ASR_DICT_YI    ,ASR_DICT_MOU   ,0xffff ,0xffff ,0xffff} ,3, 0.2},
    //     {{ASR_DICT_A     ,ASR_DICT_LAI   ,ASR_DICT_KE    ,ASR_DICT_SA,0xffff ,0xffff } ,4, 0.2},
    //     {{ASR_DICT_XIAO  ,ASR_DICT_AI    ,0xffff  ,0xffff,0xffff ,0xffff} ,2, 0.2},
    // };
	// asr_reg_kw(my_asr_kw_tbl, sizeof(my_asr_kw_tbl)/sizeof(asr_kw_t));

    asr_obj_t *self = MP_OBJ_TO_PTR(self_in);

    size_t len;
    mp_obj_t *items;

    mp_obj_get_array(tuple_in, &len, &items);
    
    if (len == 0) {
        mp_raise_ValueError("tuple_in len != 0");
    }

    if (self->asr_kw_tbl != NULL) free(self->asr_kw_tbl), self->asr_kw_tbl = NULL;

    self->asr_kw_tbl = malloc(len*sizeof(asr_kw_t));

    if (self->asr_kw_tbl == NULL) {
        mp_raise_msg(&mp_type_MemoryError, "memory allocation failed");
    }

    self->asr_kw_sum = len;
    
    for (size_t i = 0; i < len; ++i) {
            
        mp_obj_tuple_t *kw_in = MP_OBJ_TO_PTR(items[i]);

        size_t t_len;
        mp_obj_t *t_items;

        mp_obj_get_array(kw_in, &t_len, &t_items);
        if (len == 2) {
            mp_raise_ValueError("kw_in == (gate, get_asr_list('xx-xx'))");
        }

        float gate = mp_obj_get_float(MP_OBJ_TO_PTR(t_items[0]));

        // char str[32];
        // sprintf(str, "%.3f", gate);
        // printk("kw_in gate %s\r\n", str);

        size_t tt_len;
        mp_obj_t *tt_items;

        mp_obj_get_array(MP_OBJ_TO_PTR(t_items[1]), &tt_len, &tt_items);
        if (len > KW_MAX_PNY) {
            mp_raise_ValueError("kw_in(1) len > KW_MAX_PNY");
        }
        
        asr_kw_t *tmp = &self->asr_kw_tbl[i];
        for (size_t tt = 0; tt < tt_len; ++tt) {
            // printk("tt_items[%d] %d\r\n", tt, mp_obj_get_int(tt_items[tt]));
            tmp->pny[tt] = mp_obj_get_int(tt_items[tt]);
        }
        for (int t = tt_len; t < KW_MAX_PNY; t++) tmp->pny[t] = 0xffff;
        tmp->pny_cnt = tt_len;
        tmp->gate = gate;

        // printk("tmp->pny_cnt %d\r\n", tmp->pny_cnt);
    }

    asr_reg_kw(self->asr_kw_tbl, self->asr_kw_sum);
    
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(speech_asr_set_obj, speech_asr_set);

STATIC mp_obj_t speech_asr_get(mp_obj_t self_in) {
    asr_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->asr_kw_tbl != NULL) {
        mp_obj_t result = mp_obj_new_list(0, NULL);
    
        for(int p = 0; p < self->asr_kw_sum; p++ ) {
            mp_obj_t list = mp_obj_new_list(0, NULL);
            for (int i = 0, max = self->asr_kw_tbl[p].pny_cnt; i < max; i++) {
                mp_obj_list_append(list, mp_obj_new_int(self->asr_kw_tbl[p].pny[i]));
            }
            mp_obj_t tuple[2] = {
                mp_obj_new_float(self->asr_kw_tbl[p].gate),
                list,
            };
            mp_obj_list_append(result, mp_obj_new_tuple(2, tuple));
        }
        return result;

    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(speech_asr_get_obj, speech_asr_get);

STATIC mp_obj_t speech_asr_result(mp_obj_t self_in) {
    asr_obj_t *self = MP_OBJ_TO_PTR(self_in);
	if(self->asr_flag == 1) {
		self->asr_flag =0;
        if (self->asr_res_cnt) {
            // for(int i = 0; i < self->asr_res_cnt; i++ ) {
            //     char str[32];
            //     sprintf(str, "%d %.3f", self->asr_res[i].kw_idx, self->asr_res[i].p);
            //     printk("###### KWS %6s\r\n", str);
            // }
            
            mp_obj_t result = mp_obj_new_list(0, NULL);
        
            for(int i = 0; i < self->asr_res_cnt; i++ ) {
                int kw_idx = self->asr_res[i].kw_idx;
                mp_obj_t list = mp_obj_new_list(0, NULL);
                for (int i = 0, max = self->asr_kw_tbl[kw_idx].pny_cnt; i < max; i++) {
                    mp_obj_list_append(list, mp_obj_new_int(self->asr_kw_tbl[kw_idx].pny[i]));
                }
                mp_obj_t tuple[2] = {
                    mp_obj_new_float(self->asr_res[i].p),
                    list,
                };
                mp_obj_list_append(result, mp_obj_new_tuple(2, tuple));
            }

            return result;
        }
	}
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(speech_asr_result_obj, speech_asr_result);

STATIC mp_obj_t speech_asr_run(mp_obj_t self_in) {
    pp_start();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(speech_asr_run_obj, speech_asr_run);

STATIC mp_obj_t speech_asr_stop(mp_obj_t self_in) {
    pp_stop();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(speech_asr_stop_obj, speech_asr_stop);

STATIC const mp_rom_map_elem_t mp_module_asr_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),        MP_OBJ_NEW_QSTR(MP_QSTR_maix_asr) },

    { MP_ROM_QSTR(MP_QSTR_set), MP_ROM_PTR(&speech_asr_set_obj)},
    { MP_ROM_QSTR(MP_QSTR_get), MP_ROM_PTR(&speech_asr_get_obj)},
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&speech_asr_del_obj)},

    { MP_ROM_QSTR(MP_QSTR_run), MP_ROM_PTR(&speech_asr_run_obj)},
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&speech_asr_stop_obj)},
    { MP_ROM_QSTR(MP_QSTR_state), MP_ROM_PTR(&speech_asr_state_obj)},
    { MP_ROM_QSTR(MP_QSTR_result), MP_ROM_PTR(&speech_asr_result_obj)},
};

MP_DEFINE_CONST_DICT(mp_module_asr_locals_dict, mp_module_asr_locals_dict_table);

const mp_obj_type_t speech_asr_type = {
    .base = {&mp_type_type},
    .name = MP_QSTR_asr,
    .print = speech_asr_print,
    .make_new = speech_asr_make_new,
    .locals_dict = (mp_obj_dict_t *)&mp_module_asr_locals_dict,
};
