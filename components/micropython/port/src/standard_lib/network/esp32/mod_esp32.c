#include <string.h>
#include "py/objtuple.h"
#include "py/objlist.h"
#include "py/stream.h"
#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "lib/netutils/netutils.h"
#include "modnetwork.h"
#include "modmachine.h"
#include "mpconfigboard.h"

#include "esp32_spi.h"
#include "esp32_spi_io.h"
#include "fpioa.h"

typedef struct _esp32_nic_obj_t
{
    mp_obj_base_t base;

} esp32_nic_obj_t;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

STATIC void esp32_make_new_helper(esp32_nic_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    int cs, rst, rdy, mosi, miso, sclk;

    enum
    {
        ARG_cs,
        ARG_rst,
        ARG_rdy,
        ARG_mosi,
        ARG_miso,
        ARG_sclk,
    };

    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_cs, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1}},
        {MP_QSTR_rst, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1}},
        {MP_QSTR_rdy, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1}},
        {MP_QSTR_mosi, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1}},
        {MP_QSTR_miso, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1}},
        {MP_QSTR_sclk, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1}},
    };

    mp_arg_val_t args_parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args_parsed);

    //cs
    cs = args_parsed[ARG_cs].u_int;
    if (cs == -1 || cs > FUNC_GPIOHS31 || cs < FUNC_GPIOHS0)
    {
        mp_raise_ValueError("gpiohs cs value error!");
    }

    //rst
    rst = args_parsed[ARG_rst].u_int;
    if (rst != -1)//no rst we will use soft reset
    {
        if (rst > FUNC_GPIOHS31 || rst < FUNC_GPIOHS0)
        {
            mp_raise_ValueError("gpiohs rst value error!");
        }
    }

    //rdy
    rdy = args_parsed[ARG_rdy].u_int;
    if (rdy == -1 || rdy > FUNC_GPIOHS31 || rdy < FUNC_GPIOHS0)
    {
        mp_raise_ValueError("gpiohs rdy value error!");
    }

    //mosi
    mosi = args_parsed[ARG_mosi].u_int;
    if (mosi == -1 || mosi > FUNC_GPIOHS31 || mosi < FUNC_GPIOHS0)
    {
        mp_raise_ValueError("gpiohs mosi value error!");
    }

    //miso
    miso = args_parsed[ARG_miso].u_int;
    if (miso == -1 || miso > FUNC_GPIOHS31 || miso < FUNC_GPIOHS0)
    {
        mp_raise_ValueError("gpiohs miso value error!");
    }

    //sclk
    sclk = args_parsed[ARG_sclk].u_int;
    if (sclk == -1 || sclk > FUNC_GPIOHS31 || sclk < FUNC_GPIOHS0)
    {
        mp_raise_ValueError("gpiohs sclk value error!");
    }

    esp32_spi_config_io(cs - FUNC_GPIOHS0, rst, rdy - FUNC_GPIOHS0,
                        mosi - FUNC_GPIOHS0, miso - FUNC_GPIOHS0, sclk - FUNC_GPIOHS0);
    esp32_spi_init();
    mp_printf(&mp_plat_print, "ESP32_SPI init over\r\n");
}

//network.ESP32_SPI(cs=1,rst=2,rdy=3,mosi=4,miso=5,sclk=6)
STATIC mp_obj_t esp32_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    if (n_args != 0 || n_kw != 6)
    {
        mp_raise_ValueError("error argument");
        return mp_const_none;
    }
    esp32_nic_obj_t *self = m_new_obj(esp32_nic_obj_t);
    self->base.type = (mp_obj_type_t *)&mod_network_nic_type_esp32;
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    esp32_make_new_helper(self, n_args, args, &kw_args);
    mod_network_register_nic((mp_obj_t)self);
    return MP_OBJ_FROM_PTR(self);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

STATIC mp_obj_t esp32_adc(mp_obj_t self_in)
{
    uint16_t adc[ESP32_ADC_CH_NUM];

    if (esp32_spi_get_adc_val(adc) == 0)
    {
        mp_obj_t *tuple, *tmp;

        tmp = (mp_obj_t *)malloc(ESP32_ADC_CH_NUM * sizeof(mp_obj_t));

        for (uint8_t index = 0; index < ESP32_ADC_CH_NUM; index++)
            tmp[index] = mp_obj_new_int(adc[index]);

        tuple = mp_obj_new_tuple(ESP32_ADC_CH_NUM, tmp);

        free(tmp);
        return tuple;
    }
    else
    {
        mp_raise_ValueError("[MaixPy]: esp32 read adc failed!\r\n");
        return mp_const_false;
    }

    return mp_const_false;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp32_adc_obj, esp32_adc);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

STATIC const mp_rom_map_elem_t esp32_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR_adc), MP_ROM_PTR(&esp32_adc_obj)},
};

STATIC MP_DEFINE_CONST_DICT(esp32_locals_dict, esp32_locals_dict_table);

const mod_network_nic_type_t mod_network_nic_type_esp32 = {
    .base = {
        {&mp_type_type},
        .name = MP_QSTR_ESP32_SPI,
        .make_new = esp32_make_new,
        .locals_dict = (mp_obj_dict_t *)&esp32_locals_dict,
    },
    /*
    .gethostbyname = esp8285_socket_gethostbyname,
    .connect = esp8285_socket_connect,
    .socket = esp8285_socket_socket,
    .send = esp8285_socket_send,
    .recv = esp8285_socket_recv,
    .close = esp8285_socket_close,
    .bind = cc3k_socket_bind,
    .listen = cc3k_socket_listen,
    .accept = cc3k_socket_accept,
    .sendto = cc3k_socket_sendto,
    .recvfrom = cc3k_socket_recvfrom,
    .setsockopt = cc3k_socket_setsockopt,
    .settimeout = cc3k_socket_settimeout,
    .ioctl = cc3k_socket_ioctl,
*/
};