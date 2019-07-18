/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
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

#include "py/mpconfig.h"
#if MICROPY_VFS_SPIFFS

#if !MICROPY_VFS
#error "with MICROPY_VFS_SPIFFS enabled, must also enable MICROPY_VFS"
#endif

#include <string.h>
#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/binary.h"
#include "py/objarray.h"

#include "lib/timeutils/timeutils.h"

#include "vfs_spiffs.h"
#include "spiffs_config.h"
#include "global_config.h"
#if _MAX_SS == _MIN_SS
#define SECSIZE(fs) (_MIN_SS)
#else
#define SECSIZE(fs) ((fs)->ssize)
#endif

#define GET_ERR_CODE(res) ((-res)-10000+1)
#define mp_obj_spiffs_vfs_t spiffs_user_mount_t
const mp_obj_type_t mp_spiffs_vfs_type;
u8_t spiffs_work_buf[CONFIG_SPIFFS_LOGICAL_PAGE_SIZE*2];
u8_t spiffs_fds[CONFIG_SPIFFS_LOGICAL_BLOCK_SIZE/CONFIG_SPIFFS_LOGICAL_PAGE_SIZE*4];
u8_t spiffs_cache_buf[(CONFIG_SPIFFS_LOGICAL_PAGE_SIZE+CONFIG_SPIFFS_LOGICAL_BLOCK_SIZE/CONFIG_SPIFFS_LOGICAL_PAGE_SIZE)*4];

STATIC mp_import_stat_t spiffs_vfs_import_stat(void *vfs_in,const char *path) {
    spiffs_user_mount_t *vfs = vfs_in;
    spiffs_stat  fno;
    assert(vfs != NULL);
	int len = strlen(path);
    char* file_path = m_new(char, len+2);
	memset(file_path, 0, len+2);
    if(path[0] == '.' && path[1] == '/')
    {
        strcpy(file_path, path+1);
    }
    else if(path[0] != '/')
    {
        file_path[0] = '/';
        strcat(file_path, path);
    }
    int res = SPIFFS_stat(&vfs->fs,file_path,&fno);
    if (res == SPIFFS_OK) {
        if (fno.type == SPIFFS_TYPE_DIR) 
		{
            return MP_IMPORT_STAT_DIR;
        } 
		else 
		{
            return MP_IMPORT_STAT_FILE;
        }
    }
    return MP_IMPORT_STAT_NO_EXIST;
}

STATIC mp_obj_t spiffs_vfs_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    // create new object
    spiffs_user_mount_t *vfs = m_new_obj(spiffs_user_mount_t);
	vfs->base.type = &mp_spiffs_vfs_type;
    vfs->flags = MODULE_SPIFFS;
	vfs->fs.user_data = vfs;
    //TODO:load block protocol methods mp_load_method() vfs_fat.c
	//TODO: add error-returning process	
//	mp_buffer_info_t bufinfo;
//	mp_get_buffer_raise(mp_load_attr(args[0],MP_QSTR_fs_data), &bufinfo,MP_BUFFER_RW);
//  vfs->cfg.phys_addr = bufinfo.buf;
	vfs->cfg.phys_addr = 0;
	vfs->cfg.phys_size = mp_obj_get_int(mp_load_attr(args[0],MP_QSTR_fs_size));
	vfs->cfg.phys_erase_block = mp_obj_get_int(mp_load_attr(args[0],MP_QSTR_erase_block));
	vfs->cfg.log_block_size = mp_obj_get_int(mp_load_attr(args[0],MP_QSTR_log_block_size));
	vfs->cfg.log_page_size = mp_obj_get_int(mp_load_attr(args[0],MP_QSTR_log_page_size));	
	vfs->cfg.hal_read_f = spiffs_read_method;
	vfs->cfg.hal_write_f = spiffs_write_method;
	vfs->cfg.hal_erase_f = spiffs_erase_method;
    mp_load_method(args[0], MP_QSTR_write, vfs->write_obj);
	mp_load_method(args[0], MP_QSTR_read, vfs->read_obj);
    mp_load_method_maybe(args[0], MP_QSTR_erase, vfs->erase_obj);
    return MP_OBJ_FROM_PTR(vfs);
}
#if _FS_REENTRANT
STATIC mp_obj_t spiffs_vfs_del(mp_obj_t self_in) {
    spiffs_user_mount_t *self = MP_OBJ_TO_PTR(self_in);
    //f_umount only needs to be called to release the sync object
    int res = SPIFFS_unmount(&self->fs);
	if(SPIFFS_OK != res)
	{
		mp_printf(&mp_plat_print, "[MaixPy]:SPIFFS Error Code %d\n",res);
		mp_raise_OSError(SPIFFS_errno_table[GET_ERR_CODE(res)]);
	}
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(spiffs_vfs_del_obj, spiffs_vfs_del);
#endif

STATIC mp_obj_t spiffs_vfs_mkfs(mp_obj_t self_in) {

    // create new object
    spiffs_user_mount_t *vfs = MP_OBJ_TO_PTR(self_in);
    // make the filesystem:spiffs_format 
    int res = mp_module_spiffs_mount(&vfs->fs,&vfs->cfg);
	if(res == SPIFFS_ERR_NOT_A_FS)
    	res = mp_module_spiffs_format(&vfs->fs);
    if (res != SPIFFS_OK) {
       	mp_printf(&mp_plat_print, "[MaixPy]:SPIFFS Error Code %d\n",res);
		mp_raise_OSError(SPIFFS_errno_table[GET_ERR_CODE(res)]);
    }
    return mp_const_none;

}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(spiffs_vfs_mkfs_fun_obj, spiffs_vfs_mkfs);
STATIC MP_DEFINE_CONST_STATICMETHOD_OBJ(spiffs_vfs_mkfs_obj, MP_ROM_PTR(&spiffs_vfs_mkfs_fun_obj));


typedef struct _mp_vfs_spiffs_ilistdir_it_t {
    mp_obj_base_t base;
    mp_fun_1_t iternext;
    bool is_str;
    spiffs_DIR dir;
} mp_vfs_spiffs_ilistdir_it_t;


STATIC mp_obj_t mp_vfs_spiffs_ilistdir_it_iternext(mp_obj_t self_in) {
    mp_vfs_spiffs_ilistdir_it_t *self = MP_OBJ_TO_PTR(self_in);
	struct spiffs_dirent de;
	struct spiffs_dirent* de_ret;
//	static const char types[] = "?fdhs"; // file, dir, hardlink, softlink
    for (;;) {
		de_ret = SPIFFS_readdir(&self->dir, &de);		
        char *fn = (char*)de.name;
        if (de_ret == NULL || fn[0] == 0) {
            // stop on error or end of dir
            break;
        }
        // Note that FatFS already filters . and .., so we don't need to

        // make 4-tuple with info about this entry
        mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(4, NULL));
        if(fn[0] != '/') // for old code compatible
            continue;
        if (self->is_str) {
            while(*fn == '/')fn++;
            t->items[0] = mp_obj_new_str(fn, strlen(fn));
        } else {
            while(*fn == '/')fn++;
            t->items[0] = mp_obj_new_bytes((const byte*)fn, strlen(fn));
        }
        if (de.type == SPIFFS_TYPE_DIR) {
            // dir
            t->items[1] = MP_OBJ_NEW_SMALL_INT(MP_S_IFDIR);
        } 
		else{
            // file
            t->items[1] = MP_OBJ_NEW_SMALL_INT(MP_S_IFREG);
        }
        t->items[2] = MP_OBJ_NEW_SMALL_INT(0); // no inode number
        t->items[3] = mp_obj_new_int_from_uint(de.size);

        return MP_OBJ_FROM_PTR(t);
    }

    // ignore error because we may be closing a second time
    SPIFFS_closedir(&self->dir);

    return MP_OBJ_STOP_ITERATION;
}

STATIC mp_obj_t spiffs_vfs_ilistdir_func(size_t n_args, const mp_obj_t *args) {
    spiffs_user_mount_t *self = MP_OBJ_TO_PTR(args[0]);
    bool is_str_type = true;
    const char *path;
    if (n_args == 2) {
        if (mp_obj_get_type(args[1]) == &mp_type_bytes) {
            is_str_type = false;
        }
        path = mp_obj_str_get_str(args[1]);
    } else {
        path = "";
    }

    // Create a new iterator object to list the dir
    mp_vfs_spiffs_ilistdir_it_t *iter = m_new_obj(mp_vfs_spiffs_ilistdir_it_t);
    iter->base.type = &mp_type_polymorph_iter;
    iter->iternext = mp_vfs_spiffs_ilistdir_it_iternext;
    iter->is_str = is_str_type;
	if(!SPIFFS_opendir(&self->fs, path, &iter->dir))
	{
        mp_raise_OSError(MP_ENOTDIR);
    }

    return MP_OBJ_FROM_PTR(iter);
	
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(spiffs_vfs_ilistdir_obj, 1, 2, spiffs_vfs_ilistdir_func);

STATIC mp_obj_t spiffs_vfs_remove_internal(mp_obj_t vfs_in, mp_obj_t path_in, mp_int_t attr) {
//    mp_obj_fat_vfs_t *self = MP_OBJ_TO_PTR(vfs_in);
//    const char *path = mp_obj_str_get_str(path_in);

//    FILINFO fno;
//    FRESULT res = f_stat(&self->fatfs, path, &fno);

//    if (res != FR_OK) {
//        mp_raise_OSError(fresult_to_errno_table[res]);
//    }

//    // check if path is a file or directory
//    if ((fno.fattrib & AM_DIR) == attr) {
//        res = f_unlink(&self->fatfs, path);

//        if (res != FR_OK) {
//            mp_raise_OSError(fresult_to_errno_table[res]);
//        }
//        return mp_const_none;
//    } else {
//        mp_raise_OSError(attr ? MP_ENOTDIR : MP_EISDIR);
//    }
    return mp_const_none;
}

STATIC mp_obj_t spiffs_vfs_remove(mp_obj_t vfs_in, mp_obj_t path_in) {
	spiffs_user_mount_t* vfs = MP_OBJ_TO_PTR(vfs_in);
	const char *path = mp_obj_str_get_str(path_in);
    char* open_name = (char*)path;
    if(path[0] == '.' && path[1] == '/')
    {
        memmove(open_name, open_name+1, strlen(open_name));
    }
    else if(path[0] != '/')
    {
        open_name = m_new(char, 128);
        memset(open_name, 0, 128);
        open_name[0] = '/';
        strcat(open_name, path);
    }
	int res = SPIFFS_remove(&vfs->fs, open_name); 
    if (res != SPIFFS_OK) {
	   	mp_printf(&mp_plat_print, "[MaixPy]:SPIFFS Error Code %d\n",res);
		mp_raise_OSError(SPIFFS_errno_table[GET_ERR_CODE(res)]);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(spiffs_vfs_remove_obj, spiffs_vfs_remove);

STATIC mp_obj_t fat_vfs_rmdir(mp_obj_t vfs_in, mp_obj_t path_in) {
//	mp_printf(&mp_plat_print, "[MaixPy]spiffs only support flat directory\n");
	mp_raise_OSError(MP_EIO);
/*
    return fat_vfs_remove_internal(vfs_in, path_in, AM_DIR);
*/
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(spiffs_vfs_rmdir_obj, fat_vfs_rmdir);

STATIC mp_obj_t spiffs_vfs_rename(mp_obj_t vfs_in, mp_obj_t path_in, mp_obj_t path_out) {
    spiffs_user_mount_t *self = MP_OBJ_TO_PTR(vfs_in);
    const char *old_path = mp_obj_str_get_str(path_in);
    const char *new_path = mp_obj_str_get_str(path_out);
    int i = 0;
	char* old_name = (char*)&old_path[i];
    if(old_path[0] == '.' && old_path[1] == '/')
    {
        memmove(old_name, old_name+1, strlen(old_name));
    }
    else if(old_path[0] != '/')
    {
        old_name = m_new(char, 128);
        memset(old_name, 0, 128);
        old_name[0] = '/';
        strcat(old_name, old_path);
    }
    i = 0;
	char* new_name = (char*)&new_path[i];    
    if(new_path[0] == '.' && new_path[1] == '/')
    {
        memmove(new_name, new_name+1, strlen(new_name));
    }
    else if(new_path[0] != '/')
    {
        new_name = m_new(char, 128);
        memset(new_name, 0, 128);
        new_name[0] = '/';
        strcat(new_name, new_path);
    }
    int res = SPIFFS_rename(&self->fs, old_name, new_name);
    if (res == SPIFFS_ERR_CONFLICTING_NAME){
        // if new_name exists then try removing it (but only if it's a file)
        res = SPIFFS_remove(&self->fs, new_name);//remove file
        // try to rename again
        res = SPIFFS_rename(&self->fs, old_name, new_name);
    }
    if (res == SPIFFS_OK) {
        return mp_const_none;
    } else {
//	   	mp_printf(&mp_plat_print, "[MaixPy]:SPIFFS Error Code %d\n",res);
		mp_raise_OSError(SPIFFS_errno_table[GET_ERR_CODE(res)]);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(spiffs_vfs_rename_obj, spiffs_vfs_rename);

STATIC mp_obj_t spiffs_vfs_mkdir(mp_obj_t vfs_in, mp_obj_t path_o) {
//	mp_printf(&mp_plat_print, "[MaixPy]spiffs only support flat directory\n");
	mp_raise_NotImplementedError("SPIFFS not support");
/*
    mp_obj_fat_vfs_t *self = MP_OBJ_TO_PTR(vfs_in);
    const char *path = mp_obj_str_get_str(path_o);
    FRESULT res = f_mkdir(&self->fatfs, path);
    if (res == FR_OK) {
        return mp_const_none;
    } else {
        mp_raise_OSError(fresult_to_errno_table[res]);
    }
*/
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(spiffs_vfs_mkdir_obj, spiffs_vfs_mkdir);

/// Change current directory.
STATIC mp_obj_t spiffs_vfs_chdir(mp_obj_t vfs_in, mp_obj_t path_in) {
/*
    mp_obj_fat_vfs_t *self = MP_OBJ_TO_PTR(vfs_in);
    const char *path;
    path = mp_obj_str_get_str(path_in);

    FRESULT res = f_chdir(&self->fatfs, path);

    if (res != FR_OK) {
        mp_raise_OSError(fresult_to_errno_table[res]);
    }
*/
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(spiffs_vfs_chdir_obj, spiffs_vfs_chdir);

/// Get the current directory.
STATIC mp_obj_t spiffs_vfs_getcwd(mp_obj_t vfs_in) {
    return mp_obj_new_str("", 0);
/*
    mp_obj_fat_vfs_t *self = MP_OBJ_TO_PTR(vfs_in);
    char buf[MICROPY_ALLOC_PATH_MAX + 1];
    FRESULT res = f_getcwd(&self->fatfs, buf, sizeof(buf));
    if (res != FR_OK) {
        mp_raise_OSError(fresult_to_errno_table[res]);
    }
    return mp_obj_new_str(buf, strlen(buf));
*/
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(spiffs_vfs_getcwd_obj, spiffs_vfs_getcwd);

/// \function stat(path)
/// Get the status of a file or directory.
STATIC mp_obj_t spiffs_vfs_stat(mp_obj_t vfs_in, mp_obj_t path_in) {

    spiffs_user_mount_t *self = MP_OBJ_TO_PTR(vfs_in);
    const char *path = mp_obj_str_get_str(path_in);

    //FILINFO fno;
	spiffs_stat fno;
    if ( (path[0] == 0 || (path[0] == '/' && path[1] == 0)) || (path[0]=='.' && path[1]=='/') ) {
        // stat root directory
        fno.size = 0;
		fno.type = SPIFFS_TYPE_DIR;
    } else {
        int res = 0;
	    char* open_name = (char*)&path[res];
        if(path[0] == '.' && path[1] == '/')
        {
            memmove(open_name, open_name+1, strlen(open_name));
        }
        else if(path[0] != '/')
        {
            open_name = m_new(char, 128);
            memset(open_name, 0, 128);
            open_name[0] = '/';
            strcat(open_name, path);
        }
        res = SPIFFS_stat(&self->fs, open_name, &fno);
        if (res != SPIFFS_OK) {
//			mp_printf(&mp_plat_print, "[MaixPy]:SPIFFS Error Code %d\n",res);
            mp_raise_OSError(SPIFFS_errno_table[GET_ERR_CODE(res)]);
        }
    }
    mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(10, NULL));
    mp_int_t mode = 0;
    if (fno.type == SPIFFS_TYPE_DIR) {
        mode |= MP_S_IFDIR;
    } else {
        mode |= MP_S_IFREG;
    }
	//spiffs do not support feature of file creation time;
    t->items[0] = MP_OBJ_NEW_SMALL_INT(mode); // st_mode
    t->items[1] = MP_OBJ_NEW_SMALL_INT(0); // st_ino
    t->items[2] = MP_OBJ_NEW_SMALL_INT(0); // st_dev
    t->items[3] = MP_OBJ_NEW_SMALL_INT(0); // st_nlink
    t->items[4] = MP_OBJ_NEW_SMALL_INT(0); // st_uid
    t->items[5] = MP_OBJ_NEW_SMALL_INT(0); // st_gid
    t->items[6] = mp_obj_new_int_from_uint(fno.size); // st_size
    t->items[7] = MP_OBJ_NEW_SMALL_INT(0); // st_atime
    t->items[8] = MP_OBJ_NEW_SMALL_INT(0); // st_mtime
    t->items[9] = MP_OBJ_NEW_SMALL_INT(0); // st_ctime

    return MP_OBJ_FROM_PTR(t);

}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(spiffs_vfs_stat_obj, spiffs_vfs_stat);

// Get the status of a VFS.
STATIC mp_obj_t spiffs_vfs_statvfs(mp_obj_t vfs_in, mp_obj_t path_in) {

    // const char* path = mp_obj_str_get_str(path_in);

    spiffs_user_mount_t *self = MP_OBJ_TO_PTR(vfs_in);
    (void)path_in;

    spiffs *spif_fs = &self->fs;
	u32_t total, used;
    int res = SPIFFS_info(spif_fs, &total,&used);
    if (res != SPIFFS_OK) {
//	   	mp_printf(&mp_plat_print, "[MaixPy]:SPIFFS Error Code %d\n",res);
		mp_raise_OSError(SPIFFS_errno_table[GET_ERR_CODE(res)]);
    }
    mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(10, NULL));

    t->items[0] = MP_OBJ_NEW_SMALL_INT(CONFIG_SPIFFS_LOGICAL_BLOCK_SIZE); // file system block size
    t->items[1] = t->items[0]; //  fragment size
    t->items[2] = MP_OBJ_NEW_SMALL_INT(total/CONFIG_SPIFFS_LOGICAL_BLOCK_SIZE); // size of fs in f_frsize units
    t->items[3] = MP_OBJ_NEW_SMALL_INT((total-used)/CONFIG_SPIFFS_LOGICAL_BLOCK_SIZE); // f_bfree
    t->items[4] = t->items[3]; // f_bavail
    t->items[5] = MP_OBJ_NEW_SMALL_INT(0); // f_files
    t->items[6] = MP_OBJ_NEW_SMALL_INT(0); // f_ffree
    t->items[7] = MP_OBJ_NEW_SMALL_INT(0); // f_favail
    t->items[8] = MP_OBJ_NEW_SMALL_INT(0); // f_flags
    t->items[9] = MP_OBJ_NEW_SMALL_INT(SPIFFS_OBJ_NAME_LEN); // f_namemax

    return MP_OBJ_FROM_PTR(t);

}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(spiffs_vfs_statvfs_obj, spiffs_vfs_statvfs);

STATIC mp_obj_t vfs_spiffs_mount(mp_obj_t self_in, mp_obj_t readonly, mp_obj_t mkfs) {
    spiffs_user_mount_t *self = MP_OBJ_TO_PTR(self_in);
    // Read-only device indicated by writeblocks[0] == MP_OBJ_NULL.
    // User can specify read-only device by:
    //  1. readonly=True keyword argument
    //  2. nonexistent writeblocks method (then writeblocks[0] == MP_OBJ_NULL already)
    if (mp_obj_is_true(readonly)) {
        self->cfg.hal_write_f = MP_OBJ_NULL;
    }
	int res = mp_module_spiffs_mount(&self->fs,&self->cfg);
	// check if we need to make the filesystem
	if(SPIFFS_ERR_NOT_A_FS == res || res != SPIFFS_OK)
    {
//	   	mp_printf(&mp_plat_print, "[MaixPy]:SPIFFS Error Code %d\n",res);
		mp_raise_OSError(SPIFFS_errno_table[GET_ERR_CODE(res)]);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(vfs_spiffs_mount_obj, vfs_spiffs_mount);

STATIC mp_obj_t vfs_spiffs_umount(mp_obj_t self_in) {
    (void)self_in;
    // keep the FAT filesystem mounted internally so the VFS methods can still be used
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(spiffs_vfs_umount_obj, vfs_spiffs_umount);

STATIC const mp_rom_map_elem_t spiffs_vfs_locals_dict_table[] = {
    #if _FS_REENTRANT
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&spiffs_vfs_del_obj) },
    #endif
    { MP_ROM_QSTR(MP_QSTR_mkfs), MP_ROM_PTR(&spiffs_vfs_mkfs_obj) },
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&spiffs_vfs_open_obj) },
    { MP_ROM_QSTR(MP_QSTR_mount), MP_ROM_PTR(&vfs_spiffs_mount_obj) },
    { MP_ROM_QSTR(MP_QSTR_ilistdir), MP_ROM_PTR(&spiffs_vfs_ilistdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_chdir), MP_ROM_PTR(&spiffs_vfs_chdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_mkdir), MP_ROM_PTR(&spiffs_vfs_mkdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_rmdir), MP_ROM_PTR(&spiffs_vfs_rmdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_getcwd), MP_ROM_PTR(&spiffs_vfs_getcwd_obj) },
    { MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&spiffs_vfs_remove_obj) },
    { MP_ROM_QSTR(MP_QSTR_rename), MP_ROM_PTR(&spiffs_vfs_rename_obj) },
    { MP_ROM_QSTR(MP_QSTR_stat), MP_ROM_PTR(&spiffs_vfs_stat_obj) },
    { MP_ROM_QSTR(MP_QSTR_statvfs), MP_ROM_PTR(&spiffs_vfs_statvfs_obj) },
    { MP_ROM_QSTR(MP_QSTR_umount), MP_ROM_PTR(&spiffs_vfs_umount_obj) },
};
STATIC MP_DEFINE_CONST_DICT(spiffs_vfs_locals_dict, spiffs_vfs_locals_dict_table);


STATIC const mp_vfs_proto_t spiffs_vfs_proto = {
    .import_stat = spiffs_vfs_import_stat,
};

const mp_obj_type_t mp_spiffs_vfs_type = {
    { &mp_type_type },
    .name = MP_QSTR_VfsSpiffs,
    .make_new = spiffs_vfs_make_new,
    .protocol = &spiffs_vfs_proto,
    .locals_dict = (mp_obj_dict_t*)&spiffs_vfs_locals_dict,
};
#endif

