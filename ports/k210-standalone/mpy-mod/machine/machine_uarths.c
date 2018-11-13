/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "py/runtime.h"
#include "py/stream.h"
#include "py/mperrno.h"
#include "modmachine.h"

#include "uarths.h"

typedef struct _machine_uarths_obj_t {
    mp_obj_base_t base;
    uint16_t baudrate;
    uint8_t stop;
} machine_uarths_obj_t;

STATIC const char *_parity_name[] = {"None", "1", "0"};

//QueueHandle_t UART_QUEUE[UART_NUM_MAX] = {};

/******************************************************************************/
// MicroPython bindings for UART

STATIC void machine_uarths_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_uarths_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "[MAIXPY]UARTHS:baudrate=%u, stop=%u)",
        self->baudrate,self->stop);
}

STATIC void machine_uarths_init_helper(machine_uarths_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_baudrate, ARG_stop};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 115200} },
        { MP_QSTR_stop, MP_ARG_INT, {.u_int = 1} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // set baudrate

    if (args[ARG_baudrate].u_int > 0) {
        uarths_init();
        self->baudrate=args[ARG_baudrate].u_int;
	    uarths_config(args[ARG_baudrate].u_int,args[ARG_stop].u_int==1?UARTHS_STOP_1:UARTHS_STOP_2);
    }else{
        mp_raise_ValueError("[MAIXPY]UARTHS:Please enter the correct baudrate");
    }

}

STATIC mp_obj_t machine_uarths_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // create instance
    machine_uarths_obj_t *self = m_new_obj(machine_uarths_obj_t);
    self->base.type = &machine_uarths_type;
    self->stop = 1;

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    machine_uarths_init_helper(self, n_args, args, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t machine_uarths_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    machine_uarths_init_helper(args[0], n_args, args, kw_args);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_uarths_init_obj, 0, machine_uarths_init);

STATIC const mp_rom_map_elem_t machine_uarths_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_uarths_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
};

STATIC MP_DEFINE_CONST_DICT(machine_uarths_locals_dict, machine_uarths_locals_dict_table);

STATIC mp_uint_t machine_uarths_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    machine_uarths_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // make sure we want at least 1 char
    if (size == 0) {
        return 0;
    }

    int bytes_read = size;
    //int bytes_read = uarths_read_bytes(self->uarths_num, buf_in, size, time_to_wait);

    //if (bytes_read < 0) {
    //    *errcode = MP_EAGAIN;
    //    return MP_STREAM_ERROR;
    //}
    while(bytes_read--)
         *((char *)(buf_in)++)=uarths_getc();

    return size;
}

STATIC mp_uint_t machine_uarths_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode) {
    machine_uarths_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int bytes_written = size;//uarths_write_bytes(self->uarths_num, buf_in, size);
    while(bytes_written--)
    	uarths_putchar(*((char *)(buf_in)++));
    if (bytes_written < 0) {
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }
    // return number of bytes written
    return size;
}

STATIC mp_uint_t machine_uarths_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t arg, int *errcode) {
    machine_uarths_obj_t *self = self_in;
    mp_uint_t ret;
    if (request == MP_STREAM_POLL) {
        mp_uint_t flags = arg;
        ret = 0;
    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}

STATIC const mp_stream_p_t uarths_stream_p = {
    .read = machine_uarths_read,
    .write = machine_uarths_write,
    .ioctl = machine_uarths_ioctl,
    .is_text = false,
};

const mp_obj_type_t machine_uarths_type = {
    { &mp_type_type },
    .name = MP_QSTR_UART,
    .print = machine_uarths_print,
    .make_new = machine_uarths_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &uarths_stream_p,
    .locals_dict = (mp_obj_dict_t*)&machine_uarths_locals_dict,
};
