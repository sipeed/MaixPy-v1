#ifndef MYSPIFFS_H
#define MYSPIFFS_H

#include <spiffs.h>
s32_t flash_read(uint32_t addr, uint32_t size, uint8_t *buf);
s32_t flash_write(uint32_t addr, uint32_t size, uint8_t *buf);
s32_t flash_erase(uint32_t addr, uint32_t size);
#endif// MYSPIFFS_H
