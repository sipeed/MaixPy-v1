/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * LCD Python module.
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

#include "sysctl.h"


// extern uint8_t g_lcd_buf[];

#define LCD_W 320
#define LCD_H 240

// extern mp_obj_t pyb_spi_send(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
// extern mp_obj_t pyb_spi_make_new(mp_obj_t type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args);
// extern mp_obj_t pyb_spi_deinit(mp_obj_t self_in);

// static mp_obj_t spi_port = NULL;
static int width = 0;
static int height = 0;
static enum { LCD_NONE, LCD_SHIELD } type = LCD_NONE;
// static bool backlight_init = false;

// Send out 8-bit data using the SPI object.
static void lcd_write_command_byte(uint8_t data_byte)
{
    tft_set_datawidth(8);
	tft_write_command(data_byte);
	return;
}

// Send out 8-bit data using the SPI object.
static void lcd_write_data_byte(uint8_t data_byte)
{
    uint8_t temp_type;
    temp_type = data_byte;
    tft_set_datawidth(8);
    tft_write_byte(&temp_type, 1);
	return;
}

// Send out 8-bit data using the SPI object.
static void lcd_write_command(uint8_t data_byte, uint32_t len, uint8_t *dat)
{
    tft_set_datawidth(8);
	tft_write_command(data_byte);
    tft_write_byte(dat, len);
	return;
}

// Send out 8-bit data using the SPI object.
static void lcd_write_data(uint32_t len, uint8_t *dat)
{
    tft_write_byte(dat, len);
	return;
}

static mp_obj_t py_lcd_deinit()
{
    switch (type) {
        case LCD_NONE:
            return mp_const_none;
        case LCD_SHIELD:
            width = 0;
            height = 0;
            type = LCD_NONE;
            return mp_const_none;
    }
    return mp_const_none;
}

static mp_obj_t py_lcd_init(uint n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
	uint16_t color = BLACK;
    py_lcd_deinit();
	enum { 
		ARG_type,
        ARG_freq,
		ARG_color
    };
    static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_type, MP_ARG_INT, {.u_int = LCD_SHIELD} },
        { MP_QSTR_freq, MP_ARG_INT, {.u_int = 15000000} },
		{ MP_QSTR_color, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} }
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
	if(args[ARG_color].u_obj != MP_OBJ_NULL)
	{
		if (mp_obj_is_integer(args[ARG_color].u_obj)) {
			color = mp_obj_get_int(args[ARG_color].u_obj);
		} else {
			mp_obj_t *arg_color;
			mp_obj_get_array_fixed_n(args[ARG_color].u_obj, 3, &arg_color);
			color = COLOR_R8_G8_B8_TO_RGB565(IM_MAX(IM_MIN(mp_obj_get_int(arg_color[0]), COLOR_R8_MAX), COLOR_R8_MIN),
													IM_MAX(IM_MIN(mp_obj_get_int(arg_color[1]), COLOR_G8_MAX), COLOR_G8_MIN),
													IM_MAX(IM_MIN(mp_obj_get_int(arg_color[2]), COLOR_B8_MAX), COLOR_B8_MIN));
			color = color<<8 | (color>>8 & 0xFF);
		}
	}

	switch (args[ARG_type].u_int) {
		case LCD_NONE:
			return mp_const_none;
		case LCD_SHIELD:
		{
			width = LCD_W;
			height = LCD_H;
			type = LCD_SHIELD;
			// backlight_init = false;
			fpioa_set_function(37, FUNC_GPIOHS0 + RST_GPIONUM);
			fpioa_set_function(38, FUNC_GPIOHS0 + DCX_GPIONUM);
			fpioa_set_function(36, FUNC_SPI0_SS3);
			fpioa_set_function(39, FUNC_SPI0_SCLK);
			lcd_init(args[ARG_freq].u_int);
			lcd_clear(color);
			return mp_const_none;
        }
    }
    return mp_const_none;

}

static mp_obj_t py_lcd_direction(mp_obj_t dir_obj)
{
	int dir = mp_obj_get_int(dir_obj);
	lcd_set_direction(dir);
	return mp_const_none;
}

static mp_obj_t py_lcd_width()
{
    if (type == LCD_NONE) return mp_const_none;
    return mp_obj_new_int(width);
}

static mp_obj_t py_lcd_height()
{
    if (type == LCD_NONE) return mp_const_none;
    return mp_obj_new_int(height);
}

static mp_obj_t py_lcd_type()
{
    if (type == LCD_NONE) return mp_const_none;
    return mp_obj_new_int(type);
}

static mp_obj_t py_lcd_set_backlight(mp_obj_t state_obj)
{
    return mp_const_none;
}

static mp_obj_t py_lcd_get_backlight()
{
    return mp_const_none;
}

static mp_obj_t py_lcd_display(uint n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_image_cobj(args[0]);
    PY_ASSERT_TRUE_MSG(IM_IS_MUTABLE(arg_img), "Image format is not supported.");

    rectangle_t rect;
    uint16_t x,y;
    point_t oft;
	int is_cut;
	int l_pad = 0, r_pad = 0;
	int t_pad = 0, b_pad = 0;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 1, kw_args, &rect);
	py_helper_keyword_oft(arg_img, n_args, args, 2, kw_args, &oft);
    
    // Fit X. bigger or smaller, cut or pad to center
	if(oft.x < 0 || oft.y < 0)
    {
		if (rect.w > width) {
			int adjust = rect.w - width;
			rect.w -= adjust;
			rect.x += adjust / 2;
		} else if (rect.w < width) {
			int adjust = width - rect.w;
			l_pad = adjust / 2;
			r_pad = (adjust + 1) / 2;
		}
		// Fit Y. bigger or smaller, cut or pad to center
		if (rect.h > height) {
			int adjust = rect.h - height;
			rect.h -= adjust;
			rect.y += adjust / 2;
		} else if (rect.h < height) {
			int adjust = height - rect.h;
			t_pad = adjust / 2;
			b_pad = (adjust + 1) / 2;
		}
	}
	else
	{
		l_pad = oft.x;
		t_pad = oft.y;
	}
	is_cut =((rect.x != 0) || (rect.y != 0) || \
			(rect.w != arg_img->w) || (rect.h != arg_img->h));
    switch (type) {
        case LCD_NONE:
            return mp_const_none;
        case LCD_SHIELD:
			//fill pad
			if(oft.x < 0 || oft.y < 0)
			{
				lcd_fill_rectangle(0,0, width, t_pad, BLACK);
				lcd_fill_rectangle(0,height-b_pad, width, height, BLACK);
				lcd_fill_rectangle(0,t_pad, l_pad, height-b_pad, BLACK);
				lcd_fill_rectangle(width-r_pad,t_pad, width, height-b_pad, BLACK);
			}
            if(is_cut){	//cut from img
				if (IM_IS_GS(arg_img)) {
					lcd_draw_pic_grayroi(l_pad, t_pad, arg_img->w, arg_img->h, rect.x, rect.y, rect.w, rect.h, (uint8_t *)(arg_img->pixels));
				}
				else {
					lcd_draw_pic_roi(l_pad, t_pad, arg_img->w, arg_img->h, rect.x, rect.y, rect.w, rect.h, (uint32_t *)(arg_img->pixels));
				}
			}
			else{	//no cut
				if (IM_IS_GS(arg_img)) {
					lcd_draw_pic_gray(l_pad, t_pad, rect.w, rect.h, (uint8_t *)(arg_img->pixels));
				}
				else {
					lcd_draw_picture(l_pad, t_pad, rect.w, rect.h, (uint32_t *)(arg_img->pixels));
				}
			}
            return mp_const_none;
    }
    return mp_const_none;
}

static mp_obj_t py_lcd_clear(uint n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
	uint16_t color = BLACK;
	if(n_args >= 1)
	{
		if (mp_obj_is_integer(pos_args[0])) {
			color = mp_obj_get_int(pos_args[0]);
		} else {
			mp_obj_t *arg_color;
			mp_obj_get_array_fixed_n(pos_args[0], 3, &arg_color);
			color = COLOR_R8_G8_B8_TO_RGB565(IM_MAX(IM_MIN(mp_obj_get_int(arg_color[0]), COLOR_R8_MAX), COLOR_R8_MIN),
													IM_MAX(IM_MIN(mp_obj_get_int(arg_color[1]), COLOR_G8_MAX), COLOR_G8_MIN),
													IM_MAX(IM_MIN(mp_obj_get_int(arg_color[2]), COLOR_B8_MAX), COLOR_B8_MIN));
			color = color<<8 | (color>>8 & 0xFF);
		}
	}
    switch (type) {
        case LCD_NONE:
            return mp_const_none;
        case LCD_SHIELD:
            lcd_clear(color);
            return mp_const_none;
    }
    return mp_const_none;
}

//x0,y0,string,font color,bg color
static char str_buf[LCD_W/8*16*8];
static char str_cut[LCD_W/8+1];
STATIC mp_obj_t py_lcd_draw_string(uint n_args, const mp_obj_t *args)
{
    uint16_t x0 = mp_obj_get_int(args[0]);
	uint16_t y0 = mp_obj_get_int(args[1]);
	char* str  = mp_obj_str_get_str(args[2]);
	uint16_t fontc = RED;
	uint16_t bgc = BLACK;
	if(str == NULL) return mp_const_none;
	if(x0 < 0 || x0 >= LCD_W || y0 < 0 || y0 > LCD_H-16) return mp_const_none;
	int len = strlen(str);
	int width,height;
    if(n_args >= 4) fontc = mp_obj_get_int(args[3]);
	if(n_args >= 5) bgc = mp_obj_get_int(args[4]);
	if(len>(LCD_W-x0)/8) len = (LCD_W-x0)/8;
	if(len <= 0) return mp_const_none;
	memcpy(str_cut,str,len);
	str_cut[len]=0;
	width = len*8; height = 16;
	lcd_ram_draw_string(str_cut, str_buf, fontc, bgc);
	lcd_draw_picture(x0, y0, width, height, str_buf);
	return mp_const_none;
}

STATIC mp_obj_t py_lcd_freq(uint n_args, const mp_obj_t *pos_args)
{
	mp_int_t freq = lcd_get_freq();
	if(n_args >= 1)
	{
		freq =  mp_obj_get_int(pos_args[0]);
		lcd_set_freq(freq);
	}
	return mp_obj_new_int(freq);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_lcd_init_obj, 0, py_lcd_init);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_lcd_deinit_obj, py_lcd_deinit);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_lcd_width_obj, py_lcd_width);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_lcd_height_obj, py_lcd_height);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_lcd_type_obj, py_lcd_type);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(py_lcd_freq_obj, 0, 1, py_lcd_freq);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_lcd_set_backlight_obj, py_lcd_set_backlight);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_lcd_get_backlight_obj, py_lcd_get_backlight);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_lcd_display_obj, 1, py_lcd_display);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_lcd_clear_obj, 0, py_lcd_clear);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_lcd_direction_obj, py_lcd_direction);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(py_lcd_draw_string_obj, 3, 5, py_lcd_draw_string);
static const mp_map_elem_t globals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__),        MP_OBJ_NEW_QSTR(MP_QSTR_lcd) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_init),            (mp_obj_t)&py_lcd_init_obj          },
    { MP_OBJ_NEW_QSTR(MP_QSTR_deinit),          (mp_obj_t)&py_lcd_deinit_obj        },
    { MP_OBJ_NEW_QSTR(MP_QSTR_width),           (mp_obj_t)&py_lcd_width_obj         },
    { MP_OBJ_NEW_QSTR(MP_QSTR_height),          (mp_obj_t)&py_lcd_height_obj        },
    { MP_OBJ_NEW_QSTR(MP_QSTR_type),            (mp_obj_t)&py_lcd_type_obj          },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_freq),            (mp_obj_t)&py_lcd_freq_obj          },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_backlight),   (mp_obj_t)&py_lcd_set_backlight_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_backlight),   (mp_obj_t)&py_lcd_get_backlight_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_display),         (mp_obj_t)&py_lcd_display_obj       },
    { MP_OBJ_NEW_QSTR(MP_QSTR_clear),           (mp_obj_t)&py_lcd_clear_obj         },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_direction),       (mp_obj_t)&py_lcd_direction_obj     },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_draw_string),     (mp_obj_t)&py_lcd_draw_string_obj   },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_XY_RLUD),  MP_OBJ_NEW_SMALL_INT(DIR_XY_RLUD)}, 
    { MP_OBJ_NEW_QSTR(MP_QSTR_YX_RLUD),  MP_OBJ_NEW_SMALL_INT(DIR_YX_RLUD)}, 
	{ MP_OBJ_NEW_QSTR(MP_QSTR_XY_LRUD),  MP_OBJ_NEW_SMALL_INT(DIR_XY_LRUD)}, 
	{ MP_OBJ_NEW_QSTR(MP_QSTR_YX_LRUD),  MP_OBJ_NEW_SMALL_INT(DIR_YX_LRUD)}, 
	{ MP_OBJ_NEW_QSTR(MP_QSTR_XY_RLDU),  MP_OBJ_NEW_SMALL_INT(DIR_XY_RLDU)}, 
	{ MP_OBJ_NEW_QSTR(MP_QSTR_YX_RLDU),  MP_OBJ_NEW_SMALL_INT(DIR_YX_RLDU)}, 
	{ MP_OBJ_NEW_QSTR(MP_QSTR_XY_LRDU),  MP_OBJ_NEW_SMALL_INT(DIR_XY_LRDU)}, 
	{ MP_OBJ_NEW_QSTR(MP_QSTR_YX_LRDU),  MP_OBJ_NEW_SMALL_INT(DIR_YX_LRDU)},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_BLACK      ),  MP_OBJ_NEW_SMALL_INT(BLACK      )},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_NAVY       ),  MP_OBJ_NEW_SMALL_INT(NAVY       )},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_DARKGREEN  ),  MP_OBJ_NEW_SMALL_INT(DARKGREEN  )},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_DARKCYAN   ),  MP_OBJ_NEW_SMALL_INT(DARKCYAN   )},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_MAROON     ),  MP_OBJ_NEW_SMALL_INT(MAROON     )},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_PURPLE     ),  MP_OBJ_NEW_SMALL_INT(PURPLE     )},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_OLIVE      ),  MP_OBJ_NEW_SMALL_INT(OLIVE      )},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_LIGHTGREY  ),  MP_OBJ_NEW_SMALL_INT(LIGHTGREY  )},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_DARKGREY   ),  MP_OBJ_NEW_SMALL_INT(DARKGREY   )},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_BLUE       ),  MP_OBJ_NEW_SMALL_INT(BLUE       )},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_GREEN      ),  MP_OBJ_NEW_SMALL_INT(GREEN      )},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_CYAN       ),  MP_OBJ_NEW_SMALL_INT(CYAN       )},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_RED        ),  MP_OBJ_NEW_SMALL_INT(RED        )},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_MAGENTA    ),  MP_OBJ_NEW_SMALL_INT(MAGENTA    )},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_YELLOW     ),  MP_OBJ_NEW_SMALL_INT(YELLOW     )},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_WHITE      ),  MP_OBJ_NEW_SMALL_INT(WHITE      )},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_ORANGE     ),  MP_OBJ_NEW_SMALL_INT(ORANGE     )},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_GREENYELLOW),  MP_OBJ_NEW_SMALL_INT(GREENYELLOW)},
	{ MP_OBJ_NEW_QSTR(MP_QSTR_PINK       ),  MP_OBJ_NEW_SMALL_INT(PINK       )},
	
    { NULL, NULL },
};


STATIC MP_DEFINE_CONST_DICT(globals_dict, globals_dict_table);

const mp_obj_module_t lcd_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_t)&globals_dict,
};
