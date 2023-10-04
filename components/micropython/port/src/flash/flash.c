// Include required definitions first.
#include "py/obj.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "py/stream.h"
#include "string.h"
#include "w25qxx.h"

STATIC mp_obj_t flash_read(mp_obj_t addr, mp_obj_t size)
{
    vstr_t vstr;
    vstr_init_len(&vstr, mp_obj_get_int(size));
    memset(vstr.buf, 0, vstr.len);
    w25qxx_status_t res = w25qxx_read_data_dma(mp_obj_get_int(addr), vstr.buf, vstr.len, W25QXX_QUAD_FAST);
    if (res != W25QXX_OK) {
        mp_raise_ValueError("error reading flash");
    }
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(flash_read_obj, flash_read);

STATIC mp_obj_t flash_write(mp_obj_t addr, mp_obj_t buf)
{
    mp_buffer_info_t src;
    mp_get_buffer_raise(buf, &src, MP_BUFFER_READ);
    w25qxx_status_t res = w25qxx_write_data_dma(mp_obj_get_int(addr), (const uint8_t*)src.buf, src.len);
    if (res != W25QXX_OK) {
        mp_raise_ValueError("error writing to flash");
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(flash_write_obj, flash_write);

STATIC mp_obj_t flash_erase(mp_obj_t addr, mp_obj_t size)
{
    w25qxx_status_t res;
    uint32_t int_size = mp_obj_get_int(size);
    if (int_size == 4096) {
        res = w25qxx_sector_erase_dma(mp_obj_get_int(addr));
    }
    else if (int_size == 65536)
        res = w25qxx_64k_block_erase_dma(mp_obj_get_int(addr));
    while (w25qxx_is_busy_dma() == W25QXX_BUSY)
        ;
    if (res != W25QXX_OK) {
        mp_raise_ValueError("error erasing flash");
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(flash_erase_obj, flash_erase);

/****************************** MODULE ******************************/

STATIC const mp_rom_map_elem_t flash_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_flash) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&flash_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&flash_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_erase), MP_ROM_PTR(&flash_erase_obj) },
};
STATIC MP_DEFINE_CONST_DICT(flash_module_globals, flash_module_globals_table);

// Define module object.
const mp_obj_module_t flash_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&flash_module_globals,
};

// Register the module to make it available in Python
MP_REGISTER_MODULE(MP_QSTR_flash, flash_user_cmodule, MODULE_FLASH_ENABLED);
