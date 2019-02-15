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
#include "lcd.h"
#include "font.h"

static lcd_ctl_t lcd_ctl;

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

void lcd_polling_enable(void)
{
    lcd_ctl.mode = 0;
}

void lcd_interrupt_enable(void)
{
    lcd_ctl.mode = 1;
}

void lcd_init(void)
{
    uint8_t data = 0;

    tft_hard_init();
    /*soft reset*/
    tft_write_command(SOFTWARE_RESET);
    usleep(100000);
    /*exit sleep*/
    tft_write_command(SLEEP_OFF);
    usleep(100000);
    /*pixel format*/
    tft_write_command(PIXEL_FORMAT_SET);
    data = 0x55;
    tft_write_byte(&data, 1);
    lcd_set_direction(DIR_YX_RLDU);

    /*display on*/
    tft_write_command(DISPALY_ON);
    lcd_polling_enable();
}

void lcd_set_direction(lcd_dir_t dir)
{
    //dir |= 0x08;  //excahnge RGB
    lcd_ctl.dir = dir;
    if (dir & DIR_XY_MASK)
    {
        lcd_ctl.width = LCD_Y_MAX - 1;
        lcd_ctl.height = LCD_X_MAX - 1;
    }
    else
    {
        lcd_ctl.width = LCD_X_MAX - 1;
        lcd_ctl.height = LCD_Y_MAX - 1;
    }

    tft_write_command(MEMORY_ACCESS_CTL);
    tft_write_byte((uint8_t *)&dir, 1);
}

void lcd_set_area(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint8_t data[4] = {0};

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
    tft_write_byte(&color, 2);
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
    uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;

    lcd_set_area(0, 0, lcd_ctl.width, lcd_ctl.height);
    tft_fill_data(&data, LCD_X_MAX * LCD_Y_MAX / 2);
}

void lcd_fill_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	if((x1 == x2) || (y1 == y2)) return;
	uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;
    lcd_set_area(x1, y1, x2-1, y2-1);
    tft_fill_data(&data, LCD_X_MAX * LCD_Y_MAX / 2);
}

void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t width, uint16_t color)
{
    uint32_t data_buf[640] = {0};
    uint32_t *p = data_buf;
    uint32_t data = color;
    uint32_t index = 0;

    data = (data << 16) | data;
    for (index = 0; index < 160 * width; index++)
        *p++ = data;

    lcd_set_area(x1, y1, x2, y1 + width - 1);
    tft_write_byte(data_buf, ((x2 - x1 + 1) * width + 1)*2 );
    lcd_set_area(x1, y2 - width + 1, x2, y2);
    tft_write_byte(data_buf, ((x2 - x1 + 1) * width + 1)*2 );
    lcd_set_area(x1, y1, x1 + width - 1, y2);
    tft_write_byte(data_buf, ((y2 - y1 + 1) * width + 1)*2 );
    lcd_set_area(x2 - width + 1, y1, x2, y2);
    tft_write_byte(data_buf, ((y2 - y1 + 1) * width + 1)*2 );
}

void lcd_draw_picture(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint32_t *ptr)
{
    lcd_set_area(x1, y1, x1 + width - 1, y1 + height - 1);
    tft_write_byte(ptr, width * height*2);// lcd_ctl.mode ? 2 : 0);
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
		tft_write_byte((uint32_t*)p, rw*2);//, lcd_ctl.mode ? 2 : 0);
	}
	return;
}


//static uint16_t line_buf[320]; //TODO: optimize
void lcd_draw_pic_gray(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint8_t *ptr)
{
	int i;
	uint16_t tmp;
    lcd_set_area(x1, y1, x1 + width - 1, y1 + height - 1);
	for(i=0;i<width*height;i++)
	{
		tft_write_byte(gray2rgb565+(i>>2), 2);
	}
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




