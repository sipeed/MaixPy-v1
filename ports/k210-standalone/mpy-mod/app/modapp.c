#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include <stdio.h>

#include "modapp.h"

STATIC const mp_map_elem_t app_module_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_app) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_vi), MP_ROM_PTR(&app_vi_type) },

};

STATIC MP_DEFINE_CONST_DICT (
    app_module_globals,
    app_module_globals_table
);

const mp_obj_module_t app_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&app_module_globals,
};
