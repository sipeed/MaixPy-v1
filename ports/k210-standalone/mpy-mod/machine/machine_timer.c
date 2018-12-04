/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2015 Damien P. George
 * Copyright (c) 2016 Paul Sokolovsky
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

#include <stdint.h>
#include <stdio.h>

#include "timer.h"
#include "py/obj.h"
#include "py/gc.h"
#include "py/runtime.h"
#include "modmachine.h"
#include "mphalport.h"
#include "plic.h"
#include "sysctl.h"
#include "py/objtype.h"

#define TIMER_INTR_SEL TIMER_INTR_LEVEL
#define TIMER_DIVIDER  8

// TIMER_BASE_CLK is normally 80MHz. TIMER_DIVIDER ought to divide this exactly
#define TIMER_SCALE    (TIMER_BASE_CLK / TIMER_DIVIDER)

#define TIMER_FLAGS    0
typedef struct _machine_timer_obj_t {
    mp_obj_base_t base;
    mp_uint_t timer;
    mp_uint_t channel;
	uint32_t freq;
	uint32_t period;//k210 timers are 32-bit
	uint32_t div;
	uint32_t clk_freq;
	mp_obj_t callback;
	bool active;
    //mp_uint_t repeat;//timer mode
} machine_timer_obj_t;

const mp_obj_type_t machine_timer_type;

#define K210_DEBUG 0
#if K210_DEBUG==1
#define debug_print(x,arg...) printf("[MAIXPY]"x,##arg)
#else 
#define debug_print(x,arg...) 
#endif

STATIC void machine_timer_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_timer_obj_t *self = self_in;

    mp_printf(print, "[MAIXPY]TIMER:Timer(%p) ", self);

    mp_printf(print, "[MAIXPY]TIMER:timer freq=%d, ", self->freq);
    mp_printf(print, "[MAIXPY]TIMER:reload=%d, ", timer_get_reload(self->timer,self->channel));
    mp_printf(print, "[MAIXPY]TIMER:counter=%d", timer_get_count(self->timer,self->channel));
}

STATIC mp_obj_t machine_timer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    machine_timer_obj_t *self = m_new_obj(machine_timer_obj_t);
    self->base.type = &machine_timer_type;

    self->timer = mp_obj_get_int(args[0]);
    self->channel = mp_obj_get_int(args[1]);
	//mp_map_t kw_args;
    //mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
	//machine_timer_init_helper(self, n_args - 2, args + 2, &kw_args);

    return self;
}

STATIC void machine_timer_disable(machine_timer_obj_t *self) {
	timer_disable(self->timer, self->channel);
	plic_irq_disable(IRQN_TIMER0A_INTERRUPT + self->timer);
	plic_irq_deregister(IRQN_TIMER0A_INTERRUPT + self->timer);
	self->active = 0;
}

STATIC int machine_timer_isr(void *self_in) {
	int ret = 0;
	mp_obj_t member[2] = {MP_OBJ_NULL, MP_OBJ_NULL};
    machine_timer_obj_t *self = self_in;
	debug_print("[MAIXPY]TIMER:freq = %d\n",self->freq);
	debug_print("[MAIXPY]TIMER:enter machine_timer_isr\n");
	timer_channel_clear_interrupt(self->timer,self->channel);	
	debug_print("[MAIXPY]TIMER:self->callback = %p\n",self->callback);
	mp_obj_type_t *type = mp_obj_get_type(self->callback);
	debug_print("[MAIXPY]TIMER:type->call = %p\n",type->call);
	if(type != NULL)
	{
		debug_print("[MAIXPY]TIMER:type != NULL\n");
		mp_call_function_1(self->callback,MP_OBJ_FROM_PTR(self));
	}
	else
	{
		debug_print("[MAIXPY]TIMER:type == NULL\n");
		
	}
	debug_print("[MAIXPY]TIMER:quit machine_timer_isr\n");
	return 1;
}

STATIC void machine_timer_enable(machine_timer_obj_t *self) {
	debug_print("[MAIXPY]TIMER:self->timer=%d,self->channel=%d\n",self->timer,self->channel);
	if(self->channel == 0 || 1 == self->channel)
	{
		debug_print("[MAIXPY]TIMER:start init plic timer channel 0 and 1\n");
		plic_set_priority(IRQN_TIMER0A_INTERRUPT + self->timer*2, 1);
		plic_irq_enable(IRQN_TIMER0A_INTERRUPT + self->timer*2);
		plic_irq_register(IRQN_TIMER0A_INTERRUPT + self->timer*2, machine_timer_isr, (void*)self);
	}
	else if(self->channel == 2 || 3 == self->channel)
	{
		debug_print("[MAIXPY]TIMER:start init plic timer channel 2 and 3\n");
		plic_set_priority(IRQN_TIMER0B_INTERRUPT + self->timer*2, 1);
		plic_irq_enable(IRQN_TIMER0B_INTERRUPT + self->timer*2);
		plic_irq_register(IRQN_TIMER0B_INTERRUPT + self->timer*2, machine_timer_isr, (void*)self);
	}
	//timer_set_mode(self->timer, self->channel, TIMER_CR_USER_MODE);
	timer_set_reload(self->timer, self->channel, self->period);
	timer_enable_interrupt(self->timer, self->channel);
	timer_set_enable(self->timer, self->channel,1);
	self->active = 0;
	debug_print("[MAIXPY]TIMER:quit %s\n",__FUNCTION__);
}

STATIC mp_obj_t machine_timer_init_helper(machine_timer_obj_t *self, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum {
		ARG_freq,
		ARG_period,
		ARG_div,
        ARG_callback,
        //ARG_tick_hz,
        //ARG_mode,
    };
    static const mp_arg_t allowed_args[] = {
/*
#if MICROPY_PY_BUILTINS_FLOAT
		{ MP_QSTR_freq, 		MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
#else
#endif
*/
		{ MP_QSTR_freq, 		 MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_period,        MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_div,       	 MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_callback,      MP_ARG_OBJ, {.u_obj = mp_const_none} },
        //{ MP_QSTR_tick_hz,      MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1000} },
        //{ MP_QSTR_mode,         MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1} },

    };

    //machine_timer_disable(self);self->clk_freq
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
	self->clk_freq = sysctl_clock_get_freq(SYSCTL_CLOCK_TIMER0 + self->timer);
/*
#if MICROPY_PY_BUILTINS_FLOAT
    if (args[ARG_freq].u_obj != mp_const_none) {
        self->period = (uint32_t)(clk_freq / mp_obj_get_float(args[ARG_freq].u_obj));
    }
#else
#endif
*/
	/*timer prescale*/
	if(args[ARG_div].u_int == 0)
	{
		self->div = 1;
	}
	if(args[ARG_div].u_int - 1 > 0)
	{
		self->div = args[ARG_div].u_int;
		timer_set_clock_div(self->timer,self->div-1);
		
	}
	else
	{
		printf("[MAIXPY]TIMER:div must be bigger than 1,use default division parameter\n");
		self->div = 1;
	}
	
	/*set frequency,Frequency takes precedence over period*/
    if (args[ARG_freq].u_int > 0) 
	{
		self->freq = (uint32_t)args[ARG_freq].u_int;
		self->clk_freq = sysctl_clock_get_freq(SYSCTL_CLOCK_TIMER0 + self->timer);
		self->period = (uint32_t)(self->clk_freq / (uint32_t)args[ARG_freq].u_int);
    }
	else if(args[ARG_freq].u_int < 0)
	{
		printf("[MAIXPY]TIMER:frequency must be bigger than 1,use default frequency parameter\n");
		self->freq = 1;
		self->clk_freq = sysctl_clock_get_freq(SYSCTL_CLOCK_TIMER0 + self->timer);
		self->period = (uint32_t)(self->clk_freq / (uint32_t)args[ARG_freq].u_int);
	}
	else if(args[ARG_freq].u_int == 0)
	{
		if(args[ARG_period].u_int > 0)
		{
			self->period = args[ARG_period].u_int;
		}
		else
		{
			/*no set freq && no set period*/
			printf("[MAIXPY]TIMER:please set freq  or period correctly\n");
			printf("[MAIXPY]TIMER:freq > 0  and  period > 0\n");
			return mp_const_false;
		}
	}
	
    //self->repeat = args[ARG_mode].u_int;	
	if (mp_obj_is_callable(args[ARG_callback].u_obj))
	{
		self->callback = args[ARG_callback].u_obj;
		debug_print("[MAIXPY]TIMER:callback is normal\n");
	}
    else
    {
    	printf("[MAIXPY]TIMER:callback can't work,please give me a callback function\n");
		return mp_const_false;
    }
	printf("[MAIXPY]TIMER:clk_freq = %d,self->period = %d\n",self->clk_freq,self->period);
	timer_init(self->timer);
    machine_timer_enable(self);
	self->clk_freq = sysctl_clock_get_freq(SYSCTL_CLOCK_TIMER0 + self->timer);
    return mp_const_none;
}

STATIC mp_obj_t machine_timer_deinit(mp_obj_t self_in) {
    machine_timer_disable(self_in);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_deinit_obj, machine_timer_deinit);

STATIC mp_obj_t machine_timer_init(mp_uint_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return machine_timer_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_timer_init_obj, 1, machine_timer_init);


STATIC mp_obj_t machine_timer_callbcak(mp_obj_t self_in,mp_obj_t func_in) {
    machine_timer_obj_t *self = self_in;
	
    self->callback = func_in;

    return mp_const_none;  // value in ms
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_timer_callback_obj, machine_timer_callbcak);



STATIC mp_obj_t machine_timer_value(mp_obj_t self_in) {
    machine_timer_obj_t *self = self_in;
    mp_uint_t result = 0;

    result = timer_get_count(self->timer,self->channel);

    return MP_OBJ_NEW_SMALL_INT((mp_uint_t)(result * 1000));  // value in ms
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_value_obj, machine_timer_value);

STATIC mp_obj_t machine_timer_period(mp_obj_t self_in,mp_obj_t period) {
    machine_timer_obj_t *self = self_in;
    mp_uint_t result = 0;
	if (MP_OBJ_IS_SMALL_INT((period))) 
		self->period = MP_OBJ_SMALL_INT_VALUE(period);
	else
	{
		printf("[MAIXPY]TIMER:type error\n");
		return mp_const_none;
	}
	result = self->period;
	timer_disable(self->timer, self->channel);
	timer_set_reload(self->timer, self->channel, self->period);
	timer_set_enable(self->timer, self->channel,1);
    return MP_OBJ_NEW_SMALL_INT((mp_uint_t)(result * 1000));  // value in ms
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_timer_period_obj, machine_timer_period);

STATIC mp_obj_t machine_timer_stop(mp_obj_t self_in) {
    machine_timer_obj_t *self = self_in;

	timer_disable(self->timer, self->channel);
	self->active = 0;
    return mp_const_none;  // value in ms
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_stop_obj, machine_timer_stop);


STATIC mp_obj_t machine_timer_restart(mp_obj_t self_in) {
    machine_timer_obj_t *self = self_in;
    mp_uint_t result = 0;
	timer_disable(self->timer, self->channel);
	timer_set_reload(self->timer, self->channel, self->period);
	result=self->period;
	timer_set_enable(self->timer, self->channel,1);
    return MP_OBJ_NEW_SMALL_INT((mp_uint_t)(result * 1000));  // value in ms
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_restart_obj, machine_timer_restart);

STATIC mp_obj_t machine_timer_start(mp_obj_t self_in) {
    machine_timer_obj_t *self = self_in;
	if(self->active == 0)
		timer_set_enable(self->timer, self->channel,1);
	self->active = 1;
    return mp_const_none;  // value in ms
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_start_obj, machine_timer_start);

STATIC mp_obj_t machine_timer_freq(mp_obj_t self_in,mp_obj_t freq) {
    machine_timer_obj_t *self = self_in;
    mp_uint_t result = 0;
	if (MP_OBJ_IS_SMALL_INT((freq))) 
		self->freq = MP_OBJ_SMALL_INT_VALUE(freq);
	else
	{
		printf("[MAIXPY]TIMER:type error\n");
		return mp_const_none;
	}
	timer_disable(self->timer, self->channel);
	self->period = (uint32_t)(self->clk_freq / self->freq);
	timer_set_reload(self->timer, self->channel, self->period);
	timer_set_enable(self->timer, self->channel,1);
	result = self->freq;
    return MP_OBJ_NEW_SMALL_INT((mp_uint_t)(result * 1000));  // value in ms
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_timer_freq_obj, machine_timer_freq);

STATIC const mp_rom_map_elem_t machine_timer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&machine_timer_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_timer_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_timer_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_callback), MP_ROM_PTR(&machine_timer_callback_obj) },
    { MP_ROM_QSTR(MP_QSTR_period), MP_ROM_PTR(&machine_timer_period_obj) },
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&machine_timer_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&machine_timer_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_restart), MP_ROM_PTR(&machine_timer_restart_obj) },
    { MP_ROM_QSTR(MP_QSTR_freq), MP_ROM_PTR(&machine_timer_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&machine_timer_value_obj) },

	{ MP_ROM_QSTR(MP_QSTR_TIMER0), MP_ROM_INT(0) },
	{ MP_ROM_QSTR(MP_QSTR_TIMER1), MP_ROM_INT(1) },
	{ MP_ROM_QSTR(MP_QSTR_TIMER2), MP_ROM_INT(2) },
	{ MP_ROM_QSTR(MP_QSTR_CHANNEL0), MP_ROM_INT(0) },
	{ MP_ROM_QSTR(MP_QSTR_CHANNEL1), MP_ROM_INT(1) },
	{ MP_ROM_QSTR(MP_QSTR_CHANNEL2), MP_ROM_INT(2) },
	{ MP_ROM_QSTR(MP_QSTR_CHANNEL3), MP_ROM_INT(3) },
	
    { MP_ROM_QSTR(MP_QSTR_ONE_SHOT), MP_ROM_INT(false) },
    { MP_ROM_QSTR(MP_QSTR_PERIODIC), MP_ROM_INT(true) },
};
STATIC MP_DEFINE_CONST_DICT(machine_timer_locals_dict, machine_timer_locals_dict_table);

const mp_obj_type_t machine_timer_type = {
    { &mp_type_type },
    .name = MP_QSTR_Timer,
    .print = machine_timer_print,
    .make_new = machine_timer_make_new,
    .locals_dict = (mp_obj_t)&machine_timer_locals_dict,
};
