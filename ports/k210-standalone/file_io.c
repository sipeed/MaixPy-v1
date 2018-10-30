#include <string.h>
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"
//#include "extmod/misc.h"
//#include "extmod/vfs.h"
//#include "extmod/vfs_fat.h"
//#include "genhdr/mpversion.h"
#include "py/stream.h"

#if !MICROPY_VFS
#include "py/lexer.h"
#include "spiffs-port.h"

const mp_obj_type_t mp_type_vfs_spiffs_textio;
typedef struct _pyb_file_obj_t {
    mp_obj_base_t base;
    spiffs_file fd;
} pyb_file_obj_t;

STATIC void file_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    mp_printf(print, "<io.%s %p>", mp_obj_get_type_str(self_in), MP_OBJ_TO_PTR(self_in));
}


STATIC mp_uint_t file_obj_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode) {
    pyb_file_obj_t *self = MP_OBJ_TO_PTR(self_in);

    spiffs_file fd=self->fd;
	s32_t ret = SPIFFS_read(&fs, fd, buf, size);
    if(ret <  0){
        *errcode = MP_EIO;
        return MP_STREAM_ERROR;
    }
    return (mp_uint_t)ret;
}

STATIC mp_uint_t file_obj_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode) {
    pyb_file_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t ret = SPIFFS_write(&fs,self->fd, (uint8_t*)buf, size);
    if(ret < 0)
    {
        *errcode = MP_EIO;
        return MP_STREAM_ERROR;
    }
    if (ret != size) {
        // The FatFS documentation says that this means disk full.
        *errcode = MP_ENOSPC;
        return MP_STREAM_ERROR;
    }
    return (mp_uint_t)ret;
}

STATIC mp_uint_t file_obj_ioctl(mp_obj_t o_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    pyb_file_obj_t *self = MP_OBJ_TO_PTR(o_in);
    

    if (request == MP_STREAM_SEEK) {
        struct mp_stream_seek_t *s = (struct mp_stream_seek_t*)(uintptr_t)arg;
        s32_t ret = 0;

        switch (s->whence) {
            case 0: // SEEK_SET
                ret = SPIFFS_lseek(&fs,self->fd,s->offset,SPIFFS_SEEK_SET);
                break;

            case 1: // SEEK_CUR
                ret = SPIFFS_lseek(&fs,self->fd,s->offset,SPIFFS_SEEK_CUR);
                break;

            case 2: // SEEK_END
                ret = SPIFFS_lseek(&fs,self->fd,s->offset,SPIFFS_SEEK_END);
                break;
        }

        s->offset = ret;
        return 0;

    } else if (request == MP_STREAM_FLUSH) {
        uint32_t ret = SPIFFS_fflush(&fs,self->fd);
        if (ret != 0) {
            *errcode = MP_EIO;
            return MP_STREAM_ERROR;
        }
        return 0;

    } else if (request == MP_STREAM_CLOSE) {
        // if fs==NULL then the file is closed and in that case this method is a no-op
        if (self->fd > 0) {
            int32_t ret = SPIFFS_close(&fs,self->fd);
            if (ret != 0) {
                *errcode = MP_EIO;
                return MP_STREAM_ERROR;
            }
        }
        return 0;

    } else {
        *errcode = MP_EINVAL;
        return MP_STREAM_ERROR;
    }
    return MP_STREAM_ERROR;
}


// Note: encoding is ignored for now; it's also not a valid kwarg for CPython's FileIO,
// but by adding it here we can use one single mp_arg_t array for open() and FileIO's constructor
STATIC const mp_arg_t file_open_args[] = {
    { MP_QSTR_file, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)} },
    { MP_QSTR_mode, MP_ARG_OBJ, {.u_obj = MP_OBJ_NEW_QSTR(MP_QSTR_r)} },
    { MP_QSTR_encoding, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)} },
};
#define FILE_OPEN_NUM_ARGS MP_ARRAY_SIZE(file_open_args)

/*STATIC*/ mp_obj_t file_open(const char* file_name, const mp_obj_type_t *type, mp_arg_val_t *args) {
    const char *mode_s = mp_obj_str_get_str(args[1].u_obj);
    uint32_t mode = 0;
    // TODO make sure only one of r, w, x, a, and b, t are specified
    while (*mode_s) {
        switch (*mode_s++) {
            case 'r':
                mode |= SPIFFS_O_RDONLY;
                break;
            case 'w':
                mode |= SPIFFS_O_RDWR | SPIFFS_O_CREAT;
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
                type = &mp_type_vfs_spiffs_fileio;
                break;
            #endif
            case 't':
                type = &mp_type_vfs_spiffs_textio;
                break;
        }
    }
    pyb_file_obj_t *o = m_new_obj_with_finaliser(pyb_file_obj_t);
    o->base.type = type;
    uint8_t *temp_obj_patch;
    spiffs_file fd;
    if(file_name[0]!='/')
    {
        temp_obj_patch=malloc(strlen(file_name));
        memset(temp_obj_patch,'\0',strlen(file_name));
        strcpy(temp_obj_patch+1,file_name);
        temp_obj_patch[0]='/';
        fd = SPIFFS_open(&fs,temp_obj_patch,mode,0);
    }else{
        fd = SPIFFS_open(&fs,file_name,mode,0);
    }
        
    if(fd <= 0)
        mp_raise_OSError(MP_EIO);
    o->fd = fd;
    
    return MP_OBJ_FROM_PTR(o);
}

STATIC mp_obj_t file_obj_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_val_t arg_vals[FILE_OPEN_NUM_ARGS];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, FILE_OPEN_NUM_ARGS, file_open_args, arg_vals);
    return file_open(NULL, type, arg_vals);
}

STATIC const mp_rom_map_elem_t rawfile_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    // { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    // { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    // { MP_ROM_QSTR(MP_QSTR_readlines), MP_ROM_PTR(&mp_stream_unbuffered_readlines_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&mp_stream_flush_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_seek), MP_ROM_PTR(&mp_stream_seek_obj) },
    // { MP_ROM_QSTR(MP_QSTR_tell), MP_ROM_PTR(&mp_stream_tell_obj) },
    // { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&mp_stream_close_obj) },
    // { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&mp_identity_obj) },
    // { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&file_obj___exit___obj) },
};
STATIC MP_DEFINE_CONST_DICT(rawfile_locals_dict, rawfile_locals_dict_table);

STATIC const mp_stream_p_t fileio_stream_p = {
    .read = file_obj_read,
    .write = file_obj_write,
    .ioctl = file_obj_ioctl,
};

const mp_obj_type_t mp_type_vfs_spiffs_fileio = {
    { &mp_type_type },
    .name = MP_QSTR_FileIO,
    .print = file_obj_print,
    .make_new = file_obj_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &fileio_stream_p,
    .locals_dict = (mp_obj_dict_t*)&rawfile_locals_dict,
};

STATIC const mp_stream_p_t textio_stream_p = {
    .read = file_obj_read,
    .write = file_obj_write,
    .ioctl = file_obj_ioctl,
    .is_text = true,
};

const mp_obj_type_t mp_type_vfs_spiffs_textio = {
    { &mp_type_type },
    .name = MP_QSTR_TextIOWrapper,
    .print = file_obj_print,
    .make_new = file_obj_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &textio_stream_p,
    .locals_dict = (mp_obj_dict_t*)&rawfile_locals_dict,
};

// Note: buffering and encoding args are currently ignored
mp_obj_t mp_vfs_open(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_file, ARG_mode, ARG_encoding };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_file, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)} },
        { MP_QSTR_mode, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_QSTR(MP_QSTR_r)} },
        { MP_QSTR_buffering, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_encoding, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    const char* fileName = mp_obj_str_get_str(args[ARG_file].u_obj);    
    const mp_obj_type_t*  type = &mp_type_vfs_spiffs_textio;
    return file_open(fileName,type,args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_vfs_open_obj, 0, mp_vfs_open);

/*
// Note: buffering and encoding args are currently ignored
mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_file, ARG_mode, ARG_encoding };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_file, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)} },
        { MP_QSTR_mode, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_QSTR(MP_QSTR_r)} },
        { MP_QSTR_buffering, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_encoding, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    const char* fileName = mp_obj_str_get_str(args[ARG_file].u_obj);    
    const mp_obj_type_t*  type = &mp_type_vfs_spiffs_textio;
    return file_open(fileName,type,args);
}

MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);
*/

typedef struct _mp_reader_vfs_t {
    mp_obj_t file;
    uint16_t len;
    uint16_t pos;
    byte buf[24];
} mp_reader_vfs_t;

STATIC mp_uint_t mp_reader_vfs_readbyte(void *data) {
    mp_reader_vfs_t *reader = (mp_reader_vfs_t*)data;
    if (reader->pos >= reader->len) {
        if (reader->len < sizeof(reader->buf)) {
            return MP_READER_EOF;
        } else {
            int errcode;
            reader->len = mp_stream_rw(reader->file, reader->buf, sizeof(reader->buf),
                &errcode, MP_STREAM_RW_READ | MP_STREAM_RW_ONCE);
            if (errcode != 0) {
                // TODO handle errors properly
                return MP_READER_EOF;
            }
            if (reader->len == 0) {
                return MP_READER_EOF;
            }
            reader->pos = 0;
        }
    }
    return reader->buf[reader->pos++];
}

STATIC void mp_reader_vfs_close(void *data) {
    mp_reader_vfs_t *reader = (mp_reader_vfs_t*)data;
    mp_stream_close(reader->file);
    m_del_obj(mp_reader_vfs_t, reader);
}

void mp_reader_new_file(mp_reader_t *reader, const char *filename) {
    mp_reader_vfs_t *rf = m_new_obj(mp_reader_vfs_t);
    mp_obj_t arg = mp_obj_new_str(filename, strlen(filename));
    rf->file = mp_builtin_open(1, &arg, (mp_map_t*)&mp_const_empty_map);
    int errcode;
    rf->len = mp_stream_rw(rf->file, rf->buf, sizeof(rf->buf), &errcode, MP_STREAM_RW_READ | MP_STREAM_RW_ONCE);
    if (errcode != 0) {
        mp_raise_OSError(errcode);
    }
    rf->pos = 0;
    reader->data = rf;
    reader->readbyte = mp_reader_vfs_readbyte;
    reader->close = mp_reader_vfs_close;
}

mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    mp_reader_t reader;
    mp_reader_new_file(&reader, filename);
    return mp_lexer_new(qstr_from_str(filename), reader);
}
/*
mp_import_stat_t mp_import_stat(const char *path) {
    uint8_t flag = 0;
    //char* path0 = (char*)malloc(FS_PATCH_LENGTH);
    //memset(path0,0,FS_PATCH_LENGTH);
    //API_FS_RealPath(path,path0);
    int32_t fd = SPIFFS_open(&fs,path,SPIFFS_O_RDONLY,0);
    if(fd>0)
    {
        SPIFFS_close(&fs,fd);
        return MP_IMPORT_STAT_FILE;
    }
    spiffs_DIR dir;
	if (!SPIFFS_opendir (&fs, path, &dir))
    {
        return MP_IMPORT_STAT_DIR;
    }
    return MP_IMPORT_STAT_NO_EXIST;
}
*/
#endif //#if !MICROPY_VFS
