#include <stdio.h>
#include <string.h>
#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "py/mpconfig.h"
#include "fpioa.h"
#include "gpio.h"
#include "gpiohs.h"
#include "sleep.h"
#include "sysctl.h"
#include "printf.h"

const mp_obj_type_t modules_onewire_type;
typedef void (*set_pin_func_t)(uint8_t gpio, gpio_pin_value_t value);
typedef gpio_pin_value_t (*get_pin_func_t)(uint8_t gpio);
typedef void (*set_mode_func_t)(uint8_t gpio, gpio_drive_mode_t mode);

typedef struct {
    mp_obj_base_t         base;
    uint8_t               gpio;
    set_pin_func_t        set_pin;
    get_pin_func_t        get_pin;
    set_mode_func_t       set_mode;
} modules_onewire_obj_t;

/******************************************************************************/
STATIC int onewire_bus_reset(modules_onewire_obj_t *self) {
	uint8_t retries = 125;
    sysctl_disable_irq();
    self->set_mode(self->gpio, GPIO_DM_INPUT);
    sysctl_enable_irq();
	do {
		if (--retries == 0) return 0;
		mp_hal_delay_us(2);
	} while ( !self->get_pin(self->gpio) );
    sysctl_disable_irq();
    self->set_mode(self->gpio, GPIO_DM_OUTPUT);
    self->set_pin(self->gpio, GPIO_PV_LOW);
    sysctl_enable_irq();
    mp_hal_delay_us(480);
    sysctl_disable_irq();
    self->set_mode(self->gpio, GPIO_DM_INPUT);
    mp_hal_delay_us(70);
    int status = !self->get_pin(self->gpio);
    sysctl_enable_irq();
    mp_hal_delay_us(410);
    return status;
}

STATIC int onewire_bus_readbit(modules_onewire_obj_t *self) {
    sysctl_disable_irq();
    self->set_mode(self->gpio, GPIO_DM_OUTPUT);
    self->set_pin(self->gpio, GPIO_PV_LOW);
    mp_hal_delay_us(3);
    self->set_mode(self->gpio, GPIO_DM_INPUT);
    mp_hal_delay_us(10);
    int value = self->get_pin(self->gpio);
    sysctl_enable_irq();
    mp_hal_delay_us(53);
    return value;
}

STATIC void onewire_bus_writebit(modules_onewire_obj_t *self, int value) {
    if (value) {
        sysctl_disable_irq();
        self->set_mode(self->gpio, GPIO_DM_OUTPUT);
        self->set_pin(self->gpio, GPIO_PV_LOW);
        mp_hal_delay_us(10);
        self->set_pin(self->gpio, GPIO_PV_HIGH);
        sysctl_enable_irq();
        mp_hal_delay_us(55);
    }else{
        sysctl_disable_irq();
        self->set_mode(self->gpio, GPIO_DM_OUTPUT);
        self->set_pin(self->gpio, GPIO_PV_LOW);
        mp_hal_delay_us(65);
        self->set_pin(self->gpio, GPIO_PV_HIGH);
        sysctl_enable_irq();
        mp_hal_delay_us(5);
    }
}

STATIC void onewire_bus_depower(modules_onewire_obj_t *self) {
    sysctl_disable_irq();
    self->set_mode(self->gpio, GPIO_DM_INPUT);
    sysctl_enable_irq();
}

STATIC int onewire_bus_search(modules_onewire_obj_t *self, uint8_t* l_rom, int* diff) {
    int i = 64;
    uint8_t rom[8];
    int next_diff = 0;
    if(!onewire_bus_reset(self)){
        return -1;
    }
    int search_rom = 0xF0;
    for (int i = 0; i < 8; ++i) {
        onewire_bus_writebit(self, search_rom & 1);
        search_rom >>= 1;
    }
    for(int byte=0; byte<8; ++byte){
        int r_b = 0;
        for(int bit=0; bit<8; ++bit){
            int b = onewire_bus_readbit(self);
            if(onewire_bus_readbit(self)){
                if(b){
                    return -2;
                }
            }else{
                if(!b){
                    if(*diff > i || ((l_rom[byte] & (1 << bit)) && *diff != i)){
                        b = 1;
                        next_diff = i;
                    }
                }
            }
            onewire_bus_writebit(self, b);
            if(b){
                r_b |= 1 << bit;
            }
            i -= 1;
            rom[byte] = r_b;
        }
    }
    memcpy(l_rom, rom, 8);
    *diff = next_diff;
    return 0;
}

/******************************************************************************/
// MicroPython bindings
mp_obj_t modules_onewire_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    modules_onewire_obj_t *self = m_new_obj(modules_onewire_obj_t);
    self->base.type = &modules_onewire_type;
    enum {
        ARG_gpio
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_gpio,        MP_ARG_REQUIRED|MP_ARG_INT, {.u_int = 0} }
    };
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args , all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if( fpioa_get_io_by_function(args[ARG_gpio].u_int) < 0 )
        mp_raise_ValueError("gpio error");
    if(args[ARG_gpio].u_int >= FUNC_GPIO0 && args[ARG_gpio].u_int <= FUNC_GPIO7)
    {
        self->set_mode = gpio_set_drive_mode;
        self->set_pin = gpio_set_pin;
        self->get_pin = gpio_get_pin;
        self->gpio = args[ARG_gpio].u_int - FUNC_GPIO0;
    }
    else if(args[ARG_gpio].u_int >= FUNC_GPIOHS0 && args[ARG_gpio].u_int <= FUNC_GPIOHS31)
    {
        self->set_mode = gpiohs_set_drive_mode;
        self->set_pin = gpiohs_set_pin;
        self->get_pin = gpiohs_get_pin;
        self->gpio = args[ARG_gpio].u_int - FUNC_GPIOHS0;
    }
    else
    {
        mp_raise_ValueError("gpio error");
    }

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t onewire_reset(mp_obj_t self_in) {
    modules_onewire_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(onewire_bus_reset(self));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(onewire_reset_obj, onewire_reset);

STATIC mp_obj_t onewire_readbit(mp_obj_t self_in) {
    modules_onewire_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return MP_OBJ_NEW_SMALL_INT(onewire_bus_readbit(self));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(onewire_readbit_obj, onewire_readbit);

STATIC mp_obj_t onewire_readbyte(mp_obj_t self_in) {
    modules_onewire_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value |= onewire_bus_readbit(self) << i;
    }
    return MP_OBJ_NEW_SMALL_INT(value);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(onewire_readbyte_obj, onewire_readbyte);

STATIC mp_obj_t onewire_readbuffer(mp_obj_t self_in, mp_obj_t count_in) {
    modules_onewire_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t count = mp_obj_get_int(count_in);
    uint8_t data[count];
    for (int i = 0; i < count; ++i) {
        uint8_t value = 0;
        for (int i = 0; i < 8; ++i) {
            value |= onewire_bus_readbit(self) << i;
        }
        data[i] = value;
    }
    return mp_obj_new_bytearray(count, data);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(onewire_readbuffer_obj, onewire_readbuffer);

STATIC mp_obj_t onewire_writebit(mp_obj_t self_in, mp_obj_t value_in) {
    modules_onewire_obj_t* self = MP_OBJ_TO_PTR(self_in);
    mp_int_t value = mp_obj_get_int(value_in);
    onewire_bus_writebit(self, value);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(onewire_writebit_obj, onewire_writebit);

STATIC mp_obj_t onewire_writebyte(mp_obj_t self_in, mp_obj_t value_in) {
    modules_onewire_obj_t* self = MP_OBJ_TO_PTR(self_in);
    mp_int_t value = mp_obj_get_int(value_in);
    for (int i = 0; i < 8; ++i) {
        onewire_bus_writebit(self, value & 1);
        value >>= 1;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(onewire_writebyte_obj, onewire_writebyte);

STATIC mp_obj_t onewire_writebuffer(mp_obj_t self_in, mp_obj_t data_in) {
    mp_buffer_info_t bufinfo;
    modules_onewire_obj_t* self = MP_OBJ_TO_PTR(self_in);
    mp_get_buffer_raise(data_in, &bufinfo, MP_BUFFER_READ);
    for (size_t i = 0; i < bufinfo.len; ++i) {
        uint8_t byte = ((uint8_t*)bufinfo.buf)[i];
        for (int i = 0; i < 8; ++i) {
            onewire_bus_writebit(self, byte & 1);
            byte >>= 1;
        }
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(onewire_writebuffer_obj, onewire_writebuffer);

STATIC mp_obj_t onewire_select(mp_obj_t self_in, mp_obj_t rom_in) {
    mp_int_t match_rom = 0x55;
    mp_buffer_info_t bufinfo;
    modules_onewire_obj_t* self = MP_OBJ_TO_PTR(self_in);
    mp_get_buffer_raise(rom_in, &bufinfo, MP_BUFFER_READ);
    onewire_reset(self_in);
    for (int i = 0; i < 8; ++i) {
        onewire_bus_writebit(self, match_rom & 1);
        match_rom >>= 1;
    }
    for (size_t i = 0; i < bufinfo.len; ++i) {
        uint8_t byte = ((uint8_t*)bufinfo.buf)[i];
        for (int i = 0; i < 8; ++i) {
            onewire_bus_writebit(self, byte & 1);
            byte >>= 1;
        }
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(onewire_select_obj, onewire_select);

STATIC mp_obj_t onewire_search(mp_obj_t self_in, mp_obj_t diff_in) {
    modules_onewire_obj_t* self = MP_OBJ_TO_PTR(self_in);
    mp_int_t diff = mp_obj_get_int(diff_in);
    uint8_t rom[8];
    mp_obj_t roms[diff];
    mp_int_t depth = 0;
    for(int i=0; i<0xff; ++i){
        if(onewire_bus_search(self, rom, (int*)&diff) == 0){
            roms[depth++] = mp_obj_new_bytearray(8, rom);
            if(!diff){
                break;
            }
        }
    }
    return mp_obj_new_list(depth, roms);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(onewire_search_obj, onewire_search);

STATIC mp_obj_t onewire_skip(mp_obj_t self_in) {
    modules_onewire_obj_t* self = MP_OBJ_TO_PTR(self_in);
    mp_int_t skip_rom = 0xCC;
    for (int i = 0; i < 8; ++i) {
        onewire_bus_writebit(self, skip_rom & 1);
        skip_rom >>= 1;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(onewire_skip_obj, onewire_skip);

STATIC mp_obj_t onewire_depower(mp_obj_t self_in) {
    onewire_bus_depower(self_in);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(onewire_depower_obj, onewire_depower);

STATIC mp_obj_t onewire_crc8(mp_obj_t self_in, mp_obj_t data_in) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(data_in, &bufinfo, MP_BUFFER_READ);
    uint8_t crc = 0;
    for (size_t i = 0; i < bufinfo.len; ++i) {
        uint8_t byte = ((uint8_t*)bufinfo.buf)[i];
        for (int b = 0; b < 8; ++b) {
            uint8_t fb_bit = (crc ^ byte) & 0x01;
            if (fb_bit == 0x01) {
                crc = crc ^ 0x18;
            }
            crc = (crc >> 1) & 0x7f;
            if (fb_bit == 0x01) {
                crc = crc | 0x80;
            }
            byte = byte >> 1;
        }
    }
    return MP_OBJ_NEW_SMALL_INT(crc);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(onewire_crc8_obj, onewire_crc8);

STATIC void modules_onewire_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    modules_onewire_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "[Maixduino]onewire:(%p) gpio=%d\r\n", self, self->gpio);
}

STATIC const mp_rom_map_elem_t mp_modules_onewire_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_onewire) },

    { MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&onewire_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_readbit), MP_ROM_PTR(&onewire_readbit_obj) },
    { MP_ROM_QSTR(MP_QSTR_readbyte), MP_ROM_PTR(&onewire_readbyte_obj) },
    { MP_ROM_QSTR(MP_QSTR_readbuffer), MP_ROM_PTR(&onewire_readbuffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_writebit), MP_ROM_PTR(&onewire_writebit_obj) },
    { MP_ROM_QSTR(MP_QSTR_writebyte), MP_ROM_PTR(&onewire_writebyte_obj) },
    { MP_ROM_QSTR(MP_QSTR_writebuffer), MP_ROM_PTR(&onewire_writebuffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_select), MP_ROM_PTR(&onewire_select_obj) },
    { MP_ROM_QSTR(MP_QSTR_search), MP_ROM_PTR(&onewire_search_obj) },
    { MP_ROM_QSTR(MP_QSTR_skip), MP_ROM_PTR(&onewire_skip_obj) },
    { MP_ROM_QSTR(MP_QSTR_depower), MP_ROM_PTR(&onewire_depower_obj) },
    { MP_ROM_QSTR(MP_QSTR_crc8), MP_ROM_PTR(&onewire_crc8_obj) },
};

MP_DEFINE_CONST_DICT(mp_modules_onewire_locals_dict, mp_modules_onewire_locals_dict_table);

const mp_obj_type_t modules_onewire_type = {
    { &mp_type_type },
    .name = MP_QSTR_onewire,
    .print = modules_onewire_print,
    .make_new = modules_onewire_make_new,
    .locals_dict = (mp_obj_dict_t*)&mp_modules_onewire_locals_dict,
};
