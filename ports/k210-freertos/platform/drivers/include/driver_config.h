
#ifndef __DRIVER_CONFIG_H
#define __DRIVER_CONFIG_H

////////// LCD ////////////
#ifdef MAIXPY_M5STICK
    #define LCD_W_MAX 240
    #define LCD_H_MAX 135
#else
    #define LCD_W_MAX 320
    #define LCD_H_MAX 240
#endif

#define LCD_W     LCD_W_MAX
#define LCD_H     LCD_H_MAX
///////////////////////////


#endif

