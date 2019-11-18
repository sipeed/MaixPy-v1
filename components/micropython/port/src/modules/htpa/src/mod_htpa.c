#include <stdio.h>
#include <string.h>

#include "fpioa.h"
#include "gpiohs.h"
#include "sysctl.h"
#include "sipeed_i2c.h"

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "py/mpconfig.h"
#include "py_image.h"

#if CONFIG_MAIXPY_HTPA_ENABLE

#include "htpa.h"

const mp_obj_type_t modules_htpa_type;

typedef struct {
    mp_obj_base_t         base;
    htpa_t                htpa_obj;
    image_t               img;
    mp_obj_list_t*        pixels; // int list
    
} modules_htpa_obj_t;

mp_obj_t modules_htpa_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) 
{
    enum {
        ARG_i2c,
        ARG_scl_pin,
        ARG_sda_pin,
        ARG_i2c_freq
    };

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_i2c,              MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = I2C_DEVICE_0} },
        { MP_QSTR_scl_pin,              MP_ARG_INT, {.u_int = 7} },
        { MP_QSTR_sda_pin,              MP_ARG_INT, {.u_int = 6} },
        { MP_QSTR_i2c_freq,              MP_ARG_INT, {.u_int = 1000000} },
    };

    modules_htpa_obj_t *self = m_new_obj_with_finaliser(modules_htpa_obj_t);
    self->base.type = &modules_htpa_type;

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args , all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
   if(args[ARG_i2c].u_int >= I2C_DEVICE_MAX){
       mp_raise_ValueError("i2c device error");
   }
   int ret = htpa_init(&self->htpa_obj, args[ARG_i2c].u_int, args[ARG_scl_pin].u_int, args[ARG_sda_pin].u_int, args[ARG_i2c_freq].u_int);
   if(ret != 0){
       m_del_obj(modules_htpa_obj_t, self);
       mp_raise_OSError(ret);
   }   
   self->img.bpp = IMAGE_BPP_GRAYSCALE;
   self->img.data = (uint8_t*)self->htpa_obj.v;
   self->img.w = self->htpa_obj.width;
   self->img.h = self->htpa_obj.height;
    self->pixels = (mp_obj_list_t*)mp_obj_new_list(self->htpa_obj.width*self->htpa_obj.height, NULL);
    return MP_OBJ_FROM_PTR(self);
}

STATIC void modules_htpa_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    modules_htpa_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "htpa %dx%d", self->htpa_obj.width, self->htpa_obj.height);
}

STATIC mp_obj_t modules_htpa_del(mp_obj_t self_in) {
    modules_htpa_obj_t *self = MP_OBJ_TO_PTR(self_in);
    htpa_destroy(&self->htpa_obj);
    m_del_obj(modules_htpa_obj_t, self);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(modules_htpa_del_obj, modules_htpa_del);

// STATIC mp_obj_t modules_htpa_snapshot(mp_obj_t self_in) {
//     modules_htpa_obj_t *self = MP_OBJ_TO_PTR(self_in);
//     int ret = htpa_snapshot(&self->htpa_obj, &self->img.data);
//     if(ret != 0){
//         mp_raise_OSError(ret);
//     }
//     return py_image_from_struct(&self->img);
// }

STATIC mp_obj_t modules_htpa_get_data(mp_obj_t self_in) {
    modules_htpa_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t* pixels_data = NULL;
    int ret = htpa_snapshot(&self->htpa_obj, &pixels_data);
    if(ret != 0){
        mp_raise_OSError(ret);
    }
    for(int i=0; i<self->htpa_obj.width*self->htpa_obj.height; ++i)
    {
        self->pixels->items[i] = mp_obj_new_int(pixels_data[i]);
    }
    return (mp_obj_t)self->pixels;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(modules_htpa_temperature_obj, modules_htpa_get_data);

STATIC mp_obj_t modules_htpa_get_min_max(mp_obj_t self_in)
{
    modules_htpa_obj_t* self = MP_OBJ_TO_PTR(self_in);
    int32_t max = self->htpa_obj.v[0], min = self->htpa_obj.v[1];
    int max_pos = 0, min_pos = 0;
    for(int i=1; i<self->htpa_obj.width*self->htpa_obj.height; ++i)
    {
        if(self->htpa_obj.v[i] > max)
        {
            max = self->htpa_obj.v[i];
            max_pos = i;
        }
        if(self->htpa_obj.v[i] < min)
        {
            min = self->htpa_obj.v[i];
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

STATIC MP_DEFINE_CONST_FUN_OBJ_1(modules_htpa_get_min_max_obj, modules_htpa_get_min_max);


STATIC mp_obj_t modules_htpa_get_to_image(mp_obj_t self_in, mp_obj_t min_in, mp_obj_t max_in)
{
    modules_htpa_obj_t* self = MP_OBJ_TO_PTR(self_in);
    mp_int_t min = mp_obj_get_int(min_in);
    mp_int_t max = mp_obj_get_int(max_in);
    mp_int_t range = max - min;
    uint8_t* p = (uint8_t*)self->htpa_obj.v;
    *p = (self->htpa_obj.v[0] - min)/range*255;
    for(int i=1; i<self->htpa_obj.width*self->htpa_obj.height; ++i)
    {
        *p = (self->htpa_obj.v[i] - min)/(float)range*255;
        ++p;
    }        

    return py_image_from_struct(&self->img);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_3(modules_htpa_to_image_obj, modules_htpa_get_to_image);


STATIC mp_obj_t modules_htpa_width(mp_obj_t self_in){
    modules_htpa_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(self->htpa_obj.width);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(modules_htpa_width_obj, modules_htpa_width);

STATIC mp_obj_t modules_htpa_height(mp_obj_t self_in){
    modules_htpa_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(self->htpa_obj.height);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(modules_htpa_height_obj, modules_htpa_height);


static mp_obj_t py_htpa_write_reg(mp_obj_t self_in, mp_obj_t addr, mp_obj_t val) {
    modules_htpa_obj_t *self = MP_OBJ_TO_PTR(self_in);

   int ret = htpa_write_reg(&self->htpa_obj, (uint8_t)mp_obj_get_int(addr), (uint8_t)mp_obj_get_int(val));
   if(ret < 0)
   {
       mp_raise_OSError(ret);
   }
   return mp_const_none;
}

static mp_obj_t py_htpa_read_reg(mp_obj_t self_in, mp_obj_t addr) {
    modules_htpa_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int ret = htpa_read_reg(&self->htpa_obj, (uint8_t)mp_obj_get_int(addr));
    if(ret < 0)
    {
        mp_raise_OSError(ret);
    }
   return mp_obj_new_int(ret);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_3(py_htpa_write_reg_obj, py_htpa_write_reg);
STATIC MP_DEFINE_CONST_FUN_OBJ_2(py_htpa_read_reg_obj,  py_htpa_read_reg);


STATIC const mp_rom_map_elem_t mp_modules_htpa_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&modules_htpa_del_obj) },
    { MP_ROM_QSTR(MP_QSTR_temperature), MP_ROM_PTR(&modules_htpa_temperature_obj) },
    { MP_ROM_QSTR(MP_QSTR_min_max), MP_ROM_PTR(&modules_htpa_get_min_max_obj) },
    { MP_ROM_QSTR(MP_QSTR_to_image), MP_ROM_PTR(&modules_htpa_to_image_obj) },
    { MP_ROM_QSTR(MP_QSTR_width), MP_ROM_PTR(&modules_htpa_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_height), MP_ROM_PTR(&modules_htpa_height_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR___write_reg),         (mp_obj_t)&py_htpa_write_reg_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR___read_reg),          (mp_obj_t)&py_htpa_read_reg_obj },
};

MP_DEFINE_CONST_DICT(mp_modules_htpa_locals_dict, mp_modules_htpa_locals_dict_table);

const mp_obj_type_t modules_htpa_type = {
    { &mp_type_type },
    .name = MP_QSTR_htpa,
    .print = modules_htpa_print,
    .make_new = modules_htpa_make_new,
    .locals_dict = (mp_obj_dict_t*)&mp_modules_htpa_locals_dict,
};

#endif /* CONFIG_MAIXPY_HTPA_ENABLE */
