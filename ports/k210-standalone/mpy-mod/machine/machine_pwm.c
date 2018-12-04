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
#include "mpconfigport.h"
// Forward dec'l
extern const mp_obj_type_t machine_pwm_type;

typedef struct _k210_pwm_obj_t {
    mp_obj_base_t base;
	int    pin_num;
    int        tim;
    int    channel;
    uint8_t active;
    int     freq_hz;
    double     duty;
} k210_pwm_obj_t;


/******************************************************************************/

// MicroPython bindings for PWM

STATIC void k210_pwm_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    k210_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "PWM(tim:%u channel:%u", self->tim ,self->channel);
    if (self->active) {
        mp_printf(print, ", freq=%u, duty=%u%", self->freq_hz,self->duty);
    }
    mp_printf(print, ")");
}

STATIC void k210_pwm_init_helper(k210_pwm_obj_t *self,
        size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_freq, ARG_duty, ARG_pin_num, ARG_printf_en};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_freq, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_duty, MP_ARG_OBJ, {.u_obj = mp_const_none} },
		{ MP_QSTR_pin_num, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_print_en,  MP_ARG_BOOL, {.u_bool = 1}},
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args,
    MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    int channel;
    int avail = -1;

	
	
    // Find a free PWM channel, also spot if our pin is
    //  already mentioned.

    if (self->tim > 2) {
        if(args[ARG_printf_en].u_bool)
        {
            mp_raise_ValueError("out of PWM timers");
        }
        return mp_obj_new_bool(0);
    }
    if (self->channel > 3) {
        if(args[ARG_printf_en].u_bool)
        {
           mp_raise_ValueError("out of PWM channels");
        }
        return mp_obj_new_bool(0);
    }
	self->pin_num = args[ARG_pin_num].u_int;
    if(args[ARG_printf_en].u_bool)
    {
	    printf("[MAIXPY]PWM:set pin%d to pwm output pin\n",args[ARG_pin_num].u_int);
    }
	//fpioa_set_function(args[ARG_pin_num].u_int, FUNC_TIMER0_TOGGLE1 + self->tim * 4 + self->channel);
    //Maybe change PWM timer
    int tval = args[ARG_freq].u_int;
	double dval = mp_obj_get_float(args[ARG_duty].u_obj);
    if (tval != -1 || dval != -1) {
          self->freq_hz = tval;
		  self->duty = dval;
          //timer_set_clock_div(self->tim,2);
		  //timer_set_enable(self->tim, self->channel, 1);
		  pwm_init(self->tim);
		  double duty = self->duty/(double)100;
		  int fre_pwm = pwm_set_frequency(self->tim,self->channel,self->freq_hz,duty);
          if(args[ARG_printf_en].u_bool)
          {
		    printf("[MAIXPY]PWM:frquency = %d,duty = %f\n",fre_pwm,duty);
          }
		  self->freq_hz = fre_pwm;
		  pwm_set_enable(self->tim, self->channel,1);
          return mp_obj_new_bool(1);
          //timer_enable(self->tim,self->channel);
          // Set duty cycle?
    }
	else
	{
        if(args[ARG_printf_en].u_bool)
        {
		    printf("[MAIXPY]pwm；frequency is invalid!\n");
        }
        return mp_obj_new_bool(0);
	}
    return mp_obj_new_bool(1);
}

STATIC mp_obj_t k210_pwm_make_new(const mp_obj_type_t *type,
        size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    int tim_id = mp_obj_get_int(args[0]);
    int channel = mp_obj_get_int(args[1]);
    // create PWM object from the given pin
    k210_pwm_obj_t *self = m_new_obj(k210_pwm_obj_t);
    self->base.type = &machine_pwm_type;
    self->tim = tim_id;
    self->channel = channel;
    self->active = 0;
    //self->channel = -1;

    // start the PWM running for this channel
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    k210_pwm_init_helper(self, n_args - 2, args + 2, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t k210_pwm_init(size_t n_args,
        const mp_obj_t *args, mp_map_t *kw_args) {
    k210_pwm_init_helper(args[0], n_args - 2, args + 2, kw_args);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(k210_pwm_init_obj, 1, k210_pwm_init);

STATIC mp_obj_t k210_pwm_deinit(mp_obj_t self_in) {
    k210_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int chan = self->channel;

    // Valid channel?
    if ((chan >= 0) && (chan < 3)) {
        // Mark it unused, and tell the hardware to stop routing
        self->active = 0;
        self->channel = -1;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(k210_pwm_deinit_obj, k210_pwm_deinit);

STATIC mp_obj_t k210_pwm_freq(size_t n_args, const mp_obj_t *args) {
    if (n_args == 1) {
        // get
        return MP_OBJ_NEW_SMALL_INT((*(k210_pwm_obj_t *)(args[0])).freq_hz);
    }
	
    // set
	k210_pwm_obj_t *self = (k210_pwm_obj_t *)(args[0]);
    self->freq_hz = mp_obj_get_int(args[1]);
    pwm_set_frequency(self->tim,self->channel,self->freq_hz,self->duty);
    //if (!set_freq(tval)) {
    //    nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,
    //        "Bad frequency %d", tval));
    //}

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(k210_pwm_freq_obj, 1, 2, k210_pwm_freq);

STATIC mp_obj_t k210_pwm_duty(size_t n_args, const mp_obj_t *args) {
    k210_pwm_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int duty;
	
    if (n_args == 1) {
        // get
        duty = (*(k210_pwm_obj_t *)(args[0])).duty;
        return MP_OBJ_NEW_SMALL_INT(duty);
    }
    // set
    self->duty = mp_obj_get_float(args[1]);
    pwm_set_frequency(self->tim,self->channel,self->freq_hz,self->duty/(double)100);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(k210_pwm_duty_obj,1, 2, k210_pwm_duty);

STATIC const mp_rom_map_elem_t k210_pwm_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&k210_pwm_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&k210_pwm_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_freq), MP_ROM_PTR(&k210_pwm_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&k210_pwm_duty_obj) },
    
	{ MP_ROM_QSTR(MP_QSTR_TIMER0), MP_ROM_INT(0) },
	{ MP_ROM_QSTR(MP_QSTR_TIMER1), MP_ROM_INT(1) },
	{ MP_ROM_QSTR(MP_QSTR_TIMER2), MP_ROM_INT(2) },
	{ MP_ROM_QSTR(MP_QSTR_CHANEEL0), MP_ROM_INT(0) },
	{ MP_ROM_QSTR(MP_QSTR_CHANEEL1), MP_ROM_INT(1) },
	{ MP_ROM_QSTR(MP_QSTR_CHANEEL2), MP_ROM_INT(2) },
	{ MP_ROM_QSTR(MP_QSTR_CHANEEL3), MP_ROM_INT(3) },
	
};

STATIC MP_DEFINE_CONST_DICT(k210_pwm_locals_dict,k210_pwm_locals_dict_table);

const mp_obj_type_t machine_pwm_type = {
    { &mp_type_type },
    .name = MP_QSTR_PWM,
    .print = k210_pwm_print,
    .make_new = k210_pwm_make_new,
    .locals_dict = (mp_obj_dict_t*)&k210_pwm_locals_dict,
};
