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

#include "py/nlr.h"
#include "py/obj.h"
#include "py/binary.h"

#include "py/runtime.h"
#include "py/stream.h"
#include "py/mperrno.h"
#include "modmachine.h"

#include "py/parsenum.h"

#include <math.h>
#include "py/formatfloat.h"

#include "uart.h"
#define UART_NUM_MAX 3
typedef struct _machine_uart_obj_t {
    mp_obj_base_t base;
    uint32_t baudrate;
    uint8_t uart_num;
    uint8_t bits;
    uart_parity_t parity;
    uart_stopbit_t stop;
} machine_uart_obj_t;

STATIC const char *_parity_name[] = {"None", "1", "0"};

//QueueHandle_t UART_QUEUE[UART_NUM_MAX] = {};

/******************************************************************************/
// MicroPython bindings for UART

STATIC void machine_uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    //uart_get_baudrate(self->uart_num, &baudrate);
    mp_printf(print, "[MAIXPY]UART%d:( baudrate=%u, bits=%u, parity=%s, stop=%u)",
        self->uart_num,self->baudrate, self->bits, _parity_name[self->parity],
        self->stop);
}

STATIC void machine_uart_init_helper(machine_uart_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_baudrate, ARG_bits, ARG_parity, ARG_stop};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 115200} },
        { MP_QSTR_bits, MP_ARG_INT, {.u_int = UART_BITWIDTH_8BIT} },
        { MP_QSTR_parity, MP_ARG_INT, {.u_int = UART_PARITY_NONE} },
        { MP_QSTR_stop, MP_ARG_INT, {.u_int = UART_STOP_1} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // set baudrate
    if (args[ARG_baudrate].u_int > 0) {
        self->baudrate =args[ARG_baudrate].u_int;
    }

    // set data bits
    if (args[ARG_bits].u_int >=5 && args[ARG_bits].u_int <=8) {
            self->bits=args[ARG_bits].u_int;
    }else{
            mp_raise_ValueError("[MAIXPY]UART:invalid data bits");
	    return;
    }

    // set parity
    
    if (args[ARG_parity].u_int <= UART_PARITY_EVEN) {
		self->parity = args[ARG_parity].u_int;
    }
	else{
		mp_raise_ValueError("[MAIXPY]UART:invalid parity");
	}

    // set stop bits
    if(args[ARG_stop].u_int <= UART_STOP_2)
    {
	    switch (args[ARG_stop].u_int) {
	        case UART_STOP_1:
	            self->stop = UART_STOP_1;
	            break;
	        case UART_STOP_1_5:
	            self->stop = UART_STOP_1_5;
	            break;
	        case UART_STOP_2:
	            self->stop = UART_STOP_2;
	            break;
	        default:
	            mp_raise_ValueError("[MAIXPY]UART:invalid stop bits");
	            break;
	    }
    }
    uart_init(self->uart_num);
    uart_config(self->uart_num, (size_t)self->baudrate, (size_t)self->bits, self->stop,  self->parity);
}

STATIC mp_obj_t machine_uart_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get uart id
    mp_int_t uart_num = mp_obj_get_int(args[0]);
    if (uart_num < 0 || uart_num > UART_NUM_MAX) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "[MAIXPY]UART%b:does not exist", uart_num));
    }
    // create instance
    machine_uart_obj_t *self = m_new_obj(machine_uart_obj_t);
    self->base.type = &machine_uart_type;
    self->uart_num = uart_num;
    self->baudrate = 0;
    self->bits = 8;
    self->parity = 0;
    self->stop = 1;

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    machine_uart_init_helper(self, n_args - 1, args + 1, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t machine_uart_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    machine_uart_init_helper(args[0], n_args -1 , args + 1, kw_args);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_uart_init_obj, 0, machine_uart_init);


STATIC const mp_rom_map_elem_t machine_uart_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_uart_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },

	{ MP_ROM_QSTR(MP_QSTR_UART1), MP_ROM_INT(0) },
	{ MP_ROM_QSTR(MP_QSTR_UART2), MP_ROM_INT(1) },
	{ MP_ROM_QSTR(MP_QSTR_UART3), MP_ROM_INT(2) },
};

STATIC MP_DEFINE_CONST_DICT(machine_uart_locals_dict, machine_uart_locals_dict_table);

STATIC mp_uint_t machine_uart_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // make sure we want at least 1 char
    if (size == 0) {
        return 0;
    }

    int bytes_read = uart_receive_data(self->uart_num, buf_in, size);

    if (bytes_read < 0) {
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }

    return bytes_read;
}

STATIC mp_uint_t machine_uart_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int bytes_written=uart_send_data(self->uart_num,buf_in,size);
    if (bytes_written < 0) {
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }

    // return number of bytes written
    return bytes_written;
}

STATIC mp_uint_t machine_uart_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t arg, int *errcode) {
    machine_uart_obj_t *self = self_in;
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

STATIC const mp_stream_p_t uart_stream_p = {
    .read = machine_uart_read,
    .write = machine_uart_write,
    .ioctl = machine_uart_ioctl,
    .is_text = false,
};

const mp_obj_type_t machine_uart_type = {
    { &mp_type_type },
    .name = MP_QSTR_UART,
    .print = machine_uart_print,
    .make_new = machine_uart_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &uart_stream_p,
    .locals_dict = (mp_obj_dict_t*)&machine_uart_locals_dict,
};
