#include "vfs_wrapper.h"
#include "py/stream.h"
#include "extmod/vfs.h"

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

