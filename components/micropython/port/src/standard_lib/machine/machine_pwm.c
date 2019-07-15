/*
 * This file is part of the Micro Python project, http://micropython.org/
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

#include "py/nlr.h"
#include "py/runtime.h"
#include "modmachine.h"
#include "mphalport.h"
#include "pwm.h"
#include "fpioa.h"
#include "py/mpconfig.h"
#include "machine_timer.h"
#include "sysctl.h"


extern const mp_obj_type_t machine_pwm_type;

typedef struct _k210_pwm_obj_t {
    mp_obj_base_t         base;
    machine_timer_obj_t   timer;
    double                freq;    // Hz [,]
    double                duty;    // [0,100] %
    int                   pin;     // [0,47]
    bool                  active;
} machine_pwm_obj_t;


STATIC bool check_pin(int pin)
{
    if(pin<0 || pin > 47)
        return false;
    return true;
}

STATIC bool check_freq(double freq)
{
    //TODO: max frequency limit here
    if( freq<=0)
        return false;
    return true;   
}

STATIC bool check_duty(double duty)
{
    if( duty<0 || duty>100)
        return false;
    return true;
}


STATIC void k210_pwm_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "[MAIXPY]PWM:(%p) timer=%d, channel=%d, freq=%.0f, duty=%.2f, enabled=%d",
             self, self->timer.timer ,self->timer.channel, self->freq, self->duty, self->active);
}

STATIC void machine_pwm_init_helper(machine_pwm_obj_t *self,
        size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { 
        ARG_freq,
        ARG_duty,
        ARG_pin,
        ARG_enable
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_freq,    MP_ARG_OBJ,  {.u_obj  = mp_const_none} },
        { MP_QSTR_duty,    MP_ARG_OBJ,  {.u_obj  = mp_const_none} },
		{ MP_QSTR_pin,     MP_ARG_INT,  {.u_int  = -1} },
        { MP_QSTR_enable,  MP_ARG_BOOL, {.u_bool = true}},
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args,
    MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	self->pin = args[ARG_pin].u_int;
    if( self->pin != -1)//need to initilize fpioa
    {
        if( !check_pin(self->pin))
            mp_raise_ValueError("[MAIXPY]PWM:Invalid pin number");
        fpioa_set_function(self->pin, FUNC_TIMER0_TOGGLE1 + self->timer.timer * 4 + self->timer.channel);
    }	
    //Maybe change PWM timer
    double freq = mp_obj_get_float( args[ARG_freq].u_obj);
	double duty = mp_obj_get_float(args[ARG_duty].u_obj);
    if(!check_freq(freq))
        mp_raise_ValueError("[MAIXPY]PWM:Invalid freq");
    if(!check_duty(duty))
        mp_raise_ValueError("[MAIXPY]PWM:Invalid duty");
    self->freq = freq;
    self->duty = duty;
    self->active = args[ARG_enable].u_bool;
    pwm_init(self->timer.timer);
    self->freq = pwm_set_frequency(self->timer.timer, self->timer.channel, self->freq, self->duty/100.0);
    pwm_set_enable(self->timer.timer, self->timer.channel, self->active?1:0);
}

STATIC mp_obj_t k210_pwm_make_new(const mp_obj_type_t *type,
        size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 6, true);
    machine_pwm_obj_t *self = m_new_obj(machine_pwm_obj_t);
    self->timer = *(machine_timer_obj_t*)args[0];
    self->base.type = &machine_pwm_type;

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    machine_pwm_init_helper(self, n_args - 1, args + 1, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t k210_pwm_init(size_t n_args,
        const mp_obj_t *args, mp_map_t *kw_args) {
    machine_pwm_init_helper(args[0], n_args - 1, args + 1, kw_args);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(k210_pwm_init_obj, 1, k210_pwm_init);

STATIC mp_obj_t k210_pwm_deinit(mp_obj_t self_in) {
    machine_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pwm_set_enable(self->timer.timer, self->timer.channel, 0);
    //pwm_deinit()
    sysctl_clock_disable(SYSCTL_CLOCK_TIMER0+self->timer.timer);
    self->active = false;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(k210_pwm_deinit_obj, k210_pwm_deinit);

STATIC mp_obj_t k210_pwm_freq(size_t n_args, const mp_obj_t *args) {
	machine_pwm_obj_t *self = (machine_pwm_obj_t *)(args[0]);
    //get freq
    if (n_args == 1) {
        return mp_obj_new_int((*(machine_pwm_obj_t *)(args[0])).freq);
    }
    //set freq
    double freq =mp_obj_get_float(args[1]);
    if(!check_freq(freq)) 
        mp_raise_ValueError("[MAIXPY]PWM:Invalid freq");
    self->freq = freq;
    self->freq = pwm_set_frequency(self->timer.timer, self->timer.channel, self->freq, self->duty/100.0);
    return mp_obj_new_float(self->freq);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(k210_pwm_freq_obj, 1, 2, k210_pwm_freq);

STATIC mp_obj_t k210_pwm_duty(size_t n_args, const mp_obj_t *args) {
    machine_pwm_obj_t *self = (machine_pwm_obj_t *)(args[0]);
    //get freq
    if (n_args == 1) {
        return mp_obj_new_int((*(machine_pwm_obj_t *)(args[0])).duty);
    }
    //set freq
    double duty =mp_obj_get_float(args[1]);
    if(!check_duty(duty)) 
        mp_raise_ValueError("[MAIXPY]PWM:Invalid duty");
    self->duty = duty;
    pwm_set_frequency(self->timer.timer, self->timer.channel, self->freq, self->duty/100.0);
    return mp_obj_new_float(self->duty);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(k210_pwm_duty_obj,1, 2, k210_pwm_duty);


STATIC mp_obj_t k210_pwm_enable(size_t n_args, const mp_obj_t *args) {
    machine_pwm_obj_t *self = (machine_pwm_obj_t *)(args[0]);
    //enable
    if (n_args == 1) {
        pwm_set_enable(self->timer.timer, self->timer.channel, 1);
        return mp_const_none;
    }
    //enable according to param[0,1]
    int enable =mp_obj_get_int(args[1]);
    pwm_set_enable(self->timer.timer, self->timer.channel, enable);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(k210_pwm_enable_obj,1, 2, k210_pwm_enable);


STATIC mp_obj_t k210_pwm_disable(size_t n_args, const mp_obj_t *args) {
    machine_pwm_obj_t *self = (machine_pwm_obj_t *)(args[0]);
    pwm_set_enable(self->timer.timer, self->timer.channel, 0);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(k210_pwm_disable_obj,1, 1, k210_pwm_disable);


STATIC const mp_rom_map_elem_t k210_pwm_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&k210_pwm_init_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&k210_pwm_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&k210_pwm_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_freq), MP_ROM_PTR(&k210_pwm_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&k210_pwm_duty_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable), MP_ROM_PTR(&k210_pwm_enable_obj) },
    { MP_ROM_QSTR(MP_QSTR_disable), MP_ROM_PTR(&k210_pwm_disable_obj) },
	
};

STATIC MP_DEFINE_CONST_DICT(k210_pwm_locals_dict,k210_pwm_locals_dict_table);

const mp_obj_type_t machine_pwm_type = {
    { &mp_type_type },
    .name = MP_QSTR_PWM,
    .print = k210_pwm_print,
    .make_new = k210_pwm_make_new,
    .locals_dict = (mp_obj_dict_t*)&k210_pwm_locals_dict,
};
