
#ifndef __DRIVER_CONFIG_H
#define __DRIVER_CONFIG_H

#include "global_config.h"

////////// LCD ////////////
#ifdef CONFIG_BOARD_M5STICK
    #define LCD_W_MAX 320
    #define LCD_H_MAX 240
    #define LCD_W     240
    #define LCD_H     135
#else
    #define LCD_W_MAX 320
    #define LCD_H_MAX 240
    #define LCD_W     LCD_W_MAX
    #define LCD_H     LCD_H_MAX
#endif


///////////////////////////


#endif

