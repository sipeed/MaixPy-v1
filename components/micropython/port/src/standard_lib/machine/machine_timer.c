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
#include <math.h>

#include "py/gc.h"
#include "py/runtime.h"
#include "modmachine.h"
#include "mphalport.h"
#include "plic.h"
#include "sysctl.h"
#include "py/objtype.h"
#include "machine_timer.h"

////////////////////////// csdk api but not in header file (timer.h)///////////////////////////////
extern void timer_set_clock_div(timer_device_number_t timer_number, uint32_t div);
extern void timer_disable(timer_device_number_t timer_number, timer_channel_number_t channel);
///////////////////////////////////////////////////////////////////////////////////////////////////

const mp_obj_type_t machine_timer_type;

#define K210_DEBUG 0
#if K210_DEBUG==1
#define debug_print(x,arg...) mp_printf(&mp_plat_print, "[MAIXPY]"x,##arg)
#else 
#define debug_print(x,arg...) 
#endif


STATIC bool check_mode(uint32_t mode)
{
    if( mode>=MACHINE_TIMER_MODE_MAX )
        return false;
    return true;
}

STATIC bool check_unit(uint32_t unit)
{
    if( unit>=MACHINE_TIMER_UNIT_MAX )
        return false;
    return true;
}

STATIC bool check_timer(uint32_t timer)
{
    if( timer>=TIMER_DEVICE_MAX )
        return false;
    return true;
}

STATIC bool check_channel(uint32_t channel)
{
    if( channel>=TIMER_CHANNEL_MAX )
        return false;
    return true;
}

STATIC bool check_div(uint32_t div)
{

    if( div>255 )
        return false;
    return true;
}

STATIC bool check_priority(uint32_t priority)
{
    if( (priority<1) || (priority>7) )
        return false;
    return true;
}

/**
 * 
 * timerclksel=pll0clk/2 
 * timerclksel*period(unit:s) should < 2^32 and >=1
 */
STATIC bool check_period_setting(uint32_t period, uint32_t unit, uint32_t div)
{
    size_t T = 0;
    uint32_t freq_pll0;
    double freq_timer = 0;
    double counter = 0;


    if( period>=UINT32_MAX )
        return false;
    if( !check_div(div) ||
        !check_unit(unit)
    )
        return false;
    
    T = (size_t)(period * pow(1000,unit)); //time us
    freq_pll0 = sysctl_clock_get_freq(SYSCTL_CLOCK_PLL0);
    freq_timer = freq_pll0/pow(2,div+1);
    counter = freq_timer*(T/1e9);
    if(counter<1 || counter>UINT32_MAX)
        return false;
    return true;
}


STATIC void machine_timer_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_timer_obj_t *self = self_in;
    char* unit;
    if (self->unit == MACHINE_TIMER_UNIT_MS)
        unit = "ms";
    else if(self->unit == MACHINE_TIMER_UNIT_S)
        unit = "s";
    else if(self->unit == MACHINE_TIMER_UNIT_US)
        unit = "us";
    else
        unit = "ns";
    mp_printf(print, 
        "[MAIXPY]Timer:(%p) timer=%d, channel=%d, mode=%d, period=%d%s, priority=%d, div=%d, callback=%p, arg=%p",
        self, self->timer, self->channel, self->mode, self->period, unit, self->priority, self->div, self->callback, self->arg);
}

STATIC void machine_timer_disable(machine_timer_obj_t *self) {
	timer_disable(self->timer, self->channel);
	plic_irq_disable(IRQN_TIMER0A_INTERRUPT + 2*self->timer + self->channel/2);
	plic_irq_unregister(IRQN_TIMER0A_INTERRUPT + 2*self->timer + self->channel/2);
    // timer_set_enable(self->timer, self->channel, 0);
    // timer_irq_unregister(self->timer, self->channel);
    //TODO: just unregister callback or disable timer?
    //       why official csdk didn't disable plic irq, just mask timer interrupt?
	self->active = true;
}

STATIC int machine_timer_isr(void *self_in) {
    machine_timer_obj_t *self = self_in;
    if(self->mode == MACHINE_TIMER_MODE_ONE_SHOT)
        self->active = false;
	debug_print("[MAIXPY]Timer:enter machine_timer_isr\n");
	// timer_channel_clear_interrupt(self->timer,self->channel); //don't need clean	
	debug_print("[MAIXPY]Timer:self->callback = %p\n",self->callback);
	mp_obj_type_t *type = mp_obj_get_type(self->callback);
	debug_print("[MAIXPY]Timer:type->call = %p\n",type->call);
	if(type != NULL)
	{
            mp_sched_schedule(self->callback, self);
            mp_hal_wake_main_task_from_isr();
            // mp_call_function_2(self->callback, MP_OBJ_FROM_PTR(self), self->arg);
	}
	else
	{
		debug_print("[MAIXPY]Timer: callback type == NULL\n");
	}
	debug_print("[MAIXPY]Timer:quit machine_timer_isr\n");
	return 0;
}

// STATIC void machine_timer_enable(machine_timer_obj_t *self) {
// 	debug_print("[MAIXPY]TIMER:self->timer=%d,self->channel=%d\n",self->timer,self->channel);
// 	if(self->channel == 0 || 1 == self->channel)
// 	{
// 		debug_print("[MAIXPY]TIMER:start init plic timer channel 0 and 1\n");
// 		plic_set_priority(IRQN_TIMER0A_INTERRUPT + self->timer*2, 1);
// 		plic_irq_enable(IRQN_TIMER0A_INTERRUPT + self->timer*2);
// 		plic_irq_register(IRQN_TIMER0A_INTERRUPT + self->timer*2, machine_timer_isr, (void*)self);
// 	}
// 	else if(self->channel == 2 || 3 == self->channel)
// 	{
// 		debug_print("[MAIXPY]TIMER:start init plic timer channel 2 and 3\n");
// 		plic_set_priority(IRQN_TIMER0B_INTERRUPT + self->timer*2, 1);
// 		plic_irq_enable(IRQN_TIMER0B_INTERRUPT + self->timer*2);
// 		plic_irq_register(IRQN_TIMER0B_INTERRUPT + self->timer*2, machine_timer_isr, (void*)self);
// 	}
// 	//timer_set_mode(self->timer, self->channel, TIMER_CR_USER_MODE);
// 	timer_set_reload(self->timer, self->channel, self->period);
// 	timer_enable_interrupt(self->timer, self->channel);
// 	timer_set_enable(self->timer, self->channel,1);
// 	self->active = 0;
// 	debug_print("[MAIXPY]TIMER:quit %s\n",__FUNCTION__);
// }

STATIC mp_obj_t machine_timer_init_helper(machine_timer_obj_t *self, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    size_t T;
    int is_single_shot = 0;
    bool is_start;

    enum {
        ARG_mode,
        ARG_period,
        ARG_unit,
        ARG_callback,
        ARG_arg,
        ARG_start,
        ARG_priority,
        ARG_div
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode,          MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MACHINE_TIMER_MODE_ONE_SHOT} },
        { MP_QSTR_period,        MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1000} },                  // default 1000ms
        { MP_QSTR_unit,          MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MACHINE_TIMER_UNIT_MS} },
        { MP_QSTR_callback,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_arg,         MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_start,         MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = true} },
        { MP_QSTR_priority,      MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_div,           MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} }, // default div=0: timerclksel=aclk/2 
    };
    //machine_timer_disable(self);self->clk_freq
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    self->mode = args[ARG_mode].u_int;
    self->period = args[ARG_period].u_int;
    self->unit = args[ARG_unit].u_int;
    self->callback = args[ARG_callback].u_obj;
    self->arg = args[ARG_arg].u_obj;
    is_start = args[ARG_start].u_bool;
    self->priority = args[ARG_priority].u_int;
    self->div = args[ARG_div].u_int;
    //check parameters
    if( !check_mode(self->mode) )
        mp_raise_ValueError("[MAIXPY]Timer:Invalid mode");
    if( self->mode != MACHINE_TIMER_MODE_PWM)
    {
        if( !mp_obj_is_callable(self->callback) )
            mp_raise_ValueError("[MAIXPY]Timer:Invalid callback");
        if( !check_priority(self->priority) )
            mp_raise_ValueError("[MAIXPY]Timer:Invalid priority");
    }
    if( !check_period_setting(self->period, self->unit, self->div))
    {
        // m_del_obj(machine_timer_obj_t, self);//TODO:
		mp_raise_ValueError("[MAIXPY]Timer:Invalid period or div");
    }

	/*timer prescale*/
    timer_set_clock_div(self->timer,self->div);

    if( self->mode == MACHINE_TIMER_MODE_PWM)
    {
        return mp_const_none;
    }
    /* Init timer */
    timer_init(self->timer);
    /* Set timer interval */
    T = (size_t)(self->period * pow(1000,self->unit));
    timer_set_interval(self->timer, self->channel, T);
    /* Set timer callback function with repeat method */
    is_single_shot = (self->mode == MACHINE_TIMER_MODE_ONE_SHOT)?1:0;
    timer_irq_register(self->timer, self->channel, is_single_shot , self->priority, machine_timer_isr, (void*)self);
    /* Enable timer */
    timer_set_enable(self->timer, self->channel, is_start?1:0);
    if(is_start)
        self->active = true;
    return mp_const_none;
}

STATIC mp_obj_t machine_timer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 2, 9, true);
    machine_timer_obj_t *self = m_new_obj(machine_timer_obj_t);
    self->base.type = &machine_timer_type;

    self->timer = mp_obj_get_int(args[0]);
    self->channel = mp_obj_get_int(args[1]);
    if(!check_timer(self->timer) || !check_channel(self->channel))
    {
        // m_del_obj(machine_timer_obj_t, self);//TODO:
        mp_raise_ValueError("[MAIXPY]Timer:Invalid timer");
    }
	mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
	machine_timer_init_helper(self, n_args-2, args+2, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t machine_timer_deinit(mp_obj_t self_in) {
    machine_timer_disable(self_in);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_deinit_obj, machine_timer_deinit);

STATIC mp_obj_t machine_timer_init(mp_uint_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return machine_timer_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_timer_init_obj, 3, machine_timer_init);


STATIC mp_obj_t machine_timer_callbcak(mp_obj_t self_in,mp_obj_t func_in) {
    machine_timer_obj_t *self = self_in;
	
    self->callback = func_in;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_timer_callback_obj, machine_timer_callbcak);



// STATIC mp_obj_t machine_timer_value(mp_obj_t self_in) {
//     machine_timer_obj_t *self = self_in;
//     mp_uint_t result = 0;

//     result = timer_get_count(self->timer,self->channel);

//    return MP_OBJ_NEW_SMALL_INT((mp_uint_t)(result));
// }
// STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_value_obj, machine_timer_value);

STATIC mp_obj_t machine_timer_period(size_t n_args, const mp_obj_t *args) {
    machine_timer_obj_t *self = args[0];
    size_t T;

    if( n_args == 1)//get period
    {
        return mp_obj_new_int(self->period);
    }
    if( n_args == 3)
    {
        if(!check_unit(mp_obj_get_int(args[2])))
            mp_raise_ValueError("[MAIXPY]Timer:unit error");
        self->unit = mp_obj_get_int(args[2]);
    }
    self->period = mp_obj_get_int(args[1]);
    T = (size_t)(self->period * pow(1000,self->unit));
    timer_set_interval(self->timer, self->channel, T);
	// timer_disable(self->timer, self->channel);
	// timer_set_reload(self->timer, self->channel, self->period);
	// timer_set_enable(self->timer, self->channel,1);
    return mp_obj_new_int(self->period);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_timer_period_obj, 1, 3, machine_timer_period);

STATIC mp_obj_t machine_timer_stop(mp_obj_t self_in) {
    machine_timer_obj_t *self = self_in;

	timer_disable(self->timer, self->channel);
	self->active = false;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_stop_obj, machine_timer_stop);


STATIC mp_obj_t machine_timer_restart(mp_obj_t self_in) {
    machine_timer_obj_t *self = self_in;
    size_t T;

	timer_disable(self->timer, self->channel);
    T = (size_t)(self->period * pow(1000,self->unit));
	timer_set_interval(self->timer, self->channel, T);
	timer_set_enable(self->timer, self->channel,1);
    self->active = true;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_restart_obj, machine_timer_restart);

STATIC mp_obj_t machine_timer_callback_arg(mp_obj_t self_in) {
    machine_timer_obj_t *self = self_in;
    return self->arg;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_callback_arg_obj, machine_timer_callback_arg);

STATIC mp_obj_t machine_timer_start(mp_obj_t self_in) {
    machine_timer_obj_t *self = self_in;
	if(self->active == 0)
		timer_set_enable(self->timer, self->channel,1);
	self->active = 1;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_start_obj, machine_timer_start);

// STATIC mp_obj_t machine_timer_freq(mp_obj_t self_in,mp_obj_t freq) {
//     machine_timer_obj_t *self = self_in;
//     mp_uint_t result = 0;
// 	if (MP_OBJ_IS_SMALL_INT((freq))) 
// 		self->freq = MP_OBJ_SMALL_INT_VALUE(freq);
// 	else
// 	{
// 		mp_printf(&mp_plat_print, "[MAIXPY]TIMER:type error\n");
// 		return mp_const_none;
// 	}
// 	timer_disable(self->timer, self->channel);
// 	self->period = (uint32_t)(self->clk_freq / self->freq);
// 	timer_set_reload(self->timer, self->channel, self->period);
// 	timer_set_enable(self->timer, self->channel,1);
// 	result = self->freq;
//     return MP_OBJ_NEW_SMALL_INT((mp_uint_t)(result * 1000));  // value in ms
// }
// STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_timer_freq_obj, machine_timer_freq);

STATIC const mp_rom_map_elem_t machine_timer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&machine_timer_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_timer_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_timer_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_callback), MP_ROM_PTR(&machine_timer_callback_obj) },
    { MP_ROM_QSTR(MP_QSTR_period), MP_ROM_PTR(&machine_timer_period_obj) },
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&machine_timer_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&machine_timer_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_restart), MP_ROM_PTR(&machine_timer_restart_obj) },
    { MP_ROM_QSTR(MP_QSTR_callback_arg), MP_ROM_PTR(&machine_timer_callback_arg_obj) },
    // { MP_ROM_QSTR(MP_QSTR_time_left), MP_ROM_PTR(&machine_timer_time_left_obj) },
    // { MP_ROM_QSTR(MP_QSTR_freq), MP_ROM_PTR(&machine_timer_freq_obj) },
    // { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&machine_timer_value_obj) },

    //TODO: soft timer of OS, param as -1
    // { MP_ROM_QSTR(MP_QSTR_TIMER_SOFT), MP_ROM_INT(-1) },
    { MP_ROM_QSTR(MP_QSTR_TIMER0), MP_ROM_INT(TIMER_DEVICE_0) },
    { MP_ROM_QSTR(MP_QSTR_TIMER1), MP_ROM_INT(TIMER_DEVICE_1) },
    { MP_ROM_QSTR(MP_QSTR_TIMER2), MP_ROM_INT(TIMER_DEVICE_2) },
    { MP_ROM_QSTR(MP_QSTR_CHANNEL0), MP_ROM_INT(TIMER_CHANNEL_0) },
    { MP_ROM_QSTR(MP_QSTR_CHANNEL1), MP_ROM_INT(TIMER_CHANNEL_1) },
    { MP_ROM_QSTR(MP_QSTR_CHANNEL2), MP_ROM_INT(TIMER_CHANNEL_2) },
    { MP_ROM_QSTR(MP_QSTR_CHANNEL3), MP_ROM_INT(TIMER_CHANNEL_3) },
    { MP_ROM_QSTR(MP_QSTR_MODE_ONE_SHOT), MP_ROM_INT(MACHINE_TIMER_MODE_ONE_SHOT) },
    { MP_ROM_QSTR(MP_QSTR_MODE_PERIODIC), MP_ROM_INT(MACHINE_TIMER_MODE_PERIODIC) },
    { MP_ROM_QSTR(MP_QSTR_MODE_PWM),      MP_ROM_INT(MACHINE_TIMER_MODE_PWM)      },
    { MP_ROM_QSTR(MP_QSTR_UNIT_S),  MP_ROM_INT(MACHINE_TIMER_UNIT_S)  },
    { MP_ROM_QSTR(MP_QSTR_UNIT_MS), MP_ROM_INT(MACHINE_TIMER_UNIT_MS) },
    { MP_ROM_QSTR(MP_QSTR_UNIT_US), MP_ROM_INT(MACHINE_TIMER_UNIT_US) },
    { MP_ROM_QSTR(MP_QSTR_UNIT_NS), MP_ROM_INT(MACHINE_TIMER_UNIT_NS) },

};
STATIC MP_DEFINE_CONST_DICT(machine_timer_locals_dict, machine_timer_locals_dict_table);

const mp_obj_type_t machine_timer_type = {
    { &mp_type_type },
    .name = MP_QSTR_Timer,
    .print = machine_timer_print,
    .make_new = machine_timer_make_new,
    .locals_dict = (mp_obj_t)&machine_timer_locals_dict,
};
