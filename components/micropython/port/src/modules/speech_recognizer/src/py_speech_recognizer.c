#include <stdio.h>
#include <string.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "py/mpconfig.h"

#include "i2s.h"
#include "Maix_i2s.h"

#include "speech_recognizer.h"

extern const mp_obj_type_t Maix_i2s_type;
const mp_obj_type_t modules_speech_recognizer_type;

// 自定义结构体类型 speech_recognizer_obj_t
typedef struct _speech_recognizer_obj_t {
    mp_obj_base_t base; // 定义的结构体对象需要包含改成员
    Maix_i2s_obj_t* dev;
    uint32_t key_word_num;
    uint32_t model_num;
} speech_recognizer_obj_t;


STATIC mp_obj_t modules_speech_recognizer_record(size_t n_args, const mp_obj_t *args)
{
    mp_printf(&mp_plat_print, "modules_speech_recognizer_record\r\n");
  
    mp_int_t key_word_num = mp_obj_get_int(args[1]);
    mp_int_t model_num = mp_obj_get_int(args[2]);
    if(key_word_num >10 )
    {
        mp_printf(&mp_plat_print, "[MAIXPY]:  key_word_num>10\n");
        return mp_const_false;
    }
    if(model_num > 4)
    {
        mp_printf(&mp_plat_print, "[MAIXPY]:  key_word_num>4\n");
        return mp_const_false;
    }
    mp_printf(&mp_plat_print, "[MAIXPY]:  record[%d:%d]\n", key_word_num, key_word_num);
    speech_recognizer_record(key_word_num, key_word_num);
    return mp_const_true;
}
STATIC mp_obj_t modules_speech_recognizer_print_model(size_t n_args, const mp_obj_t *args)
{
    mp_printf(&mp_plat_print, "modules_speech_recognizer_record\r\n");
  
    mp_int_t key_word_num = mp_obj_get_int(args[1]);
    mp_int_t model_num = mp_obj_get_int(args[2]);
    if(key_word_num >10 )
    {
        mp_printf(&mp_plat_print, "[MAIXPY]:  key_word_num>10\n");
        return mp_const_false;
    }
    if(model_num > 4)
    {
        mp_printf(&mp_plat_print, "[MAIXPY]:  key_word_num>4\n");
        return mp_const_false;
    }
    mp_printf(&mp_plat_print, "[MAIXPY]:  print_model[%d:%d]\n", key_word_num, key_word_num);
    speech_recognizer_print_model(key_word_num, key_word_num);
    return mp_const_true;
}

STATIC mp_obj_t modules_speech_recognizer_init_helper(speech_recognizer_obj_t *self_in, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    //parse paremeter
    enum {ARG_dev,};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_dev, MP_ARG_OBJ , {.u_obj = mp_const_none} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
    speech_recognizer_obj_t* self = MP_OBJ_TO_PTR(self_in);

    if(args[ARG_dev].u_obj != mp_const_none)
    {
        mp_obj_t I2S_dev = args[ARG_dev].u_obj;
        if(&Maix_i2s_type != mp_obj_get_type(I2S_dev))
            mp_raise_ValueError("Invaild I2S device");
        Maix_i2s_obj_t* i2s_dev = MP_OBJ_TO_PTR(I2S_dev);
        self->dev = i2s_dev;
        mp_printf(&mp_plat_print, "[MAIXPY]:  i2s_num:%d\r\n",self->dev->i2s_num );
        speech_recognizer_init(self->dev->i2s_num);
    }
    else
    {
        mp_raise_ValueError("init speech recognizer error\r\n");
        return mp_const_false;
    }
    return mp_const_true;
}

STATIC mp_obj_t modules_speech_recognizer_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return modules_speech_recognizer_init_helper(args[0], n_args -1 , args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(modules_speech_recognizer_init_obj, 1 ,modules_speech_recognizer_init);


STATIC mp_obj_t modules_speech_recognizer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    mp_arg_check_num(n_args, n_kw, 1, 1, true); // 检查参数个数

    speech_recognizer_obj_t *self = m_new_obj_with_finaliser(speech_recognizer_obj_t);   // 创建对象, 分配空间
    self->base.type = &modules_speech_recognizer_type;       // 定义对象类型
    
    // init instance
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    if(mp_const_false == modules_speech_recognizer_init_helper(self, n_args, args, &kw_args))
        return mp_const_false;

    return MP_OBJ_FROM_PTR(self);   // 返回对象的指针
}

STATIC mp_obj_t modules_speech_recognizer_recognize(mp_obj_t self_in)
{
    mp_printf(&mp_plat_print, "[MaixPy] speech_recognizer_recognize\r\n");
    speech_recognizer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t rev_val = speech_recognizer_recognize();
    return mp_obj_new_int(rev_val);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modules_speech_recognizer_record_obj, 3, 3, modules_speech_recognizer_record);
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modules_speech_recognizer_print_model_obj, 3, 3, modules_speech_recognizer_print_model);
MP_DEFINE_CONST_FUN_OBJ_1(modules_speech_recognizer_recognize_obj, modules_speech_recognizer_recognize);

STATIC const mp_rom_map_elem_t mp_module_speech_recognizer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&modules_speech_recognizer_init_obj) },
    {MP_ROM_QSTR(MP_QSTR_record), MP_ROM_PTR(&modules_speech_recognizer_record_obj)},
    {MP_ROM_QSTR(MP_QSTR_print_model), MP_ROM_PTR(&modules_speech_recognizer_print_model_obj)},
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