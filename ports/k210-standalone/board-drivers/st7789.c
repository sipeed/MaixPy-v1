#include "st7789.h"
#include "sysctl.h"
#include "fpioa.h"
#include "gpiohs.h"
#include "spi.h"
#include "dmac.h"
#include "plic.h"
#include <stdio.h>
#include <sleep.h>

#define SPI_CHANNEL		0
#define SPI_DMA_CHANNEL		0
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


static inline void set_dc_low(void)
{
	gpiohs_set_pin_value(30, GPIO_PV_Low);
}

static inline void set_dc_high(void)
{
	gpiohs_set_pin_value(30, GPIO_PV_High);
}

static inline void set_rst_low(void)
{
	gpiohs_set_pin_value(31, GPIO_PV_Low);
}

static inline void set_rst_high(void)
{
	gpiohs_set_pin_value(31, GPIO_PV_High);
}

static void st7789_io_init(void)
{
	//rst
    fpioa_set_function(37, FUNC_GPIOHS31);
    gpiohs_set_drive_mode(31, GPIO_DM_Output);
    gpiohs_set_pin_value(31, GPIO_PV_Low);
	//dc
	fpioa_set_function(38, FUNC_GPIOHS30);
    gpiohs_set_drive_mode(30, GPIO_DM_Output);
    gpiohs_set_pin_value(30, GPIO_PV_Low);
}

static void tft_set_spi_freq(uint32_t freq_mhz)
{
	//ch unused
	uint8_t div = 0;

	uint32_t pll0_freq = sysctl_clock_get_freq(SYSCTL_CLOCK_PLL0);

	pll0_freq /= 1000000;
	pll0_freq /= 2;

	div = pll0_freq / freq_mhz;

	div /= 2;
	div -= 1;
	sysctl_reset(SPI_SYSCTL(RESET));
	sysctl_clock_set_threshold(SPI_SYSCTL(THRESHOLD), div);
	sysctl_clock_enable(SPI_SYSCTL(CLOCK));

	printf("div:%d want:%dMhz real:%d\r\n",div,freq_mhz,sysctl_clock_get_freq(SYSCTL_CLOCK_SPI0));
}

// init hardware
void tft_hard_init(void)
{
	tft_set_spi_freq(20);

	fpioa_set_function(36, SPI_SS);
	fpioa_set_function(39, SPI(SCLK));

	sysctl->misc.spi_dvp_data_enable = 1;		//SPI_D0....D7 output

	st7789_io_init();
	set_rst_low();
    msleep(100);
    set_rst_high();

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
	set_dc_low();
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

	set_dc_high();
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

	set_dc_high();
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
	set_dc_high();
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

