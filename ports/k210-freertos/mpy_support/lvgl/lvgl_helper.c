



#include "binary.h"
#include "lvgl.h"
#include "lcd.h"
#include "mperrno.h"


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

DEFINE_PTR_OBJ(lcd_fill);
DEFINE_PTR_OBJ(lcd_flush);


STATIC const mp_rom_map_elem_t lvgl_helper_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_lvgl_helper) },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_fill), MP_ROM_PTR(&PTR_OBJ(lcd_fill)) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_flush), MP_ROM_PTR(&PTR_OBJ(lcd_flush)) },
};


STATIC MP_DEFINE_CONST_DICT (
    mp_module_lvgl_helper_globals,
    lvgl_helper_globals_table
);

const mp_obj_module_t mp_module_lvgl_helper = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_lvgl_helper_globals
};

