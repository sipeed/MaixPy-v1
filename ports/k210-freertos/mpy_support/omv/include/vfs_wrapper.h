#ifndef __VFS_WRAPPER_H
#define __VFS_WRAPPER_H

#include "stdint.h"
#include "stdbool.h"
#include "py/mpconfig.h"

mp_uint_t vfs_save_data(const char* path, uint8_t* data, mp_uint_t length, int* error_code);

#endif

