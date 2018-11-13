/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "py/obj.h"

#include "py/runtime.h"
#include "py/mphal.h"
#include "modmachine.h"

#define gpio_num_t int
typedef struct _machine_devmem_obj_t {
    mp_obj_base_t base;
} machine_devmem_obj_t;

#define GPIO_MAX_PINNO 8

// pin.init(mode, pull=None, *, value)
STATIC mp_obj_t machine_devmem_obj_init_helper(const machine_devmem_obj_t *self,size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    long long int *addr = NULL;
	long long int value = 0;
	enum { ARG_addr, ARG_width, ARG_value, ARG_print_en };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_addr, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
		{ MP_QSTR_width, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
        { MP_QSTR_value,  MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
		{ MP_QSTR_print_en,  MP_ARG_BOOL, {.u_bool = 1}},
    };
    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (args[ARG_addr].u_obj != MP_OBJ_NULL) {
		addr = strtoll(mp_obj_str_get_str(args[ARG_addr].u_obj),NULL,0);
		if (args[ARG_value].u_obj != MP_OBJ_NULL) {
			value = strtoll(mp_obj_str_get_str(args[ARG_value].u_obj),NULL,0);

			switch (mp_obj_get_int(args[ARG_width].u_obj)) {
			case 8:
				*(volatile uint8_t*)addr= value;
				break;
			case 16:
				*(volatile uint16_t*)addr = value;
				break; 
			case 32: 
				*(volatile uint32_t*)addr = value;
				break; 
			case 64: 
				*(volatile uint64_t*)addr = value;
				break;
			default:
				if(args[ARG_print_en].u_bool)
				{
					mp_raise_ValueError("[MAIXPY]Devmem:Bad width");
				}
				return mp_obj_new_bool(0);
			}
			if(args[ARG_print_en].u_bool)
			{
				printf("[MAIXPY]Devmem:Wite ok\n");
			}
			return mp_obj_new_bool(1);
			
		}else{
			switch (mp_obj_get_int(args[ARG_width].u_obj)) {
			case 8:
				value = *(volatile uint8_t*)addr;
				break;
			case 16:
				value = *(volatile uint16_t*)addr;
				break;
			case 32:
				value = *(volatile uint32_t*)addr;
				break;
			case 64:
				value = *(volatile uint64_t*)addr;
				break;
			default:
				if(args[ARG_print_en].u_bool)
				{
					mp_raise_ValueError("[MAIXPY]Devmem:Bad width");
				}
				return mp_obj_new_bool(0);
			}
			if(args[ARG_print_en].u_bool)
			{
				printf("[MAIXPY]Devmem:Read %x %x\n",addr,value);
			}
			return MP_OBJ_NEW_SMALL_INT(value);
		}
    }else{
		if(args[ARG_print_en].u_bool)
		{
			mp_raise_ValueError("[MAIXPY]Devmem:Addr vaild");
		}
	}
    return mp_obj_new_bool(0);
}

mp_obj_t mp_devmem_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
	mp_map_t kw_args;
	machine_devmem_obj_t *self = m_new_obj(machine_devmem_obj_t);

	mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
	machine_devmem_obj_init_helper( self, n_args, args, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}

// pin.init(mode, pull)
STATIC mp_obj_t machine_devmem_obj_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return machine_devmem_obj_init_helper(args[0],n_args - 1, args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_devmem_init_obj, 0, machine_devmem_obj_init);

STATIC const mp_rom_map_elem_t machine_devmem_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_devmem_init_obj) },
};

STATIC MP_DEFINE_CONST_DICT(machine_devmem_locals_dict, machine_devmem_locals_dict_table);

const mp_obj_type_t machine_devmem_type = {
    { &mp_type_type },
    .name = MP_QSTR_Devmem,
    .make_new = mp_devmem_make_new,
    .locals_dict = (mp_obj_t)&machine_devmem_locals_dict,
};

