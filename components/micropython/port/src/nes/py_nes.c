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
#include "fpioa.h"
#include "ps2.h"



int nes_stick=0;
int nes_volume=5;  //0~8
int nes_cycle_us=0;  //60fps,  63us per cycle
int repeat_n = 16;

mp_obj_t py_nes_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
	int cs_num, mosi_num, miso_num, clk_num;

	enum {  ARG_rc_type,
			ARG_CS,
			ARG_MOSI,
			ARG_MISO,
			ARG_CLK,
			ARG_repeat,
			ARG_vol
        };

    const mp_arg_t machine_nes_init_allowed_args[] = {
        { MP_QSTR_rc_type,    	MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_cs,     	 	MP_ARG_INT|MP_ARG_KW_ONLY, {.u_int = -1} },
		{ MP_QSTR_mosi,			MP_ARG_INT|MP_ARG_KW_ONLY, {.u_int = -1} },
		{ MP_QSTR_miso,			MP_ARG_INT|MP_ARG_KW_ONLY, {.u_int = -1} },
		{ MP_QSTR_clk,     	 	MP_ARG_INT|MP_ARG_KW_ONLY, {.u_int = -1} },
		{ MP_QSTR_repeat,     	MP_ARG_INT|MP_ARG_KW_ONLY, {.u_int = 16} },
		{ MP_QSTR_vol,     		MP_ARG_INT|MP_ARG_KW_ONLY, {.u_int = 5} }
    };

    mp_arg_val_t args_parsed[MP_ARRAY_SIZE(machine_nes_init_allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(machine_nes_init_allowed_args), machine_nes_init_allowed_args, args_parsed);

	//0->uart, 1->ps2
	nes_stick = args_parsed[ARG_rc_type].u_int;
	repeat_n  = args_parsed[ARG_repeat].u_int;
	nes_volume = args_parsed[ARG_vol].u_int;

	if(nes_stick == 1)
	{
		cs_num = args_parsed[ARG_CS].u_int;
		if(cs_num == -1 || cs_num > FUNC_GPIOHS31 || cs_num < FUNC_GPIOHS0)
		{
			mp_raise_ValueError("CS value error");
			return mp_const_false;
		}

		mosi_num = args_parsed[ARG_MOSI].u_int;
		if(mosi_num == -1 || mosi_num > FUNC_GPIOHS31 || mosi_num < FUNC_GPIOHS0)
		{
			mp_raise_ValueError("MOSI value error");
			return mp_const_false;
		}

		miso_num = args_parsed[ARG_MISO].u_int;
		if(miso_num == -1 || miso_num > FUNC_GPIOHS31 || miso_num < FUNC_GPIOHS0)
		{
			mp_raise_ValueError("MISO value error");
			return mp_const_false;
		}

		clk_num = args_parsed[ARG_CLK].u_int;
		if(clk_num == -1 || clk_num > FUNC_GPIOHS31 || clk_num < FUNC_GPIOHS0)
		{
			mp_raise_ValueError("CLK value error");
			return mp_const_false;
		}

		PS2X_confg_io(cs_num - FUNC_GPIOHS0, clk_num - FUNC_GPIOHS0, mosi_num - FUNC_GPIOHS0, miso_num - FUNC_GPIOHS0);

		int err = 0;
		uint8_t type;

		err = PS2X_config_gamepad(0, 0);
		if (err == 0)
		{
			mp_printf(&mp_plat_print, "Found Controller, configured successful \r\n");
		}
		else if (err == 1)
		{
			mp_printf(&mp_plat_print, "No controller found, check wiring. \r\n");
			return mp_const_false;
		}
		else if (err == 2)
		{
			mp_printf(&mp_plat_print, "Controller found but not accepting commands. \r\n");
			return mp_const_false;
		}
		else if (err == 3)
		{
			mp_printf(&mp_plat_print, "Controller refusing to enter Pressures mode, may not support it. \r\n");
			return mp_const_false;
		}
		else
		{
			mp_raise_OSError(MP_EFAULT);
			return mp_const_false;
		}
		
		type = PS2X_readType();
		switch (type)
		{
		case 0:
			mp_printf(&mp_plat_print, "Unknown Controller type found \r\n");
			break;
		case 1:
			mp_printf(&mp_plat_print, "DualShock Controller found \r\n");
			break;
		case 2:
			mp_printf(&mp_plat_print, "GuitarHero Controller found \r\n");
			mp_raise_OSError(MP_EFAULT);
			return mp_const_false;
			break;
		case 3:
			mp_printf(&mp_plat_print, "Wireless Sony DualShock Controller found \r\n");
			break;
		}
	}

	lcd_set_direction(DIR_YX_RLDU|0x08);  //RLDU
	//we DO NOT initialize here for we want user to set in python layer
	lcd_clear(BLACK);

    return mp_const_none;
}

static mp_obj_t py_nes_run(mp_obj_t path_obj)
{
    const char *path = mp_obj_str_get_str(path_obj);
    mp_printf(&mp_plat_print, "path: %s\n", path);
	if(InfoNES_Load(path) == 0)
	{
		InfoNES_Main();
	}	
	
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_nes_init_obj, 1, py_nes_init);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_nes_run_obj, py_nes_run);


static const mp_map_elem_t globals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_nes) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_init),   (mp_obj_t)&py_nes_init_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_run),   (mp_obj_t)&py_nes_run_obj },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_JOYSTICK),   MP_ROM_INT(1) },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_KEYBOARD),   MP_ROM_INT(0) },
};

STATIC MP_DEFINE_CONST_DICT(globals_dict, globals_dict_table);

const mp_obj_module_t nes_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_t)&globals_dict,
};

