/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2015 Damien P. George
 * Copyright (c) 2016 Paul Sokolovsky
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


#include "stdlib.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "esp8285.h"
#include "sleep.h"
#include "uart.h"

#include "plic.h"
#include "sysctl.h"
#include "utils.h"
#include "atomic.h"
#include "fpioa.h"

#include "py/obj.h"
#include "py/gc.h"
#include "py/runtime.h"
#include "py/objtype.h"
#include "py/objstr.h"
#include "py/objint.h"

#include "modmachine.h"
#include "mphalport.h"
#include "plic.h"
#include "sysctl.h"
#include "py/objtype.h"


typedef struct _machine_esp8285_obj_t {
    mp_obj_base_t base;
    //mp_uint_t repeat;//timer mode
} machine_esp8285_obj_t;

const mp_obj_type_t machine_esp8285_type;

#define K210_DEBUG 0
#if K210_DEBUG==1
#define debug_print(x,arg...) printf("[MAIXPY]:"x,##arg)
#else 
#define debug_print(x,arg...) 
#endif

STATIC void machine_esp8285_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
	return mp_const_none;
}
STATIC mp_obj_t machine_esp8285_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    machine_esp8285_obj_t *self = m_new_obj(machine_esp8285_obj_t);
    self->base.type = &machine_esp8285_type;

    return self;
}

STATIC mp_obj_t machine_esp8285_init_helper(machine_esp8285_obj_t *self, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum {
		ARG_ssid,
		ARG_passwd,
    };
    static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_ssid,		 MP_ARG_OBJ, {.u_obj = mp_const_none} },
		{ MP_QSTR_passwd,	 MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
	fpioa_set_function(6,64);
    fpioa_set_function(7,65);
	esp8285_init();
	if(mp_const_none == args[ARG_ssid].u_obj || mp_const_none == args[ARG_passwd].u_obj)
	{
		mp_raise_ValueError("[MAIXPY]ESP8285:Please enter ssid and password\n");
	}
	mp_buffer_info_t buf_id;
	mp_buffer_info_t buf_pswd;
    mp_obj_str_get_buffer(args[ARG_ssid].u_obj, &buf_id, MP_BUFFER_READ);
	mp_obj_str_get_buffer(args[ARG_passwd].u_obj, &buf_pswd, MP_BUFFER_READ);
	unsigned char *id_ptr =buf_id.buf;
	unsigned char *pswd_ptr =buf_pswd.buf;
	//printf("id is %s\n",id_ptr);
	//printf("passwd is %s\n",pswd_ptr);	
	wifista_config(id_ptr,pswd_ptr);
	/*
	int res = 1;
	res = esp8285_send_cmd("AT+CIPDOMAIN=\"www.baidu.com\"","OK",0);
	unsigned char* ret_ptr = rev_buf_addr();
	printf("%s\n",ret_ptr);
	*/
    return mp_const_none;
}



STATIC mp_obj_t machine_esp8285_init(mp_uint_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return machine_esp8285_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_esp8285_init_obj, 1, machine_esp8285_init);


STATIC mp_obj_t machine_esp8285_send_cmd(machine_esp8285_obj_t *self, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum {
		ARG_cmd,
		ARG_ack,
    };
    static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_cmd,		 MP_ARG_OBJ, {.u_obj = mp_const_none} },
		{ MP_QSTR_ack,	 MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
	if(mp_const_none == args[ARG_cmd].u_obj || mp_const_none == args[ARG_ack].u_obj)
	{
		mp_raise_ValueError("[MAIXPY]ESP8285:Please enter cmd and check_ack\n");
	}
	mp_buffer_info_t buf_cmd;
	mp_buffer_info_t buf_ack;
    mp_obj_str_get_buffer(args[ARG_cmd].u_obj, &buf_cmd, MP_BUFFER_READ);
	mp_obj_str_get_buffer(args[ARG_ack].u_obj, &buf_ack, MP_BUFFER_READ);
	unsigned char *cmd_ptr =buf_cmd.buf;
	unsigned char *buf_ptr =buf_ack.buf;
	debug_print("cmd is %s\n",cmd_ptr);
	debug_print("ack is %s\n",buf_ptr);
	if(buf_cmd.len == 0 || buf_ack.len == 0)
	{
		mp_raise_ValueError("[MAIXPY]ESP8285:parameter is empty\n");
	}
	if(send_cmd(cmd_ptr,buf_ptr,2))
	{
		return mp_obj_new_bool(1);
	}else{
		return mp_obj_new_bool(0);
	}
    
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_esp8285_send_cmd_obj, 1, machine_esp8285_send_cmd);

STATIC const mp_rom_map_elem_t machine_timer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_esp8285_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&machine_esp8285_send_cmd_obj) },
    { MP_ROM_QSTR(MP_QSTR_ONE_SHOT), MP_ROM_INT(false) },
    { MP_ROM_QSTR(MP_QSTR_PERIODIC), MP_ROM_INT(true) },
};
STATIC MP_DEFINE_CONST_DICT(machine_timer_locals_dict, machine_timer_locals_dict_table);

const mp_obj_type_t machine_esp8285_type = {
    { &mp_type_type },
    .name = MP_QSTR_Timer,
    .print = machine_esp8285_print,
    .make_new = machine_esp8285_make_new,
    .locals_dict = (mp_obj_t)&machine_timer_locals_dict,
};
