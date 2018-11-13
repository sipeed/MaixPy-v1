/*
 * This file is part of the Micro Python project, http://micropython.org/
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

#include "py/nlr.h"
#include "py/runtime.h"
#include "modmachine.h"
#include "mphalport.h"
#include "w25qxx.h"

#define FLASH_CHIP_SIZE 16*1024*1024
extern const mp_obj_type_t machine_spiflash_type;

typedef struct _k210_spiflash_obj_t {
    mp_obj_base_t base;
    int    spin;
} k210_spiflash_obj_t;

STATIC void spiflash_init(void) {

}

STATIC void k210_spiflash_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    k210_spiflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "SPIFLASH(spi:%u ss:%u)", self->spin);
}

STATIC mp_obj_t k210_flash_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args){

    enum { ARG_offset, ARG_buf};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_offset, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_buf, MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args,MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    int offset = args[ARG_offset].u_int;
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_buf].u_obj, &bufinfo, MP_BUFFER_WRITE);
    /*new version not have this api*/
    //w25qxx_status_t res = w25qxx_read_data_dma(offset, bufinfo.buf, bufinfo.len,W25QXX_QUAD);
    enum w25qxx_status_t res = w25qxx_read_data_dma(offset, bufinfo.buf, bufinfo.len);
    if (res != W25QXX_OK) {
        mp_raise_ValueError("SPIFLASH read err");
    }
    printf("data %d\n",(int)bufinfo.len);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(k210_flash_read_obj, 1, k210_flash_read);

STATIC mp_obj_t k210_flash_write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    enum { ARG_offset, ARG_buf};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_offset, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_buf, MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args,MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    int offset = args[ARG_offset].u_int;
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_buf].u_obj, &bufinfo, MP_BUFFER_READ);
    enum w25qxx_status_t res = w25qxx_write_data(offset, bufinfo.buf, bufinfo.len);
    if (res != W25QXX_OK) {
        mp_raise_ValueError("SPIFLASH write err");
    }
    printf("write data len:%d\n",(int)bufinfo.len);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(k210_flash_write_obj,1, k210_flash_write);

STATIC mp_obj_t k210_flash_erase(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_addr};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_addr, MP_ARG_INT, {.u_int = -1} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args,MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
    int addr = args[ARG_addr].u_int;
    enum w25qxx_status_t res = w25qxx_sector_erase(addr);
    if (res != W25QXX_OK) {
        mp_raise_ValueError("SPIFLASH erase err");
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(k210_flash_erase_obj, 1, k210_flash_erase);

STATIC mp_obj_t k210_flash_size(void) {
    return mp_obj_new_int_from_uint(FLASH_CHIP_SIZE);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(k210_flash_size_obj, k210_flash_size);

STATIC void k210_spiflash_init_helper(k210_spiflash_obj_t *self,
        size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    //enum { ARG_spin, ARG_ss };
    //static const mp_arg_t allowed_args[] = {
        //{ MP_QSTR_spin, MP_ARG_INT, {.u_int = -1} },
        //{ MP_QSTR_ss, MP_ARG_INT, {.u_int = -1} }, /*new version not have this parameter*/
    //};
    //mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    //mp_arg_parse_all(n_args, pos_args, kw_args,
    //MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    // Find a free SPIFLASH channel, also spot if our pin is
    //  already mentioned.

    
    //if (args[ARG_spin].u_int > 3) {
    //        mp_raise_ValueError("out of SPIFLASH timers");
    //        printf(" timern=%u \n",args[ARG_spin].u_int);
    //        return;
    //}
	/* new version not have this parameter
    if (args[ARG_ss].u_int !=0 && args[ARG_ss].u_int !=1 ) {
           mp_raise_ValueError("SPIFLASH ss err");
           printf(" ss=%u\n",args[ARG_ss].u_int);
           return;
    }
	*/
    self->spin = 0;
    w25qxx_init(3);//args[ARG_ss].u_int);
	w25qxx_enable_quad_mode();
    uint8_t status,manuf_id,device_id;
    status=w25qxx_read_id(&manuf_id,&device_id);
    printf("spiflash:%d m_id: %x , d_id:%x \n",status,manuf_id,device_id);
}

STATIC mp_obj_t k210_spiflash_make_new(const mp_obj_type_t *type,
        size_t n_args, size_t n_kw, const mp_obj_t *args) {
    //mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
   
    //int spin = mp_obj_get_int(args[0]);
    //int ss = mp_obj_get_int(args[1]);
    // create SPIFLASH object from the given pin
    k210_spiflash_obj_t *self = m_new_obj(k210_spiflash_obj_t);
    self->base.type = &machine_spiflash_type;
    //self->spin = spin;
    //self->ss = ss;

    // start the SPIFLASH running for this channel
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    k210_spiflash_init_helper(self, n_args, args, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t k210_spiflash_init(size_t n_args,
        const mp_obj_t *args, mp_map_t *kw_args) {
    k210_spiflash_init_helper(args[0], n_args -1, args + 1, kw_args);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(k210_spiflash_init_obj, 1, k210_spiflash_init);

STATIC mp_obj_t k210_spiflash_deinit(mp_obj_t self_in) {
    k210_spiflash_obj_t *self = MP_OBJ_TO_PTR(self_in);
    printf("DEINIT SPIFLASH(spi:%u)", self->spin);//,self->ss);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(k210_spiflash_deinit_obj, k210_spiflash_deinit);


STATIC const mp_rom_map_elem_t k210_spiflash_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&k210_spiflash_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&k210_spiflash_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_read), MP_ROM_PTR(&k210_flash_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_write), MP_ROM_PTR(&k210_flash_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_erase), MP_ROM_PTR(&k210_flash_erase_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_size), MP_ROM_PTR(&k210_flash_size_obj) },
};

STATIC MP_DEFINE_CONST_DICT(k210_spiflash_locals_dict,
    k210_spiflash_locals_dict_table);

const mp_obj_type_t machine_spiflash_type = {
    { &mp_type_type },
    .name = MP_QSTR_SPIFLASH,
    .print = k210_spiflash_print,
    .make_new = k210_spiflash_make_new,
    .locals_dict = (mp_obj_dict_t*)&k210_spiflash_locals_dict,
};
