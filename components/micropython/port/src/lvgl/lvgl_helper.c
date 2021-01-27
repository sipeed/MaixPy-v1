



#include "binary.h"
#include "lvgl.h"
#include "lv_color.h"
#include "lcd.h"
#include "mperrno.h"
#include "py/runtime.h"
#include LV_MEM_CUSTOM_INCLUDE

#include "global_config.h"
#if CONFIG_MAIXPY_TOUCH_SCREEN_ENABLE
	#include "touchscreen.h"
#endif
#include "py/objarray.h"


typedef struct mp_ptr_t
{
    mp_obj_base_t base;
    void *ptr;
} mp_ptr_t;

STATIC mp_int_t mp_ptr_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags)
{
    mp_ptr_t *self = MP_OBJ_TO_PTR(self_in);

    if (flags & MP_BUFFER_WRITE) {
        // read-only ptr
        return 1;
    }

    bufinfo->buf = &self->ptr;
    bufinfo->len = sizeof(self->ptr);
    bufinfo->typecode = BYTEARRAY_TYPECODE;
    return 0;
}

#define PTR_OBJ(ptr_global) ptr_global ## _obj
#define DEFINE_PTR_OBJ_TYPE(ptr_obj_type, ptr_type_qstr)\
STATIC const mp_obj_type_t ptr_obj_type = {\
    { &mp_type_type },\
    .name = ptr_type_qstr,\
    .buffer_p = { .get_buffer = mp_ptr_get_buffer }\
}

#define DEFINE_PTR_OBJ(ptr_global)\
DEFINE_PTR_OBJ_TYPE(ptr_global ## _type, MP_QSTR_ ## ptr_global);\
STATIC const mp_ptr_t PTR_OBJ(ptr_global) = {\
    { &ptr_global ## _type },\
    &ptr_global\
}


// STATIC void lcd_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2, lv_color_t color)
// {
// 	lcd_fill_rectangle(x1, y1, x2, y2, color.full);
// }


STATIC void lcd_flush(struct _disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_map)
{
	int32_t w = area->x2-area->x1+1;
	int32_t h = area->y2-area->y1+1;
	int32_t x,y;
	int32_t i=0;
	uint16_t* data = LV_MEM_CUSTOM_ALLOC( w*h*2 );
	uint16_t* pixels = data;
	if(!data)
		mp_raise_OSError(MP_ENOMEM);
	for(y=area->y1; y<=area->y2; ++y)
	{
		for(x=area->x1; x<=area->x2; ++x)
		{
			#if LV_COLOR_16_SWAP
				pixels[i++]= color_map->full;
			#else
				pixels[i++]= (color_map->ch.red<<3) | (color_map->ch.blue<<8) | ( ((color_map->ch.green>>3)&0x07) | (color_map->ch.green<<13));
			#endif
			 ++color_map;
		}
	}
	lcd->draw_picture(area->x1, area->y1, w, h, (uint32_t*)data);
	LV_MEM_CUSTOM_FREE(data);
	lv_disp_flush_ready(disp_drv);
}

#if CONFIG_MAIXPY_TOUCH_SCREEN_ENABLE
/**
 * Get the current position and state of the mouse
 * @param data store the mouse data here
 * @return false: because the points are not buffered, so no more data to be read
 */
bool mouse_read(struct _lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
	int ret, status, x, y;

	if(!touchscreen_is_init())
	{
		touchscreen_config_t config;
		config.i2c = NULL;
        config.calibration[0] = -6;
        config.calibration[1] = -5941;
        config.calibration[2] = 22203576;
        config.calibration[3] = 4232;
        config.calibration[4] = -8;
        config.calibration[5] = -700369;
        config.calibration[6] = 65536;
		ret = touchscreen_init((void*)&config);
		if( ret != 0)
			mp_raise_OSError(ret);
	}
	ret = touchscreen_read(&status, &x, &y);
	if(ret != 0)
		mp_raise_OSError(ret);
    /*Store the collected data*/
	switch(status)
	{
		case TOUCHSCREEN_STATUS_RELEASE:
    		data->state =  LV_INDEV_STATE_REL;
			break;
		case TOUCHSCREEN_STATUS_PRESS:
		case TOUCHSCREEN_STATUS_MOVE:
			data->state = LV_INDEV_STATE_PR;
			break;
		default:
			return false;
	}
    data->point.x = x;
    data->point.y = y;
    return false;
}
#endif // CONFIG_MAIXPY_TOUCH_SCREEN_ENABLE

#if LV_USE_LOG
static void lv_helper_log(lv_log_level_t level, const char * file, uint32_t line, const char * msg)
{
	mp_printf(&mp_plat_print, "[lvgl]:[%d] %s:%d %s\r\n", level, file, line, msg);
}
#endif

static mp_obj_t rgba8888_to_bgra5658(mp_obj_t image_data)
{
	mp_obj_array_t* self = (mp_obj_array_t*)image_data;
	if(self->base.type != &mp_type_memoryview)
		mp_raise_ValueError("need memoryview obj");
	uint8_t* in = (uint8_t*)self->items;
	uint8_t* out = in;
	size_t size = self->len / 4;
	for( size_t i=0; i<size; ++i)
	{
		out[0] = ((in[2]&0xF8) >> 3) | ((in[1] & 0x1C) << 3);
		out[1] = ((in[1]&0xE0) >> 5) | ((in[0] & 0xF8));
		out[2] = in[3];
		in += 4;
		out += 3;
	}
	return mp_const_none;
}


// DEFINE_PTR_OBJ(lcd_fill);
DEFINE_PTR_OBJ(lcd_flush);
#if CONFIG_MAIXPY_TOUCH_SCREEN_ENABLE
DEFINE_PTR_OBJ(mouse_read);
#endif
#if LV_USE_LOG
DEFINE_PTR_OBJ(lv_helper_log);
#endif
STATIC MP_DEFINE_CONST_FUN_OBJ_1(rgba8888_to_bgra5658_obj, rgba8888_to_bgra5658);


STATIC const mp_rom_map_elem_t lvgl_helper_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_lvgl_helper) },
	// { MP_OBJ_NEW_QSTR(MP_QSTR_fill), MP_ROM_PTR(&PTR_OBJ(lcd_fill)) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_flush), MP_ROM_PTR(&PTR_OBJ(lcd_flush)) },
#if CONFIG_MAIXPY_TOUCH_SCREEN_ENABLE
	{ MP_OBJ_NEW_QSTR(MP_QSTR_read), MP_ROM_PTR(&PTR_OBJ(mouse_read))},
#endif
#if LV_USE_LOG
	{ MP_OBJ_NEW_QSTR(MP_QSTR_log), MP_ROM_PTR(&PTR_OBJ(lv_helper_log))},
#endif
	{ MP_OBJ_NEW_QSTR(MP_QSTR_rgba8888_to_5658), MP_ROM_PTR(&rgba8888_to_bgra5658_obj) },
};


STATIC MP_DEFINE_CONST_DICT (
    mp_module_lvgl_helper_globals,
    lvgl_helper_globals_table
);

const mp_obj_module_t mp_module_lvgl_helper = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_lvgl_helper_globals
};

