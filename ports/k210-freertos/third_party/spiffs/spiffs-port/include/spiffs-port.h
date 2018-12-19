#ifndef MYSPIFFS_H
#define MYSPIFFS_H

#include <spiffs.h>
s32_t flash_read(int addr, int size, char *buf);
s32_t flash_write(int addr, int size, char *buf);
s32_t flash_erase(int addr, int size);
#endif// MYSPIFFS_H
