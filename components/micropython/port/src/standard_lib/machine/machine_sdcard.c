#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "lib/oofatfs/ff.h"
#include "extmod/vfs_fat.h"
#include "sdcard.h"
#include "machine_sdcard.h"

#define BP_IOCTL_INIT MP_BLOCKDEV_IOCTL_INIT          
#define BP_IOCTL_DEINIT MP_BLOCKDEV_IOCTL_DEINIT        
#define BP_IOCTL_SYNC MP_BLOCKDEV_IOCTL_SYNC          
#define BP_IOCTL_BLOCK_COUNT MP_BLOCKDEV_IOCTL_BLOCK_COUNT   
#define BP_IOCTL_BLOCK_SIZE MP_BLOCKDEV_IOCTL_BLOCK_SIZE    
#define BP_IOCTL_ERASE MP_BLOCKDEV_IOCTL_BLOCK_ERASE   
#define BP_IOCTL_SEC_COUNT MP_BLOCKDEV_IOCTL_SEC_COUNT
#define BP_IOCTL_SEC_SIZE MP_BLOCKDEV_IOCTL_SEC_SIZE 

uint64_t sdcard_get_capacity_in_bytes(void) {
    return cardinfo.CardCapacity;
}

bool sdcard_is_present(void) {
    return cardinfo.active;
}

bool sdcard_power_on(void) {
    if (!sdcard_is_present()) {
        return false;
    }
	if(0xff == sd_init())
		 return false;
	cardinfo.active = 1;
    return true;
}

void sdcard_power_off(void) {
    cardinfo.active = 0;
}

STATIC mp_obj_t machine_sdcard_write_sector(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_READ);
    mp_uint_t ret = sd_write_sector_dma(bufinfo.buf,mp_obj_get_int(block_num),bufinfo.len / SDCARD_BLOCK_SIZE);
    return mp_obj_new_bool(ret == 0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(machine_sdcard_write_sector_obj, machine_sdcard_write_sector);

/*
STATIC mp_obj_t machine_sdcard_ioctl(mp_obj_t self, mp_obj_t cmd_in, mp_obj_t arg_in) {
    mp_int_t cmd = mp_obj_get_int(cmd_in);
    switch (cmd) {
        case BP_IOCTL_INIT:
            if (!sdcard_power_on()) {
                return MP_OBJ_NEW_SMALL_INT(-1); // error
            }
            return MP_OBJ_NEW_SMALL_INT(0); // success

        case BP_IOCTL_DEINIT:
            sdcard_power_off();
            return MP_OBJ_NEW_SMALL_INT(0); // success

        case BP_IOCTL_SYNC:
            // nothing to do
            return MP_OBJ_NEW_SMALL_INT(0); // success

        case BP_IOCTL_SEC_COUNT:
            return MP_OBJ_NEW_SMALL_INT(sdcard_get_capacity_in_bytes() / SDCARD_BLOCK_SIZE);

        case BP_IOCTL_SEC_SIZE:
            return MP_OBJ_NEW_SMALL_INT(SDCARD_BLOCK_SIZE);

        default: // unknown command
            return MP_OBJ_NEW_SMALL_INT(-1); // error
    }
}*/
STATIC mp_obj_t machine_sdcard_ioctl(mp_obj_t self, mp_obj_t cmd_in, mp_obj_t arg_in) {
    mp_int_t cmd = mp_obj_get_int(cmd_in);
    switch (cmd) {
        case MP_BLOCKDEV_IOCTL_INIT:
            if (!sdcard_power_on()) {
                return MP_OBJ_NEW_SMALL_INT(-1); // error
            }
            return MP_OBJ_NEW_SMALL_INT(0); // success

        case MP_BLOCKDEV_IOCTL_DEINIT:
            sdcard_power_off();
            return MP_OBJ_NEW_SMALL_INT(0); // success

        case MP_BLOCKDEV_IOCTL_SYNC:
            // nothing to do
            return MP_OBJ_NEW_SMALL_INT(0); // success

        case MP_BLOCKDEV_IOCTL_BLOCK_COUNT:
            return MP_OBJ_NEW_SMALL_INT(sdcard_get_capacity_in_bytes() / SDCARD_BLOCK_SIZE);

        case MP_BLOCKDEV_IOCTL_BLOCK_SIZE:
            return MP_OBJ_NEW_SMALL_INT(SDCARD_BLOCK_SIZE);

        default: // unknown command
            return MP_OBJ_NEW_SMALL_INT(-1); // error
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_3(machine_sdcard_ioctl_obj, machine_sdcard_ioctl);

const mp_obj_base_t machine_sdcard_obj = {&machine_sdcard_type};

STATIC const mp_rom_map_elem_t machine_sdcard_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_ioctl), MP_ROM_PTR(&machine_sdcard_ioctl_obj) },
};

STATIC MP_DEFINE_CONST_DICT(machine_sdcard_locals_dict, machine_sdcard_locals_dict_table);

const mp_obj_type_t machine_sdcard_type = {
    { &mp_type_type },
    .name = MP_QSTR_SDCard,
    .make_new = NULL,
    .locals_dict = (mp_obj_dict_t*)&machine_sdcard_locals_dict,
};

void sdcard_init_vfs(fs_user_mount_t *vfs, int part) {
    vfs->base.type = &mp_fat_vfs_type;
    vfs->blockdev.flags |= MP_BLOCKDEV_FLAG_NATIVE | MP_BLOCKDEV_FLAG_HAVE_IOCTL;
    vfs->fatfs.drv = vfs;
    vfs->fatfs.part = part;
    vfs->blockdev.readblocks[0] = NULL;
    vfs->blockdev.readblocks[1] = NULL;
    vfs->blockdev.readblocks[2] = MP_OBJ_FROM_PTR(sd_read_sector_dma); // native version
    vfs->blockdev.writeblocks[0] = MP_OBJ_FROM_PTR(&machine_sdcard_write_sector_obj);
    vfs->blockdev.writeblocks[1] = NULL;
    vfs->blockdev.writeblocks[2] = MP_OBJ_FROM_PTR(sd_write_sector_dma); // native version
	vfs->blockdev.u.ioctl[0] = MP_OBJ_FROM_PTR(&machine_sdcard_ioctl_obj);
    vfs->blockdev.u.ioctl[1] = MP_OBJ_FROM_PTR(&machine_sdcard_obj);
	//vfs->readblocks[0] = MP_OBJ_FROM_PTR(&pyb_sdcard_readblocks_obj);
    //vfs->readblocks[1] = MP_OBJ_FROM_PTR(&pyb_sdcard_obj);
    //vfs->writeblocks[0] = MP_OBJ_FROM_PTR(&pyb_sdcard_writeblocks_obj);
    //vfs->writeblocks[1] = MP_OBJ_FROM_PTR(&pyb_sdcard_obj);

}

