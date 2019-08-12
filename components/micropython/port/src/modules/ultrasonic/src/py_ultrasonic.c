#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "py/mpconfig.h"
#include "ultrasonic.h"
#include "fpioa.h"

#if MAIXPY_PY_MODULES_ULTRASONIC

#define ULTRASONIC_UNIT_CM   0
#define ULTRASONIC_UNIT_INCH 1

const mp_obj_type_t modules_ultrasonic_type;

typedef struct {
    mp_obj_base_t         base;
    uint8_t               gpio;
} modules_ultrasonic_obj_t;

mp_obj_t modules_ultrasonic_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

    modules_ultrasonic_obj_t *self = m_new_obj(modules_ultrasonic_obj_t);
    self->base.type = &modules_ultrasonic_type;
    enum {
        ARG_gpio
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_gpio,        MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = 0} }
    };
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args , all_args, &kw_args,
        MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
    if(  (args[ARG_gpio].u_int >= FUNC_GPIO0) && (args[ARG_gpio].u_int <= FUNC_GPIO7) )
    {
        self->gpio = args[ARG_gpio].u_int;
        return MP_OBJ_FROM_PTR(self);
    }
    else if( (args[ARG_gpio].u_int >= FUNC_GPIOHS0) && (args[ARG_gpio].u_int <= FUNC_GPIOHS31) )
    {
        self->gpio = args[ARG_gpio].u_int;
        return MP_OBJ_FROM_PTR(self);
    }
    else
    {
        mp_raise_ValueError("gpio error");
    }
}


STATIC void modules_ultrasonic_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    modules_ultrasonic_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "[MAIXPY]ultrasonic:(%p) gpio=%d\r\n", self, self->gpio);
}

STATIC mp_obj_t modules_ultrasonic_measure(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum {
        ARG_unit,
        ARG_timeout,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_unit,        MP_ARG_INT, {.u_int = ULTRASONIC_UNIT_CM} },
        { MP_QSTR_timeout,     MP_ARG_INT, {.u_int = 1000000} }
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    long ret;
    modules_ultrasonic_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
        MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    if(args[ARG_unit].u_int == ULTRASONIC_UNIT_CM)
    {
        ret = ultrasonic_measure_cm(self->gpio, args[ARG_timeout].u_int);
    }
    else
    {
        ret = ultrasonic_measure_inch(self->gpio, args[ARG_timeout].u_int);
    }
    if( ret == -1)
    {
        mp_raise_ValueError("gpio error");
    }
    if( ret == -2)
    {
        mp_raise_ValueError("gpio not register pin");
    }
    if( ret == 0)
        mp_raise_msg(&mp_type_OSError, "time out");
    return mp_obj_new_int(ret);
}
MP_DEFINE_CONST_FUN_OBJ_KW(modules_ultrasonic_measure_obj, 1, modules_ultrasonic_measure);

STATIC const mp_rom_map_elem_t mp_modules_ultrasonic_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_measure), MP_ROM_PTR(&modules_ultrasonic_measure_obj) },
    { MP_ROM_QSTR(MP_QSTR_UNIT_CM),   MP_ROM_INT(ULTRASONIC_UNIT_CM) },
    { MP_ROM_QSTR(MP_QSTR_UNIT_INCH),   MP_ROM_INT(ULTRASONIC_UNIT_INCH) },
};

MP_DEFINE_CONST_DICT(mp_modules_ultrasonic_locals_dict, mp_modules_ultrasonic_locals_dict_table);

const mp_obj_type_t modules_ultrasonic_type = {
    { &mp_type_type },
    .name = MP_QSTR_ultrasonic,
    .print = modules_ultrasonic_print,
    .make_new = modules_ultrasonic_make_new,
    .locals_dict = (mp_obj_dict_t*)&mp_modules_ultrasonic_locals_dict,
};

#endif // MAIXPY_PY_MODULES_ULTRASONIC
