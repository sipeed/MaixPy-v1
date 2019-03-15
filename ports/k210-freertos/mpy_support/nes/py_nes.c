/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Time Python module.
 *
 */
#include <mp.h>
#include <objstr.h>
#include <spi.h>
#include "imlib.h"
#include "fb_alloc.h"
#include "vfs_wrapper.h"
#include "py_assert.h"
#include "py_helper.h"
#include "py_image.h"
#include "lcd.h"
#include "sleep.h"
/*****freeRTOS****/
#include "FreeRTOS.h"
#include "task.h"
#include "InfoNES_System.h"
#include "InfoNES.h"
#include "sysctl.h"
#include "myspi.h"

#if MAIXPY_NES_EMULATOR_SUPPORT

#define printf(...)
extern uint8_t g_dvp_buf[];

#define LCD_W 320
#define LCD_H 240

int nes_stick=0;
int nes_volume=8;  //0~8
int nes_cycle_us=0;  //60fps,  63us per cycle
int repeat_n = 16;

mp_obj_t py_nes_init(uint n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
	enum {  ARG_rc_type,
            ARG_repeat
        };
    const mp_arg_t machine_nes_init_allowed_args[] = {
        { MP_QSTR_rc_type,    MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_repeat,     MP_ARG_INT|MP_ARG_KW_ONLY, {.u_int = 16} }
    };
    mp_arg_val_t args_parsed[MP_ARRAY_SIZE(machine_nes_init_allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args,
        MP_ARRAY_SIZE(machine_nes_init_allowed_args), machine_nes_init_allowed_args, args_parsed);
	//0->uart, 1->ps2
	nes_stick = args_parsed[ARG_rc_type].u_int;
	repeat_n  = args_parsed[ARG_repeat].u_int;
	lcd_init(20000000);
	lcd_set_direction(DIR_YX_RLDU|0x08);  //RLDU
	//we DO NOT initialize here for we want user to set in python layer
	lcd_clear(BLACK);
	if(nes_stick == 1)
	{
		fpioa_set_function(20, FUNC_GPIOHS0 + SS_GPIONUM);   //ss
		fpioa_set_function(19, FUNC_GPIOHS0 + SCLK_GPIONUM); //clk
		fpioa_set_function(21, FUNC_GPIOHS0 + MOSI_GPIONUM); //mosi
		fpioa_set_function(18, FUNC_GPIOHS0 + MISO_GPIONUM); //miso
		soft_spi_init();
		ps2_mode_config();
	}
    return mp_const_none;
}

static mp_obj_t py_nes_run(mp_obj_t path_obj)
{
    const char *path = mp_obj_str_get_str(path_obj);
    printf("path: %s\n", path);
	if(InfoNES_Load(path) == 0)
	{
		InfoNES_Main();
	}	
	
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_nes_init_obj, 1, py_nes_init);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_nes_run_obj, py_nes_run);

#endif //MAIXPY_NES_EMULATOR_SUPPORT



static const mp_map_elem_t globals_dict_table[] = {

#if MAIXPY_NES_EMULATOR_SUPPORT	
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_nes) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_init),   (mp_obj_t)&py_nes_init_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_run),   (mp_obj_t)&py_nes_run_obj },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_JOTSTICK),   MP_ROM_INT(1) },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_KEYBOARD),   MP_ROM_INT(0) },
#endif

};

STATIC MP_DEFINE_CONST_DICT(globals_dict, globals_dict_table);

const mp_obj_module_t nes_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_t)&globals_dict,
};

