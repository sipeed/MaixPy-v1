#ifndef __VFS_WRAPPER_H
#define __VFS_WRAPPER_H

#include "stdint.h"
#include "stdbool.h"
#include "py/mpconfig.h"
#include "vfs_internal.h"

mp_uint_t vfs_save_data(const char* path, uint8_t* data, mp_uint_t length, int* error_code);
void read_word_raise(mp_obj_t fp, uint16_t* data);
void vfs_file_corrupted_raise(mp_int_t fp);
mp_uint_t vfs_seek_raise(mp_obj_t fs, mp_int_t offset, uint8_t whence);
bool read_data(mp_obj_t fp, void *data, mp_uint_t size, int* err);
NORETURN void vfs_unsupported_format_raise(mp_obj_t fp);
void read_long_expect_raise(mp_obj_t fp, uint32_t value);
void read_long_ignore(mp_obj_t fp);
void read_long_raise(mp_obj_t fp, uint32_t *value);
void read_byte_expect_raise(mp_obj_t fp, uint8_t value);
bool read_byte(mp_obj_t fp, uint8_t* value, int* err);
mp_uint_t vfs_seek_raise(mp_obj_t fs, mp_int_t offset, uint8_t whence);
void vfs_file_corrupted_raise(mp_int_t fp);
void read_word_ignore_raise(mp_obj_t fp);
void read_byte_ignore_raise(mp_obj_t fp);
bool read_byte_ignore(mp_obj_t fp, int* err);
void read_word_expect_raise(mp_obj_t fp, uint16_t value);
bool read_word(mp_obj_t fp, uint16_t* data, int* err);
void read_word_raise(mp_obj_t fp, uint16_t* data);




#endif

