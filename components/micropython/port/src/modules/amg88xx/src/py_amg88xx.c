#include <stdio.h>
#include <math.h>
#include <string.h>

#include "fpioa.h"
#include "xalloc.h"
#include "gpiohs.h"
#include "sysctl.h"
#include "sipeed_i2c.h"

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "py/mpconfig.h"
#include "py_image.h"

// REF: https://micropython-usermod.readthedocs.io/en/latest/usermods_10.html

#if CONFIG_MAIXPY_AMG88XX_ENABLE

#include "amg88xx.h"

#define  METHOD_NEAREST  0
#define  METHOD_BILINEAR 1

const mp_obj_type_t modules_amg88xx_type;

typedef struct {
	mp_obj_base_t       base;
	amg88xx_t           amg88xx_obj;
	uint8_t            *out;
	uint8_t             bk_scale;
	mp_obj_list_t      *pixels;

} modules_amg88xx_obj_t;

mp_obj_t modules_amg88xx_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw,
									const mp_obj_t *all_args)
{
	enum {
		ARG_i2c,
		ARG_i2c_addr,
		ARG_i2c_freq
	};

	static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_i2c,           MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = I2C_DEVICE_0} },
		{ MP_QSTR_i2c_addr,      MP_ARG_INT, {.u_int = 0x68} },
		{ MP_QSTR_i2c_freq,      MP_ARG_INT, {.u_int = 100000} },
	};

	modules_amg88xx_obj_t *self = m_new_obj_with_finaliser(modules_amg88xx_obj_t);
	self->base.type = &modules_amg88xx_type;

	self->bk_scale  = 1;
	self->out       = NULL;

	mp_map_t kw_args;
	mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args , all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	int ret = amg88xx_init(&self->amg88xx_obj,
							args[ARG_i2c].u_int,
							args[ARG_i2c_addr].u_int,
							args[ARG_i2c_freq].u_int);

	if (ret != 0)
	{
		m_del_obj(modules_amg88xx_obj_t, self);
		mp_raise_OSError(ret);
	}

	if(args[ARG_i2c].u_int >= I2C_DEVICE_MAX)
	{
		mp_raise_ValueError("i2c device error");
	}

    self->pixels = (mp_obj_list_t*)mp_obj_new_list(self->amg88xx_obj.len, NULL);

	return MP_OBJ_FROM_PTR(self);
}

STATIC void modules_amg88xx_print(const mp_print_t *print, mp_obj_t self_in,
														mp_print_kind_t kind)
{
	modules_amg88xx_obj_t *self = MP_OBJ_TO_PTR(self_in);
	mp_printf(print, "amg88xx [%d]i2c addr 0x%X scl %d sda %d freq %d\n",
			self->amg88xx_obj.i2c_num,
			self->amg88xx_obj.i2c_addr,
			self->amg88xx_obj.scl_pin,
			self->amg88xx_obj.sda_pin,
			self->amg88xx_obj.i2c_freq);
}

STATIC mp_obj_t modules_amg88xx_del(mp_obj_t self_in)
{
	modules_amg88xx_obj_t *self = MP_OBJ_TO_PTR(self_in);

	if (self->out)
	{
		xfree(self->out);
		self->out = NULL;
	}

	amg88xx_destroy(&self->amg88xx_obj);
	m_del_obj(modules_amg88xx_obj_t, self);
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(modules_amg88xx_del_obj, modules_amg88xx_del);

STATIC mp_obj_t modules_amg88xx_get_data(mp_obj_t self_in)
{
	modules_amg88xx_obj_t *self = MP_OBJ_TO_PTR(self_in);
	int32_t * pixels_data = NULL;
	int ret = amg88xx_snapshot(&self->amg88xx_obj, &pixels_data);

	if (ret != 0) mp_raise_OSError(ret);

	uint8_t i;
	for (i=0; i<self->amg88xx_obj.len; i++)
	{
		self->pixels->items[i] = mp_obj_new_int(pixels_data[i]);
	}

	return (mp_obj_t)self->pixels;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(modules_amg88xx_temperature_obj, modules_amg88xx_get_data);

STATIC mp_obj_t modules_amg88xx_get_min_max(mp_obj_t self_in)
{
	modules_amg88xx_obj_t *self = MP_OBJ_TO_PTR(self_in);
	int32_t max = self->amg88xx_obj.v[0];
	int32_t min = self->amg88xx_obj.v[0];
	int max_pos = 0;
	int min_pos = 0;

	int i;
	for (i = 1; i<self->amg88xx_obj.len; i++)
	{
		if (self->amg88xx_obj.v[i] > max)
		{
			max = self->amg88xx_obj.v[i];
			max_pos = i;
		}

		if (self->amg88xx_obj.v[i] < min)
		{
			min = self->amg88xx_obj.v[i];
			min_pos = i;
		}
	}

	mp_obj_t res[4];
	res[0] = mp_obj_new_int(min);
	res[1] = mp_obj_new_int(max);
	res[2] = mp_obj_new_int(min_pos);
	res[3] = mp_obj_new_int(max_pos);

	return mp_obj_new_tuple(4, res);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(modules_amg88xx_get_min_max_obj, modules_amg88xx_get_min_max);

STATIC mp_obj_t modules_amg88xx_get_to_image(size_t n_args, const mp_obj_t *args)
{


	modules_amg88xx_obj_t *self = MP_OBJ_TO_PTR(args[0]);

	mp_int_t min    = mp_obj_get_int(args[1]);
	mp_int_t max    = mp_obj_get_int(args[2]);
	mp_int_t scale  = mp_obj_get_int(args[3]);
	mp_int_t method = mp_obj_get_int(args[4]);
	mp_int_t range  = max - min;

	uint16_t w, h, x, y, vx, vy;
	float v[4], rx, ry, wx, wy, pixel;

	w = AMG88XX_WIDTH  * scale;     // new width
	h = AMG88XX_HEIGHT * scale;     // new height

	rx = (float)AMG88XX_WIDTH  / (float)w; // ratio in x
	ry = (float)AMG88XX_HEIGHT / (float)h; // ratio in y


	if (self->bk_scale != scale)
	{
		if (self->out)
		{
			xfree(self->out);
			self->out = NULL;
		}

		self->out = xalloc(w*h);
		self->bk_scale = scale & 0xFF;
	}

	int32_t * p = self->amg88xx_obj.v;

	if (method == METHOD_NEAREST)
	{
		// naerest neighbor interpolation
		for (y=0; y<w; y++)
		{
			for (x=0; x<h; x++)
			{
				vx = x * rx;
				vy = y * ry;
				v[0] = (p[vy * AMG88XX_WIDTH + vx] - min)/(float)range * 255;
				self->out[y * w + x] = v[0];
			}
		}
	}

	if (method == METHOD_BILINEAR)
	{
		// bilinear interpolation
		for (y=0; y<w; y++)
		{
			for (x=0; x<h; x++)
			{
				vx = x * rx;
				vy = y * ry;

				wx = ((float)x * rx) - (float)vx; // x weight
				wy = ((float)y * ry) - (float)vy; // y weight

				v[0] = (p[vy     * AMG88XX_WIDTH +  vx   ] - min)/(float)range * 255;
				v[1] = (p[vy     * AMG88XX_WIDTH + (vx+1)] - min)/(float)range * 255;
				v[2] = (p[(vy+1) * AMG88XX_WIDTH +  vx   ] - min)/(float)range * 255;
				v[4] = (p[(vy+1) * AMG88XX_WIDTH + (vx+1)] - min)/(float)range * 255;

				pixel = (uint8_t)v[0] * (1 - wx) * (1 - wy) +
						(uint8_t)v[1] *      wx  * (1 - wy) +
						(uint8_t)v[2] *      wy  * (1 - wx) +
						(uint8_t)v[4] *      wx  *      wy;

			self->out[y * w + x] = pixel;

			}
		}
	}

	mp_obj_t image = py_image(w, h, IMAGE_BPP_GRAYSCALE, self->out);

	return image;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modules_amg88xx_to_image_obj,
		5, 5, modules_amg88xx_get_to_image);


STATIC const mp_rom_map_elem_t mp_modules_amg88xx_locals_dict_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___del__),            MP_ROM_PTR(&modules_amg88xx_del_obj) },
	{ MP_ROM_QSTR(MP_QSTR_temperature),        MP_ROM_PTR(&modules_amg88xx_temperature_obj) },
	{ MP_ROM_QSTR(MP_QSTR_min_max),            MP_ROM_PTR(&modules_amg88xx_get_min_max_obj) },
	{ MP_ROM_QSTR(MP_QSTR_to_image),           MP_ROM_PTR(&modules_amg88xx_to_image_obj) },
	{ MP_ROM_QSTR(MP_QSTR_METHOD_NEAREST),     MP_ROM_INT(METHOD_NEAREST)                },
	{ MP_ROM_QSTR(MP_QSTR_METHOD_BILINEAR),    MP_ROM_INT(METHOD_BILINEAR)               },
};

MP_DEFINE_CONST_DICT(mp_modules_amg88xx_locals_dict, mp_modules_amg88xx_locals_dict_table);

const mp_obj_type_t modules_amg88xx_type = {
	{ &mp_type_type },
	.name = MP_QSTR_amg88xx,
	.print = modules_amg88xx_print,
	.make_new = modules_amg88xx_make_new,
	.locals_dict = (mp_obj_dict_t*)&mp_modules_amg88xx_locals_dict,
};

#endif
