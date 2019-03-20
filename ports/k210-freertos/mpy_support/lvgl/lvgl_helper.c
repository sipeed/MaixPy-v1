



#include "binary.h"
#include "lvgl.h"
#include "lcd.h"
#include "mperrno.h"
#include "touchscreen.h"


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


STATIC void lcd_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2, lv_color_t color)
{
	// printf("fill %d %d %d %d %x\n", x1, x2, y1, y2, color.full);
	lcd_fill_rectangle(x1, y1, x2, y2, color.full);
}


STATIC void lcd_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_map)
{
	int32_t w = x2-x1+1;
	int32_t h = y2-y1+1;
	// printf("flush %d %d %d %d %d %d \n", x1, x2, y1, y2, w, h);
	int32_t x,y;
	int32_t i=0;
	uint16_t* data = malloc( w*h*2 );
	uint16_t* pixels = data;
	if(!data)
		mp_raise_OSError(MP_ENOMEM);
	for(y=y1; y<=y2; ++y)
	{
		for(x=x1; x<=x2; ++x)
		{
			pixels[i++]= (color_map->red<<3) | (color_map->blue<<8) | (color_map->green>>3&0x07 | color_map->green<<13);
			// or LV_COLOR_16_SWAP = 1
			 ++color_map;
		}
	}
	lcd_draw_picture(x1, y1, w, h, (uint32_t*)data);
	free(data);
	// printf("flush end\n");
	lv_flush_ready();
}


/**
 * Get the current position and state of the mouse
 * @param data store the mouse data here
 * @return false: because the points are not buffered, so no more data to be read
 */
bool mouse_read(lv_indev_data_t * data)
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


DEFINE_PTR_OBJ(lcd_fill);
DEFINE_PTR_OBJ(lcd_flush);
DEFINE_PTR_OBJ(mouse_read);


STATIC const mp_rom_map_elem_t lvgl_helper_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_lvgl_helper) },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_fill), MP_ROM_PTR(&PTR_OBJ(lcd_fill)) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_flush), MP_ROM_PTR(&PTR_OBJ(lcd_flush)) },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_read), MP_ROM_PTR(&PTR_OBJ(mouse_read))},
};


STATIC MP_DEFINE_CONST_DICT (
    mp_module_lvgl_helper_globals,
    lvgl_helper_globals_table
);

const mp_obj_module_t mp_module_lvgl_helper = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_lvgl_helper_globals
};

