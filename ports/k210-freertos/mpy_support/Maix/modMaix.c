#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include <stdio.h>

#include "modMaix.h"

#if MICROPY_PY_MACHINE

STATIC const mp_map_elem_t maix_module_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_machine) },
    { MP_ROM_QSTR(MP_QSTR_FPIOA), MP_ROM_PTR(&Maix_fpioa_type) },
    { MP_ROM_QSTR(MP_QSTR_GPIO),  MP_ROM_PTR(&Maix_gpio_type) },
    { MP_ROM_QSTR(MP_QSTR_I2S),  MP_ROM_PTR(&Maix_i2s_type) },
    { MP_ROM_QSTR(MP_QSTR_AUDIO),  MP_ROM_PTR(&Maix_audio_type) },
    { MP_ROM_QSTR(MP_QSTR_FFT),  MP_ROM_PTR(&Maix_fft_type) },
};

STATIC MP_DEFINE_CONST_DICT (
    maix_module_globals,
    maix_module_globals_table
);

const mp_obj_module_t maix_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&maix_module_globals,
};

#endif // MICROPY_PY_MACHINE
