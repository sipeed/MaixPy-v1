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

static mp_obj_t py_nes_init(mp_obj_t stick_obj)//, mp_obj_t wait_obj, mp_obj_t audio_obj)
{
	//0->uart, 1->ps2
	nes_stick = mp_obj_get_int(stick_obj);
	//wait_us = mp_obj_get_int(wait_obj);
	//audio_turn = mp_obj_get_int(audio_obj);
	// backlight_init = false;
	fpioa_set_function(37, FUNC_GPIOHS0 + RST_GPIONUM);
	fpioa_set_function(38, FUNC_GPIOHS0 + DCX_GPIONUM);
	fpioa_set_function(36, FUNC_SPI0_SS3);
	fpioa_set_function(39, FUNC_SPI0_SCLK);
	// lcd_init();
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

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_nes_init_obj, py_nes_init);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_nes_run_obj, py_nes_run);

#endif //MAIXPY_NES_EMULATOR_SUPPORT



static const mp_map_elem_t globals_dict_table[] = {

#if MAIXPY_NES_EMULATOR_SUPPORT	
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_nes) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_init),   (mp_obj_t)&py_nes_init_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_run),   (mp_obj_t)&py_nes_run_obj },
#endif

};

STATIC MP_DEFINE_CONST_DICT(globals_dict, globals_dict_table);

const mp_obj_module_t nes_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_t)&globals_dict,
};

