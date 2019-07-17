#include "vfs_wrapper.h"
#include "py/stream.h"
#include "extmod/vfs.h"
#include "py/runtime.h"

/************ File OP ************/
int file_write_open(mp_obj_t* fp, const char *path)
{
    int err;
    *fp = vfs_internal_open(path, "wb", &err);
    return err;
}

int file_write_open_raise(mp_obj_t* fp, const char *path)
{
    int err;
    *fp = vfs_internal_open(path, "wb", &err);
    if(*fp == MP_OBJ_NULL || err != 0)
        mp_raise_OSError(err);
    return err;
}

int file_read_open(mp_obj_t* fp, const char *path)
{
    int err;
    *fp = vfs_internal_open(path, "rb", &err);
    return err;
}

int file_read_open_raise(mp_obj_t* fp, const char *path)
{
    int err;
    *fp = vfs_internal_open(path, "rb", &err);
    if(*fp == MP_OBJ_NULL || err != 0)
        mp_raise_OSError(err);
    return err;
}


int file_close(mp_obj_t fp)
{
    int err;
    vfs_internal_close(fp, &err);
    return err;
}

int file_seek(mp_obj_t fp, mp_int_t offset, uint8_t whence)
{
    int err;
    vfs_internal_seek(fp, offset, whence, &err);
    return err;
}

bool file_eof(mp_obj_t fp)
{
    //TODO: recode this function
    int err;
    mp_uint_t size = vfs_internal_size(fp);
    mp_uint_t curr = vfs_internal_seek(fp, 0, VFS_SEEK_CUR, &err);
    return curr<size;
}

int file_seek_raise(mp_obj_t fp, mp_int_t offset, uint8_t whence)
{
    int err;
    vfs_internal_seek(fp, offset, whence, &err);
	if (err != 0)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
    return err;
}

mp_uint_t file_save_data(const char* path, uint8_t* data, mp_uint_t length, int* error_code)
{
    mp_uint_t len;
    mp_obj_t file;

    file = vfs_internal_open(path, "wb",error_code);
    if( file == MP_OBJ_NULL || *error_code != 0 )
        return 0;
    len = vfs_internal_write(file, data, length, error_code);
    vfs_internal_close(file, error_code);
    return len;    
}

mp_uint_t file_size(mp_obj_t fp)
{
	return vfs_internal_size(fp);
}


void file_buffer_on(mp_obj_t fp)
{
	//TODO
}

void file_buffer_off(mp_obj_t fp)
{
	//TODO
}

/************ Raise ************/
NORETURN static void fs_fail(mp_obj_t fp, int res)
{
    int err;
    if(fp)
        vfs_internal_close(fp, &err);
    nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, ffs_strerror(res)));
}

NORETURN static void fs_read_fail(mp_obj_t fp)
{
    int err;
    if(fp)
        vfs_internal_close(fp, &err);
    nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Failed to read requested bytes!"));
}

NORETURN static void fs_write_fail(mp_obj_t fp)
{
    int err;
    if(fp)
        vfs_internal_close(fp, &err);
    nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Failed to write requested bytes!"));
}

NORETURN static void fs_expect_fail(mp_obj_t fp)
{
    int err;
    if(fp)
        vfs_internal_close(fp, &err);
    nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Unexpected value read!"));
}

NORETURN void fs_unsupported_format(mp_obj_t fp)
{
    int err;
    if(fp)
        vfs_internal_close(fp, &err);
    nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Unsupported format!"));
}

NORETURN void fs_file_corrupted(mp_obj_t fp)
{
    int err;
    if(fp)
        vfs_internal_close(fp, &err);
    nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "File corrupted!"));
}

NORETURN void fs_not_equal(mp_obj_t fp)
{
    int err;
    if(fp)
        vfs_internal_close(fp, &err);
    nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Images not equal!"));
}

NORETURN void fs_no_intersection(mp_obj_t fp)
{
    int err;
    if(fp)
        vfs_internal_close(fp, &err);
    nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "No intersection!"));
}

int file_corrupted_raise(mp_obj_t fp)
{
    int err;
    if(fp)
    {
        vfs_internal_close(fp, &err);
    }
	mp_raise_ValueError("file_corrupted_raise!");
	return err;
}

const char *ffs_strerror(int res)
{
    return "ERROR CODE: TODO";
}



//before raise, must close fp.
/************ RW byte ************/
int read_byte(mp_obj_t fp, uint8_t* value)
{
    int err;
    vfs_internal_read(fp, (void*)value, 1, &err);
    return err;
}

int read_byte_raise(mp_obj_t fp, uint8_t* value)
{
    int err;
    vfs_internal_read(fp, (void*)value, 1, &err);
	if (err != 0)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
    return err;
}

int read_byte_expect(mp_obj_t fp, uint8_t value)
{
    uint8_t tmp;
	read_byte_raise(fp, &tmp);
    return (value != tmp);
}

int read_byte_ignore(mp_obj_t fp)
{
    uint8_t tmp;
    int err;
    vfs_internal_read(fp, (void*)&tmp, 1, &err);
    return err;
}

int write_byte(mp_obj_t fp, uint8_t value)
{
    int err;
    vfs_internal_write(fp, (void*)&value, 1, &err);
    return err;
}

int write_byte_raise(mp_obj_t fp, uint8_t value)
{
    int err;
    vfs_internal_write(fp, (void*)&value, 1, &err);
	if (err != 0)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
    return err;
}

/************ RW word ************/
int read_word(mp_obj_t fp, uint16_t* value)
{
    int err;
    vfs_internal_read(fp, (void*)value, 2, &err);
    return err;
}

int read_word_raise(mp_obj_t fp, uint16_t* value)
{
    int err;
    vfs_internal_read(fp, (void*)value, 2, &err);
	if (err != 0)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
    return err;
}

int read_word_expect(mp_obj_t fp, uint16_t value)
{
    uint16_t tmp;
	read_word_raise(fp, &tmp);
    return (value != tmp);
}

int read_word_ignore(mp_obj_t fp)
{
    uint16_t tmp;
    int err;
    vfs_internal_read(fp, (void*)&tmp, 2, &err);
    return err;
}

int write_word(mp_obj_t fp, uint16_t value)
{
    int err;
    vfs_internal_write(fp, (void*)&value, 2, &err);
    return err;
}

int write_word_raise(mp_obj_t fp, uint16_t value)
{
    int err;
    vfs_internal_write(fp, (void*)&value, 2, &err);
	if (err != 0)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
    return err;
}

/************ RW long ************/
int read_long(mp_obj_t fp, uint32_t* value)
{
    int err;
    vfs_internal_read(fp, (void*)value, 4, &err);
    return err;
}

int read_long_raise(mp_obj_t fp, uint32_t* value)
{
    int err;
    vfs_internal_read(fp, (void*)value, 4, &err);
	if (err != 0)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
    return err;
}

int read_long_expect(mp_obj_t fp, uint32_t value)
{
    uint32_t tmp;
	read_long_raise(fp, &tmp);
    return (value != tmp);
}

int read_long_ignore(mp_obj_t fp)
{
    uint32_t tmp;
    int err;
    vfs_internal_read(fp, (void*)&tmp, 4, &err);
    return err;
}

int write_long(mp_obj_t fp, uint32_t value)
{
    int err;
    vfs_internal_write(fp, (void*)&value, 4, &err);
    return err;
}

int write_long_raise(mp_obj_t fp, uint32_t value)
{
    int err;
    vfs_internal_write(fp, (void*)&value, 4, &err);
	if (err != 0)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
    return err;
}

/************ RW Data ************/
int read_data(mp_obj_t fp, void *data, mp_uint_t size)
{
    int err;
    vfs_internal_read(fp, data, size, &err);
    return err;
}

int file_read(mp_obj_t fp, void *data, mp_uint_t size, mp_uint_t* size_out)
{
    int err;
    *size_out = vfs_internal_read(fp, data, size, &err);
    return err;
}

int read_data_raise(mp_obj_t fp, void *data, mp_uint_t size)
{
    int err;
    vfs_internal_read(fp, data, size, &err);
	if (err != 0)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
    return err;
}

int write_data(mp_obj_t fp, const void *data, mp_uint_t size)
{
    int err;
    vfs_internal_write(fp, (void*)data, size, &err);
    return err;
}  

int file_write(mp_obj_t fp, void *data, mp_uint_t size, mp_uint_t* size_out)
{
    int err;
    *size_out = vfs_internal_write(fp, data, size, &err);
    return err;
}

int write_data_raise(mp_obj_t fp, const void *data, mp_uint_t size)
{
    int err;
    vfs_internal_write(fp, (void*)data, size, &err);
	if (err != 0)
    {
        vfs_internal_close(fp, &err);
        mp_raise_OSError(err);
    }
    return err;
} 




