

=============================

Files:

* maixpy_*.bin: firmware
* elf_*.7z: elf files, just used for debug

You can customize your firmware at https://www.maixhub.com/compile.html
Or build by yourself according to  https://github.com/sipeed/MaixPy/blob/master/build.md

=============================

maixpy_*.bin: normal firmware, including
    * basic api
    * kmodel V4 support
    * no LVGL support
    * NES emulator support
    * AVI format video support
    * IDE support
    * more detail see: https://github.com/sipeed/MaixPy/blob/master/projects/maixpy_k210/config_defaults.mk

maixpy_*_minimum.bin: minimum function firmware, including
    * basic api
    * only few OpenMV's API, no some API like find_lines
    * only kmodel V3 support
    * no LVGL support
    * no NES emulator support
    * no AVI format video support
    * no IDE support
    * more detail see: https://github.com/sipeed/MaixPy/blob/master/projects/maixpy_k210_minimum/config_defaults.mk

maixpy_*_minimum_with_kmodel_v4_support: minimum function firmware, including
    * the same as minimum.bin
    * add kmodel v4 support

maixpy_*_openmv_kmodel_v4_with_ide_support: minimum function firmware, including
    * the same as normal
    * add kmodel v4 support
    * IDE support

maixpy_*_minimum_with_ide_support.bin: minimum function firmware, including
    * same as minimum
    * IDE support
    * more detail see: https://github.com/sipeed/MaixPy/blob/master/projects/maixpy_k210_minimum/config_with_ide_support.mk

maixpy_*_with_lvgl.bin: add lvgl support, including
    * basic api
    * only kmodel V3 support
    * LVGL support
    * NES emulator support
    * AVI format video support
    * IDE support
    * more detail see: https://github.com/sipeed/MaixPy/blob/master/projects/maixpy_k210/config_with_lvgl.mk

maixpy_*_m5stickv.bin: especially for M5StickV board, including
    * same as normal
    * more detail see: https://github.com/sipeed/MaixPy/blob/master/projects/maixpy_m5stickv/config_defaults.mk

maixpy_*_amigo*.bin: especially for board amigo
    * the same as uppper


