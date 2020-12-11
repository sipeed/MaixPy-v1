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

#include "hub75e.h"

const mp_obj_type_t modules_hub75e_type;
typedef struct {
    mp_obj_base_t   base;
    hub75e_t*       hub75e_obj;
} modules_hub75e_obj_t;

STATIC void hub75e_make_new_helper(modules_hub75e_obj_t *self, size_t n_args, size_t n_kw,
                                     const mp_obj_t *args) {
    // check arguments
    // mp_arg_check_num(n_args, n_kw, 0, 19, true);

    enum {
        ARG_spi,
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

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_spi,                 MP_ARG_KW_ONLY|MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_cs_pin,              MP_ARG_KW_ONLY|MP_ARG_INT,                 {.u_int = -1} },
        { MP_QSTR_r1_pin,              MP_ARG_KW_ONLY|MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_g1_pin,              MP_ARG_KW_ONLY|MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_b1_pin,              MP_ARG_KW_ONLY|MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_r2_pin,              MP_ARG_KW_ONLY|MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_g2_pin,              MP_ARG_KW_ONLY|MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_b2_pin,              MP_ARG_KW_ONLY|MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_a_gpio,              MP_ARG_KW_ONLY|MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_b_gpio,              MP_ARG_KW_ONLY|MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_c_gpio,              MP_ARG_KW_ONLY|MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_d_gpio,              MP_ARG_KW_ONLY|MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_e_gpio,              MP_ARG_KW_ONLY|MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_oe_gpio,             MP_ARG_KW_ONLY|MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_latch_gpio,          MP_ARG_KW_ONLY|MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_clk_pin,             MP_ARG_KW_ONLY|MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_dma_channel,         MP_ARG_KW_ONLY|MP_ARG_INT,                 {.u_int = DMAC_CHANNEL0} },
        { MP_QSTR_width,               MP_ARG_KW_ONLY|MP_ARG_INT,                 {.u_int = 64} },
        { MP_QSTR_height,              MP_ARG_KW_ONLY|MP_ARG_INT,                 {.u_int = 64} }
    };

    mp_arg_val_t args_parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_printf(&mp_plat_print, "%d\r\n", MP_ARRAY_SIZE(allowed_args));

    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args),
                                allowed_args, args_parsed);
    mp_printf(&mp_plat_print, "1\r\n");
    if (args_parsed[ARG_spi].u_int != 1 || args_parsed[ARG_spi].u_int != 0) {
        mp_raise_TypeError("spi num should be 0 or 1");
        return mp_const_false;
    }
    mp_printf(&mp_plat_print, "2\r\n");

    if(args_parsed[ARG_cs_pin].u_int < FUNC_GPIOHS0 || args_parsed[ARG_cs_pin].u_int > FUNC_GPIOHS31)
    {
        mp_raise_ValueError("cs gpio error");
        return mp_const_false;
    }
    if(args_parsed[ARG_r1_pin].u_int < 0 || args_parsed[ARG_r1_pin].u_int >= FPIOA_NUM_IO)
    {
        mp_raise_ValueError("r1 pin error");
        return mp_const_false;
    }
    if(args_parsed[ARG_g1_pin].u_int < 0 || args_parsed[ARG_g1_pin].u_int >= FPIOA_NUM_IO)
    {
        mp_raise_ValueError("g1 pin error");
        return mp_const_false;
    }
    if(args_parsed[ARG_b1_pin].u_int < 0 || args_parsed[ARG_b1_pin].u_int >= FPIOA_NUM_IO)
    {
        mp_raise_ValueError("b1 pin error");
        return mp_const_false;
    }
    if(args_parsed[ARG_r2_pin].u_int < 0 || args_parsed[ARG_r2_pin].u_int >= FPIOA_NUM_IO)
    {
        mp_raise_ValueError("r2 pin error");
        return mp_const_false;
    }
    if(args_parsed[ARG_g2_pin].u_int < 0 || args_parsed[ARG_g2_pin].u_int >= FPIOA_NUM_IO)
    {
        mp_raise_ValueError("g2 pin error");
        return mp_const_false;
    }
    if(args_parsed[ARG_b2_pin].u_int < 0 || args_parsed[ARG_b2_pin].u_int >= FPIOA_NUM_IO)
    {
        mp_raise_ValueError("b2 pin error");
        return mp_const_false;
    }
    if(args_parsed[ARG_a_gpio].u_int < FUNC_GPIOHS0 || args_parsed[ARG_a_gpio].u_int > FUNC_GPIOHS31)
    {
        mp_raise_ValueError("a gpio error");
        return mp_const_false;
    }
    if(args_parsed[ARG_b_gpio].u_int < FUNC_GPIOHS0 || args_parsed[ARG_b_gpio].u_int > FUNC_GPIOHS31)
    {
        mp_raise_ValueError("b gpio error");
        return mp_const_false;
    }
    if(args_parsed[ARG_c_gpio].u_int < FUNC_GPIOHS0 || args_parsed[ARG_c_gpio].u_int > FUNC_GPIOHS31)
    {
        mp_raise_ValueError("c gpio error");
        return mp_const_false;
    }
    if(args_parsed[ARG_d_gpio].u_int < FUNC_GPIOHS0 || args_parsed[ARG_d_gpio].u_int > FUNC_GPIOHS31)
    {
        mp_raise_ValueError("d gpio error");
        return mp_const_false;
    }
    if(args_parsed[ARG_e_gpio].u_int < FUNC_GPIOHS0 || args_parsed[ARG_e_gpio].u_int > FUNC_GPIOHS31)
    {
        mp_raise_ValueError("e gpio error");
        return mp_const_false;
    }
    if(args_parsed[ARG_oe_gpio].u_int < FUNC_GPIOHS0 || args_parsed[ARG_oe_gpio].u_int > FUNC_GPIOHS31)
    {
        mp_raise_ValueError("oe gpio error");
        return mp_const_false;
    }
    if(args_parsed[ARG_latch_gpio].u_int < FUNC_GPIOHS0 || args_parsed[ARG_latch_gpio].u_int > FUNC_GPIOHS31)
    {
        mp_raise_ValueError("latch gpio error");
        return mp_const_false;
    }
    if(args_parsed[ARG_clk_pin].u_int < 0 || args_parsed[ARG_clk_pin].u_int >= FPIOA_NUM_IO)
    {
        mp_raise_ValueError("clk pin error");
        return mp_const_false;
    }
    if( ( args_parsed[ARG_dma_channel].u_int >=  DMAC_CHANNEL_MAX) )
    {
        mp_raise_ValueError("dmac channel error, should < 6");
        return mp_const_false;
    }
    
    self->hub75e_obj->spi = args_parsed[ARG_spi].u_int;
    self->hub75e_obj->cs_pin = args_parsed[ARG_cs_pin].u_int - FUNC_GPIOHS0;
    self->hub75e_obj->r1_pin = args_parsed[ARG_r1_pin].u_int;
    self->hub75e_obj->g1_pin = args_parsed[ARG_g1_pin].u_int;
    self->hub75e_obj->b1_pin = args_parsed[ARG_b1_pin].u_int;
    self->hub75e_obj->r2_pin = args_parsed[ARG_r2_pin].u_int;
    self->hub75e_obj->g2_pin = args_parsed[ARG_g2_pin].u_int;
    self->hub75e_obj->b2_pin = args_parsed[ARG_b2_pin].u_int;
    self->hub75e_obj->a_gpio = args_parsed[ARG_a_gpio].u_int - FUNC_GPIOHS0;
    self->hub75e_obj->b_gpio = args_parsed[ARG_b_gpio].u_int - FUNC_GPIOHS0;
    self->hub75e_obj->c_gpio = args_parsed[ARG_c_gpio].u_int - FUNC_GPIOHS0;
    self->hub75e_obj->d_gpio = args_parsed[ARG_d_gpio].u_int - FUNC_GPIOHS0;
    self->hub75e_obj->e_gpio = args_parsed[ARG_e_gpio].u_int - FUNC_GPIOHS0;
    self->hub75e_obj->oe_gpio = args_parsed[ARG_oe_gpio].u_int - FUNC_GPIOHS0;
    self->hub75e_obj->latch_gpio = args_parsed[ARG_latch_gpio].u_int - FUNC_GPIOHS0;
    self->hub75e_obj->clk_pin = args_parsed[ARG_clk_pin].u_int - FUNC_GPIOHS0;
    self->hub75e_obj->dma_channel = args_parsed[ARG_dma_channel].u_int;
    self->hub75e_obj->width = args_parsed[ARG_width].u_int;
    self->hub75e_obj->height = args_parsed[ARG_height].u_int;
}

mp_obj_t modules_hub75e_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) 
{
    modules_hub75e_obj_t *self = m_new_obj(modules_hub75e_obj_t);
    self->base.type = &modules_hub75e_type;
    hub75e_make_new_helper(self, n_args, n_kw, all_args);
    hub75e_init(self->hub75e_obj);
    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t py_hub75e_display(mp_obj_t self_in, size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    image_t *arg_img = py_image_cobj(args[0]);
    PY_ASSERT_TRUE_MSG(IM_IS_MUTABLE(arg_img), "Image format is not supported.");
    
    modules_hub75e_obj_t *self = MP_OBJ_TO_PTR(self_in);

    hub75e_display(self->hub75e_obj, (uint8_t *)(arg_img->pixels));
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_hub75e_display_obj, py_hub75e_display);

STATIC const mp_rom_map_elem_t mp_modules_hub75e_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_onewire) },

    { MP_ROM_QSTR(MP_QSTR_display), MP_ROM_PTR(&py_hub75e_display_obj) }
};

MP_DEFINE_CONST_DICT(mp_modules_hub75e_locals_dict, mp_modules_hub75e_locals_dict_table);

const mp_obj_type_t modules_hub75e_type = {
    { &mp_type_type },
    .name = MP_QSTR_hub75e,
    .make_new = modules_hub75e_make_new,
    .locals_dict = (mp_obj_dict_t*)&mp_modules_hub75e_locals_dict,
};
