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
#include <string.h>
#include <unistd.h>
#include "stdlib.h"
#include "lcd.h"
#include "font.h"
#include "sleep.h"
#include "global_config.h"

#define SWAP_16(x) ((x>>8&0xff) | (x<<8))

static lcd_ctl_t lcd_ctl;

static uint16_t* g_lcd_display_buff = NULL;
static uint16_t g_lcd_w = 0;
static uint16_t g_lcd_h = 0;
static bool g_lcd_init = false;

#if LCD_SWAP_COLOR_BYTES
static uint16_t gray2rgb565[64]={
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
static uint16_t gray2rgb565[64]={
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

void lcd_polling_enable(void)
{
    lcd_ctl.mode = 0;
}

void lcd_interrupt_enable(void)
{
    lcd_ctl.mode = 1;
}

int lcd_init(uint32_t freq, bool oct, uint16_t offset_w, uint16_t offset_h, uint16_t offset_w1, uint16_t offset_h1, bool invert_color, uint16_t width, uint16_t height)
{
    uint8_t data = 0;
    lcd_ctl.start_offset_w0 = offset_w;
    lcd_ctl.start_offset_h0 = offset_h;
    lcd_ctl.start_offset_w1 = offset_w1;
    lcd_ctl.start_offset_h1 = offset_h1;
    if(g_lcd_w != width || g_lcd_h != height)
    {
        if(g_lcd_display_buff)
        {
            free(g_lcd_display_buff);
        }
        g_lcd_display_buff = (uint16_t*)malloc(width*height*2);
        if(!g_lcd_display_buff)
            return 12; //ENOMEM
        g_lcd_w = width;
        g_lcd_h = height;
    }
    tft_hard_init(freq, oct);
    /*soft reset*/
    tft_write_command(SOFTWARE_RESET);
    msleep(150);
    /*exit sleep*/
    tft_write_command(SLEEP_OFF);
    msleep(500);
    /*pixel format*/
    tft_write_command(PIXEL_FORMAT_SET);
    data = 0x55;
    tft_write_byte(&data, 1);
    msleep(10);
    
    g_lcd_init = true;

    lcd_set_direction(DIR_YX_RLDU);
    if(invert_color)
    {
        tft_write_command(INVERSION_DISPALY_ON);
        msleep(10);
    }
    tft_write_command(NORMAL_DISPALY_ON);
    msleep(10);
    /*display on*/
    tft_write_command(DISPALY_ON);
    msleep(100);
    lcd_polling_enable();
    return 0;
}

void lcd_destroy()
{
    if(g_lcd_display_buff)
    {
        free(g_lcd_display_buff);
        g_lcd_display_buff = NULL;
    }
    g_lcd_w = 0;
    g_lcd_h = 0;
}


uint16_t lcd_get_width()
{
    return g_lcd_w;
}

uint16_t lcd_get_height()
{
    return g_lcd_h;
}


void lcd_set_direction(lcd_dir_t dir)
{
    if(!g_lcd_init)
        return;
    //dir |= 0x08;  //excahnge RGB
    lcd_ctl.dir = dir;
    if (dir & DIR_XY_MASK)
    {
        lcd_ctl.width = g_lcd_w - 1;
        lcd_ctl.height = g_lcd_h - 1;
        lcd_ctl.start_offset_w = lcd_ctl.start_offset_w1;
        lcd_ctl.start_offset_h = lcd_ctl.start_offset_h1;
    }
    else
    {
        lcd_ctl.width = g_lcd_h - 1;
        lcd_ctl.height = g_lcd_w - 1;
        lcd_ctl.start_offset_w = lcd_ctl.start_offset_w0;
        lcd_ctl.start_offset_h = lcd_ctl.start_offset_h0;
    }
    
    tft_write_command(MEMORY_ACCESS_CTL);
    tft_write_byte((uint8_t *)&dir, 1);
}

void lcd_set_offset(uint16_t offset_w, uint16_t offset_h)
{
    lcd_ctl.start_offset_w = offset_w;
    lcd_ctl.start_offset_h = offset_h;
}

static uint32_t lcd_freq = CONFIG_LCD_DEFAULT_FREQ;
void lcd_set_freq(uint32_t freq)
{
    tft_set_clk_freq(freq);
    lcd_freq = freq;
}

uint32_t lcd_get_freq()
{
    return lcd_freq;
}


void lcd_set_area(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint8_t data[4] = {0};

    x1 += lcd_ctl.start_offset_w;
    x2 += lcd_ctl.start_offset_w;
    y1 += lcd_ctl.start_offset_h;
    y2 += lcd_ctl.start_offset_h;

    data[0] = (uint8_t)(x1 >> 8);
    data[1] = (uint8_t)(x1);
    data[2] = (uint8_t)(x2 >> 8);
    data[3] = (uint8_t)(x2);
    tft_write_command(HORIZONTAL_ADDRESS_SET);
    tft_write_byte(data, 4);

    data[0] = (uint8_t)(y1 >> 8);
    data[1] = (uint8_t)(y1);
    data[2] = (uint8_t)(y2 >> 8);
    data[3] = (uint8_t)(y2);
    tft_write_command(VERTICAL_ADDRESS_SET);
    tft_write_byte(data, 4);

    tft_write_command(MEMORY_WRITE);
}

void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_set_area(x, y, x, y);
    tft_write_byte((uint8_t*)&color, 2);
}

void lcd_draw_char(uint16_t x, uint16_t y, char c, uint16_t color)
{
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t data = 0;

    for (i = 0; i < 16; i++)
    {
        data = ascii0816[c * 16 + i];
        for (j = 0; j < 8; j++)
        {
            if (data & 0x80)
                lcd_draw_point(x + j, y, color);
            data <<= 1;
        }
        y++;
    }
}

void lcd_draw_string(uint16_t x, uint16_t y, char *str, uint16_t color)
{
    #if LCD_SWAP_COLOR_BYTES
        color = SWAP_16(color);
    #endif
    while (*str)
    {
        lcd_draw_char(x, y, *str, color);
        str++;
        x += 8;
    }
}

void lcd_ram_draw_string(char *str, uint32_t *ptr, uint16_t font_color, uint16_t bg_color)
{
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t data = 0;
    uint8_t *pdata = NULL;
    uint16_t width = 0;
    uint32_t *pixel = NULL;
#if LCD_SWAP_COLOR_BYTES
    font_color = (font_color<<8) | (font_color>>8&0x00ff);
    bg_color   = (bg_color<<8)   | (bg_color>>8&0x00ff);
#endif
    width = 4 * strlen(str);
    while (*str)
    {
        pdata = (uint8_t *)&ascii0816[(*str) * 16];
        for (i = 0; i < 16; i++)
        {
            data = *pdata++;
            pixel = ptr + i * width;
            for (j = 0; j < 4; j++)
            {
                switch (data >> 6)
                {	
                    case 0:
                        *pixel =  bg_color | ((uint32_t)bg_color << 16);
                        break;
                    case 2:
                        *pixel = font_color | ((uint32_t)bg_color << 16) ;
                        break;
                    case 1:
                        *pixel = bg_color | ((uint32_t)font_color << 16) ;
                        break;
                    case 3:
                        *pixel = font_color | ((uint32_t)font_color << 16) ;
                        break;
                    default:
                        *pixel = 0;
                        break;
                }
                data <<= 2;
                pixel++;
            }
        }
        str++;
        ptr += 4;
    }
}

void lcd_clear(uint16_t color)
{
    #if LCD_SWAP_COLOR_BYTES
        color = SWAP_16(color);
    #endif
    uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;
    lcd_set_area(0, 0, lcd_ctl.width, lcd_ctl.height);
    tft_fill_data(&data, g_lcd_h * g_lcd_w / 2);
}

void lcd_fill_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	if((x1 == x2) || (y1 == y2))
        return;
    #if LCD_SWAP_COLOR_BYTES
        color = SWAP_16(color);
    #endif
	uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;
    lcd_set_area(x1, y1, x2-1, y2-1);
    tft_fill_data(&data, (x2 - x1) * (y2 - y1) / 2);
}

void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t width, uint16_t color)
{
    uint32_t data_buf[640] = {0};
    uint32_t *p = data_buf;
    uint32_t data;
    uint32_t index = 0;

    #if LCD_SWAP_COLOR_BYTES
        color = SWAP_16(color);
    #endif

    data = ((uint32_t)color << 16) | color;
    for (index = 0; index < 160 * width; index++)
        *p++ = data;

    lcd_set_area(x1, y1, x2, y1 + width - 1);
    tft_write_byte((uint8_t*)data_buf, ((x2 - x1 + 1) * width + 1)*2 );
    lcd_set_area(x1, y2 - width + 1, x2, y2);
    tft_write_byte((uint8_t*)data_buf, ((x2 - x1 + 1) * width + 1)*2 );
    lcd_set_area(x1, y1, x1 + width - 1, y2);
    tft_write_byte((uint8_t*)data_buf, ((y2 - y1 + 1) * width + 1)*2 );
    lcd_set_area(x2 - width + 1, y1, x2, y2);
    tft_write_byte((uint8_t*)data_buf, ((y2 - y1 + 1) * width + 1)*2 );
}


typedef int (*dual_func_t)(int);
extern volatile dual_func_t dual_func;
static uint16_t* g_pixs_draw_pic = NULL;
static uint32_t g_pixs_draw_pic_size = 0;
static uint32_t g_pixs_draw_pic_half_size = 0;

static int swap_pixs_half(int core)
{
    uint32_t i;
    uint16_t* p = g_pixs_draw_pic;
    for(i=g_pixs_draw_pic_half_size; i<g_pixs_draw_pic_size ; i+=2)
    {
        #if LCD_SWAP_COLOR_BYTES
            g_lcd_display_buff[i] = SWAP_16(*(p+1));
            g_lcd_display_buff[i+1] = SWAP_16(*(p));
        #else
            g_lcd_display_buff[i] = *(p+1);
            g_lcd_display_buff[i+1] = *p;
        #endif
        p+=2;
    }
    return 0;
}

void lcd_draw_picture(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint32_t *ptr)
{
    uint32_t i;
    uint16_t* p = (uint16_t*)ptr;
    bool odd = false;

    lcd_set_area(x1, y1, x1 + width - 1, y1 + height - 1);
    g_pixs_draw_pic_size = width*height;
    // printk("\t%d %d %d %d, %d %d %d\r\n", x1, y1, x1 + width - 1, y1 + height - 1, width, height, g_pixs_draw_pic_size);
    /*
    for(i=0; i<g_pixs_draw_pic_size; ++i )
    {
        g_lcd_display_buff[i] = SWAP_16(*p);
        ++p;
    }
    tft_write_half(g_lcd_display_buff, g_pixs_draw_pic_size);
    */
    if(g_pixs_draw_pic_size % 2)
    {
        odd = true;
        g_pixs_draw_pic_size -= 1;
    }
    if( g_pixs_draw_pic_size > 0)
    {
        g_pixs_draw_pic_half_size = g_pixs_draw_pic_size/2;
        g_pixs_draw_pic_half_size = (g_pixs_draw_pic_half_size%2) ? (g_pixs_draw_pic_half_size+1) : g_pixs_draw_pic_half_size;
        g_pixs_draw_pic = p+g_pixs_draw_pic_half_size;
        dual_func = swap_pixs_half;
        for(i=0; i< g_pixs_draw_pic_half_size; i+=2)
        {
            #if LCD_SWAP_COLOR_BYTES
                g_lcd_display_buff[i] = SWAP_16(*(p+1));
                g_lcd_display_buff[i+1] = SWAP_16(*(p));
            #else
                g_lcd_display_buff[i] = *(p+1);
                g_lcd_display_buff[i+1] = *p;
            #endif
            p+=2;
        }
        while(dual_func){}
        tft_write_word((uint32_t*)g_lcd_display_buff, g_pixs_draw_pic_size / 2);
    }
    if( odd )
    {
        #if LCD_SWAP_COLOR_BYTES
            g_lcd_display_buff[0] = SWAP_16( ((uint16_t*)ptr)[g_pixs_draw_pic_size]);
        #else
            g_lcd_display_buff[0] = ((uint16_t*)ptr)[g_pixs_draw_pic_size];
        #endif
        lcd_set_area(x1 + width - 1, y1 + height - 1, x1 + width - 1, y1 + height - 1);
        tft_write_half(g_lcd_display_buff, 1);
    }
}

//draw pic's roi on (x,y)
//x,y of LCD, w,h is pic; rx,ry,rw,rh is roi
void lcd_draw_pic_roi(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh, uint32_t *ptr)
{	
	int y_oft;
	uint8_t* p;
	for(y_oft = 0; y_oft < rh; y_oft++)
	{	//draw line by line
		p = (uint8_t *)(ptr) + w*2*(y_oft+ry) + 2*rx;
		lcd_set_area(x, y+y_oft, x + rw - 1, y+y_oft);
		tft_write_byte((uint8_t*)p, rw*2);//, lcd_ctl.mode ? 2 : 0);
	}
}


void lcd_draw_pic_gray(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint8_t *ptr)
{	
    uint32_t i;
    lcd_set_area(x1, y1, x1 + width - 1, y1 + height - 1);
    uint32_t size = width*height;
    for(i=0; i< size; i+=2)
    {
        g_lcd_display_buff[i] = gray2rgb565[ptr[i+1]>>2];
        g_lcd_display_buff[i+1] = gray2rgb565[ptr[i]>>2];
    }
    tft_write_word((uint32_t*)g_lcd_display_buff, width * height / 2);
}

void lcd_draw_pic_grayroi(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh, uint8_t *ptr)
{
	int y_oft;
	uint8_t* p;
	for(y_oft = 0; y_oft < rh; y_oft++)
	{	//draw line by line
		p = (uint8_t *)(ptr) + w*(y_oft+ry) + rx;
		lcd_draw_pic_gray(x, y+y_oft, rw, 1, p);
	}
	return;
}

void lcd_ram_cpyimg(char* lcd, int lcdw, char* img, int imgw, int imgh, int x, int y)
{
	int i;
	for(i=0;i<imgh;i++)
	{
		memcpy(lcd+lcdw*2*(y+i)+x*2,img+imgw*2*i,imgw*2);
	}
	return;
}




