#ifndef __VFS_WRAPPER_H
#define __VFS_WRAPPER_H

#include "stdint.h"
#include "stdbool.h"
#include "py/mpconfig.h"
#include "vfs_internal.h"

int file_write_open_raise(mp_obj_t* fp, const char *path);
int file_read_open_raise(mp_obj_t* fp, const char *path);
int file_write_open(mp_obj_t* fp, const char *path);
int file_read_open(mp_obj_t* fp, const char *path);
int file_close(mp_obj_t fp);
int file_seek(mp_obj_t fp, mp_int_t offset, uint8_t whence);
bool file_eof(mp_obj_t fp);
int file_seek_raise(mp_obj_t fp, mp_int_t offset, uint8_t whence);
mp_uint_t file_save_data(const char* path, uint8_t* data, mp_uint_t length, int* error_code);
mp_uint_t file_size(mp_obj_t fp);
void file_buffer_on(mp_obj_t fp);
void file_buffer_off(mp_obj_t fp);

void fs_unsupported_format(mp_obj_t fp);
void fs_file_corrupted(mp_obj_t fp);
void fs_not_equal(mp_obj_t fp);
void fs_no_intersection(mp_obj_t fp);



int file_corrupted_raise(mp_obj_t fp);
const char *ffs_strerror(int res);


int read_byte(mp_obj_t fp, uint8_t* value);
int read_byte_raise(mp_obj_t fp, uint8_t* value);
int read_byte_expect(mp_obj_t fp, uint8_t value);
int read_byte_ignore(mp_obj_t fp);
int write_byte(mp_obj_t fp, uint8_t value);
int write_byte_raise(mp_obj_t fp, uint8_t value);

int read_word(mp_obj_t fp, uint16_t* value);
int read_word_raise(mp_obj_t fp, uint16_t* value);
int read_word_expect(mp_obj_t fp, uint16_t value);
int read_word_ignore(mp_obj_t fp);
int write_word(mp_obj_t fp, uint16_t value);
int write_word_raise(mp_obj_t fp, uint16_t value);

int read_long(mp_obj_t fp, uint32_t* value);
int read_long_raise(mp_obj_t fp, uint32_t* value);
int read_long_expect(mp_obj_t fp, uint32_t value);
int read_long_ignore(mp_obj_t fp);
int write_long(mp_obj_t fp, uint32_t value);
int write_long_raise(mp_obj_t fp, uint32_t value);

int read_data(mp_obj_t fp, void *data, mp_uint_t size);
int read_data_raise(mp_obj_t fp, void *data, mp_uint_t size);
int write_data(mp_obj_t fp, const void *data, mp_uint_t size);
int write_data_raise(mp_obj_t fp, const void *data, mp_uint_t size);
int file_write(mp_obj_t fp, void *data, mp_uint_t size, mp_uint_t* size_out);
int file_read(mp_obj_t fp, void *data, mp_uint_t size, mp_uint_t* size_out);
int read_data_raise(mp_obj_t fp, void *data, mp_uint_t size);

#endif

