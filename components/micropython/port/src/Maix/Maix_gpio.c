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

#include "gpio.h"
#include "gpiohs.h"
#include "plic.h"

#include "py/runtime.h"
#include "py/mphal.h"
#include "mphalport.h"
#include "modmachine.h"
#include "extmod/virtpin.h"

const mp_obj_type_t Maix_gpio_type;

typedef int gpio_num_t;
enum {
    GPIO_DM_PULL_NONE = -1,
};

typedef enum _gpio_type_t{
    GPIOHS = 0,
    GPIO = 1,
}gpio_type_t;

typedef struct _Maix_gpio_obj_t {
    mp_obj_base_t base;
    gpio_num_t num;
    gpio_type_t gpio_type;
    gpio_num_t id;
    mp_obj_t   callback;
    gpio_drive_mode_t mode;
    
} Maix_gpio_obj_t;

typedef struct _Maix_gpio_irq_obj_t {
    mp_obj_base_t base;
    gpio_num_t num;
    gpio_num_t id;
} Maix_gpio_irq_obj_t;

typedef enum __gpiohs_t{
    GPIO_NUM_0 = 0,
    GPIO_NUM_1,
    GPIO_NUM_2,
    GPIO_NUM_3,
    GPIO_NUM_4,
    GPIO_NUM_5,
    GPIO_NUM_6,
    GPIO_NUM_7,
}_gpiohs_t;

typedef enum __gpio_t{
    GPIOHS_NUM_0 = 0,
    GPIOHS_NUM_1,
    GPIOHS_NUM_2,
    GPIOHS_NUM_3,
    GPIOHS_NUM_4,
    GPIOHS_NUM_5,
    GPIOHS_NUM_6,
    GPIOHS_NUM_7,
    GPIOHS_NUM_8,
    GPIOHS_NUM_9,
    GPIOHS_NUM_10,
    GPIOHS_NUM_11,
    GPIOHS_NUM_12,
    GPIOHS_NUM_13,
    GPIOHS_NUM_14,
    GPIOHS_NUM_15,
    GPIOHS_NUM_16,
    GPIOHS_NUM_17,
    GPIOHS_NUM_18,
    GPIOHS_NUM_19,
    GPIOHS_NUM_20,
    GPIOHS_NUM_21,
    GPIOHS_NUM_22,
    GPIOHS_NUM_23,
    GPIOHS_NUM_24,
    GPIOHS_NUM_25,
    GPIOHS_NUM_26,
    GPIOHS_NUM_27,
    GPIOHS_NUM_28,
    GPIOHS_NUM_29,
    GPIOHS_NUM_30,
    GPIOHS_NUM_31,
} _gpio_t;

STATIC const Maix_gpio_obj_t Maix_gpio_obj[] = {
    {{&Maix_gpio_type}, 0, GPIOHS, GPIOHS_NUM_0, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 1, GPIOHS, GPIOHS_NUM_1, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 2, GPIOHS, GPIOHS_NUM_2, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 3, GPIOHS, GPIOHS_NUM_3, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 4, GPIOHS, GPIOHS_NUM_4, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 5, GPIOHS, GPIOHS_NUM_5, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 6, GPIOHS, GPIOHS_NUM_6, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 7, GPIOHS, GPIOHS_NUM_7, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 8, GPIOHS, GPIOHS_NUM_8, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 9, GPIOHS, GPIOHS_NUM_9, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 10, GPIOHS, GPIOHS_NUM_10, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 11, GPIOHS, GPIOHS_NUM_11, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 12, GPIOHS, GPIOHS_NUM_12, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 13, GPIOHS, GPIOHS_NUM_13, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 14, GPIOHS, GPIOHS_NUM_14, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 15, GPIOHS, GPIOHS_NUM_15, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 16, GPIOHS, GPIOHS_NUM_16, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 17, GPIOHS, GPIOHS_NUM_17, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 18, GPIOHS, GPIOHS_NUM_18, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 19, GPIOHS, GPIOHS_NUM_19, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 20, GPIOHS, GPIOHS_NUM_20, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 21, GPIOHS, GPIOHS_NUM_21, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 22, GPIOHS, GPIOHS_NUM_22, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 23, GPIOHS, GPIOHS_NUM_23, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 24, GPIOHS, GPIOHS_NUM_24, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 25, GPIOHS, GPIOHS_NUM_25, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 26, GPIOHS, GPIOHS_NUM_26, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 27, GPIOHS, GPIOHS_NUM_27, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 28, GPIOHS, GPIOHS_NUM_28, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 29, GPIOHS, GPIOHS_NUM_29, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 30, GPIOHS, GPIOHS_NUM_30, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 31, GPIOHS, GPIOHS_NUM_31, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 32, GPIO, GPIO_NUM_0, MP_OBJ_NULL, GPIO_DM_INPUT},//32
    {{&Maix_gpio_type}, 33, GPIO, GPIO_NUM_1, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 34, GPIO, GPIO_NUM_2, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 35, GPIO, GPIO_NUM_3, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 36, GPIO, GPIO_NUM_4, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 37, GPIO, GPIO_NUM_5, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 38, GPIO, GPIO_NUM_6, MP_OBJ_NULL, GPIO_DM_INPUT},
    {{&Maix_gpio_type}, 39, GPIO, GPIO_NUM_7, MP_OBJ_NULL, GPIO_DM_INPUT},

};

// forward declaration
STATIC const Maix_gpio_irq_obj_t Maix_gpio_irq_object[];

void Maix_gpios_init(void) {
    // memset(&MP_STATE_PORT(Maix_gpio_irq_handler[0]), 0, sizeof(MP_STATE_PORT(Maix_gpio_irq_handler)));
}

void Maix_gpios_deinit(void) {
    
    for (int i = 0; i < MP_ARRAY_SIZE(Maix_gpio_obj); ++i) {
        if (Maix_gpio_obj[i].gpio_type != GPIO) {
            plic_irq_disable(IRQN_GPIOHS0_INTERRUPT + Maix_gpio_obj[i].id);
        }
    }
}

STATIC int Maix_gpio_isr_handler(void *arg) {
   Maix_gpio_obj_t *self = arg;
   //only gpiohs support irq,so only support gpiohs in this func
   mp_obj_t handler = self->callback;
//    mp_call_function_2(handler, MP_OBJ_FROM_PTR(self), mp_obj_new_int_from_uint(self->id));
   mp_sched_schedule(handler, MP_OBJ_FROM_PTR(self));
   mp_hal_wake_main_task_from_isr();
    return 0;
}

gpio_num_t Maix_gpio_get_id(mp_obj_t pin_in) {
   if (mp_obj_get_type(pin_in) != &Maix_gpio_type) {
       mp_raise_ValueError("expecting a pin");
   }
   Maix_gpio_obj_t *self = pin_in;
   return self->id;
}

STATIC void Maix_gpio_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    Maix_gpio_obj_t *self = self_in;
    
    mp_printf(print, "Pin(%u)", self->id);
}

// pin.init(mode, pull=None, *, value)
STATIC mp_obj_t Maix_gpio_obj_init_helper(Maix_gpio_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_mode, ARG_pull, ARG_value };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_OBJ, {.u_obj = mp_const_none}},
        { MP_QSTR_pull, MP_ARG_OBJ, {.u_obj = mp_const_none}},
        { MP_QSTR_value, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // configure mode
    if (args[ARG_mode].u_obj != mp_const_none) {
        mp_int_t pin_io_mode = mp_obj_get_int(args[ARG_mode].u_obj);
        if (0 <= self->num && self->num < MP_ARRAY_SIZE(Maix_gpio_obj)) {
            self = (Maix_gpio_obj_t*)&Maix_gpio_obj[self->num];
            if(pin_io_mode == GPIO_DM_OUTPUT && args[ARG_pull].u_obj != mp_const_none && mp_obj_get_int(args[ARG_pull].u_obj) != GPIO_DM_PULL_NONE){
                mp_raise_ValueError("When this pin is in output mode, it is not allowed to pull up and down.");
            }else{
                if(args[ARG_pull].u_obj != mp_const_none && mp_obj_get_int(args[ARG_pull].u_obj) != GPIO_DM_PULL_NONE ){
                    if(mp_obj_get_int(args[ARG_pull].u_obj) == GPIO_DM_INPUT_PULL_UP || mp_obj_get_int(args[ARG_pull].u_obj) == GPIO_DM_INPUT_PULL_DOWN){
                        pin_io_mode = mp_obj_get_int(args[ARG_pull].u_obj);
                    }else{
                        mp_raise_ValueError("this mode not support.");
                    }
                }
                if(self->gpio_type == GPIO){
                    gpio_set_drive_mode(self->id, pin_io_mode);
                }else{
                    gpiohs_set_drive_mode(self->id, pin_io_mode);
                }
                self->mode = pin_io_mode;
            }

            //set initial value (dont this before configuring mode/pull)
            if (args[ARG_value].u_obj != MP_OBJ_NULL) {
                if(self->gpio_type == GPIOHS){
                    gpiohs_set_pin((uint8_t)self->id,mp_obj_is_true(args[ARG_value].u_obj));
                }else{
                    gpio_set_pin((uint8_t)self->id,mp_obj_is_true(args[ARG_value].u_obj));
                }
            }
        }else{
            mp_raise_ValueError("pin not found");
        }
    }

    return mp_const_none;
}

// constructor(id, ...)
mp_obj_t mp_maixpy_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get the wanted pin object
    int wanted_pin = mp_obj_get_int(args[0]);
    Maix_gpio_obj_t *self = NULL;
    if (0 <= wanted_pin && wanted_pin < MP_ARRAY_SIZE(Maix_gpio_obj)) {
        self = (Maix_gpio_obj_t*)&Maix_gpio_obj[wanted_pin];
    }
    if (self == NULL || self->base.type == NULL) {
        mp_raise_ValueError("invalid pin");
    }

    if (n_args > 1 || n_kw > 0) {
        // pin mode given, so configure this GPIO
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        Maix_gpio_obj_init_helper(self, n_args - 1, args + 1, &kw_args);
    }

    return MP_OBJ_FROM_PTR(self);
}

// fast method for getting/setting pin value
STATIC mp_obj_t Maix_gpio_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    Maix_gpio_obj_t *self = self_in;
    if (n_args == 0) {
        // get pin
        if(self->gpio_type == GPIO){
            return MP_OBJ_NEW_SMALL_INT(gpio_get_pin((uint8_t)self->id));
        }else{
            return MP_OBJ_NEW_SMALL_INT(gpiohs_get_pin((uint8_t)self->id));
        }
        
    } else {
        // set pin
        if(self->gpio_type == GPIO){
            gpio_set_pin(self->id, mp_obj_is_true(args[0]));
        }else{
            gpiohs_set_pin(self->id, mp_obj_is_true(args[0]));
        }
        return mp_const_none;
    }
}

// pin.init(mode, pull)
STATIC mp_obj_t Maix_gpio_obj_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return Maix_gpio_obj_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(Maix_gpio_init_obj, 1, Maix_gpio_obj_init);

// pin.value([value])
STATIC mp_obj_t Maix_gpio_value(size_t n_args, const mp_obj_t *args) {
    return Maix_gpio_call(args[0], n_args - 1, 0, args + 1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(Maix_gpio_value_obj, 1, 2, Maix_gpio_value);

// pin.irq(handler=None, trigger=IRQ_FALLING|IRQ_RISING)
STATIC mp_obj_t Maix_gpio_irq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_handler, ARG_trigger, ARG_wake ,ARG_priority};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_handler, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_trigger, MP_ARG_INT, {.u_int = GPIO_PE_BOTH} },
        { MP_QSTR_wake, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_priority, MP_ARG_INT, {.u_int = 7} },
    };
    Maix_gpio_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (self->gpio_type != GPIO && (n_args > 1 || kw_args->used != 0)) {
        // configure irq
        mp_obj_t handler = args[ARG_handler].u_obj;
        uint32_t trigger = args[ARG_trigger].u_int;
        mp_obj_t wake_obj = args[ARG_wake].u_obj;
        mp_int_t temp_wake_int;
        mp_obj_get_int_maybe(args[ARG_wake].u_obj,&temp_wake_int);
        
        if(wake_obj != mp_const_none && temp_wake_int != 0){
            mp_raise_ValueError("This platform does not support interrupt wakeup");
        }else{
            if (trigger == GPIO_PE_NONE || trigger == GPIO_PE_RISING || trigger == GPIO_PE_FALLING || trigger == GPIO_PE_BOTH) {

                if (handler == mp_const_none) {
                    handler = MP_OBJ_NULL;
                    trigger = 0;
                }
                self->callback = handler;
                gpiohs_set_pin_edge((uint8_t)self->id,trigger);
                gpiohs_irq_register((uint8_t)self->id, args[ARG_priority].u_int, Maix_gpio_isr_handler, (void *)self);
            }else{

            }
        }

        }

        
    //return the irq object
    return MP_OBJ_FROM_PTR(&Maix_gpio_irq_object[self->num]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(Maix_gpio_irq_obj, 1, Maix_gpio_irq);

STATIC mp_obj_t Maix_gpio_disirq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    Maix_gpio_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    if (self->gpio_type != GPIO) {
        plic_irq_disable(IRQN_GPIOHS0_INTERRUPT + (uint8_t)self->id);
    }
    return mp_const_none;
}

STATIC  MP_DEFINE_CONST_FUN_OBJ_KW(Maix_gpio_disirq_obj,1,Maix_gpio_disirq);


STATIC mp_obj_t Maix_gpio_mode(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    if(!mp_obj_is_type(pos_args[0], &Maix_gpio_type))
        mp_raise_ValueError("only for object");
    Maix_gpio_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    enum { ARG_mode};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_INT, {.u_int = -1} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if(args[ARG_mode].u_int == -1)
    {
        return mp_obj_new_int(self->mode);
    }
    else if(args[ARG_mode].u_int != GPIO_DM_INPUT && 
            args[ARG_mode].u_int != GPIO_DM_OUTPUT && 
            args[ARG_mode].u_int != GPIO_DM_PULL_NONE && 
            args[ARG_mode].u_int != GPIO_DM_INPUT_PULL_UP && 
            args[ARG_mode].u_int != GPIO_DM_INPUT_PULL_DOWN
            )
    {
        mp_raise_ValueError("arg error");
    }
    if (self->gpio_type == GPIO) {
        gpio_set_drive_mode(self->id, (gpio_drive_mode_t)args[ARG_mode].u_int);
    }else{
        gpiohs_set_drive_mode(self->id, (gpio_drive_mode_t)args[ARG_mode].u_int);
    }
    self->mode = (gpio_drive_mode_t)args[ARG_mode].u_int;
    return mp_const_none;
}

STATIC  MP_DEFINE_CONST_FUN_OBJ_KW(Maix_gpio_mode_obj,1,Maix_gpio_mode);


STATIC const mp_rom_map_elem_t Maix_gpio_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&Maix_gpio_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&Maix_gpio_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq), MP_ROM_PTR(&Maix_gpio_irq_obj) },
    { MP_ROM_QSTR(MP_QSTR_disirq), MP_ROM_PTR(&Maix_gpio_disirq_obj) },
    { MP_ROM_QSTR(MP_QSTR_mode), MP_ROM_PTR(&Maix_gpio_mode_obj) },
    // class constants
    { MP_ROM_QSTR(MP_QSTR_IN), MP_ROM_INT(GPIO_DM_INPUT) },
    { MP_ROM_QSTR(MP_QSTR_OUT), MP_ROM_INT(GPIO_DM_OUTPUT) },
    { MP_ROM_QSTR(MP_QSTR_PULL_NONE), MP_ROM_INT(GPIO_DM_PULL_NONE) },
    { MP_ROM_QSTR(MP_QSTR_PULL_UP), MP_ROM_INT(GPIO_DM_INPUT_PULL_UP) },
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN), MP_ROM_INT(GPIO_DM_INPUT_PULL_DOWN) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_NONE), MP_ROM_INT(GPIO_PE_NONE) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_RISING), MP_ROM_INT(GPIO_PE_RISING) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_FALLING), MP_ROM_INT(GPIO_PE_FALLING) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_BOTH), MP_ROM_INT(GPIO_PE_BOTH) },

    // gpio constant
    { MP_ROM_QSTR(MP_QSTR_GPIOHS0), MP_ROM_INT(GPIOHS_NUM_0) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS1), MP_ROM_INT(GPIOHS_NUM_1) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS2), MP_ROM_INT(GPIOHS_NUM_2) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS3), MP_ROM_INT(GPIOHS_NUM_3) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS4), MP_ROM_INT(GPIOHS_NUM_4) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS5), MP_ROM_INT(GPIOHS_NUM_5) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS6), MP_ROM_INT(GPIOHS_NUM_6) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS7), MP_ROM_INT(GPIOHS_NUM_7) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS8), MP_ROM_INT(GPIOHS_NUM_8) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS9), MP_ROM_INT(GPIOHS_NUM_9) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS10), MP_ROM_INT(GPIOHS_NUM_10) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS11), MP_ROM_INT(GPIOHS_NUM_11) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS12), MP_ROM_INT(GPIOHS_NUM_12) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS13), MP_ROM_INT(GPIOHS_NUM_13) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS14), MP_ROM_INT(GPIOHS_NUM_14) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS15), MP_ROM_INT(GPIOHS_NUM_15) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS16), MP_ROM_INT(GPIOHS_NUM_16) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS17), MP_ROM_INT(GPIOHS_NUM_17) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS18), MP_ROM_INT(GPIOHS_NUM_18) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS19), MP_ROM_INT(GPIOHS_NUM_19) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS20), MP_ROM_INT(GPIOHS_NUM_20) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS21), MP_ROM_INT(GPIOHS_NUM_21) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS22), MP_ROM_INT(GPIOHS_NUM_22) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS23), MP_ROM_INT(GPIOHS_NUM_23) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS24), MP_ROM_INT(GPIOHS_NUM_24) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS25), MP_ROM_INT(GPIOHS_NUM_25) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS26), MP_ROM_INT(GPIOHS_NUM_26) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS27), MP_ROM_INT(GPIOHS_NUM_27) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS28), MP_ROM_INT(GPIOHS_NUM_28) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS29), MP_ROM_INT(GPIOHS_NUM_29) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS30), MP_ROM_INT(GPIOHS_NUM_30) },
    { MP_ROM_QSTR(MP_QSTR_GPIOHS31), MP_ROM_INT(GPIOHS_NUM_31) },
    { MP_ROM_QSTR(MP_QSTR_GPIO0), MP_ROM_INT(32) },
    { MP_ROM_QSTR(MP_QSTR_GPIO1), MP_ROM_INT(33) },
    { MP_ROM_QSTR(MP_QSTR_GPIO2), MP_ROM_INT(34) },
    { MP_ROM_QSTR(MP_QSTR_GPIO3), MP_ROM_INT(35) },
    { MP_ROM_QSTR(MP_QSTR_GPIO4), MP_ROM_INT(36) },
    { MP_ROM_QSTR(MP_QSTR_GPIO5), MP_ROM_INT(37) },
    { MP_ROM_QSTR(MP_QSTR_GPIO6), MP_ROM_INT(38) },
    { MP_ROM_QSTR(MP_QSTR_GPIO7), MP_ROM_INT(39) },

    //wakeup not support
    { MP_ROM_QSTR(MP_QSTR_WAKEUP_NOT_SUPPORT), MP_ROM_INT(0) },
};

STATIC mp_uint_t pin_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    (void)errcode;
    Maix_gpio_obj_t *self = self_in;

    switch (request) {
        case MP_PIN_READ: {
            if(self->gpio_type == GPIO){
                return gpio_get_pin((uint8_t)self->id);
            }else{
                return gpio_get_pin((uint8_t)self->id);
            }
        }
        case MP_PIN_WRITE: {
            if(self->gpio_type == GPIO){
                gpio_set_pin((uint8_t)self->id, arg);
            }else{
                gpiohs_set_pin((uint8_t)self->id, arg);
            }
            return 0;
        }
    }
    return -1;
}

STATIC MP_DEFINE_CONST_DICT(Maix_gpio_locals_dict, Maix_gpio_locals_dict_table);

STATIC const mp_pin_p_t pin_pin_p = {
  .ioctl = pin_ioctl,
};

const mp_obj_type_t Maix_gpio_type = {
    { &mp_type_type },
    .name = MP_QSTR_Pin,
    .print = Maix_gpio_print,
    .make_new = mp_maixpy_pin_make_new,
    .call = Maix_gpio_call,
    .protocol = &pin_pin_p,
    .locals_dict = (mp_obj_t)&Maix_gpio_locals_dict,
};

/******************************************************************************/
// Pin IRQ object

STATIC const mp_obj_type_t Maix_gpio_irq_type;

STATIC const Maix_gpio_irq_obj_t Maix_gpio_irq_object[] = {
    {{&Maix_gpio_irq_type}, 0,  GPIOHS_NUM_0},
    {{&Maix_gpio_irq_type}, 1,  GPIOHS_NUM_1},
    {{&Maix_gpio_irq_type}, 2,  GPIOHS_NUM_2},
    {{&Maix_gpio_irq_type}, 3,  GPIOHS_NUM_3},
    {{&Maix_gpio_irq_type}, 4,  GPIOHS_NUM_4},
    {{&Maix_gpio_irq_type}, 5,  GPIOHS_NUM_5},
    {{&Maix_gpio_irq_type}, 6,  GPIOHS_NUM_6},
    {{&Maix_gpio_irq_type}, 7,  GPIOHS_NUM_7},
    {{&Maix_gpio_irq_type}, 8,  GPIOHS_NUM_8},
    {{&Maix_gpio_irq_type}, 9,  GPIOHS_NUM_9},
    {{&Maix_gpio_irq_type}, 10, GPIOHS_NUM_10},
    {{&Maix_gpio_irq_type}, 11, GPIOHS_NUM_11},
    {{&Maix_gpio_irq_type}, 12, GPIOHS_NUM_12},
    {{&Maix_gpio_irq_type}, 13, GPIOHS_NUM_13},
    {{&Maix_gpio_irq_type}, 14, GPIOHS_NUM_14},
    {{&Maix_gpio_irq_type}, 15, GPIOHS_NUM_15},
    {{&Maix_gpio_irq_type}, 16, GPIOHS_NUM_16},
    {{&Maix_gpio_irq_type}, 17, GPIOHS_NUM_17},
    {{&Maix_gpio_irq_type}, 18, GPIOHS_NUM_18},
    {{&Maix_gpio_irq_type}, 19, GPIOHS_NUM_19},
    {{&Maix_gpio_irq_type}, 20, GPIOHS_NUM_20},
    {{&Maix_gpio_irq_type}, 21, GPIOHS_NUM_21},
    {{&Maix_gpio_irq_type}, 22, GPIOHS_NUM_22},
    {{&Maix_gpio_irq_type}, 23, GPIOHS_NUM_23},
    {{&Maix_gpio_irq_type}, 24, GPIOHS_NUM_24},
    {{&Maix_gpio_irq_type}, 25, GPIOHS_NUM_25},
    {{&Maix_gpio_irq_type}, 26, GPIOHS_NUM_26},
    {{&Maix_gpio_irq_type}, 27, GPIOHS_NUM_27},
    {{&Maix_gpio_irq_type}, 28, GPIOHS_NUM_28},
    {{&Maix_gpio_irq_type}, 29, GPIOHS_NUM_29},
    {{&Maix_gpio_irq_type}, 30, GPIOHS_NUM_30},
    {{&Maix_gpio_irq_type}, 31, GPIOHS_NUM_31},
};

STATIC mp_obj_t Maix_gpio_irq_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    Maix_gpio_irq_obj_t *self = self_in;
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    Maix_gpio_isr_handler((void*)&Maix_gpio_obj[self->num]);
    return mp_const_none;
}

STATIC mp_obj_t Maix_gpio_irq_trigger(size_t n_args, const mp_obj_t *args) {
    Maix_gpio_irq_obj_t *self = args[0];
    if (n_args == 2) {
        // set trigger
            gpiohs_set_pin_edge(self->id,mp_obj_get_int(args[1]));
    }else{
        mp_raise_ValueError("Reading this property is not supported");
    }
    // not support to return original trigger value
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(Maix_gpio_irq_trigger_obj, 1, 2, Maix_gpio_irq_trigger);

STATIC const mp_rom_map_elem_t Maix_gpio_irq_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_trigger), MP_ROM_PTR(&Maix_gpio_irq_trigger_obj) },
};
STATIC MP_DEFINE_CONST_DICT(Maix_gpio_irq_locals_dict, Maix_gpio_irq_locals_dict_table);

STATIC const mp_obj_type_t Maix_gpio_irq_type = {
    { &mp_type_type },
    .name = MP_QSTR_IRQ,
    .call = Maix_gpio_irq_call,
    .locals_dict = (mp_obj_dict_t*)&Maix_gpio_irq_locals_dict,
};
