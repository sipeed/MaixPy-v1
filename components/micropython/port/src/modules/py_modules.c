#include "py/mpconfig.h"
#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include <stdio.h>
#include "mod_modules.h"
#include "sysctl.h"


#if MAIXPY_PY_MODULES

STATIC const mp_map_elem_t modules_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_modules) },
    { MP_ROM_QSTR(MP_QSTR_ultrasonic),  MP_ROM_PTR(&modules_ultrasonic_type) },
    { MP_ROM_QSTR(MP_QSTR_onewire),  MP_ROM_PTR(&modules_onewire_type) },
#if CONFIG_MAIXPY_WS2812_ENABLE
    { MP_ROM_QSTR(MP_QSTR_ws2812),  MP_ROM_PTR(&modules_ws2812_type) },
#endif
#if CONFIG_MAIXPY_HTPA_ENABLE
    { MP_ROM_QSTR(MP_QSTR_htpa),  MP_ROM_PTR(&modules_htpa_type) },
#endif
#if CONFIG_MAIXPY_AMG88XX_ENABLE
    { MP_ROM_QSTR(MP_QSTR_amg88xx),  MP_ROM_PTR(&modules_amg88xx_type) },
#endif
};

STATIC MP_DEFINE_CONST_DICT (
    modules_globals,
    modules_globals_table
);

const mp_obj_module_t mp_module_modules = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&modules_globals,
};

#endif // MAIXPY_PY_MODULES
