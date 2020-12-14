#include <mp.h>
#include <objstr.h>
#include <spi.h>
#include "imlib.h"
#include "fb_alloc.h"
#include "vfs_wrapper.h"
#include "py_assert.h"
#include "py_helper.h"
#include "py_image.h"
#include "sleep.h"
#include "sysctl.h"

// #if 1
#include "gpiohs.h"
#include "hub75e.h"

const mp_obj_type_t modules_hub75e_type;
typedef struct {
    mp_obj_base_t   base;
    hub75e_t       hub75e_obj;
} modules_hub75e_obj_t;

STATIC mp_obj_t hub75e_make_new_helper(modules_hub75e_obj_t *self, size_t n_args, size_t n_kw,
                                     const mp_obj_t *args) {
    // check arguments
    // mp_arg_check_num(n_args, n_kw, 0, 19, true);

    size_t len;
    mp_obj_t *items;

    mp_obj_get_array(args[0], &len, &items);
    if (len == 0) {
        mp_raise_ValueError("tuple_in len != 0");
    }
    
    enum {
        ARG_spi = 0,
        ARG_cs_pin,
        ARG_r1_pin,
        ARG_g1_pin,
        ARG_b1_pin,
        ARG_r2_pin,
        ARG_g2_pin,
        ARG_b2_pin,
        ARG_a_gpio,
        ARG_b_gpio,
        ARG_c_gpio,
        ARG_d_gpio,
        ARG_e_gpio,
        ARG_oe_gpio,
        ARG_latch_gpio,
        ARG_clk_pin,
        ARG_dma_channel,
        ARG_width,
        ARG_height,
    };

    self->hub75e_obj.spi         = mp_obj_get_int(items[ARG_spi]);
    self->hub75e_obj.cs_pin      = mp_obj_get_int(items[ARG_cs_pin]);
    self->hub75e_obj.r1_pin      = mp_obj_get_int(items[ARG_r1_pin]);
    self->hub75e_obj.g1_pin      = mp_obj_get_int(items[ARG_g1_pin]);
    self->hub75e_obj.b1_pin      = mp_obj_get_int(items[ARG_b1_pin]);
    self->hub75e_obj.r2_pin      = mp_obj_get_int(items[ARG_r2_pin]);
    self->hub75e_obj.g2_pin      = mp_obj_get_int(items[ARG_g2_pin]);
    self->hub75e_obj.b2_pin      = mp_obj_get_int(items[ARG_b2_pin]);
    self->hub75e_obj.a_gpio      = mp_obj_get_int(items[ARG_a_gpio]) - FUNC_GPIOHS0;
    self->hub75e_obj.b_gpio      = mp_obj_get_int(items[ARG_b_gpio]) - FUNC_GPIOHS0;
    self->hub75e_obj.c_gpio      = mp_obj_get_int(items[ARG_c_gpio]) - FUNC_GPIOHS0;
    self->hub75e_obj.d_gpio      = mp_obj_get_int(items[ARG_d_gpio]) - FUNC_GPIOHS0;
    self->hub75e_obj.e_gpio      = mp_obj_get_int(items[ARG_e_gpio]) - FUNC_GPIOHS0;
    self->hub75e_obj.oe_gpio     = mp_obj_get_int(items[ARG_oe_gpio]) - FUNC_GPIOHS0;
    self->hub75e_obj.latch_gpio  = mp_obj_get_int(items[ARG_latch_gpio]) - FUNC_GPIOHS0;
    self->hub75e_obj.clk_pin     = mp_obj_get_int(items[ARG_clk_pin]);
    self->hub75e_obj.dma_channel = mp_obj_get_int(items[ARG_dma_channel]);
    self->hub75e_obj.width       = mp_obj_get_int(items[ARG_width]);
    self->hub75e_obj.height      = mp_obj_get_int(items[ARG_height]);

    

    if (self->hub75e_obj.spi  != 1 && self->hub75e_obj.spi  != 0) {
        mp_raise_TypeError("spi num should be 0 or 1");
        return mp_const_false;
    }

    if((self->hub75e_obj.r1_pin) >= FPIOA_NUM_IO)
    {
        mp_raise_ValueError("cs gpio error");
        return mp_const_false;
    }
    if((self->hub75e_obj.r1_pin) >= FPIOA_NUM_IO)
    {
        mp_raise_ValueError("r1 pin error");
        return mp_const_false;
    }
    if((self->hub75e_obj.g1_pin) >= FPIOA_NUM_IO)
    {
        mp_raise_ValueError("g1 pin error");
        return mp_const_false;
    }
    if((self->hub75e_obj.b1_pin) >= FPIOA_NUM_IO)
    {
        mp_raise_ValueError("b1 pin error");
        return mp_const_false;
    }
    if((self->hub75e_obj.r2_pin) >= FPIOA_NUM_IO)
    {
        mp_raise_ValueError("r2 pin error");
        return mp_const_false;
    }
    if((self->hub75e_obj.g2_pin) >= FPIOA_NUM_IO)
    {
        mp_raise_ValueError("g2 pin error");
        return mp_const_false;
    }
    if((self->hub75e_obj.b2_pin) >= FPIOA_NUM_IO)
    {
        mp_raise_ValueError("b2 pin error");
        return mp_const_false;
    }
    if((self->hub75e_obj.a_gpio + FUNC_GPIOHS0) < FUNC_GPIOHS0 || (self->hub75e_obj.a_gpio + FUNC_GPIOHS0) > FUNC_GPIOHS31)
    {
        mp_raise_ValueError("a gpio error");
        return mp_const_false;
    }
    if((self->hub75e_obj.b_gpio + FUNC_GPIOHS0) < FUNC_GPIOHS0 || (self->hub75e_obj.b_gpio + FUNC_GPIOHS0) > FUNC_GPIOHS31)
    {
        mp_raise_ValueError("b gpio error");
        return mp_const_false;
    }
    if((self->hub75e_obj.c_gpio + FUNC_GPIOHS0) < FUNC_GPIOHS0 || (self->hub75e_obj.c_gpio + FUNC_GPIOHS0) > FUNC_GPIOHS31)
    {
        mp_raise_ValueError("c gpio error");
        return mp_const_false;
    }
    if((self->hub75e_obj.d_gpio + FUNC_GPIOHS0) < FUNC_GPIOHS0 || (self->hub75e_obj.d_gpio + FUNC_GPIOHS0) > FUNC_GPIOHS31)
    {
        mp_raise_ValueError("d gpio error");
        return mp_const_false;
    }
    if((self->hub75e_obj.e_gpio + FUNC_GPIOHS0) < FUNC_GPIOHS0 || (self->hub75e_obj.e_gpio + FUNC_GPIOHS0) > FUNC_GPIOHS31)
    {
        mp_raise_ValueError("e gpio error");
        return mp_const_false;
    }
    if((self->hub75e_obj.oe_gpio + FUNC_GPIOHS0) < FUNC_GPIOHS0 || (self->hub75e_obj.oe_gpio + FUNC_GPIOHS0) > FUNC_GPIOHS31)
    {
        mp_raise_ValueError("oe gpio error");
        return mp_const_false;
    }
    if((self->hub75e_obj.latch_gpio + FUNC_GPIOHS0) < FUNC_GPIOHS0 || (self->hub75e_obj.latch_gpio + FUNC_GPIOHS0) > FUNC_GPIOHS31)
    {
        mp_raise_ValueError("latch gpio error");
        return mp_const_false;
    }
    if((self->hub75e_obj.clk_pin) >= FPIOA_NUM_IO)
    {
        mp_raise_ValueError("clk pin error");
        return mp_const_false;
    }
    if( ( (self->hub75e_obj.dma_channel) >=  DMAC_CHANNEL_MAX) )
    {
        mp_raise_ValueError("dmac channel error, should < 6");
        return mp_const_false;
    }
    return mp_const_true;
}

mp_obj_t modules_hub75e_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) 
{
    modules_hub75e_obj_t *self = m_new_obj(modules_hub75e_obj_t);
    self->base.type = &modules_hub75e_type;
    hub75e_make_new_helper(self, n_args, n_kw, all_args);
    hub75e_init(&(self->hub75e_obj));
    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t py_hub75e_display(mp_obj_t self_in, mp_obj_t image) {
    modules_hub75e_obj_t *self = MP_OBJ_TO_PTR(self_in);
    image_t *arg_img = py_image_cobj(image);
    PY_ASSERT_TRUE_MSG(IM_IS_RGB565(arg_img), "Image format is not supported.");
    hub75e_display_start(&(self->hub75e_obj), (uint16_t *)(arg_img->data));
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(py_hub75e_display_obj, py_hub75e_display);

STATIC mp_obj_t py_hub75e_stop(mp_obj_t self_in) {
    hub75e_display_stop();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_hub75e_stop_obj, py_hub75e_stop);

STATIC const mp_rom_map_elem_t mp_modules_hub75e_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_onewire) },

    { MP_ROM_QSTR(MP_QSTR_display), MP_ROM_PTR(&py_hub75e_display_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&py_hub75e_stop_obj) }
};

MP_DEFINE_CONST_DICT(mp_modules_hub75e_locals_dict, mp_modules_hub75e_locals_dict_table);

const mp_obj_type_t modules_hub75e_type = {
    { &mp_type_type },
    .name = MP_QSTR_hub75e,
    .make_new = modules_hub75e_make_new,
    .locals_dict = (mp_obj_dict_t*)&mp_modules_hub75e_locals_dict,
};
