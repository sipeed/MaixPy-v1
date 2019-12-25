/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _LCD_H_
#define _LCD_H_

#include <stdint.h>
#include "st7789.h"

#define LCD_SWAP_COLOR_BYTES 1

/* clang-format off */
#define BLACK       0x0000
#define NAVY        0x0F00
#define DARKGREEN   0xE003
#define DARKCYAN    0xEF03
#define MAROON      0x0078
#define PURPLE      0x0F78
#define OLIVE       0xE07B
#define LIGHTGREY   0x18C6
#define DARKGREY    0xEF7B
#define BLUE        0x1F00
#define GREEN       0xE007
#define CYAN        0xFF07
#define RED         0x00F8
#define MAGENTA     0x1FF8
#define YELLOW      0xE0FF
#define WHITE       0xFFFF
#define ORANGE      0x20FD
#define GREENYELLOW 0xE5AF
#define PINK        0x1FF8
#define USER_COLOR  0x55AA
/* clang-format on */

typedef enum _lcd_dir
{
    DIR_XY_RLUD = 0x00,
    DIR_YX_RLUD = 0x20,
    DIR_XY_LRUD = 0x40,
    DIR_YX_LRUD = 0x60,
    DIR_XY_RLDU = 0x80,
    DIR_YX_RLDU = 0xA0,
    DIR_XY_LRDU = 0xC0,
    DIR_YX_LRDU = 0xE0,
    DIR_XY_MASK = 0x20,
    DIR_MASK = 0xE0,
} lcd_dir_t;

typedef struct _lcd_ctl
{
    uint8_t mode;
    uint8_t dir;
    uint16_t width;
    uint16_t height;
    uint16_t start_offset_w0;
    uint16_t start_offset_h0;
    uint16_t start_offset_w1;
    uint16_t start_offset_h1;
    uint16_t start_offset_w;
    uint16_t start_offset_h;
} lcd_ctl_t;

void lcd_polling_enable(void);
void lcd_interrupt_enable(void);
int lcd_init(uint32_t freq, bool oct, uint16_t offset_w, uint16_t offset_h, uint16_t offset_w1, uint16_t offset_h1, bool invert_color, uint16_t width, uint16_t height);
void lcd_destroy();
void lcd_clear(uint16_t color);
void lcd_set_freq(uint32_t freq);
uint32_t lcd_get_freq();
void lcd_set_direction(lcd_dir_t dir);
void lcd_set_area(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color);
void lcd_draw_string(uint16_t x, uint16_t y, char *str, uint16_t color);
void lcd_draw_picture(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint32_t *ptr);
void lcd_draw_pic_roi(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh, uint32_t *ptr);
void lcd_draw_pic_gray(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint8_t *ptr);
void lcd_draw_pic_grayroi(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh, uint8_t *ptr);
void lcd_fill_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t width, uint16_t color);
void lcd_ram_draw_string(char *str, uint32_t *ptr, uint16_t font_color, uint16_t bg_color);
void lcd_ram_cpyimg(char* lcd, int lcdw, char* img, int imgw, int imgh, int x, int y);
void lcd_set_offset(uint16_t offset_w, uint16_t offset_h);

uint16_t lcd_get_width();
uint16_t lcd_get_height();
#endif

