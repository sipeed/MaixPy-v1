#ifndef MYSPIFFS_H
#define MYSPIFFS_H

#include <spiffs.h>
void my_spiffs_mount();
void my_spiffs_init();
int format_fs(void);
extern spiffs fs;
#endif// MYSPIFFS_H
