#ifndef __VFS_INTERNAL_H__
#define __VFS_INTERNAL_H__

#include "stdbool.h"
#include "stdint.h"
#include "py/mpconfig.h"
#include "py/obj.h"

mp_obj_t vfs_internal_open(const char* path, const char* mode, int* error_code);
mp_uint_t vfs_internal_write(mp_obj_t fs, void* data, mp_uint_t length, int* error_code);
mp_uint_t vfs_internal_read(mp_obj_t fs, void* data, mp_uint_t length, int* error_code);
void vfs_internal_close(mp_obj_t fs, int* error_code);


#endif

