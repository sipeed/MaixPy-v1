#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include <stdio.h>

#include "mod_test.h"
#include "w25qxx.h"

/*****API****/
#include "sipeed_gpio.h"

typedef struct test_gpio_obj_t {
    mp_obj_base_t base;
} test_gpio_obj_t;
const mp_obj_type_t test_gpio_type;

STATIC mp_obj_t test_gpio_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
	test_gpio();
    test_gpio_obj_t *self = m_new_obj(test_gpio_obj_t);
    self->base.type = &test_gpio_type;
    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t test_gpio_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    test_gpio();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(test_gpio_init_obj, 0, test_gpio_init);
STATIC const mp_rom_map_elem_t test_gpio_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&test_gpio_init_obj) },
};

STATIC MP_DEFINE_CONST_DICT(test_gpio_locals_dict, test_gpio_locals_dict_table);

const mp_obj_type_t test_gpio_type= {
    { &mp_type_type },
    .name = MP_QSTR_test_gpio,
    .make_new = test_gpio_make_new,
    .locals_dict = (mp_obj_dict_t*)&test_gpio_locals_dict,
};

//******************************************************************************************

typedef struct test_flash_obj_t {
    mp_obj_base_t base;
} test_flash_obj_t;
const mp_obj_type_t test_flash_type;

STATIC mp_obj_t test_flash_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    test_flash_obj_t *self = m_new_obj(test_flash_obj_t);
    self->base.type = &test_flash_type;
    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t test_flash_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    w25qxx_init_dma(3, 0);
    uint8_t manuf_id;uint8_t device_id;
    w25qxx_read_id_dma(&manuf_id, &device_id);
    printf("manuf_id %x device_id %x\n",manuf_id,device_id);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(test_flash_init_obj, 0, test_flash_init);
STATIC const mp_rom_map_elem_t test_flash_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&test_flash_init_obj) },
};

STATIC MP_DEFINE_CONST_DICT(test_flash_locals_dict, test_flash_locals_dict_table);

const mp_obj_type_t test_flash_type= {
    { &mp_type_type },
    .name = MP_QSTR_test_flash,
    .make_new = test_flash_make_new,
    .locals_dict = (mp_obj_dict_t*)&test_flash_locals_dict,
};

//******************************************************************************************

STATIC const mp_map_elem_t test_module_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_test) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_gpio), MP_ROM_PTR(&test_gpio_type) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_flash), MP_ROM_PTR(&test_flash_type) },

};

STATIC MP_DEFINE_CONST_DICT (
    test_module_globals,
    test_module_globals_table
);

const mp_obj_module_t test_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&test_module_globals,
};