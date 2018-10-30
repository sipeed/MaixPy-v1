/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Josef Gajdusek
 * Copyright (c) 2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <string.h>

#include "py/objtuple.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "extmod/misc.h"
#if MICROPY_VFS
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"
#endif
#include "genhdr/mpversion.h"
#if !MICROPY_VFS
#include "spiffs-port.h"
#include "py/lexer.h"
#endif

extern int vi_main(unsigned char * fn);

typedef struct app_vi_obj_t {
    mp_obj_base_t base;
} app_vi_obj_t;
const mp_obj_type_t app_vi_type;

STATIC mp_obj_t app_vi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    const char* path = mp_obj_str_get_str(args[0]);
	vi_main(path);
    app_vi_obj_t *self = m_new_obj(app_vi_obj_t);
    self->base.type = &app_vi_type;
    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t app_vi_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    const char* path = mp_obj_str_get_str(args[0]);
	vi_main(path);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(app_vi_init_obj, 0, app_vi_init);

STATIC const mp_rom_map_elem_t app_vi_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&app_vi_init_obj) },
};

STATIC MP_DEFINE_CONST_DICT(app_vi_locals_dict, app_vi_locals_dict_table);

const mp_obj_type_t app_vi_type= {
    { &mp_type_type },
    .name = MP_QSTR_vi,
    .make_new = app_vi_make_new,
    .locals_dict = (mp_obj_dict_t*)&app_vi_locals_dict,
};
