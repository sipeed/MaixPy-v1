#include "st7789.h"
#include "sysctl.h"
#include "fpioa.h"
#include "gpiohs.h"
#include "spi.h"
#include "dmac.h"
#include "plic.h"
#include <stdio.h>
#include <sleep.h>
#include <string.h>
#include "font.h"

#include <stdio.h>

#define SPI_CHANNEL		0
#define SPI_DMA_CHANNEL		1
#define SPI_SLAVE_SELECT	3

#define __SPI_SYSCTL(x, y)	SYSCTL_##x##_SPI##y
#define _SPI_SYSCTL(x, y)	__SPI_SYSCTL(x, y)
#define SPI_SYSCTL(x)		_SPI_SYSCTL(x, SPI_CHANNEL)
#define __SPI_SS(x, y)		FUNC_SPI##x##_SS##y
#define _SPI_SS(x, y)		__SPI_SS(x, y)
#define SPI_SS			_SPI_SS(SPI_CHANNEL, SPI_SLAVE_SELECT)
#define __SPI(x, y)		FUNC_SPI##x##_##y
#define _SPI(x, y)		__SPI(x, y)
#define SPI(x)			_SPI(SPI_CHANNEL, x)
#define __SPI_HANDSHAKE(x, y)	SYSCTL_DMA_SELECT_SSI##x##_##y##_REQ
#define _SPI_HANDSHAKE(x, y)	__SPI_HANDSHAKE(x, y)
#define SPI_HANDSHAKE(x)	_SPI_HANDSHAKE(SPI_CHANNEL, x)
#define __SPI_DMA_INTERRUPT(x)	IRQN_DMA##x##_INTERRUPT
#define _SPI_DMA_INTERRUPT(x)	__SPI_DMA_INTERRUPT(x)
#define SPI_DMA_INTERRUPT	_SPI_DMA_INTERRUPT(SPI_DMA_CHANNEL)


/**
 *lcd_cs	36
 *lcd_rst	37
 *lcd_dc	38
 *lcd_wr 	39
 * */

static int dma_finish_irq(void *ctx);

static volatile uint8_t tft_status;

static void init_rst(void)
{
    //fpioa_set_function(37, FUNC_GPIOHS0 + 1);//rst
    gpiohs_set_drive_mode(1, GPIO_DM_OUTPUT);
    gpiohs_set_pin(1, GPIO_PV_LOW);
}

void set_rst_value(uint8_t val)
{
    if(val)
        gpiohs_set_pin(1, 1);  //rst high
    else
        gpiohs_set_pin(1, 0);  //rst low
}

// init hardware
void tft_hard_init(void)
{
	sysctl->misc.spi_dvp_data_enable = 1;		//SPI_D0....D7 output

	//fpioa_set_function(38, FUNC_GPIOHS2);//dc
	gpiohs->output_en.u32[0] |= 0x00000004;

	sysctl_reset(SPI_SYSCTL(RESET));
	sysctl_clock_set_threshold(SPI_SYSCTL(THRESHOLD), 2);		//pll0/2
	sysctl_clock_enable(SPI_SYSCTL(CLOCK));

	//fpioa_set_function(36, SPI_SS);
	//fpioa_set_function(39, SPI(SCLK));

	init_rst();
	set_rst_value(0);
    msleep(100);
    set_rst_value(1);

	spi[SPI_CHANNEL]->ser = 0x00;
	spi[SPI_CHANNEL]->ssienr = 0x00;
	spi[SPI_CHANNEL]->endian = 0x00;
	spi[SPI_CHANNEL]->ctrlr0 = 0x00670180;
	spi[SPI_CHANNEL]->baudr = 10;
	spi[SPI_CHANNEL]->imr = 0x00;
	spi[SPI_CHANNEL]->spi_ctrlr0 = 0x0202;
	spi[SPI_CHANNEL]->dmacr = 0x02;
	spi[SPI_CHANNEL]->dmatdlr = 0x0F;
	sysctl_dma_select(SPI_DMA_CHANNEL, SPI_HANDSHAKE(TX));
	dmac->channel[SPI_DMA_CHANNEL].ctl = (((uint64_t)1 << 47) | ((uint64_t)15 << 48) |
				((uint64_t)1 << 38) | ((uint64_t)15 << 39) |
				((uint64_t)3 << 18) | ((uint64_t)3 << 14) |
				((uint64_t)2 << 11) | ((uint64_t)2 << 8) |
				((uint64_t)1 << 6));
	dmac->channel[SPI_DMA_CHANNEL].cfg = (((uint64_t)4 << 49) | ((uint64_t)SPI_DMA_CHANNEL << 44) |
				((uint64_t)SPI_DMA_CHANNEL << 39) | ((uint64_t)1 << 32));
	dmac->channel[SPI_DMA_CHANNEL].intstatus_en = 0x02;
}

void tft_set_datawidth(uint8_t width)
{
	if (width == 32) {
		spi[SPI_CHANNEL]->ctrlr0 = 0x007F0180;
		spi[SPI_CHANNEL]->spi_ctrlr0 = 0x22;
	} else if (width == 16) {
		spi[SPI_CHANNEL]->ctrlr0 = 0x006F0180;
		spi[SPI_CHANNEL]->spi_ctrlr0 = 0x0302;
	} else {
		spi[SPI_CHANNEL]->ctrlr0 = 0x00670180;
		spi[SPI_CHANNEL]->spi_ctrlr0 = 0x0202;
	}
}

// command write
void tft_write_command(uint8_t cmd)
{
	gpiohs->output_val.bits.b2 = 0;
	spi[SPI_CHANNEL]->ssienr = 0x01;
	spi[SPI_CHANNEL]->dr[0] = cmd;
	spi[SPI_CHANNEL]->ser = 0x01 << SPI_SLAVE_SELECT;
	while ((spi[SPI_CHANNEL]->sr & 0x05) != 0x04)
		;
	spi[SPI_CHANNEL]->ser = 0x00;
	spi[SPI_CHANNEL]->ssienr = 0x00;
}

// data write
void tft_write_byte(uint8_t *data_buf, uint32_t length)
{
	uint32_t index, fifo_len;

	gpiohs->output_val.bits.b2 = 1;
	spi[SPI_CHANNEL]->ssienr = 0x01;
	fifo_len = length < 32 ? length : 32;
	for (index = 0; index < fifo_len; index++)
		spi[SPI_CHANNEL]->dr[0] = *data_buf++;
	length -= fifo_len;
	spi[SPI_CHANNEL]->ser = 0x01 << SPI_SLAVE_SELECT;
	while (length) {
		fifo_len = 32 - spi[SPI_CHANNEL]->txflr;
		fifo_len = fifo_len < length ? fifo_len : length;
		for (index = 0; index < fifo_len; index++)
			spi[SPI_CHANNEL]->dr[0] = *data_buf++;
		length -= fifo_len;
	}
	while ((spi[SPI_CHANNEL]->sr & 0x05) != 0x04)
		;
	spi[SPI_CHANNEL]->ser = 0x00;
	spi[SPI_CHANNEL]->ssienr = 0x00;
}

// data write
void tft_write_half(uint16_t *data_buf, uint32_t length)
{
	uint32_t index, fifo_len;

	gpiohs->output_val.bits.b2 = 1;
	spi[SPI_CHANNEL]->ssienr = 0x01;
	fifo_len = length < 32 ? length : 32;
	for (index = 0; index < fifo_len; index++)
		spi[SPI_CHANNEL]->dr[0] = *data_buf++;
	length -= fifo_len;
	spi[SPI_CHANNEL]->ser = 0x01 << SPI_SLAVE_SELECT;
	while (length) {
		fifo_len = 32 - spi[SPI_CHANNEL]->txflr;
		fifo_len = fifo_len < length ? fifo_len : length;
		for (index = 0; index < fifo_len; index++)
			spi[SPI_CHANNEL]->dr[0] = *data_buf++;
		length -= fifo_len;
	}
	while ((spi[SPI_CHANNEL]->sr & 0x05) != 0x04)
		;
	spi[SPI_CHANNEL]->ser = 0x00;
	spi[SPI_CHANNEL]->ssienr = 0x00;
}

// data write
void tft_write_word(uint32_t *data_buf, uint32_t length, uint32_t flag)
{
	gpiohs->output_val.bits.b2 = 1;
	if (flag & 0x01)
		dmac->channel[SPI_DMA_CHANNEL].ctl |= (((uint64_t)1 << 47) | ((uint64_t)15 << 48) |
				((uint64_t)1 << 38) | ((uint64_t)15 << 39) |
				((uint64_t)3 << 18) | ((uint64_t)3 << 14) |
				((uint64_t)2 << 11) | ((uint64_t)2 << 8) |
				((uint64_t)1 << 6) | ((uint64_t)1 << 4));
	else
		dmac->channel[SPI_DMA_CHANNEL].ctl = (((uint64_t)1 << 47) | ((uint64_t)15 << 48) |
				((uint64_t)1 << 38) | ((uint64_t)15 << 39) |
				((uint64_t)3 << 18) | ((uint64_t)3 << 14) |
				((uint64_t)2 << 11) | ((uint64_t)2 << 8) |
				((uint64_t)1 << 6));
	dmac->channel[SPI_DMA_CHANNEL].sar = (uint64_t)data_buf;
	dmac->channel[SPI_DMA_CHANNEL].dar = (uint64_t)(&spi[SPI_CHANNEL]->dr[0]);
	dmac->channel[SPI_DMA_CHANNEL].block_ts = length - 1;
	dmac->channel[SPI_DMA_CHANNEL].intclear = 0xFFFFFFFF;
	spi[SPI_CHANNEL]->ssienr = 0x01;
	spi[SPI_CHANNEL]->ser = 0x01 << SPI_SLAVE_SELECT;
	if (flag & 0x02) {
		plic_irq_deregister(SPI_DMA_INTERRUPT);
		plic_irq_enable(SPI_DMA_INTERRUPT);
		plic_set_priority(SPI_DMA_INTERRUPT, 3);
		plic_irq_register(SPI_DMA_INTERRUPT, dma_finish_irq, 0);
		tft_status = 1;
		dmac->chen = 0x0101 << SPI_DMA_CHANNEL;
		return;
	}
	dmac->chen = 0x0101 << SPI_DMA_CHANNEL;
	while ((dmac->channel[SPI_DMA_CHANNEL].intstatus & 0x02) == 0)
		;
	while ((spi[SPI_CHANNEL]->sr & 0x05) != 0x04)
		;
	spi[SPI_CHANNEL]->ser = 0x00;
	spi[SPI_CHANNEL]->ssienr = 0x00;
}

static int dma_finish_irq(void *ctx)
{
	dmac->channel[SPI_DMA_CHANNEL].intclear = 0xFFFFFFFF;
	tft_status = 2;
	return 0;
}

int tft_busy(void)
{
	if (tft_status == 2) {
		plic_irq_disable(SPI_DMA_INTERRUPT);
		tft_status = 3;
	} else if (tft_status == 3) {
		if ((spi[SPI_CHANNEL]->sr & 0x05) == 0x04) {
			tft_status = 0;
			spi[SPI_CHANNEL]->ser = 0x00;
			spi[SPI_CHANNEL]->ssienr = 0x00;
		}
	}
	return tft_status;
}

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