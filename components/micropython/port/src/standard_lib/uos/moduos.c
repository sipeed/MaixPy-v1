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
#include <stdlib.h>

#include "py/objtuple.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/lexer.h"
#include "extmod/misc.h"
#include "py/mphal.h"
#include "extmod/misc.h"
#include "modmachine.h"
#include "lib/oofatfs/ff.h"
#include "extmod/vfs_fat.h"
#include "machine_uart.h"
#if MICROPY_VFS
#include "extmod/vfs.h"
#endif
#include "genhdr/mpversion.h"
#if MICROPY_spiffs
#include "spiffs.h"
#endif
#include "spiffs_config.h"
#include "rng.h"
#include "machine_uart.h"
#include "vfs_spiffs.h"

//extern mp_obj_t file_open(const char* file_name, const mp_obj_type_t *type, mp_arg_val_t *args);
extern const mp_obj_type_t mp_type_spiffs_textio;
unsigned char current_dir[FS_PATCH_LENGTH];

STATIC const qstr os_uname_info_fields[] = {
    MP_QSTR_sysname, MP_QSTR_nodename,
    MP_QSTR_release, MP_QSTR_version, MP_QSTR_machine
};
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_sysname_obj, MICROPY_PY_SYS_PLATFORM);
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_nodename_obj, MICROPY_PY_SYS_PLATFORM);
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_release_obj, MICROPY_VERSION_STRING);
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_version_obj, MICROPY_GIT_TAG " on " MICROPY_BUILD_DATE);
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_machine_obj, MICROPY_HW_BOARD_NAME " with " MICROPY_HW_MCU_NAME);

STATIC MP_DEFINE_ATTRTUPLE(
    os_uname_info_obj,
    os_uname_info_fields,
    5,
    (mp_obj_t)&os_uname_info_sysname_obj,
    (mp_obj_t)&os_uname_info_nodename_obj,
    (mp_obj_t)&os_uname_info_release_obj,
    (mp_obj_t)&os_uname_info_version_obj,
    (mp_obj_t)&os_uname_info_machine_obj
);

STATIC mp_obj_t os_uname(void) {
    return (mp_obj_t)&os_uname_info_obj;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(os_uname_obj, os_uname);

STATIC mp_obj_t os_urandom(mp_obj_t num) {
    mp_int_t n = mp_obj_get_int(num);
    vstr_t vstr;
    vstr_init_len(&vstr, n);
    uint32_t r = 0;
    for (int i = 0; i < n; i++) {
        r = rng_get(); // returns 32-bit hardware random number
        vstr.buf[i] = r;
    }
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(os_urandom_obj, os_urandom);

#if MICROPY_PY_OS_DUPTERM

STATIC mp_obj_t uos_dupterm(size_t n_args, const mp_obj_t *args) {
    mp_obj_t prev_uart_obj = mp_uos_dupterm_obj.fun.var(n_args, args);
    if (mp_obj_get_type(prev_uart_obj) == &machine_uart_type) {
        uart_attach_to_repl(MP_OBJ_TO_PTR(prev_uart_obj), false);
    }
    if (mp_obj_get_type(args[0]) == &machine_uart_type) {
        uart_attach_to_repl(MP_OBJ_TO_PTR(args[0]), true);
    }
    return prev_uart_obj;
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(uos_dupterm_obj, 1, 2, uos_dupterm);
#endif 

#if MICROPY_HW_UART_REPL
STATIC mp_obj_t uos_set_REPLio(const mp_obj_t args) {

	mp_obj_t prev_uart_obj;
	if(mp_obj_get_type(MP_STATE_PORT(Maix_stdio_uart)) == &machine_uart_type)
		prev_uart_obj = MP_STATE_PORT(Maix_stdio_uart);
	else 
		prev_uart_obj = mp_const_none;
	//judget type 
	if(mp_obj_get_type(args) == &machine_uart_type)
	{
		uart_attach_to_repl(MP_STATE_PORT(Maix_stdio_uart), false);
		MP_STATE_PORT(Maix_stdio_uart) = MP_OBJ_TO_PTR(args);;
		uart_attach_to_repl(MP_STATE_PORT(Maix_stdio_uart), true);
	}
	else
		return  mp_const_none;
	return prev_uart_obj;
	
}
MP_DEFINE_CONST_FUN_OBJ_1(uos_set_REPLio_obj, uos_set_REPLio);
#endif

STATIC mp_obj_t os_sync(void) {
    #if MICROPY_VFS
	//TODO
    #endif
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(mod_os_sync_obj, os_sync);

STATIC mp_obj_t mod_os_flash_format(void) {

    spiffs_user_mount_t* spiffs = NULL;
    mp_vfs_mount_t *m = MP_STATE_VM(vfs_mount_table);
    for(;NULL != m ; m = m->next)
    {
        if(0 == strcmp(m->str,"/flash"))
        {
            spiffs = MP_OBJ_TO_PTR(m->obj);
            break;
        }
    }
    SPIFFS_unmount(&spiffs->fs);
    mp_printf(&mp_plat_print, "[MAIXPY]:Spiffs Unmount.\n");
    mp_printf(&mp_plat_print, "[MAIXPY]:Spiffs Formating...\n");
    uint32_t format_res=SPIFFS_format(&spiffs->fs);
    mp_printf(&mp_plat_print, "[MAIXPY]:Spiffs Format %s \n",format_res?"failed":"successful");
    if(0 != format_res)
    {
        return mp_const_false;
    }
    uint32_t res = 0;

    res = SPIFFS_mount(&spiffs->fs,
        &spiffs->cfg,
        spiffs_work_buf,
        spiffs_fds,
        sizeof(spiffs_fds),
        spiffs_cache_buf,
        sizeof(spiffs_cache_buf),
        0);
    mp_printf(&mp_plat_print, "[MAIXPY]:Spiffs Mount %s \n", res?"failed":"successful");
    if(!res)
    {
        return mp_const_true;
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(mod_os_flash_format_obj, mod_os_flash_format);

STATIC const mp_rom_map_elem_t os_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_uos) },
    { MP_ROM_QSTR(MP_QSTR_uname), MP_ROM_PTR(&os_uname_obj) },
    { MP_ROM_QSTR(MP_QSTR_urandom), MP_ROM_PTR(&os_urandom_obj) },
   	#if MICROPY_HW_UART_REPL
    { MP_ROM_QSTR(MP_QSTR_set_REPLio), MP_ROM_PTR(&uos_set_REPLio_obj) },
    #endif
    #if MICROPY_PY_OS_DUPTERM
    { MP_ROM_QSTR(MP_QSTR_dupterm), MP_ROM_PTR(&uos_dupterm_obj) },
    #endif
    #if MICROPY_VFS
    { MP_ROM_QSTR(MP_QSTR_ilistdir), MP_ROM_PTR(&mp_vfs_ilistdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_listdir), MP_ROM_PTR(&mp_vfs_listdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_mkdir), MP_ROM_PTR(&mp_vfs_mkdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_rmdir), MP_ROM_PTR(&mp_vfs_rmdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_chdir), MP_ROM_PTR(&mp_vfs_chdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_getcwd), MP_ROM_PTR(&mp_vfs_getcwd_obj) },
    { MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&mp_vfs_remove_obj) },
    { MP_ROM_QSTR(MP_QSTR_rename), MP_ROM_PTR(&mp_vfs_rename_obj) },
    { MP_ROM_QSTR(MP_QSTR_stat), MP_ROM_PTR(&mp_vfs_stat_obj) },
    { MP_ROM_QSTR(MP_QSTR_statvfs), MP_ROM_PTR(&mp_vfs_statvfs_obj) },
    { MP_ROM_QSTR(MP_QSTR_mount), MP_ROM_PTR(&mp_vfs_mount_obj) },
    { MP_ROM_QSTR(MP_QSTR_umount), MP_ROM_PTR(&mp_vfs_umount_obj) },
    { MP_ROM_QSTR(MP_QSTR_sync), MP_ROM_PTR(&mod_os_sync_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_format), MP_ROM_PTR(&mod_os_flash_format_obj) },
    #endif
	#if MICROPY_VFS_SPIFFS
	{ MP_ROM_QSTR(MP_QSTR_VfsSpiffs), MP_ROM_PTR(&mp_spiffs_vfs_type) },
	{ MP_ROM_QSTR(MP_QSTR_VfsFat), MP_ROM_PTR(&mp_fat_vfs_type) },
	#endif
};

STATIC MP_DEFINE_CONST_DICT(os_module_globals, os_module_globals_table);

const mp_obj_module_t uos_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&os_module_globals,
};
