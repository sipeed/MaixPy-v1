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
#include "sleep.h"
#include "global_config.h"

#include "lcd.h"
#include "st7789.h"

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
} mcu_lcd_ctl_t;


static uint16_t* g_lcd_display_buff = NULL;
static uint16_t g_lcd_w = 0;
static uint16_t g_lcd_h = 0;
static bool g_lcd_init = false;
static mcu_lcd_ctl_t lcd_ctl;

static void mcu_lcd_clear(uint16_t color);
static void mcu_lcd_set_direction(lcd_dir_t dir);

static void lcd_polling_enable(void)
{
    lcd_ctl.mode = 0;
}

static void lcd_interrupt_enable(void)
{
    lcd_ctl.mode = 1;
}

typedef void (*lcd_preinit_handler_t)(void);

lcd_preinit_handler_t lcd_preinit_handler = NULL;

/**
 * Register Pre-initialization handler for lcd
 */
static void lcd_preinit_register_handler(lcd_preinit_handler_t handler)
{
    lcd_preinit_handler = handler;
}

#include "printf.h"

static void lcd_init_sequence_for_ili9481(void)
{
    // printk("lcd_init_sequence_for_ili9481\r\n");

    uint8_t t[24];
    // lcd.clear((99, 99, 99))
    mcu_lcd_clear(0xe28d);

    // lcd.register(0xd0, [0x07,0x42,0x1b])
    tft_write_command(0xD0);
    t[0] = (0x07);
    t[1] = (0x42);
    t[2] = (0x1b);
    tft_write_byte(t, 3);

    // lcd.register(0xd1, [0x00,0x05,0x0c])
    tft_write_command(0xD1); /* Unk */
    t[0] = (0x00);
    t[1] = (0x05);
    t[2] = (0x0c);
    tft_write_byte(t, 3);


}

static void lcd_init_sequence_for_ili9486(void)
{
    uint8_t t[15];
    tft_write_command(0XF1); /* Unk */
    t[0] = (0x36);
    t[1] = (0x04);
    t[2] = (0x00);
    t[3] = (0x3C);
    t[4] = (0X0F);
    t[5] = (0x8F);
    tft_write_byte(t, 6);

    tft_write_command(0XF2); /* Unk */
    t[0] = (0x18);
    t[1] = (0xA3);
    t[2] = (0x12);
    t[3] = (0x02);
    t[4] = (0XB2);
    t[5] = (0x12);
    t[6] = (0xFF);
    t[7] = (0x10);
    t[8] = (0x00);
    tft_write_byte(t, 9);

    tft_write_command(0XF8); /* Unk */
    t[0] = (0x21);
    t[1] = (0x04);
    tft_write_byte(t, 2);

    tft_write_command(0XF9); /* Unk */
    t[0] = (0x00);
    t[1] = (0x08);
    tft_write_byte(t, 2);

    tft_write_command(0x36); /* Memory Access Control */
    t[0] = (0x28);
    tft_write_byte(t, 1);

    tft_write_command(0xB4); /* Display Inversion Control */
    t[0] = (0x00);
    tft_write_byte(t, 1);

    // tft_write_command(0xB6); /* Display Function Control */
    // t[0] = (0x02);
    // // t[1] = (0x22);
    // tft_write_byte(t, 1);

    tft_write_command(0xC1); /* Power Control 2 */
    t[0] = (0x41);
    tft_write_byte(t, 1);
    
    tft_write_command(0xC5); /* Vcom Control */
    t[0] = (0x00);
    t[1] = (0x18);
    tft_write_byte(t, 2);

    tft_write_command(0xE0); /* Positive Gamma Control */
    t[0] = (0x0F);
    t[1] = (0x1F);
    t[2] = (0x1C);
    t[3] = (0x0C);
    t[4] = (0x0F);
    t[5] = (0x08);
    t[6] = (0x48);
    t[7] = (0x98);
    t[8] = (0x37);
    t[9] = (0x0A);
    t[10] = (0x13);
    t[11] = (0x04);
    t[12] = (0x11);
    t[13] = (0x0D);
    t[14] = (0x00);
    tft_write_byte(t, 15);

    tft_write_command(0xE1); /* Negative Gamma Control */
    t[0] = (0x0F);
    t[1] = (0x32);
    t[2] = (0x2E);
    t[3] = (0x0B);
    t[4] = (0x0D);
    t[5] = (0x05);
    t[6] = (0x47);
    t[7] = (0x75);
    t[8] = (0x37);
    t[9] = (0x06);
    t[10] = (0x10);
    t[11] = (0x03);
    t[12] = (0x24);
    t[13] = (0x20);
    t[14] = (0x00);
    tft_write_byte(t, 15);

    tft_write_command(0x3A); /* Interface Pixel Format */
    t[0] = (0x55);
    tft_write_byte(t, 1);

}

#include "syslog.h"

static int mcu_lcd_init(lcd_para_t *lcd_para)
{
    uint8_t data = 0;
    lcd_ctl.dir = lcd_para->dir;
    lcd_ctl.width = lcd_para->width, lcd_ctl.height = lcd_para->height;
    lcd_ctl.start_offset_w0 = lcd_para->offset_w0;
    lcd_ctl.start_offset_h0 = lcd_para->offset_h0;
    lcd_ctl.start_offset_w1 = lcd_para->offset_w1;
    lcd_ctl.start_offset_h1 = lcd_para->offset_h1;
    // printk("w: %d, h: %d, freq: %d, invert: %d, %d, %d, %d, %d, %d\r\n", lcd_para->width, 
    // lcd_para->height, lcd_para->freq, lcd_para->invert, lcd_para->offset_w0, lcd_para->offset_w1,
    // lcd_para->offset_h1, lcd_para->offset_h0, lcd_para->oct);
    if(g_lcd_w != lcd_para->width || g_lcd_h != lcd_para->height)
    {
        if(g_lcd_display_buff)
        {
            free(g_lcd_display_buff);
        }
        g_lcd_display_buff = (uint16_t*)malloc(lcd_para->width*lcd_para->height*2);
        if(!g_lcd_display_buff)
            return 12; //ENOMEM
        g_lcd_w = lcd_para->width;
        g_lcd_h = lcd_para->height;
    }
    tft_hard_init(lcd_para->freq, lcd_para->oct);
    /*soft reset*/
    tft_write_command(SOFTWARE_RESET);
    msleep(50);
    if (lcd_preinit_handler != NULL)
    {
        lcd_preinit_handler();
    }

    /*exit sleep*/
    tft_write_command(SLEEP_OFF);
    msleep(120);
    /*pixel format*/
    tft_write_command(PIXEL_FORMAT_SET);
    data = 0x55;
    tft_write_byte(&data, 1);
    msleep(10);
    
    g_lcd_init = true;

    mcu_lcd_set_direction(lcd_ctl.dir);
    if(lcd_para->invert)
    {
        tft_write_command(INVERSION_DISPALY_ON);
        msleep(10);
    }
    tft_write_command(NORMAL_DISPALY_ON);
    msleep(10);
    /*display on*/
    tft_write_command(DISPALY_ON);
    // msleep(100);
    lcd_polling_enable();
    return 0;
}

static int mcu_lcd_init_shield(lcd_para_t *lcd_para){
    if (lcd_para->lcd_type == LCD_TYPE_ILI9486)
    {
        lcd_preinit_register_handler(&lcd_init_sequence_for_ili9486);
    }
    mcu_lcd_init(lcd_para);
    if (lcd_para->lcd_type == LCD_TYPE_ILI9481)
    {
        lcd_preinit_register_handler(&lcd_init_sequence_for_ili9481);
    }
    if (0 != lcd_para->dir)
    {
        mcu_lcd_set_direction(lcd_para->dir);
    }
    return 0;
}

static void mcu_lcd_destroy()
{
    if(g_lcd_display_buff)
    {
        free(g_lcd_display_buff);
        g_lcd_display_buff = NULL;
    }
    g_lcd_w = 0;
    g_lcd_h = 0;
}


static uint16_t mcu_lcd_get_width()
{
    return g_lcd_w;
}

static uint16_t mcu_lcd_get_height()
{
    return g_lcd_h;
}

static void mcu_lcd_bgr_to_rgb(bool enable){
    lcd_ctl.dir = enable ? (lcd_ctl.dir | DIR_RGB2BRG) : (lcd_ctl.dir & DIR_MASK);
    mcu_lcd_set_direction(lcd_ctl.dir);
}

static void mcu_lcd_set_direction(lcd_dir_t dir)
{
    if(!g_lcd_init)
        return;
    //dir |= 0x08;  //excahnge RGB
    lcd_ctl.dir = ((lcd_ctl.dir & DIR_RGB2BRG) == DIR_RGB2BRG) ? (dir | DIR_RGB2BRG) : dir;

    if (lcd_ctl.dir & DIR_XY_MASK)
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
    tft_write_byte((uint8_t *)&lcd_ctl.dir, 1);
}

static void mcu_lcd_set_offset(uint16_t offset_w, uint16_t offset_h)
{
    lcd_ctl.start_offset_w = offset_w;
    lcd_ctl.start_offset_h = offset_h;
}

static uint32_t lcd_freq = CONFIG_LCD_DEFAULT_FREQ;
static void mcu_lcd_set_freq(uint32_t freq)
{
    tft_set_clk_freq(freq);
    lcd_freq = freq;
}

uint32_t mcu_lcd_get_freq()
{
    return lcd_freq;
}


static void lcd_set_area(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
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

static void mcu_lcd_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_set_area(x, y, x, y);
    tft_write_byte((uint8_t*)&color, 2);
}

static void mcu_lcd_draw_char(uint16_t x, uint16_t y, char c, uint16_t color)
{
    // uint8_t i = 0;
    // uint8_t j = 0;
    // uint8_t data = 0;

    // for (i = 0; i < 16; i++)
    // {
    //     data = ascii0816[c * 16 + i];
    //     for (j = 0; j < 8; j++)
    //     {
    //         if (data & 0x80)
    //             mcu_lcd_draw_point(x + j, y, color);
    //         data <<= 1;
    //     }
    //     y++;
    // }
}

static void mcu_lcd_draw_string(uint16_t x, uint16_t y, char *str, uint16_t color)
{
    #if LCD_SWAP_COLOR_BYTES
        color = SWAP_16(color);
    #endif
    while (*str)
    {
        mcu_lcd_draw_char(x, y, *str, color);
        str++;
        x += 8;
    }
}

static void lcd_ram_draw_string(char *str, uint32_t *ptr, uint16_t font_color, uint16_t bg_color)
{
    // uint8_t i = 0;
    // uint8_t j = 0;
    // uint8_t data = 0;
    // uint8_t *pdata = NULL;
    // uint16_t width = 0;
    // uint32_t *pixel = NULL;
    // width = 4 * strlen(str);
    // while (*str)
    // {
    //     pdata = (uint8_t *)&ascii0816[(*str) * 16];
    //     for (i = 0; i < 16; i++)
    //     {
    //         data = *pdata++;
    //         pixel = ptr + i * width;
    //         for (j = 0; j < 4; j++)
    //         {
    //             switch (data >> 6)
    //             {	
    //                 case 0:
    //                     *pixel =  bg_color | ((uint32_t)bg_color << 16);
    //                     break;
    //                 case 2:
    //                     *pixel = font_color | ((uint32_t)bg_color << 16) ;
    //                     break;
    //                 case 1:
    //                     *pixel = bg_color | ((uint32_t)font_color << 16) ;
    //                     break;
    //                 case 3:
    //                     *pixel = font_color | ((uint32_t)font_color << 16) ;
    //                     break;
    //                 default:
    //                     *pixel = 0;
    //                     break;
    //             }
    //             data <<= 2;
    //             pixel++;
    //         }
    //     }
    //     str++;
    //     ptr += 4;
    // }
}

static void mcu_lcd_clear(uint16_t color)
{
    #if LCD_SWAP_COLOR_BYTES
        color = SWAP_16(color);
    #endif
    uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;
    lcd_set_area(0, 0, lcd_ctl.width, lcd_ctl.height);
    tft_fill_data(&data, g_lcd_h * g_lcd_w / 2);
}

static void mcu_lcd_fill_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
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

static void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t width, uint16_t color)
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

static void mcu_lcd_draw_picture(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint8_t *ptr)
{
    uint32_t i;
    uint16_t* p = (uint16_t*)ptr;
    bool odd = false;
    extern volatile bool maixpy_sdcard_loading;

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
        if (maixpy_sdcard_loading) {
            for(i=0; i< g_pixs_draw_pic_size; i+=2)
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
        } else {
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
        }
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
static void mcu_lcd_draw_pic_roi(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh, uint8_t *ptr)
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


static void mcu_lcd_draw_pic_gray(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint8_t *ptr)
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

static void mcu_lcd_draw_pic_grayroi(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh, uint8_t *ptr)
{
	int y_oft;
	uint8_t* p;
	for(y_oft = 0; y_oft < rh; y_oft++)
	{	//draw line by line
		p = (uint8_t *)(ptr) + w*(y_oft+ry) + rx;
		mcu_lcd_draw_pic_gray(x, y+y_oft, rw, 1, p);
	}
	return;
}

static void lcd_ram_cpyimg(char* lcd, int lcdw, char* img, int imgw, int imgh, int x, int y)
{
	int i;
	for(i=0;i<imgh;i++)
	{
		memcpy(lcd+lcdw*2*(y+i)+x*2,img+imgw*2*i,imgw*2);
	}
	return;
}

/************************* MCU 屏参数  ****************************/
static lcd_para_t mcu_lcd_default = {
	.lcd_type   = LCD_TYPE_ST7789,
	.width      = 320,
	.height     = 240,
	.dir        = 0,
	.extra_para = NULL,
};

lcd_t lcd_mcu = {
	.lcd_para      = &mcu_lcd_default,

	.init      = mcu_lcd_init_shield,
	.deinit    = mcu_lcd_destroy,
	.clear         = mcu_lcd_clear,
	.set_direction = mcu_lcd_set_direction,
	.set_freq		= mcu_lcd_set_freq,
	.get_freq		= mcu_lcd_get_freq,
	.set_offset		= mcu_lcd_set_offset,
    .get_width      = mcu_lcd_get_width,
    .get_height     = mcu_lcd_get_height,
    .bgr_to_rgb     = mcu_lcd_bgr_to_rgb,

    .draw_point     = mcu_lcd_draw_point,
    // .draw_string    = mcu_lcd_draw_string,
	.draw_picture  = mcu_lcd_draw_picture,
	.draw_pic_roi  = mcu_lcd_draw_pic_roi,
	.draw_pic_gray = mcu_lcd_draw_pic_gray,
	.draw_pic_grayroi = mcu_lcd_draw_pic_grayroi,
	.fill_rectangle	= mcu_lcd_fill_rectangle,
};
