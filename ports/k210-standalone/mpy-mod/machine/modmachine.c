#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include <stdio.h>

#include "modmachine.h"

#if MICROPY_PY_MACHINE

STATIC const mp_map_elem_t machine_module_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_machine) },
    { MP_ROM_QSTR(MP_QSTR_uarths), MP_ROM_PTR(&machine_uarths_type) },
    { MP_ROM_QSTR(MP_QSTR_uart), MP_ROM_PTR(&machine_uart_type) },
    { MP_ROM_QSTR(MP_QSTR_GPIO), MP_ROM_PTR(&machine_pin_type) },
    { MP_ROM_QSTR(MP_QSTR_pwm), MP_ROM_PTR(&machine_pwm_type) },
    { MP_ROM_QSTR(MP_QSTR_timer), MP_ROM_PTR(&machine_timer_type) },
    { MP_ROM_QSTR(MP_QSTR_st7789), MP_ROM_PTR(&machine_st7789_type) },
    { MP_ROM_QSTR(MP_QSTR_ov2640), MP_ROM_PTR(&machine_ov2640_type) },
    { MP_ROM_QSTR(MP_QSTR_burner), MP_ROM_PTR(&machine_burner_type) },
    { MP_ROM_QSTR(MP_QSTR_face_detect), MP_ROM_PTR(&machine_demo_face_detect_type) },
    { MP_ROM_QSTR(MP_QSTR_spiflash), MP_ROM_PTR(&machine_spiflash_type) },
    { MP_ROM_QSTR(MP_QSTR_zmodem), MP_ROM_PTR(&machine_zmodem_type) },
    { MP_ROM_QSTR(MP_QSTR_fpioa), MP_ROM_PTR(&machine_fpioa_type) },
    { MP_ROM_QSTR(MP_QSTR_ws2812), MP_ROM_PTR(&machine_ws2812_type) },
    { MP_ROM_QSTR(MP_QSTR_test), MP_ROM_PTR(&machine_test_type) },
    { MP_ROM_QSTR(MP_QSTR_devmem), MP_ROM_PTR(&machine_devmem_type) },
    { MP_ROM_QSTR(MP_QSTR_esp8285), MP_ROM_PTR(&machine_esp8285_type) },
};

STATIC MP_DEFINE_CONST_DICT (
    machine_module_globals,
    machine_module_globals_table
);

const mp_obj_module_t machine_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&machine_module_globals,
};

#endif // MICROPY_PY_MACHINE
