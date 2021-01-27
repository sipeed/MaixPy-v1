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

#ifndef __LCD__H__
#define __LCD__H__
#include "spi.h"


#define DCX_GPIONUM             31// FUNC_GPIOHS31
#define RST_GPIONUM             30 // FUNC_GPIOHS30

#define SPI_CHANNEL             SPI_DEVICE_0
#define SPI_DMA_CH				DMAC_CHANNEL2
#define LCD_SPI_SLAVE_SELECT    SPI_CHIP_SELECT_3

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

#define LCD_SWAP_COLOR_BYTES 1
#define SWAP_16(x) ((x>>8&0xff) | (x<<8))  // gbrg -> rgb

#if LCD_SWAP_COLOR_BYTES
static const uint16_t gray2rgb565[64]={
0x0000, 0x0020, 0x0841, 0x0861, 0x1082, 0x10a2, 0x18c3, 0x18e3, 
0x2104, 0x2124, 0x2945, 0x2965, 0x3186, 0x31a6, 0x39c7, 0x39e7, 
0x4208, 0x4228, 0x4a49, 0x4a69, 0x528a, 0x52aa, 0x5acb, 0x5aeb, 
0x630c, 0x632c, 0x6b4d, 0x6b6d, 0x738e, 0x73ae, 0x7bcf, 0x7bef, 
0x8410, 0x8430, 0x8c51, 0x8c71, 0x9492, 0x94b2, 0x9cd3, 0x9cf3, 
0xa514, 0xa534, 0xad55, 0xad75, 0xb596, 0xb5b6, 0xbdd7, 0xbdf7, 
0xc618, 0xc638, 0xce59, 0xce79, 0xd69a, 0xd6ba, 0xdedb, 0xdefb, 
0xe71c, 0xe73c, 0xef5d, 0xef7d, 0xf79e, 0xf7be, 0xffdf, 0xffff,
};
#else
static const uint16_t gray2rgb565[64]={
0x0000, 0x2000, 0x4108, 0x6108, 0x8210, 0xa210, 0xc318, 0xe318, 
0x0421, 0x2421, 0x4529, 0x6529, 0x8631, 0xa631, 0xc739, 0xe739, 
0x0842, 0x2842, 0x494a, 0x694a, 0x8a52, 0xaa52, 0xcb5a, 0xeb5a, 
0x0c63, 0x2c63, 0x4d6b, 0x6d6b, 0x8e73, 0xae73, 0xcf7b, 0xef7b, 
0x1084, 0x3084, 0x518c, 0x718c, 0x9294, 0xb294, 0xd39c, 0xf39c, 
0x14a5, 0x34a5, 0x55ad, 0x75ad, 0x96b5, 0xb6b5, 0xd7bd, 0xf7bd, 
0x18c6, 0x38c6, 0x59ce, 0x79ce, 0x9ad6, 0xbad6, 0xdbde, 0xfbde, 
0x1ce7, 0x3ce7, 0x5def, 0x7def, 0x9ef7, 0xbef7, 0xdfff, 0xffff,
};
#endif

typedef enum 
{
    LCD_TYPE_ST7789,
    LCD_TYPE_ILI9486,
    LCD_TYPE_ILI9481,
	//  rgb 屏， 使用转接板设备时传入的屏幕类型
	LCD_TYPE_5P0_7P0,
	LCD_TYPE_5P0_IPS,
	LCD_TYPE_480_272_4P3,
}lcd_type_t;

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
    DIR_RGB2BRG = 0x08, // lcd_set_direction(DIR_YX_RLUD | DIR_RGB2BRG); // 0x28
} lcd_dir_t;

//屏幕参数结构体
typedef struct
{	//基础参数
	// Hardward Interface config
    uint8_t rst_pin;
    uint8_t dcx_pin;
    uint8_t cs_pin; // cs or nss
    uint8_t bl_pin;
    uint8_t clk_pin;
    uint32_t freq; //频率
    // LCD parmater config
    uint16_t height;
    uint16_t width;
    uint16_t offset_h0;
    uint16_t offset_w0;
    uint16_t offset_h1;
    uint16_t offset_w1;

    lcd_type_t lcd_type;
	bool oct; // 是否使用非标准 spi
    lcd_dir_t dir;
    bool invert; // corlor invert
	//扩展参数
	void* extra_para;
}lcd_para_t;

typedef struct
{
    // 屏幕参数
	lcd_para_t* lcd_para;

	// setting
	int (*init)(lcd_para_t* lcd_para); // 初始化屏幕
	void (*deinit)(void); // 释放屏幕
	void (*set_direction)(lcd_dir_t dir); // 设置屏幕方向
	void (*bgr_to_rgb)(bool enable); // 是否将 bgr 数据转 rgb 后显示
	void (*set_offset)(uint16_t offset_w, uint16_t offset_h); // 设置屏幕偏移量
	void (*set_freq)(uint32_t freq);// 设置 spi 速率, hz 为单位
	uint32_t (*get_freq)(void); // 获取当前速率
	uint16_t (*get_width)(void); // 获取当前屏幕宽度
	uint16_t (*get_height)(void); // 获取当前屏幕高度
	
	// ui
	void (*clear)(uint16_t rgb565_color); // 清屏
	void (*draw_point)(uint16_t x, uint16_t y, uint16_t color); 
	// void (*draw_string)(uint16_t x, uint16_t y, char *str, uint16_t color);
	void (*draw_picture)(uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint8_t *img);
	void (*draw_pic_roi)(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh, uint8_t *img);
	void (*draw_pic_gray)(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, uint8_t *img);
	void (*draw_pic_grayroi)(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh, uint8_t *img);
	void (*fill_rectangle)(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
} lcd_t;

extern lcd_t lcd_mcu;
extern lcd_t lcd_rgb;

// 当前的 lcd 屏，全局唯一
extern lcd_t *lcd;

#endif  //!__LCD__H__