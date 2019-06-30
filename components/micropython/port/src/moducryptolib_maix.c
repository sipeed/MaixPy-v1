/*
 * This file is part of the MicroPython K210 project, https://github.com/loboris/MicroPython_K210_LoBo
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 LoBo (https://github.com/loboris)
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

#include "py/mpconfig.h"

#if MICROPY_PY_UCRYPTOLIB_MAIX

#include <string.h>
#include "aes.h"
#include "py/objstr.h"
#include "py/runtime.h"

enum {
    UCRYPTOLIB_MODE_MIN = 0,
    UCRYPTOLIB_MODE_GCM,
    UCRYPTOLIB_MODE_CBC,
    UCRYPTOLIB_MODE_ECB,
    UCRYPTOLIB_MODE_MAX,
};

#define AES_KEYLEN_128  0
#define AES_KEYLEN_192  1
#define AES_KEYLEN_256  2

typedef struct _context
{
    /* The buffer holding the encryption or decryption key. */
    uint8_t *input_key;
    /* The initialization vector. must be 96 bit for gcm, 128 bit for cbc*/
    uint8_t iv[16];
} context_t;

typedef struct _mp_obj_aes_t {
    mp_obj_base_t base;
    context_t ctx;
    uint8_t mode;
    uint8_t key_len;
} mp_obj_aes_t;

const mp_obj_module_t mp_module_ucryptolib;

//------------------------------------------------------------------------------------------------------------------
STATIC mp_obj_t ucryptolib_aes_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    mp_arg_check_num(n_args, n_kw, 2, 3, false);

    uint8_t iv_len = 12;
    // create aes object
    mp_obj_aes_t *o = m_new_obj(mp_obj_aes_t);
    o->base.type = type;
    // get mode
    o->mode = mp_obj_get_int(args[1]);
    if (o->mode <= UCRYPTOLIB_MODE_MIN || o->mode >= UCRYPTOLIB_MODE_MAX) {
        mp_raise_ValueError("wrong mode");
    }
    if (o->mode == UCRYPTOLIB_MODE_CBC) iv_len = 16;

    // get key
    mp_buffer_info_t keyinfo;
    mp_get_buffer_raise(args[0], &keyinfo, MP_BUFFER_READ);
    if ((32 != keyinfo.len) && (24 != keyinfo.len) && (16 != keyinfo.len)) {
        mp_raise_ValueError("wrong key length");
    }
    o->ctx.input_key = keyinfo.buf;
    if (keyinfo.len == 32) o->key_len = AES_KEYLEN_256;
    else if (keyinfo.len == 24) o->key_len = AES_KEYLEN_192;
    else o->key_len = AES_KEYLEN_128;

    // get the (optional) IV
    memset(o->ctx.iv, 0, sizeof(o->ctx.iv));
    for (uint8_t i=0; i<iv_len; i++) {
        o->ctx.iv[i] = i;
    }
    mp_buffer_info_t ivinfo;
    ivinfo.buf = NULL;
    if ((n_args > 2) && (args[2] != mp_const_none)) {
        mp_get_buffer_raise(args[2], &ivinfo, MP_BUFFER_READ);

        if (iv_len != ivinfo.len) {
            mp_raise_ValueError("wrong IV length");
        }
        memcpy(o->ctx.iv, ivinfo.buf, iv_len);
    }

    return MP_OBJ_FROM_PTR(o);
}

//----------------------------------------------------------------------------
STATIC mp_obj_t AES_run(size_t n_args, const mp_obj_t *args, bool encrypt)
{
    mp_obj_aes_t *self = MP_OBJ_TO_PTR(args[0]);

    // get input
    mp_obj_t in_buf = args[1];
    mp_obj_t out_buf = MP_OBJ_NULL;
    if (n_args > 2) {
        // separate output is used
        out_buf = args[2];
    }

    // create output buffer
    mp_buffer_info_t in_bufinfo;
    mp_get_buffer_raise(in_buf, &in_bufinfo, MP_BUFFER_READ);

    if ((in_bufinfo.len % 16) != 0) {
        mp_raise_ValueError("input length must be multiple of 16");
    }

    vstr_t vstr;
    mp_buffer_info_t out_bufinfo;
    uint8_t *out_buf_ptr;

    if (out_buf != MP_OBJ_NULL) {
        mp_get_buffer_raise(out_buf, &out_bufinfo, MP_BUFFER_WRITE);
        if (out_bufinfo.len < in_bufinfo.len) {
            mp_raise_ValueError("output too small");
        }
        out_buf_ptr = out_bufinfo.buf;
    }
    else {
        vstr_init_len(&vstr, in_bufinfo.len);
        out_buf_ptr = (uint8_t*)vstr.buf;
    }

    if (self->mode == UCRYPTOLIB_MODE_GCM) {
        uint8_t gcm_tag[4];
        gcm_context_t ctx;
        ctx.input_key = self->ctx.input_key;
        ctx.iv = self->ctx.iv;
        ctx.gcm_aad = NULL;
        ctx.gcm_aad_len = 0;
        if (!encrypt) {
            if (self->key_len == AES_KEYLEN_256) aes_gcm256_hard_decrypt(&ctx, in_bufinfo.buf, in_bufinfo.len, out_buf_ptr, gcm_tag);
            else if (self->key_len == AES_KEYLEN_192) aes_gcm192_hard_decrypt(&ctx, in_bufinfo.buf, in_bufinfo.len, out_buf_ptr, gcm_tag);
            else aes_gcm128_hard_decrypt(&ctx, in_bufinfo.buf, in_bufinfo.len, out_buf_ptr, gcm_tag);
        }
        else {
            if (self->key_len == AES_KEYLEN_256) aes_gcm256_hard_encrypt(&ctx, in_bufinfo.buf, in_bufinfo.len, out_buf_ptr, gcm_tag);
            else if (self->key_len == AES_KEYLEN_192) aes_gcm192_hard_encrypt(&ctx, in_bufinfo.buf, in_bufinfo.len, out_buf_ptr, gcm_tag);
            else aes_gcm128_hard_encrypt(&ctx, in_bufinfo.buf, in_bufinfo.len, out_buf_ptr, gcm_tag);
        }
    }
    else if (self->mode == UCRYPTOLIB_MODE_CBC) {
        cbc_context_t ctx;
        ctx.input_key = self->ctx.input_key;
        ctx.iv = self->ctx.iv;
        if (!encrypt) {
            if (self->key_len == AES_KEYLEN_256) aes_cbc256_hard_decrypt(&ctx, in_bufinfo.buf, in_bufinfo.len, out_buf_ptr);
            else if (self->key_len == AES_KEYLEN_192) aes_cbc192_hard_decrypt(&ctx, in_bufinfo.buf, in_bufinfo.len, out_buf_ptr);
            else aes_cbc128_hard_decrypt(&ctx, in_bufinfo.buf, in_bufinfo.len, out_buf_ptr);
        }
        else {
            if (self->key_len == AES_KEYLEN_256) aes_cbc256_hard_encrypt(&ctx, in_bufinfo.buf, in_bufinfo.len, out_buf_ptr);
            else if (self->key_len == AES_KEYLEN_192) aes_cbc192_hard_encrypt(&ctx, in_bufinfo.buf, in_bufinfo.len, out_buf_ptr);
            else aes_cbc128_hard_encrypt(&ctx, in_bufinfo.buf, in_bufinfo.len, out_buf_ptr);
        }
    }
    else {
        if (!encrypt) {
            if (self->key_len == AES_KEYLEN_256) aes_ecb256_hard_decrypt(self->ctx.input_key, in_bufinfo.buf, in_bufinfo.len, out_buf_ptr);
            else if (self->key_len == AES_KEYLEN_192) aes_ecb192_hard_decrypt(self->ctx.input_key, in_bufinfo.buf, in_bufinfo.len, out_buf_ptr);
            else aes_ecb128_hard_decrypt(self->ctx.input_key, in_bufinfo.buf, in_bufinfo.len, out_buf_ptr);
        }
        else {
            if (self->key_len == AES_KEYLEN_256) aes_ecb256_hard_encrypt(self->ctx.input_key, in_bufinfo.buf, in_bufinfo.len, out_buf_ptr);
            else if (self->key_len == AES_KEYLEN_192) aes_ecb192_hard_encrypt(self->ctx.input_key, in_bufinfo.buf, in_bufinfo.len, out_buf_ptr);
            else aes_ecb128_hard_encrypt(self->ctx.input_key, in_bufinfo.buf, in_bufinfo.len, out_buf_ptr);
        }
    }

    if (out_buf != MP_OBJ_NULL) {
        return out_buf;
    }
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}

//-------------------------------------------------------------------------
STATIC mp_obj_t ucryptolib_aes_encrypt(size_t n_args, const mp_obj_t *args)
{
    return AES_run(n_args, args, true);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(ucryptolib_aes_encrypt_obj, 2, 3, ucryptolib_aes_encrypt);

//-------------------------------------------------------------------------
STATIC mp_obj_t ucryptolib_aes_decrypt(size_t n_args, const mp_obj_t *args)
{
    return AES_run(n_args, args, false);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(ucryptolib_aes_decrypt_obj, 2, 3, ucryptolib_aes_decrypt);

/*
//----------------------------------------------------
STATIC mp_obj_t ucryptolib_aes_getIV(mp_obj_t self_in)
{
    mp_obj_aes_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_str_of_type(&mp_type_bytes, self->ctx.iv, (self->mode == UCRYPTOLIB_MODE_GCM) ? 12 : 16);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ucryptolib_aes_getIV_obj, ucryptolib_aes_getIV);
*/

//===================================================================
STATIC const mp_rom_map_elem_t ucryptolib_aes_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_encrypt), MP_ROM_PTR(&ucryptolib_aes_encrypt_obj) },
    { MP_ROM_QSTR(MP_QSTR_decrypt), MP_ROM_PTR(&ucryptolib_aes_decrypt_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_getIV), MP_ROM_PTR(&ucryptolib_aes_getIV_obj) },
};
STATIC MP_DEFINE_CONST_DICT(ucryptolib_aes_locals_dict, ucryptolib_aes_locals_dict_table);

//================================================
STATIC const mp_obj_type_t ucryptolib_aes_type = {
    { &mp_type_type },
    .name = MP_QSTR_aes,
    .make_new = ucryptolib_aes_make_new,
    .locals_dict = (void*)&ucryptolib_aes_locals_dict,
};

//=====================================================================
STATIC const mp_rom_map_elem_t mp_module_ucryptolib_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ucryptolib) },
    { MP_ROM_QSTR(MP_QSTR_aes), MP_ROM_PTR(&ucryptolib_aes_type) },

    { MP_ROM_QSTR(MP_QSTR_MODE_GCM), MP_ROM_INT(UCRYPTOLIB_MODE_GCM) },
    { MP_ROM_QSTR(MP_QSTR_MODE_CBC), MP_ROM_INT(UCRYPTOLIB_MODE_CBC) },
    { MP_ROM_QSTR(MP_QSTR_MODE_ECB), MP_ROM_INT(UCRYPTOLIB_MODE_ECB) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_ucryptolib_globals, mp_module_ucryptolib_globals_table);

const mp_obj_module_t mp_module_ucryptolib = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_ucryptolib_globals,
};

#endif //MICROPY_PY_UCRYPTOLIB
