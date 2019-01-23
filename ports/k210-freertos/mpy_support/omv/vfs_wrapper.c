#include "vfs_wrapper.h"
#include "py/stream.h"
#include "extmod/vfs.h"
#include "py/runtime.h"

mp_uint_t vfs_save_data(const char* path, uint8_t* data, mp_uint_t length, int* error_code)
{
    mp_uint_t len;
    mp_obj_t file;

    file = vfs_internal_open(path, "wb",error_code);
    if( file == NULL )
        return 0;
    len = vfs_internal_write(file, data, length, error_code);
    vfs_internal_close(file, error_code);
    return len;    
}

void read_word_raise(mp_obj_t fp, uint16_t* data)
{
    int err;
    vfs_internal_read(fp, (void*)data, 2, &err);
    if(err != 0)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
}

bool read_word(mp_obj_t fp, uint16_t* data, int* err)
{
    vfs_internal_read(fp, (void*)data, 2, &err);
    if(err != 0)
        return false;
    return true;
}


void read_word_expect_raise(mp_obj_t fp, uint16_t value)
{
    uint16_t compare;
    int err;
    read_word_raise(fp, &compare);
    if (value != compare)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
}

bool read_byte_ignore(mp_obj_t fp, int* err)
{
    uint8_t tmp;

    vfs_internal_read(fp, (void*)&tmp, 1, &err);
    if(err != 0)
        return false;
    return true;
}

void read_byte_ignore_raise(mp_obj_t fp)
{
    int err;
    uint8_t tmp;

    vfs_internal_read(fp, (void*)&tmp, 1, &err);
    if(err != 0)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
}


void read_word_ignore_raise(mp_obj_t fp)
{
    int err;
    uint8_t tmp;

    vfs_internal_read(fp, (void*)&tmp, 2, &err);
    if(err != 0)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
}

void vfs_file_corrupted_raise(mp_int_t fp)
{
    int err;
    vfs_internal_close(fp, &err);
    mp_raise_OSError(EINVAL);
}

mp_uint_t vfs_seek_raise(mp_obj_t fs, mp_int_t offset, uint8_t whence)
{
    int err;
    vfs_internal_seek(fs, offset, whence, &err);
    if(err != 0)
    {
        vfs_internal_close(fs, &err);
        mp_raise_OSError(err);
    }
}

bool read_byte(mp_obj_t fp, uint8_t* value, int* err)
{
    vfs_internal_read(fp, (void*)value, 1, &err);
    if(err != 0)
        return false;
    return true;
}

void read_byte_expect_raise(mp_obj_t fp, uint8_t value)
{
    int err;
    uint8_t tmp;

    vfs_internal_read(fp, (void*)&tmp, 1, &err);
    if(err != 0)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
    if (value != tmp)
    {
        vfs_internal_close(fp, &err);
        mp_raise_ValueError("read not expect");
    }
}


void read_long_raise(mp_obj_t fp, uint32_t *value)
{
    int err;
    vfs_internal_read(fp, (void*)value, 4, &err);
    if(err != 0)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
}

void read_long_ignore(mp_obj_t fp)
{
    uint32_t trash;
    read_long_raise(fp, &trash);
}


void read_long_expect_raise(mp_obj_t fp, uint32_t value)
{
    int err;
    uint32_t compare;
    read_long_raise(fp, &compare);
    if (value != compare)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
}

void read_long_expect2_raise(mp_obj_t fp, uint32_t value, uint32_t value2)
{
    int err;
    uint32_t compare;
    read_long_raise(fp, &compare);
    if (value != compare && value2 != compare)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
}


NORETURN void vfs_unsupported_format_raise(mp_obj_t fp)
{
    int err;
    if(fp)
    {
        vfs_internal_close(fp, &err);
        mp_raise_ValueError("Unsupported format!");
    }
}

bool read_data(mp_obj_t fp, void *data, mp_uint_t size, int* err)
{
    vfs_internal_read(fp, data, size, err);
    if(err != 0)
        return false;
    return true;
}

NORETURN void vfs_no_intersection_raise(mp_obj_t fp)
{
    int err;
    if (fp)
        vfs_internal_close(fp, &err);
    nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "No intersection!"));
}

void write_byte_raise(mp_obj_t fp, uint8_t value)
{
    int err;
    vfs_internal_write(fp, (void*)&value, 1, &err);
    if(err != 0)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
}

void write_long_raise(mp_obj_t fp, uint32_t value)
{
    int err;
    vfs_internal_write(fp, (void*)&value, 4, &err);
    if(err != 0)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
}

void write_word_raise(mp_obj_t fp, uint16_t value)
{
    int err;
    vfs_internal_write(fp, (void*)&value, 2, &err);
    if(err != 0)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
}

void write_data_raise(mp_obj_t fp, const void *data, mp_uint_t size)
{
    int err;
    vfs_internal_write(fp, (void*)data, size, &err);
    if(err != 0)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
}  

