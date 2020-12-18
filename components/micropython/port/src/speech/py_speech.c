#include "py/mpconfig.h"
#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include <stdio.h>
#include "mod_speech_recognizer.h"
#include "sysctl.h"

#if CONFIG_MAIXPY_SPEECH_RECOGNIZER_ENABLE

STATIC const mp_map_elem_t speech_recognizer_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_speech_recognizer) },
    { MP_ROM_QSTR(MP_QSTR_isolated_word),  MP_ROM_PTR(&speech_isolated_word_type) },
    { MP_ROM_QSTR(MP_QSTR_asr),  MP_ROM_PTR(&speech_asr_type) },
};

STATIC MP_DEFINE_CONST_DICT (
    speech_recognizer_globals,
    speech_recognizer_globals_table
);

const mp_obj_module_t mp_module_speech_recognizer = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&speech_recognizer_globals,
};

#endif // MAIXPY_PY_SPEECH
