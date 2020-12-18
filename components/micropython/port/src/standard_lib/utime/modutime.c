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
#include <stdint.h>
#include <time.h>
#include <stdlib.h>

#include "encoding.h"
#include "sysctl.h"
#include "stdio.h"
#include "sleep.h"
#include "rtc.h"

#include "py/runtime.h"
#include "extmod/utime_mphal.h"
#include "lib/timeutils/timeutils.h"
#include "mphalport.h"

/* Clock Type */
typedef struct _py_clock_obj_t {
    mp_obj_base_t base;
    uint32_t t_start;
    uint32_t t_ticks;
    uint32_t t_frame;
} py_clock_obj_t;


STATIC mp_obj_t time_localtime(size_t n_args, const mp_obj_t *args) {
    timeutils_struct_time_t tm;
	mp_int_t seconds;
    if (n_args == 0 || args[0] == mp_const_none) {
		uint32_t year = 0;uint32_t mon = 0;uint32_t mday = 0;uint32_t hour = 0;
		uint32_t min = 0;uint32_t sec = 0;uint32_t wday = 0;uint32_t yday = 0;
		rtc_timer_get((int*)&year, (int*)&mon, (int*)&mday, (int*)&hour, (int*)&min, (int*)&sec);
		wday = rtc_get_wday(year,mon,mday);
		yday = rtc_get_yday(year,mon,mday);
		tm.tm_year = (uint16_t)year;
		tm.tm_mon = (uint8_t)mon;
		tm.tm_mday = (uint8_t)mday;
		tm.tm_hour = (uint8_t)hour;
		tm.tm_min = (uint8_t)min;
		tm.tm_sec = (uint8_t)sec;
		tm.tm_wday = (uint8_t)wday;
		tm.tm_yday = (uint8_t)yday;
    } else {
        seconds = mp_obj_get_int(args[0]);
		timeutils_seconds_since_2000_to_struct_time(seconds, &tm);
    }
    mp_obj_t tuple[8] = {
        tuple[0] = mp_obj_new_int(tm.tm_year),
        tuple[1] = mp_obj_new_int(tm.tm_mon),
        tuple[2] = mp_obj_new_int(tm.tm_mday),
        tuple[3] = mp_obj_new_int(tm.tm_hour),
        tuple[4] = mp_obj_new_int(tm.tm_min),
        tuple[5] = mp_obj_new_int(tm.tm_sec),
        tuple[6] = mp_obj_new_int(tm.tm_wday),
        tuple[7] = mp_obj_new_int(tm.tm_yday),
    };
    return mp_obj_new_tuple(8, tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(time_localtime_obj, 0, 1, time_localtime);

STATIC mp_obj_t time_mktime(mp_obj_t tuple) {
    size_t len;
    mp_obj_t *elem;
    mp_obj_get_array(tuple, &len, &elem);

    // localtime generates a tuple of len 8. CPython uses 9, so we accept both.
    if (len < 8 || len > 9) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "mktime needs a tuple of length 8 or 9 (%d given)", len));
    }

    return mp_obj_new_int_from_uint(timeutils_mktime(mp_obj_get_int(elem[0]),
            mp_obj_get_int(elem[1]), mp_obj_get_int(elem[2]), mp_obj_get_int(elem[3]),
            mp_obj_get_int(elem[4]), mp_obj_get_int(elem[5])));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(time_mktime_obj, time_mktime);

STATIC mp_obj_t time_time(void) {
    mp_uint_t seconds;
	volatile int year = 0, mon = 0, mday = 0, hour = 0;
	volatile int min = 0, sec = 0;
	rtc_timer_get((int*)&year, (int*)&mon, (int*)&mday, (int*)&hour, (int*)&min, (int*)&sec);
	seconds = timeutils_seconds_since_2000(year,mon, mday, hour, min,sec);
    return mp_obj_new_int_from_uint(seconds);
}
MP_DEFINE_CONST_FUN_OBJ_0(time_time_obj, time_time);

STATIC mp_obj_t time_set_time(mp_obj_t tuple) {
	size_t len;
	mp_obj_t *elem;
    mp_obj_get_array(tuple, &len, &elem);
	bool flag = rtc_timer_set(mp_obj_get_int(elem[0]),mp_obj_get_int(elem[1]),mp_obj_get_int(elem[2]),
				  mp_obj_get_int(elem[3]),mp_obj_get_int(elem[4]),mp_obj_get_int(elem[5]));
	if(0 == flag) 
		return mp_const_true;
	else
		return mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(time_set_time_obj, time_set_time);

/////////////////////////////////////////////////////////////////////////////
// For Openmv compatible
mp_obj_t py_clock_tick(mp_obj_t clock_obj)
{
    py_clock_obj_t *clock = (py_clock_obj_t*) clock_obj;
    clock->t_start = systick_current_millis();
    return mp_const_none;
}

mp_obj_t py_clock_fps(mp_obj_t clock_obj)
{
    py_clock_obj_t *clock = (py_clock_obj_t*) clock_obj;
    clock->t_frame++;
    clock->t_ticks += (systick_current_millis()-clock->t_start);
    float fps = 1000.0f / (clock->t_ticks/(float)clock->t_frame);
    if (clock->t_ticks >= 2000) {
        // Reset the FPS clock every 2s
        clock->t_frame = 0;
        clock->t_ticks = 0;
    }
    return mp_obj_new_float(fps);
}

mp_obj_t py_clock_avg(mp_obj_t clock_obj)
{
    py_clock_obj_t *clock = (py_clock_obj_t*) clock_obj;
    clock->t_frame++;
    clock->t_ticks += (systick_current_millis()-clock->t_start);
    return mp_obj_new_float(clock->t_ticks/(float)clock->t_frame);
}

mp_obj_t py_clock_reset(mp_obj_t clock_obj)
{
    py_clock_obj_t *clock = (py_clock_obj_t*) clock_obj;
    clock->t_start = 0;
    clock->t_ticks = 0;
    clock->t_frame = 0;
    return mp_const_none;
}

static void py_clock_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_clock_obj_t *self = self_in;

    /* print some info */
    mp_printf(print, "t_start:%d t_ticks:%d t_frame:%d\n",
            self->t_start, self->t_ticks, self->t_frame);
}

static MP_DEFINE_CONST_FUN_OBJ_1(py_clock_tick_obj,  py_clock_tick);
static MP_DEFINE_CONST_FUN_OBJ_1(py_clock_fps_obj,   py_clock_fps);
static MP_DEFINE_CONST_FUN_OBJ_1(py_clock_avg_obj,   py_clock_avg);
static MP_DEFINE_CONST_FUN_OBJ_1(py_clock_reset_obj, py_clock_reset);

static const mp_map_elem_t locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_tick),   (mp_obj_t)&py_clock_tick_obj},
    { MP_OBJ_NEW_QSTR(MP_QSTR_fps),    (mp_obj_t)&py_clock_fps_obj},
    { MP_OBJ_NEW_QSTR(MP_QSTR_avg),    (mp_obj_t)&py_clock_avg_obj},
    { MP_OBJ_NEW_QSTR(MP_QSTR_reset),  (mp_obj_t)&py_clock_reset_obj},
    { NULL, NULL },
};

STATIC MP_DEFINE_CONST_DICT(locals_dict, locals_dict_table);

static const mp_obj_type_t py_clock_type = {
    { &mp_type_type },
    .name  = MP_QSTR_clock,
    .print = py_clock_print,
    .locals_dict = (mp_obj_t)&locals_dict,
};


static mp_obj_t py_time_clock()
{
    py_clock_obj_t *clock =NULL;
    clock = m_new_obj(py_clock_obj_t);
    clock->base.type = &py_clock_type;
    clock->t_start = 0;
    clock->t_ticks = 0;
    clock->t_frame = 0;

    return clock;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_time_clock_obj, py_time_clock);
// For Openmv compatible end
/////////////////////////////////////////////////////////////////////////////

STATIC const mp_rom_map_elem_t time_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_utime) },

    { MP_ROM_QSTR(MP_QSTR_localtime), MP_ROM_PTR(&time_localtime_obj) },
    { MP_ROM_QSTR(MP_QSTR_mktime), MP_ROM_PTR(&time_mktime_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_time), MP_ROM_PTR(&time_set_time_obj) },
    { MP_ROM_QSTR(MP_QSTR_time), MP_ROM_PTR(&time_time_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep), MP_ROM_PTR(&mp_utime_sleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep_ms), MP_ROM_PTR(&mp_utime_sleep_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep_us), MP_ROM_PTR(&mp_utime_sleep_us_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_ms), MP_ROM_PTR(&mp_utime_ticks_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_us), MP_ROM_PTR(&mp_utime_ticks_us_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_cpu), MP_ROM_PTR(&mp_utime_ticks_cpu_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_add), MP_ROM_PTR(&mp_utime_ticks_add_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_diff), MP_ROM_PTR(&mp_utime_ticks_diff_obj) },

    // openmv compatible
    { MP_OBJ_NEW_QSTR(MP_QSTR_ticks),   MP_ROM_PTR(&mp_utime_ticks_ms_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_clock),   MP_ROM_PTR(&py_time_clock_obj) },

};

STATIC MP_DEFINE_CONST_DICT(time_module_globals, time_module_globals_table);

const mp_obj_module_t utime_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&time_module_globals,
};

