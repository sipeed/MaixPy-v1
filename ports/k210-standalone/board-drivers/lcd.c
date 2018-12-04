#include <string.h>
#include "lcd.h"
#include "st7789.h"
#include "font.h"
//#include "common.h"
#include "sleep.h"

typedef struct
{
	uint8_t mode;
	uint8_t dir;
	uint16_t width;
	uint16_t height;
} lcd_ctl_t;

static lcd_ctl_t lcd_ctl;

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
	uint8_t data;

	tft_hard_init();
	// soft reset
	tft_write_command(SOFTWARE_RESET);
	msleep(100);
	// exit sleep
	tft_write_command(SLEEP_OFF);
	msleep(100);
	// pixel format
	tft_write_command(PIXEL_FORMAT_SET);
	data = 0x55;
	tft_write_byte(&data, 1);
	lcd_set_direction(DIR_YX_RLDU);
	//lcd_set_direction(DIR_YX_RLDU);
	// display on
	tft_write_command(DISPALY_ON);
	lcd_polling_enable();
}

void lcd_set_direction(enum lcd_dir_t dir)
{
	// dir |= 0x08;
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
	tft_set_datawidth(8);
	tft_write_command(MEMORY_ACCESS_CTL);
	tft_write_byte((uint8_t *)&dir, 1);
}

void lcd_set_area(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	uint8_t data[4];

	tft_set_datawidth(8);

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
	tft_set_datawidth(16);
	tft_write_half(&color, 1);
}

void lcd_draw_char(uint16_t x, uint16_t y, char c, uint16_t color)
{
	uint8_t i, j, data;

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
	uint8_t i, j, data, *pdata;
	uint16_t width;
	uint32_t *pixel;

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
					*pixel = ((uint32_t)bg_color << 16) | bg_color;
					break;
				case 1:
					*pixel = ((uint32_t)bg_color << 16) | font_color;
					break;
				case 2:
					*pixel = ((uint32_t)font_color << 16) | bg_color;
					break;
				case 3:
					*pixel = ((uint32_t)font_color << 16) | font_color;
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
	tft_set_datawidth(32);
	tft_write_word(&data, LCD_X_MAX * LCD_Y_MAX / 2, lcd_ctl.mode ? 3 : 1);
}

void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t width, uint16_t color)
{
	uint32_t data_buf[640];
	uint32_t *p = data_buf;
	uint32_t data = color;
	uint32_t index;

	data = (data << 16) | data;
	for (index = 0; index < 160 * width; index++)
		*p++ = data;

	lcd_set_area(x1, y1, x2, y1 + width - 1);
	tft_set_datawidth(32);
	tft_write_word(data_buf, ((x2 - x1 + 1) * width + 1) / 2, 0);
	lcd_set_area(x1, y2 - width + 1, x2, y2);
	tft_set_datawidth(32);
	tft_write_word(data_buf, ((x2 - x1 + 1) * width + 1) / 2, 0);
	lcd_set_area(x1, y1, x1 + width - 1, y2);
	tft_set_datawidth(32);
	tft_write_word(data_buf, ((y2 - y1 + 1) * width + 1) / 2, 0);
	lcd_set_area(x2 - width + 1, y1, x2, y2);
	tft_set_datawidth(32);
	tft_write_word(data_buf, ((y2 - y1 + 1) * width + 1) / 2, 0);
}

void lcd_draw_rectangle_cpu(uint32_t *ptr, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	uint32_t *addr1, *addr2;
	uint16_t i;
	uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;

	if (x1 == 319)
		x1 = 318;
	if (x2 == 0)
		x2 = 1;
	if (y1 == 239)
		y1 = 238;
	if (y2 == 0)
		y2 = 1;

	addr1 = ptr + (320 * y1 + x1) / 2;
	addr2 = ptr + (320 * (y2 - 1) + x1) / 2;
	for (i = x1; i < x2; i += 2)
	{
		*addr1 = data;
		*(addr1 + 160) = data;
		*addr2 = data;
		*(addr2 + 160) = data;
		addr1++;
		addr2++;
	}
	addr1 = ptr + (320 * y1 + x1) / 2;
	addr2 = ptr + (320 * y1 + x2 - 1) / 2;
	for (i = y1; i < y2; i++)
	{
		*addr1 = data;
		*addr2 = data;
		addr1 += 160;
		addr2 += 160;
	}
}

void lcd_draw_picture(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint32_t *ptr)
{
	lcd_set_area(x1, y1, x1 + width - 1, y1 + height - 1);
	tft_set_datawidth(32);
	tft_write_word(ptr, width * height / 2, lcd_ctl.mode ? 2 : 0);
}

void lcd_draw_picture_slow(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint32_t *ptr)
{
	uint32_t index;
	uint8_t dat[4];
	if(width == 320 && height == 240)
		lcd_set_area(x1, y1, x1 + width, y1 + height);
	else
		lcd_set_area(x1, y1, x1 + width - 1, y1 + height - 1);

	for (index = 0; index < width * height / 2; index++)
	{
		dat[2] = *ptr >> 24 & 0xff;
		dat[3] = *ptr >> 16 & 0xff;
		dat[0] = *ptr >> 8 & 0xff;
		dat[1] = *ptr & 0xff;
		tft_write_byte(&dat, 4);
		ptr++;
	}
}

int lcd_busy(void)
{
	return tft_busy();
}
