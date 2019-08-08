#include <stdio.h>
#include <string.h>

#include "fpioa.h"
#include "gpiohs.h"
#include "sysctl.h"

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "py/mpconfig.h"

#if CONFIG_MAIXPY_WS2812_ENABLE

#include "ws2812.h"

const mp_obj_type_t modules_ws2812_type;

typedef struct {
    mp_obj_base_t         base;
    ws2812_info*          dat;

    uint8_t               len_pin;

    uint8_t               i2s_num;
    uint8_t               i2s_chn;
    uint8_t               i2s_dma_chn;
} modules_ws2812_obj_t;

mp_obj_t modules_ws2812_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) 
{
    enum {
        ARG_led_pin,
        ARG_led_num,
        ARG_i2s_num,
        ARG_i2s_chn,
        ARG_i2s_dma_chn
    };

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_led_pin,              MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_led_num,              MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = -1} },

        { MP_QSTR_i2s_num,              MP_ARG_INT, {.u_int = I2S_DEVICE_2} },
        { MP_QSTR_i2s_chn,              MP_ARG_INT, {.u_int = I2S_CHANNEL_3} },
        { MP_QSTR_i2s_dma_chn,          MP_ARG_INT, {.u_int = DMAC_CHANNEL1} },
    };

    modules_ws2812_obj_t *self = m_new_obj_with_finaliser(modules_ws2812_obj_t);
    self->base.type = &modules_ws2812_type;

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args , all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
    if( ( args[ARG_led_pin].u_int < 0 ) || ( args[ARG_led_pin].u_int > 47 ) )
    {
        mp_raise_ValueError("led pin error, should be 0 ~ 47");
    }
    self->len_pin = args[ARG_led_pin].u_int;

    if( ( args[ARG_led_num].u_int < 0 ) )
    {
        mp_raise_ValueError("len num error, should > 0");
    }

    if( ( args[ARG_i2s_num].u_int >=  I2S_DEVICE_MAX) )
    {
        mp_raise_ValueError("i2s num error, should <= 2");
    }

    if( ( args[ARG_i2s_chn].u_int >  I2S_CHANNEL_3) )
    {
        mp_raise_ValueError("i2s channel error, should <= 3");
    }

    if( ( args[ARG_i2s_dma_chn].u_int >=  DMAC_CHANNEL_MAX) )
    {
        mp_raise_ValueError("dma channel error, should <= 5");
    }

    self->dat = ws2812_init_buf(args[ARG_led_num].u_int);
    if(!(self->dat))
    {
        mp_raise_OSError(MP_ENOMEM);
    }

    ws2812_clear(self->dat);

    //set pll2 freq
    sysctl_pll_set_freq(SYSCTL_PLL2, 45158400UL);

    ws2812_init_i2s(args[ARG_led_pin].u_int,
                    args[ARG_i2s_num].u_int,
                    args[ARG_i2s_chn].u_int);

    self->i2s_num = args[ARG_i2s_num].u_int;
    self->i2s_chn = args[ARG_i2s_chn].u_int;
    self->i2s_dma_chn = args[ARG_i2s_dma_chn].u_int;

    return MP_OBJ_FROM_PTR(self);
}

STATIC void modules_ws2812_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    modules_ws2812_obj_t *self = MP_OBJ_TO_PTR(self_in);
    ws2812_info *info = (ws2812_info*)self->dat;
    mp_printf(print, "[MAIXPY]ws2812:(%p) \r\nled pin=%d num: %d buf:%p\r\ni2s: I2S_DEVICE_%d I2S_CHANNEL_%d\t DMAC_CHANNEL%d", 
                    self, self->len_pin,self->dat->ws_num,self->dat->ws_buf,self->i2s_num,self->i2s_chn,self->i2s_dma_chn);
}

STATIC void modules_ws2812_del(mp_obj_t self_in) {
    modules_ws2812_obj_t *self = MP_OBJ_TO_PTR(self_in);
    ws2812_release_buf(self->dat);
}
MP_DEFINE_CONST_FUN_OBJ_1(modules_ws2812_del_obj, modules_ws2812_del);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

STATIC mp_obj_t modules_ws2812_set_led(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum {
        ARG_num,
        ARG_color,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_num,          MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_color,        MP_ARG_OBJ, {.u_obj = mp_const_none} }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    modules_ws2812_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    ws2812_info *info = (ws2812_info*)self->dat;

    if(args[ARG_num].u_int < 0 && args[ARG_num].u_int > info->ws_num )
    {
        mp_raise_ValueError("led num error");    
    }

    uint8_t rgb[3];

    mp_obj_t color = args[ARG_color].u_obj;
    if(color != mp_const_none)
    {
        size_t size;
        mp_obj_t* tuple_data;
        mp_obj_tuple_get(color, &size, &tuple_data);
        if(size != 3)
        {
            mp_raise_ValueError("tuple size must be 3");
        }

        rgb[0] = mp_obj_get_int(tuple_data[0]);
        rgb[1] = mp_obj_get_int(tuple_data[1]);
        rgb[2] = mp_obj_get_int(tuple_data[2]);
    }else
    {
        rgb[0] = 0;
        rgb[1] = 0;
        rgb[2] = 0;
    }

    bool ret =  ws2812_set_data(info,args[ARG_num].u_int,rgb[0],rgb[1],rgb[2]);

    return ret ? mp_const_true : mp_const_false;
}
MP_DEFINE_CONST_FUN_OBJ_KW(modules_ws2812_set_led_obj, 0, modules_ws2812_set_led);

STATIC mp_obj_t modules_ws2812_dis(mp_obj_t self_in) {
    modules_ws2812_obj_t *self = MP_OBJ_TO_PTR(self_in);

    ws2812_i2s_enable_channel(self->i2s_num, self->i2s_chn);

    bool ret = ws2812_send_data_i2s(self->i2s_num, self->i2s_dma_chn, self->dat);

    // if(self->i2s_chn == I2S_CHANNEL_0)
    // {
        usleep(3*1000);
    // }
    return ret ? mp_const_true : mp_const_false;
}
MP_DEFINE_CONST_FUN_OBJ_1(modules_ws2812_dis_obj, modules_ws2812_dis);

STATIC const mp_rom_map_elem_t mp_modules_ws2812_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&modules_ws2812_del_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_led), MP_ROM_PTR(&modules_ws2812_set_led_obj) },
    { MP_ROM_QSTR(MP_QSTR_display), MP_ROM_PTR(&modules_ws2812_dis_obj) },
};

MP_DEFINE_CONST_DICT(mp_modules_ws2812_locals_dict, mp_modules_ws2812_locals_dict_table);

const mp_obj_type_t modules_ws2812_type = {
    { &mp_type_type },
    .name = MP_QSTR_ws2812,
    .print = modules_ws2812_print,
    .make_new = modules_ws2812_make_new,
    .locals_dict = (mp_obj_dict_t*)&mp_modules_ws2812_locals_dict,
};

#endif /* CONFIG_MAIXPY_WS2812_ENABLE */
