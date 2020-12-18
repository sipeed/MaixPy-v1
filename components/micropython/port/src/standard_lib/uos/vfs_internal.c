
#include "vfs_internal.h"
#include "extmod/vfs.h"
#include "vfs_spiffs.h"
#include  "extmod/vfs_fat.h"
#include "py/misc.h"
#include "py/mperrno.h"
#include "py/stream.h"

#define FLAG_SPIFFS  0x00000001
#define FLAG_FATFS   0x00000002

typedef struct {
    mp_obj_base_t base;
} fs_info_t;

typedef struct _spiffs_FILE{
    spiffs* fs;
    spiffs_file fd;
} spiffs_FILE;

// @attention **MUST** the same as defined in `vfs_spiffs_file.c`
typedef struct {
    mp_obj_base_t base;
    spiffs_FILE fp;
} pyb_file_spiffs_obj_t;

// @attention **MUST** the same as defined in `vfs_fat_file.c`
typedef struct {
    mp_obj_base_t base;
    FIL fp;
} pyb_file_fatfs_obj_t;


mp_obj_t vfs_internal_spiffs_open(spiffs_user_mount_t* vfs, const char* path, const char* mode_s, int* error_code)
{
    uint32_t mode = 0;
    mp_obj_type_t* type = (mp_obj_type_t*)&mp_type_vfs_spiffs_textio;
    
    *error_code = 0;
    char* open_name = (char*)path;
    if(open_name[0] == '.' && open_name[1] == '/')
    {
        memmove(open_name, open_name+1, strlen(open_name));
    }
    else if(open_name[0] != '/')
    {
        open_name = m_new(char, 128);
        memset(open_name, 0, 128);
        open_name[0] = '/';
        strcat(open_name, path);
    }
    while (*mode_s) {
        switch (*mode_s++) {
            case 'r':
                mode |= SPIFFS_O_RDONLY;
                break;
            case 'w':
                mode |= SPIFFS_O_RDWR | SPIFFS_O_CREAT | SPIFFS_O_TRUNC;
                break;
            case 'x':
                mode |= SPIFFS_O_RDWR | SPIFFS_O_CREAT | SPIFFS_O_TRUNC;
                break;
            case 'a':
                mode |= SPIFFS_O_RDWR | SPIFFS_O_APPEND;
                break;
            case '+':
                mode |= SPIFFS_O_RDWR;
                break;
#if MICROPY_PY_IO_FILEIO
            case 'b':
                type = (mp_obj_type_t*)&mp_type_vfs_spiffs_fileio;
                break;
#endif
            case 't':
                type = (mp_obj_type_t*)&mp_type_vfs_spiffs_textio;
                break;
        }
    }
    pyb_file_spiffs_obj_t *o = m_new_obj(pyb_file_spiffs_obj_t);
    o->base.type = type;
    spiffs_FILE fp;
	fp.fd = SPIFFS_open(&vfs->fs,open_name,mode,0);
    fp.fs = &vfs->fs;  
    if(fp.fd <= 0)
    {
        *error_code = MP_EIO;
        m_del_obj(pyb_file_spiffs_obj_t, o);
        return MP_OBJ_NULL;
    }
    o->fp = fp;
    return MP_OBJ_FROM_PTR(o);
}

mp_obj_t vfs_internal_fatfs_open(fs_user_mount_t* vfs, const char* path, const char* mode_s, int* error_code)
{
    int mode = 0;
    mp_obj_type_t* type = (mp_obj_type_t*)&mp_type_vfs_spiffs_textio;

    *error_code = 0;
    // TODO: make sure only one of r, w, x, a, and b, t are specified
    while (*mode_s) {
        switch (*mode_s++) {
            case 'r':
                mode |= FA_READ;
                break;
            case 'w':
                mode |= FA_WRITE | FA_CREATE_ALWAYS;
                break;
            case 'x':
                mode |= FA_WRITE | FA_CREATE_NEW;
                break;
            case 'a':
                mode |= FA_WRITE | FA_OPEN_ALWAYS;
                break;
            case '+':
                mode |= FA_READ | FA_WRITE;
                break;
            #if MICROPY_PY_IO_FILEIO
            case 'b':
                type = (mp_obj_type_t*)&mp_type_vfs_fat_fileio;
                break;
            #endif
            case 't':
                type = (mp_obj_type_t*)&mp_type_vfs_fat_textio;
                break;
        }
    }

    pyb_file_fatfs_obj_t *o = m_new_obj(pyb_file_fatfs_obj_t);
    o->base.type = type;
    FRESULT res = f_open(&vfs->fatfs, &o->fp, path, mode);
    if (res != FR_OK) {
        m_del_obj(pyb_file_fatfs_obj_t, o);
        *error_code = MP_ENOENT;
        return MP_OBJ_NULL;
    }

    // for 'a' mode, we must begin at the end of the file
    if ((mode & FA_OPEN_ALWAYS) != 0) {
        f_lseek(&o->fp, f_size(&o->fp));
    }
    return MP_OBJ_FROM_PTR(o);
}


void vfs_internal_spiffs_remove(spiffs_user_mount_t* vfs, const char* path, int* error_code)
{
    *error_code = 0;
    char* open_name = (char*)path;
    if(open_name[0] == '.' && open_name[1] == '/')
    {
        memmove(open_name, open_name+1, strlen(open_name));
    }
    else if(open_name[0] != '/')
    {
        open_name = m_new(char, 128);
        memset(open_name, 0, 128);
        open_name[0] = '/';
        strcat(open_name, path);
    }
	SPIFFS_remove(&vfs->fs,open_name);
}

void vfs_internal_fatfs_remove(fs_user_mount_t* vfs, const char* path, int* error_code)
{
    *error_code = 0;

    FRESULT res = f_unlink(&vfs->fatfs, path);
    if (res != FR_OK) {
        *error_code = MP_ENOENT;
    }
}

/**
 * 
 * 
 * 
 * @return mp_obj_t: MP_OBJ_NULL if open fail
 */
mp_obj_t vfs_internal_open(const char* path, const char* mode, int* error_code)
{
    const char *real_path;
    *error_code = 0;
    mp_vfs_mount_t *vfs = mp_vfs_lookup_path(path, &real_path);
    if (vfs == MP_VFS_NONE || vfs == MP_VFS_ROOT) {
        *error_code = MP_ENOENT;
        return MP_OBJ_NULL;
    }
    fs_info_t* fs = (fs_info_t*)vfs->obj;
    if( fs->base.type == &mp_spiffs_vfs_type)
    {
        // *error_code =  EPERM;
        // return MP_OBJ_NULL;
        return vfs_internal_spiffs_open((spiffs_user_mount_t*)fs, real_path, mode, error_code);
    }
    else if( fs->base.type == &mp_fat_vfs_type)
    {
        return vfs_internal_fatfs_open((fs_user_mount_t*)fs, real_path, mode, error_code);
    }
    *error_code = MP_ENOENT;
    return MP_OBJ_NULL;
}

void vfs_internal_remove(const char* path, int* error_code)
{
    const char *real_path;
    *error_code = 0;
    mp_vfs_mount_t *vfs = mp_vfs_lookup_path(path, &real_path);
    if (vfs == MP_VFS_NONE || vfs == MP_VFS_ROOT) {
        *error_code = MP_EINVAL;
        return;
    }
    fs_info_t* fs = (fs_info_t*)vfs->obj;
    if( fs->base.type == &mp_spiffs_vfs_type)
    {
        // *error_code =  EPERM;
        // return MP_OBJ_NULL;
        vfs_internal_spiffs_remove((spiffs_user_mount_t*)fs, real_path, error_code);
    }
    else if( fs->base.type == &mp_fat_vfs_type)
    {
        vfs_internal_fatfs_remove((fs_user_mount_t*)fs, real_path, error_code);
    }
    *error_code = MP_ENOENT;
}

mp_uint_t vfs_internal_write(mp_obj_t fs, void* data, mp_uint_t length, int* error_code)
{
    fs_info_t* fs_info = (fs_info_t*)fs;
    mp_stream_p_t* stream = (mp_stream_p_t*)fs_info->base.type->protocol;
    *error_code = 0;
    return stream->write(fs, data, length, error_code);
}

mp_uint_t vfs_internal_read(mp_obj_t fs, void* data, mp_uint_t length, int* error_code)
{
    fs_info_t* fs_info = (fs_info_t*)fs;
    mp_stream_p_t* stream = (mp_stream_p_t*)fs_info->base.type->protocol;
    *error_code = 0;
    return stream->read(fs, data, length, error_code);
}

void vfs_internal_close(mp_obj_t fs, int* error_code)
{
    fs_info_t* fs_info = (fs_info_t*)fs;
    mp_stream_p_t* stream = (mp_stream_p_t*)fs_info->base.type->protocol;
    *error_code = 0;
    stream->ioctl(fs, MP_STREAM_CLOSE, 0, error_code);
    if(fs_info->base.type == &mp_type_vfs_spiffs_fileio ||
        fs_info->base.type == &mp_type_vfs_spiffs_textio)
    {
        m_del_obj(pyb_file_spiffs_obj_t, fs);
    }
    else if(fs_info->base.type == &mp_type_vfs_fat_fileio ||
            fs_info->base.type == &mp_type_vfs_fat_textio)
    {
        m_del_obj(pyb_file_fatfs_obj_t, fs);
    }
}

mp_uint_t vfs_internal_seek(mp_obj_t fs, mp_int_t offset, uint8_t whence, int* error_code)
{
    fs_info_t* fs_info = (fs_info_t*)fs;
    mp_stream_p_t* stream = (mp_stream_p_t*)fs_info->base.type->protocol;
    *error_code = 0;
    struct mp_stream_seek_t seek;
    seek.offset = offset;
    seek.whence = whence;
    return stream->ioctl(fs, MP_STREAM_SEEK, (uintptr_t)&seek, error_code);
}

mp_uint_t vfs_internal_size(mp_obj_t fs)
{
    fs_info_t* fs_info = (fs_info_t*)fs;
    // mp_stream_p_t* stream = (mp_stream_p_t*)fs_info->base.type->protocol;
    if(fs_info->base.type == &mp_type_vfs_spiffs_fileio ||
        fs_info->base.type == &mp_type_vfs_spiffs_textio)
    {
        spiffs_FILE spiffs_f = ((pyb_file_spiffs_obj_t*)fs)->fp;
        spiffs* f = spiffs_f.fs;
        spiffs_file fd = spiffs_f.fd;
        spiffs_stat stat;
        SPIFFS_fstat(f, fd, &stat);
        return stat.size;
    }
    else if(fs_info->base.type == &mp_type_vfs_fat_fileio ||
            fs_info->base.type == &mp_type_vfs_fat_textio)
    {
        FIL f = ((pyb_file_fatfs_obj_t*)fs)->fp;
        return f_size(&f);
    }
    return EIO;
}



