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

#include "py/runtime.h"
#include "py/mphal.h"
#include "modmachine.h"
#include "gpio.h"

#define gpio_num_t int
typedef struct _machine_pin_obj_t {
    mp_obj_base_t base;
    gpio_num_t id;
} machine_pin_obj_t;

#define GPIO_MAX_PINNO 8
STATIC const machine_pin_obj_t machine_pin_obj[] = {
    {{&machine_pin_type}, 0},
    {{&machine_pin_type}, 1},
    {{&machine_pin_type}, 2},
    {{&machine_pin_type}, 3},
    {{&machine_pin_type}, 4},
    {{&machine_pin_type}, 5},
    {{&machine_pin_type}, 6},
    {{&machine_pin_type}, 7},
};

void machine_pins_init(void) {
}

void machine_pins_deinit(void) {
}

gpio_num_t machine_pin_get_id(mp_obj_t pin_in) {
    if (mp_obj_get_type(pin_in) != &machine_pin_type) {
        mp_raise_ValueError("[MAIXPY]pin:expecting a pin");
    }
    machine_pin_obj_t *self = pin_in;
    return self->id;
}

STATIC void machine_pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_pin_obj_t *self = self_in;
    mp_printf(print, "Pin(%u)", self->id);
}

// pin.init(mode, pull=None, *, value)
STATIC mp_obj_t machine_pin_obj_init_helper(const machine_pin_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_mode, ARG_value, ARG_printf_en };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_OBJ, {.u_obj = mp_const_none}},
        { MP_QSTR_value,  MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
        { MP_QSTR_print_en,  MP_ARG_BOOL, {.u_bool = 1}},
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // configure the pin for gpio
    //gpio_pin_init(size_t pin_num, size_t gpio_pin);

    // set initial value (do this before configuring mode/pull)
    // configure mode
    mp_int_t pin_io_mode = 0;
    if (args[ARG_mode].u_obj != mp_const_none) {
		pin_io_mode = mp_obj_get_int(args[ARG_mode].u_obj);
        if (self->id >= GPIO_MAX_PINNO && (pin_io_mode >GPIO_DM_OUTPUT) && (pin_io_mode <GPIO_DM_INPUT) ) {
            if(args[ARG_printf_en].u_bool)
            {
                mp_raise_ValueError("[MAIXPY]pin:pin can only be input");
            }
            return mp_obj_new_bool(0);
        } else {
			gpio_set_drive_mode(self->id, pin_io_mode);
        }
    }
	
	if(pin_io_mode ==GPIO_DM_OUTPUT ){
	    if (args[ARG_value].u_obj != MP_OBJ_NULL) {
	        gpio_set_pin(self->id, mp_obj_is_true(args[ARG_value].u_obj)==0?GPIO_PV_LOW:GPIO_PV_HIGH);
            return mp_obj_new_bool(1);
	    }
	}


    return mp_const_none;
}

// constructor(id, ...)
mp_obj_t mp_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get the wanted pin object
    int wanted_pin = mp_obj_get_int(args[0]);
    machine_pin_obj_t* self = NULL;
    if (0 <= wanted_pin && wanted_pin < MP_ARRAY_SIZE(machine_pin_obj)) {
        self = (machine_pin_obj_t*)&machine_pin_obj[wanted_pin];
    }
    if (self == NULL || self->base.type == NULL) {
        mp_raise_ValueError("[MAIXPY]pin:invalid pin");
        return mp_const_false;
    }

    if (n_args > 1 || n_kw > 0) {
        // pin mode given, so configure this GPIO
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        machine_pin_obj_init_helper(self, n_args - 1, args + 1, &kw_args);
    }

    return MP_OBJ_FROM_PTR(self);
}

// fast method for getting/setting pin value
STATIC mp_obj_t machine_pin_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    machine_pin_obj_t *self = self_in;
    if (n_args == 0) {
        // get pin
        return MP_OBJ_NEW_SMALL_INT(gpio_get_pin(self->id));
    } else {
        // set pin
        gpio_set_pin(self->id, mp_obj_is_true(args[0])==0?GPIO_PV_LOW:GPIO_PV_HIGH);
        return mp_obj_new_bool(1);
    }
}

// pin.init(mode, pull)
STATIC mp_obj_t machine_pin_obj_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return machine_pin_obj_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_pin_init_obj, 1, machine_pin_obj_init);

// pin.value([value])
STATIC mp_obj_t machine_pin_value(size_t n_args, const mp_obj_t *args) {
    return machine_pin_call(args[0], n_args - 1, 0, args + 1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pin_value_obj, 1, 2, machine_pin_value);

// pin.toggle(pinnum)
STATIC mp_obj_t machine_pin_toggle(mp_obj_t self_in) {
    // set pin
    machine_pin_obj_t *self = self_in;
    gpio_set_pin(self->id, !gpio_get_pin(self->id));
    return MP_OBJ_NEW_SMALL_INT(gpio_get_pin(self->id));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_toggle_obj,  machine_pin_toggle);

STATIC mp_obj_t machine_pin_help(machine_pin_obj_t self) {
printf("pin - Provide gpio operation\n\
\e[1mpin\e[0m(\e[1mgpio_num\e[0m,\e[1mgpio_mode\e[0m,\e[1minit_value\e[0m)\n\
    \e[1mgpio_num\e[0m:Explain which gpio to use\n\
    \e[1mgpio_mode\e[0m:\n\
        \e[1mDM_INPUT\e[0m,\e[1mDM_INPUT_PULL_DOWN\e[0m,\e[1mDM_INPUT_PULL_UP\e[0m,\e[1mDM_OUTPUT\e[0m\n\
        \e[1minit_value\e[0m:\n\
            \e[1mHIGH_LEVEL\e[0m:High level\n\
            \e[1mLOW_LEVEL\e[0m:Low level\n\
\e[1mMethod:\e[0m\n\
    \e[1minit\e[0m(\e[1mgpio_num\e[0m,\e[1mgpio_mode\e[0m,\e[1minit_value\e[0m)\n\
        \e[1mgpio_mode\e[0m:\n\
        \e[1mDM_INPUT\e[0m,\e[1mDM_INPUT_PULL_DOWN\e[0m,\e[1mDM_INPUT_PULL_UP\e[0m,\e[1mDM_OUTPUT\e[0m\n\
        \e[1minit_value\e[0m:\n\
            \e[1mHIGH_LEVEL\e[0m:High level\n\
            \e[1mLOW_LEVEL\e[0m:Low level\n\
    \e[1mvalue\e[0m(\e[1marg\e[0m)\n\
        \e[1marg\e[0m:\n\
            If you want to read the level status of gpio, it is empty.\n\
            If you want to set the level state of gpio, fill in the specific level state.\n\
            \e[1mHIGH_LEVEL\e[0m:High level\n\
            \e[1mLOW_LEVEL\e[0m:Low level\n\
    \e[1mtoggle\e[0m()\n\
         Flip gpio level status\n\
");
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_pin_help_obj, machine_pin_help);

STATIC const mp_rom_map_elem_t machine_pin_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_pin_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&machine_pin_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_toggle), MP_ROM_PTR(&machine_pin_toggle_obj) },
    { MP_ROM_QSTR(MP_QSTR_help), MP_ROM_PTR(&machine_pin_help_obj) },
    { MP_ROM_QSTR(MP_QSTR_DM_INPUT), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_DM_INPUT_PULL_DOWN), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_DM_INPUT_PULL_UP), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_DM_OUTPUT), MP_ROM_INT(3) },
    
    { MP_ROM_QSTR(MP_QSTR_GPIO0), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_GPIO1), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_GPIO2), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_GPIO3), MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_GPIO4), MP_ROM_INT(4) },
    { MP_ROM_QSTR(MP_QSTR_GPIO5), MP_ROM_INT(5) },
    { MP_ROM_QSTR(MP_QSTR_GPIO6), MP_ROM_INT(6) },
    { MP_ROM_QSTR(MP_QSTR_GPIO7), MP_ROM_INT(7) },
    
    { MP_ROM_QSTR(MP_QSTR_HIGH_LEVEL), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_LOW_LEVEL), MP_ROM_INT(0) },
};

STATIC mp_uint_t pin_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    (void)errcode;
    machine_pin_obj_t *self = self_in;

    switch (request) {
        case MP_PIN_READ: {
            return gpio_get_pin(self->id);
        }
        case MP_PIN_WRITE: {
            gpio_set_pin(self->id, arg==0?GPIO_PV_LOW:GPIO_PV_HIGH);
            return 0;
        }
    }
    return -1;
}

STATIC MP_DEFINE_CONST_DICT(machine_pin_locals_dict, machine_pin_locals_dict_table);

STATIC const mp_pin_p_t pin_pin_p = {
  .ioctl = pin_ioctl,
};

const mp_obj_type_t machine_pin_type = {
    { &mp_type_type },
    .name = MP_QSTR_Pin,
    .print = machine_pin_print,
    .make_new = mp_pin_make_new,
    .call = machine_pin_call,
    .protocol = &pin_pin_p,
    .locals_dict = (mp_obj_t)&machine_pin_locals_dict,
};

