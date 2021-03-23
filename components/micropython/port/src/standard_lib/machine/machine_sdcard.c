#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "lib/oofatfs/ff.h"
#include "extmod/vfs_fat.h"
#include "sdcard.h"
#include "machine_sdcard.h"
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

extern void mount_sdcard(void);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_sdcard_remount_obj, mount_sdcard);

STATIC mp_obj_t machine_sdcard_write_sector(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_READ);
    mp_uint_t ret = sd_write_sector_dma(bufinfo.buf,mp_obj_get_int(block_num),bufinfo.len / SDCARD_BLOCK_SIZE);
    return mp_obj_new_bool(ret == 0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(machine_sdcard_write_sector_obj, machine_sdcard_write_sector);


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
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(machine_sdcard_ioctl_obj, machine_sdcard_ioctl);

const mp_obj_base_t machine_sdcard_obj = {&machine_sdcard_type};

STATIC const mp_rom_map_elem_t machine_sdcard_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_ioctl), MP_ROM_PTR(&machine_sdcard_ioctl_obj) },
    { MP_ROM_QSTR(MP_QSTR_remount), MP_ROM_PTR(&machine_sdcard_remount_obj) }
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
    vfs->flags |= FSUSER_NATIVE | FSUSER_HAVE_IOCTL;
    vfs->fatfs.drv = vfs;
    vfs->fatfs.part = part;
    vfs->readblocks[0] = NULL;
    vfs->readblocks[1] = NULL;
    vfs->readblocks[2] = MP_OBJ_FROM_PTR(sd_read_sector_dma); // native version
    vfs->writeblocks[0] = MP_OBJ_FROM_PTR(&machine_sdcard_write_sector_obj);
    vfs->writeblocks[1] = NULL;
    vfs->writeblocks[2] = MP_OBJ_FROM_PTR(sd_write_sector_dma); // native version
	vfs->u.ioctl[0] = MP_OBJ_FROM_PTR(&machine_sdcard_ioctl_obj);
    vfs->u.ioctl[1] = MP_OBJ_FROM_PTR(&machine_sdcard_obj);
	//vfs->readblocks[0] = MP_OBJ_FROM_PTR(&pyb_sdcard_readblocks_obj);
    //vfs->readblocks[1] = MP_OBJ_FROM_PTR(&pyb_sdcard_obj);
    //vfs->writeblocks[0] = MP_OBJ_FROM_PTR(&pyb_sdcard_writeblocks_obj);
    //vfs->writeblocks[1] = MP_OBJ_FROM_PTR(&pyb_sdcard_obj);

}

