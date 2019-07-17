#include <stdio.h>
#include <string.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "py/objarray.h"
#include "py/binary.h"
#include "py_assert.h"
#include "mperrno.h"
#include "mphalport.h"
#include "modMaix.h"
#include "imlib.h"

#include "sleep.h"
#include "lcd.h"
#include "sysctl.h"
#include "fpioa.h"
#include "lib_mic.h"
#include "sipeed_sk9822.h"
#include "py_image.h"

#define PLL2_OUTPUT_FREQ 45158400UL

STATIC uint16_t colormap_parula[64] = {
    0x3935, 0x4156, 0x4178, 0x4199, 0x41ba, 0x41db, 0x421c, 0x423d,
    0x4a7e, 0x429e, 0x42df, 0x42ff, 0x431f, 0x435f, 0x3b7f, 0x3bbf,
    0x33ff, 0x2c1f, 0x2c3e, 0x2c7e, 0x2c9d, 0x24bd, 0x24dd, 0x251c,
    0x1d3c, 0x1d5c, 0x1d7b, 0x159a, 0x05ba, 0x05d9, 0x05d8, 0x0df7,
    0x1e16, 0x2615, 0x2e34, 0x3634, 0x3652, 0x3e51, 0x4e70, 0x566f,
    0x666d, 0x766c, 0x866b, 0x8e49, 0x9e48, 0xae27, 0xbe26, 0xc605,
    0xd5e4, 0xdde5, 0xe5c5, 0xf5c6, 0xfdc7, 0xfde7, 0xfe27, 0xfe46,
    0xfe86, 0xfea5, 0xf6e5, 0xf704, 0xf744, 0xf764, 0xffa3, 0xffc2};

STATIC uint16_t colormap_parula_rect[64][14 * 14] __attribute__((aligned(128)));

STATIC int init_colormap_parula_rect()
{
    for (uint32_t i = 0; i < 64; i++)
    {
        for (uint32_t j = 0; j < 14 * 14; j++)
        {
            colormap_parula_rect[i][j] = colormap_parula[i];
        }
    }
    return 0;
}
STATIC uint8_t lib_init_flag = 0;

STATIC volatile uint8_t mic_done = 0;
STATIC uint8_t thermal_map_data[256];

STATIC void lib_mic_cb(void)
{
    mic_done = 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

STATIC mp_obj_t Maix_mic_array_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    // sysctl_pll_set_freq(SYSCTL_PLL2, PLL2_OUTPUT_FREQ); //如果使用i2s,必须设置PLL2

    //evil code
    fpioa_set_function(23, FUNC_I2S0_IN_D0);
    fpioa_set_function(22, FUNC_I2S0_IN_D1);
    fpioa_set_function(21, FUNC_I2S0_IN_D2);
    fpioa_set_function(20, FUNC_I2S0_IN_D3);
    fpioa_set_function(19, FUNC_I2S0_WS);
    fpioa_set_function(18, FUNC_I2S0_SCLK);
//TODO: optimize Soft SPI
    fpioa_set_function(24, FUNC_GPIOHS0 + SK9822_DAT_GPIONUM);
    fpioa_set_function(25, FUNC_GPIOHS0 + SK9822_CLK_GPIONUM);

    // init_colormap_parula_rect();
    sipeed_init_mic_array_led();

    int ret = lib_mic_init(DMAC_CHANNEL4, lib_mic_cb, thermal_map_data);
    if(ret != 0)
    {
        char tmp[64];
        sprintf(tmp,"lib_mic init error with %d",ret);
        mp_raise_ValueError((const char*)tmp);
        return mp_const_false;
    }
    lib_init_flag = 1;
    // sysctl_enable_irq();
    return mp_const_true;
}

MP_DEFINE_CONST_FUN_OBJ_KW(Maix_mic_array_init_obj, 0, Maix_mic_array_init);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

STATIC mp_obj_t Maix_mic_array_deinit(void)
{
    if(lib_init_flag)
    {
        lib_mic_deinit();
        lib_init_flag = 0;
    }
    return mp_const_true;
}

MP_DEFINE_CONST_FUN_OBJ_0(Maix_mic_array_deinit_obj, Maix_mic_array_deinit);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

STATIC mp_obj_t Maix_mic_array_get_map(void)
{
    image_t out;

    out.w = 16;
    out.h = 16;
    out.bpp = IMAGE_BPP_GRAYSCALE;
    out.data = xalloc(256);
    mic_done = 0;

    volatile uint8_t retry = 100;

    while(mic_done == 0)
    {
        retry--;
        msleep(1);
    }

    if(mic_done == 0 && retry == 0)
    {
        xfree(out.data);
        mp_raise_OSError(MP_ETIMEDOUT);
        return mp_const_false;
    }

    memcpy(out.data, thermal_map_data, 256);

    return py_image_from_struct(&out);
}

MP_DEFINE_CONST_FUN_OBJ_0(Maix_mic_array_get_map_obj, Maix_mic_array_get_map);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

STATIC uint8_t voice_strength_len[12] = {14, 20, 14, 14, 20, 14, 14, 20, 14, 14, 20, 14};

//voice strength, to calc direction
STATIC uint8_t voice_strength[12][32] = {
    {197, 198, 199, 213, 214, 215, 228, 229, 230, 231, 244, 245, 246, 247},                               //14
    {178, 179, 192, 193, 194, 195, 196, 208, 209, 210, 211, 212, 224, 225, 226, 227, 240, 241, 242, 243}, //20
    {128, 129, 130, 131, 144, 145, 146, 147, 160, 161, 162, 163, 176, 177},
    {64, 65, 80, 81, 82, 83, 96, 97, 98, 99, 112, 113, 114, 115},
    {0, 1, 2, 3, 16, 17, 18, 19, 32, 33, 34, 35, 36, 48, 49, 50, 51, 52, 66, 67},
    {4, 5, 6, 7, 20, 21, 22, 23, 37, 38, 39, 53, 54, 55},
    {8, 9, 10, 11, 24, 25, 26, 27, 40, 41, 42, 56, 57, 58},
    {12, 13, 14, 15, 28, 29, 30, 31, 43, 44, 45, 46, 47, 59, 60, 61, 62, 63, 76, 77},
    {78, 79, 92, 93, 94, 95, 108, 109, 110, 111, 124, 125, 126, 127},
    {140, 141, 142, 143, 156, 157, 158, 159, 173, 172, 174, 175, 190, 191},
    {188, 189, 203, 204, 205, 206, 207, 219, 220, 221, 222, 223, 236, 237, 238, 239, 252, 253, 254, 255},
    {200, 201, 202, 216, 217, 218, 232, 233, 234, 235, 248, 249, 250, 251},
};

STATIC void calc_voice_strength(uint8_t *voice_data, uint8_t *led_brightness)
{
    uint32_t tmp_sum[12] = {0};
    uint8_t i, index, tmp;

    for (index = 0; index < 12; index++)
    {
        tmp_sum[index] = 0;
        for (i = 0; i < voice_strength_len[index]; i++)
        {
            tmp_sum[index] += voice_data[voice_strength[index][i]];
        }
        tmp = (uint8_t)tmp_sum[index] / voice_strength_len[index];
        led_brightness[index] = tmp > 15 ? 15 : tmp;
    }
}

STATIC mp_obj_t Maix_mic_array_get_dir(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    uint8_t led_brightness[12]={0};

    image_t *arg_img = py_image_cobj(pos_args[0]);
    PY_ASSERT_TRUE_MSG(IM_IS_MUTABLE(arg_img), "Image format is not supported.");

    if(arg_img->w!=16 || arg_img->h!=16 || arg_img->bpp!=IMAGE_BPP_GRAYSCALE)
    {
        mp_raise_ValueError("image type error, only support 16*16 grayscale image");
        return mp_const_false;
    }

    calc_voice_strength(arg_img->data, led_brightness);

    mp_obj_t *tuple, *tmp;

    tmp = (mp_obj_t *)malloc(12 * sizeof(mp_obj_t));

    for (uint8_t index = 0; index < 12; index++)
        tmp[index] = mp_obj_new_int(led_brightness[index]);

    tuple = mp_obj_new_tuple(12, tmp);

    free(tmp);

    return tuple;
}

MP_DEFINE_CONST_FUN_OBJ_KW(Maix_mic_array_get_dir_obj, 1, Maix_mic_array_get_dir);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

STATIC mp_obj_t Maix_mic_array_set_led(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    int index, brightness[12] = {0}, led_color[12] = {0}, color[3] = {0};

    mp_obj_t *items;
    mp_obj_get_array_fixed_n(pos_args[0], 12, &items);

    for(index= 0; index < 12; index++)
        brightness[index] = mp_obj_get_int(items[index]);

    mp_obj_get_array_fixed_n(pos_args[1], 3, &items);
    for(index = 0; index < 3; index++)
        color[index] = mp_obj_get_int(items[index]);

    //rgb 
    uint32_t set_color = (color[2] << 16) | (color[1] << 8) | (color[0]);

    for (index = 0; index < 12; index++)
    {
        led_color[index] = (brightness[index] / 2) > 1 ? (((0xe0 | (brightness[index] * 2)) << 24) | set_color) : 0xe0000000;
    }

//FIXME close irq?
sysctl_disable_irq();
    sk9822_start_frame();
    for (index = 0; index < 12; index++)
    {
        sk9822_send_data(led_color[index]);
    }
    sk9822_stop_frame();

sysctl_enable_irq();

    return mp_const_true;
}

MP_DEFINE_CONST_FUN_OBJ_KW(Maix_mic_array_set_led_obj, 2, Maix_mic_array_set_led);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


STATIC const mp_rom_map_elem_t Maix_mic_array_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&Maix_mic_array_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&Maix_mic_array_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_dir), MP_ROM_PTR(&Maix_mic_array_get_dir_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_led), MP_ROM_PTR(&Maix_mic_array_set_led_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_map), MP_ROM_PTR(&Maix_mic_array_get_map_obj) },
};

STATIC MP_DEFINE_CONST_DICT(Maix_mic_array_dict, Maix_mic_array_locals_dict_table);

const mp_obj_type_t Maix_mic_array_type = {
    { &mp_type_type },
    .name = MP_QSTR_MIC_ARRAY,
    .locals_dict = (mp_obj_dict_t*)&Maix_mic_array_dict,
};