#ifndef __VFS_INTERNAL_H__
#define __VFS_INTERNAL_H__

#include "stdbool.h"
#include "stdint.h"
#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/mperrno.h"

typedef enum{
    VFS_SEEK_SET = 0,
    VFS_SEEK_CUR = 1,
    VFS_SEEK_END = 2
} vfs_seek_t;

mp_obj_t vfs_internal_open(const char* path, const char* mode, int* error_code);
mp_uint_t vfs_internal_write(mp_obj_t fs, void* data, mp_uint_t length, int* error_code);
mp_uint_t vfs_internal_read(mp_obj_t fs, void* data, mp_uint_t length, int* error_code);
void vfs_internal_close(mp_obj_t fs, int* error_code);
mp_uint_t vfs_internal_seek(mp_obj_t fs, mp_int_t offset, uint8_t whence, int* err);
mp_uint_t vfs_internal_size(mp_obj_t fp);
void vfs_internal_remove(const char* path, int* error_code);
#endif

