#ifndef MICROPY_MACHINE_k210_SDCARD_H
#define MICROPY_MACHINE_k210_SDCARD_H


//extern const struct _mp_obj_type_t machine_sdcard_type;
//extern const struct _mp_obj_base_t machine_sdcard_obj;

void sdcard_init_vfs(struct _fs_user_mount_t *vfs, int part);
#define SDCARD_BLOCK_SIZE (512)

extern const struct _mp_obj_type_t machine_sdcard_type;

#endif // MICROPY_INCLUDED_STM32_SDCARD_H

