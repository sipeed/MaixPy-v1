/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
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
#ifndef MICROPY_INCLUDED_EXTMOD_VFS_SPIFFS_H
#define MICROPY_INCLUDED_EXTMOD_VFS_SPIFFS_H

#include "py/lexer.h"
#include "py/obj.h"
#include "extmod/vfs.h"
#include "spiffs.h"
// these are the values for fs_user_mount_t.flags
#define MODULE_SPIFFS       (0x0001) // readblocks[2]/writeblocks[2] contain native func
#define SYS_SPIFFS     (0x0002) // fs_user_mount_t obj should be freed on umount
#define FSUSER_HAVE_IOCTL   (0x0004) // new protocol with ioctl
#define FSUSER_NO_FILESYSTEM (0x0008) // the block device has no filesystem on it

typedef struct _spiffs_user_mount_t {
    mp_obj_base_t base;
    uint16_t flags;
	spiffs_config cfg;
    spiffs fs;
	mp_obj_t read_obj[5];
	mp_obj_t write_obj[5];
	mp_obj_t erase_obj[4];
	mp_vfs_proto_t *protocol;
} spiffs_user_mount_t;
typedef enum {
	FS_OK = 0				 ,
	ERR_NOT_MOUNTED 		 ,
	ERR_FULL         		 ,        
	ERR_NOT_FOUND     		 ,       
	ERR_END_OF_OBJECT 		 ,      
	ERR_DELETED              ,
	ERR_NOT_FINALIZED  		 ,     
	ERR_NOT_INDEX       	 ,    
	ERR_OUT_OF_FILE_DESCS 	 ,   
	ERR_FILE_CLOSED          ,  
	ERR_FILE_DELETED         , 
	ERR_BAD_DESCRIPTOR       ,
	ERR_IS_INDEX             ,
	ERR_IS_FREE              ,
	ERR_INDEX_SPAN_MISMATCH  ,
	ERR_DATA_SPAN_MISMATCH   ,
	ERR_INDEX_REF_FREE       ,
	ERR_INDEX_REF_LU         ,
	ERR_INDEX_REF_INVALID    ,
	ERR_INDEX_FREE           ,
	ERR_INDEX_LU             ,
	ERR_INDEX_INVALID        ,
	ERR_NOT_WRITABLE         ,
	ERR_NOT_READABLE         ,
	ERR_CONFLICTING_NAME     ,
	ERR_NOT_CONFIGURED       ,
	ERR_NOT_A_FS             ,
	ERR_MOUNTED              ,
	ERR_ERASE_FAIL           ,
	ERR_MAGIC_NOT_POSSIBLE   ,
	ERR_NO_DELETED_BLOCKS    ,
	ERR_FILE_EXISTS          ,
	ERR_NOT_A_FILE           ,
	ERR_RO_NOT_IMPL          ,
	ERR_RO_ABORTED_OPERATION ,
	ERR_PROBE_TOO_FEW_BLOCKS ,
	ERR_PROBE_NOT_A_FS       ,
	ERR_NAME_TOO_LONG        ,
	ERR_IX_MAP_UNMAPPED      ,
	ERR_IX_MAP_MAPPED        ,
	ERR_IX_MAP_BAD_RANGE     ,
	ERR_SEEK_BOUNDS          ,
}SPIFFS_ERR;


extern u8_t spiffs_work_buf[SPIFFS_CFG_LOG_PAGE_SZ(fs)*2];
extern u8_t spiffs_fds[32*4];
extern u8_t spiffs_cache_buf[(SPIFFS_CFG_LOG_PAGE_SZ(fs)+32)*4];

extern const byte SPIFFS_errno_table[43];
extern const mp_obj_type_t mp_spiffs_vfs_type;
extern const mp_obj_type_t mp_type_vfs_spiffs_fileio;
extern const mp_obj_type_t mp_type_vfs_spiffs_textio;

s32_t spiffs_read_method(spiffs* fs,uint32_t addr, uint32_t size, uint8_t *buf);
s32_t spiffs_write_method(spiffs* fs,uint32_t addr, uint32_t size, uint8_t *buf);
s32_t spiffs_erase_method(spiffs* fs,uint32_t addr, uint32_t size);
s32_t sys_spiffs_read(uint32_t addr, uint32_t size, uint8_t *buf);
s32_t sys_spiffs_write(uint32_t addr, uint32_t size, uint8_t *buf);
s32_t sys_spiffs_erase(uint32_t addr, uint32_t size);
int mp_module_spiffs_mount(spiffs* fs,spiffs_config* cfg);
int mp_module_spiffs_format(spiffs* fs);

MP_DECLARE_CONST_FUN_OBJ_3(spiffs_vfs_open_obj);

#endif // MICROPY_INCLUDED_EXTMOD_VFS_FAT_H
