
#include <stdio.h>
#include <string.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "py/objtype.h"
#include "py/objstr.h"
#include "py/objint.h"

#include "fpioa.h"

typedef struct _machine_fpioa_obj_t {
    mp_obj_base_t base;
} machine_fpioa_obj_t;

const mp_obj_type_t machine_fpioa_type;

STATIC mp_obj_t machine_set_function(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
		enum {
			ARG_pin,
			ARG_func,
		};
		static const mp_arg_t allowed_args[] = {
			{ MP_QSTR_pin, 	MP_ARG_INT, {.u_int = 0} },
			{ MP_QSTR_func,	 MP_ARG_INT, {.u_int = 0} },
		};
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
	uint16_t pin_num = args[ARG_pin].u_int;
	fpioa_function_t func_num = args[ARG_func].u_int;
	fpioa_set_function(pin_num,(fpioa_function_t)func_num);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_set_function_obj, 0,machine_set_function);

STATIC mp_obj_t machine_fpioa_make_new() {
    
    machine_fpioa_obj_t *self = m_new_obj(machine_fpioa_obj_t);
    self->base.type = &machine_fpioa_type;

    return self;
}

STATIC const mp_rom_map_elem_t pyb_fpioa_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_set_function), MP_ROM_PTR(&machine_set_function_obj) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_fpioa_locals_dict, pyb_fpioa_locals_dict_table);

const mp_obj_type_t machine_fpioa_type = {
    { &mp_type_type },
    .name = MP_QSTR_Fpioa,
    .make_new = machine_fpioa_make_new,
    .locals_dict = (mp_obj_dict_t*)&pyb_fpioa_locals_dict,
};

