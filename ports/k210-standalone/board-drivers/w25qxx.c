#include "w25qxx.h"
#include "fpioa.h"
#include "spi.h"
#include "sysctl.h"
#include "dmac.h"

/* clang-format off */
#define SPI_SLAVE_SELECT			(0x01)

#define w25qxx_FLASH_PAGE_SIZE			256
#define w25qxx_FLASH_SECTOR_SIZE		4096
#define w25qxx_FLASH_PAGE_NUM_PER_SECTOR	16
#define w25qxx_FLASH_CHIP_SIZE			(16777216 UL)

#define WRITE_ENABLE				0x06
#define WRITE_DISABLE				0x04
#define READ_REG1				0x05
#define READ_REG2				0x35
#define READ_REG3				0x15
#define WRITE_REG1				0x01
#define WRITE_REG2				0x31
#define WRITE_REG3				0x11
#define READ_DATA				0x03
#define FAST_READ				0x0B
#define FAST_READ_DUAL_OUTPUT			0x3B
#define FAST_READ_QUAL_OUTPUT			0x6B
#define FAST_READ_DUAL_IO			0xBB
#define FAST_READ_QUAL_IO			0xEB
#define DUAL_READ_RESET				0xFFFF
#define QUAL_READ_RESET				0xFF
#define PAGE_PROGRAM				0x02
#define QUAD_PAGE_PROGRAM			0x32
#define SECTOR_ERASE				0x20
#define BLOCK_32K_ERASE				0x52
#define BLOCK_64K_ERASE				0xD8
#define CHIP_ERASE				0x60
#define READ_ID					0x90
#define ENABLE_QPI				0x38
#define EXIT_QPI				0xFF
#define ENABLE_RESET				0x66
#define RESET_DEVICE				0x99

#define REG1_BUSY_MASK				0x01
#define REG2_QUAL_MASK				0x02

#define LETOBE(x)				((x >> 24) | ((x & 0x00FF0000) >> 8) | ((x & 0x0000FF00) << 8) | (x << 24))
/* clang-format on */

#define SPI_DMA_CHANNEL		2
enum w25qxx_status_t (*w25qxx_page_program_fun)(uint32_t addr, uint8_t *data_buf, uint32_t length);
enum w25qxx_status_t (*w25qxx_read_fun)(uint32_t addr, uint8_t *data_buf, uint32_t length);

static enum w25qxx_status_t w25qxx_stand_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length);
static enum w25qxx_status_t w25qxx_quad_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length);
static enum w25qxx_status_t w25qxx_page_program(uint32_t addr, uint8_t *data_buf, uint32_t length);
static enum w25qxx_status_t w25qxx_quad_page_program(uint32_t addr, uint8_t *data_buf, uint32_t length);

static volatile spi_t *spi_handle;
static uint8_t dfs_offset, tmod_offset, frf_offset, dma_tx_line, dma_rx_line;

static enum w25qxx_status_t w25qxx_receive_data(uint8_t *cmd_buff, uint8_t cmd_len, uint8_t *rx_buff, uint32_t rx_len)
{
	uint32_t index, fifo_len;
	spi_handle->ctrlr0 = (0x07 << dfs_offset) | (0x03 << tmod_offset);
	spi_handle->ctrlr1 = rx_len - 1;
	spi_handle->ssienr = 0x01;
	while (cmd_len--)
		spi_handle->dr[0] = *cmd_buff++;
	spi_handle->ser = SPI_SLAVE_SELECT;
	while (rx_len) {
		fifo_len = spi_handle->rxflr;
		fifo_len = fifo_len < rx_len ? fifo_len : rx_len;
		for (index = 0; index < fifo_len; index++)
			*rx_buff++ = spi_handle->dr[0];
		rx_len -= fifo_len;
	}
	spi_handle->ser = 0x00;
	spi_handle->ssienr = 0x00;
	return W25QXX_OK;
}

static enum w25qxx_status_t w25qxx_send_data(uint8_t *cmd_buff, uint8_t cmd_len, uint8_t *tx_buff, uint32_t tx_len)
{
	uint32_t index, fifo_len;

	spi_handle->ctrlr0 = (0x07 << dfs_offset) | (0x01 << tmod_offset);
	spi_handle->ssienr = 0x01;
	while (cmd_len--)
		spi_handle->dr[0] = *cmd_buff++;
	fifo_len = 32 - spi_handle->txflr;
	fifo_len = fifo_len < tx_len ? fifo_len : tx_len;
	for (index = 0; index < fifo_len; index++)
		spi_handle->dr[0] = *tx_buff++;
	tx_len -= fifo_len;
	spi_handle->ser = SPI_SLAVE_SELECT;
	while (tx_len) {
		fifo_len = 32 - spi_handle->txflr;
		fifo_len = fifo_len < tx_len ? fifo_len : tx_len;
		for (index = 0; index < fifo_len; index++)
			spi_handle->dr[0] = *tx_buff++;
		tx_len -= fifo_len;
	}
	while ((spi_handle->sr & 0x05) != 0x04)
		;
	spi_handle->ser = 0x00;
	spi_handle->ssienr = 0x00;
	return W25QXX_OK;
}

static enum w25qxx_status_t w25qxx_receive_data_enhanced(uint32_t *cmd_buff, uint8_t cmd_len, uint8_t *rx_buff, uint32_t rx_len)
{
	uint32_t index, fifo_len;

	spi_handle->ctrlr1 = rx_len - 1;
	spi_handle->ssienr = 0x01;
	while (cmd_len--)
		spi_handle->dr[0] = *cmd_buff++;
	spi_handle->ser = SPI_SLAVE_SELECT;
	while (rx_len) {
		fifo_len = spi_handle->rxflr;
		fifo_len = fifo_len < rx_len ? fifo_len : rx_len;
		for (index = 0; index < fifo_len; index++)
			*rx_buff++ = spi_handle->dr[0];
		rx_len -= fifo_len;
	}
	spi_handle->ser = 0x00;
	spi_handle->ssienr = 0x00;
	return W25QXX_OK;
}

static enum w25qxx_status_t w25qxx_send_data_enhanced(uint32_t *cmd_buff, uint8_t cmd_len, uint8_t *tx_buff, uint32_t tx_len)
{
	uint32_t index, fifo_len;

	spi_handle->ssienr = 0x01;
	while (cmd_len--)
		spi_handle->dr[0] = *cmd_buff++;
	fifo_len = 32 - spi_handle->txflr;
	fifo_len = fifo_len < tx_len ? fifo_len : tx_len;
	for (index = 0; index < fifo_len; index++)
		spi_handle->dr[0] = *tx_buff++;
	tx_len -= fifo_len;
	spi_handle->ser = SPI_SLAVE_SELECT;
	while (tx_len) {
		fifo_len = 32 - spi_handle->txflr;
		fifo_len = fifo_len < tx_len ? fifo_len : tx_len;
		for (index = 0; index < fifo_len; index++)
			spi_handle->dr[0] = *tx_buff++;
		tx_len -= fifo_len;
	}
	while ((spi_handle->sr & 0x05) != 0x04)
		;
	spi_handle->ser = 0x00;
	spi_handle->ssienr = 0x00;
	return W25QXX_OK;
}

enum w25qxx_status_t w25qxx_init(uint8_t spi_index)
{
	uint8_t data[2] = {0xFF, 0xFF};

	sysctl_clock_enable(SYSCTL_CLOCK_FPIOA);
	if (spi_index == 0) {
		sysctl_reset(SYSCTL_RESET_SPI0);
		sysctl_clock_enable(SYSCTL_CLOCK_SPI0);
		sysctl_clock_set_threshold(SYSCTL_THRESHOLD_SPI0, 0);
		fpioa_set_function(27, FUNC_SPI0_SS0);
		fpioa_set_function(24, FUNC_SPI0_SCLK);
		fpioa_set_function(26, FUNC_SPI0_D0);
		fpioa_set_function(25, FUNC_SPI0_D1);
		fpioa_set_function(23, FUNC_SPI0_D2);
		fpioa_set_function(22, FUNC_SPI0_D3);
		spi_handle = spi[0];
		dfs_offset = 16;
		tmod_offset = 8;
		frf_offset = 21;
		dma_rx_line = 0;
		dma_tx_line = 1;
	} else if (spi_index == 1) {
		sysctl_reset(SYSCTL_RESET_SPI1);
		sysctl_clock_enable(SYSCTL_CLOCK_SPI1);
		sysctl_clock_set_threshold(SYSCTL_THRESHOLD_SPI1, 0);
		fpioa_set_function(27, FUNC_SPI1_SS0);
		fpioa_set_function(24, FUNC_SPI1_SCLK);
		fpioa_set_function(26, FUNC_SPI1_D0);
		fpioa_set_function(25, FUNC_SPI1_D1);
		fpioa_set_function(23, FUNC_SPI1_D2);
		fpioa_set_function(22, FUNC_SPI1_D3);
		spi_handle = spi[1];
		dfs_offset = 16;
		tmod_offset = 8;
		frf_offset = 21;
		dma_rx_line = 2;
		dma_tx_line = 3;
	} else {
		sysctl->clk_sel0.spi3_clk_sel = 1;
		sysctl_clock_set_threshold(SYSCTL_THRESHOLD_SPI3, 4);
		sysctl_clock_enable(SYSCTL_CLOCK_SPI3);
		sysctl->peri.spi3_xip_en = 0;
		spi_handle = spi[3];
		dfs_offset = 0;
		tmod_offset = 10;
		frf_offset = 22;
		dma_rx_line = 6;
		dma_tx_line = 7;
	}

	spi_handle->baudr = 0x0A;
	spi_handle->imr = 0x00;
	spi_handle->dmatdlr = 0x10;
	spi_handle->dmardlr = 0x0F;
	spi_handle->ser = 0x00;
	spi_handle->ssienr = 0x00;

	w25qxx_page_program_fun = w25qxx_page_program;
	w25qxx_read_fun = w25qxx_stand_read_data;
	w25qxx_send_data(data, 2, 0, 0);
	return W25QXX_OK;
}

enum w25qxx_status_t w25qxx_read_id(uint8_t *manuf_id, uint8_t *device_id)
{
	uint8_t cmd[4] = {READ_ID, 0x00, 0x00, 0x00};
	uint8_t data[2];

	w25qxx_receive_data(cmd, 4, data, 2);
	*manuf_id = data[0];
	*device_id = data[1];
	return W25QXX_OK;
}

static enum w25qxx_status_t w25qxx_write_enable(void)
{
	uint8_t cmd[1] = {WRITE_ENABLE};

	w25qxx_send_data(cmd, 1, 0, 0);
	return W25QXX_OK;
}

enum w25qxx_status_t w25qxx_write_status_reg(uint8_t reg1_data, uint8_t reg2_data)
{
	uint8_t cmd[3] = {WRITE_REG1, reg1_data, reg2_data};

	w25qxx_write_enable();
	w25qxx_send_data(cmd, 3, 0, 0);
	return W25QXX_OK;
}

enum w25qxx_status_t w25qxx_read_status_reg1(uint8_t *reg_data)
{
	uint8_t cmd[1] = {READ_REG1};
	uint8_t data[1];

	w25qxx_receive_data(cmd, 1, data, 1);
	*reg_data = data[0];
	return W25QXX_OK;
}

enum w25qxx_status_t w25qxx_read_status_reg2(uint8_t *reg_data)
{
	uint8_t cmd[1] = {READ_REG2};
	uint8_t data[1];

	w25qxx_receive_data(cmd, 1, data, 1);
	*reg_data = data[0];
	return W25QXX_OK;
}

enum w25qxx_status_t w25qxx_is_busy(void)
{
	uint8_t status;

	w25qxx_read_status_reg1(&status);
	if (status & REG1_BUSY_MASK)
		return W25QXX_BUSY;
	return W25QXX_OK;
}

enum w25qxx_status_t w25qxx_sector_erase(uint32_t addr)
{
	uint8_t cmd[4] = {SECTOR_ERASE};

	cmd[1] = (uint8_t)(addr >> 16);
	cmd[2] = (uint8_t)(addr >> 8);
	cmd[3] = (uint8_t)(addr);
	w25qxx_write_enable();
	w25qxx_send_data(cmd, 4, 0, 0);
	return W25QXX_OK;
}

enum w25qxx_status_t w25qxx_32k_block_erase(uint32_t addr)
{
	uint8_t cmd[4] = {BLOCK_32K_ERASE};

	cmd[1] = (uint8_t)(addr >> 16);
	cmd[2] = (uint8_t)(addr >> 8);
	cmd[3] = (uint8_t)(addr);
	w25qxx_write_enable();
	w25qxx_send_data(cmd, 4, 0, 0);
	return W25QXX_OK;
}

enum w25qxx_status_t w25qxx_64k_block_erase(uint32_t addr)
{
	uint8_t cmd[4] = {BLOCK_64K_ERASE};

	cmd[1] = (uint8_t)(addr >> 16);
	cmd[2] = (uint8_t)(addr >> 8);
	cmd[3] = (uint8_t)(addr);
	w25qxx_write_enable();
	w25qxx_send_data(cmd, 4, 0, 0);
	return W25QXX_OK;
}

enum w25qxx_status_t w25qxx_chip_erase(void)
{
	uint8_t cmd[1] = {CHIP_ERASE};

	w25qxx_write_enable();
	w25qxx_send_data(cmd, 1, 0, 0);
	return W25QXX_OK;
}

enum w25qxx_status_t w25qxx_enable_quad_mode(void)
{
	uint8_t reg_data;

	w25qxx_read_status_reg2(&reg_data);
	if (!(reg_data & REG2_QUAL_MASK)) {
		reg_data |= REG2_QUAL_MASK;
		w25qxx_write_status_reg(0x00, reg_data);
	}
	w25qxx_page_program_fun = w25qxx_quad_page_program;
	w25qxx_read_fun = w25qxx_quad_read_data;
	return W25QXX_OK;
}

enum w25qxx_status_t w25qxx_disable_quad_mode(void)
{
	uint8_t reg_data;

	w25qxx_read_status_reg2(&reg_data);
	if (reg_data & REG2_QUAL_MASK) {
		reg_data &= (~REG2_QUAL_MASK);
		w25qxx_write_status_reg(0x00, reg_data);
	}
	w25qxx_page_program_fun = w25qxx_page_program;
	w25qxx_read_fun = w25qxx_stand_read_data;
	return W25QXX_OK;
}

static enum w25qxx_status_t w25qxx_page_program(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
	uint8_t cmd[4] = {PAGE_PROGRAM};

	cmd[1] = (uint8_t)(addr >> 16);
	cmd[2] = (uint8_t)(addr >> 8);
	cmd[3] = (uint8_t)(addr);
	w25qxx_write_enable();
	w25qxx_send_data(cmd, 4, data_buf, length);
	return W25QXX_OK;
}

static enum w25qxx_status_t w25qxx_quad_page_program(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
	uint32_t cmd[2];

	cmd[0] = QUAD_PAGE_PROGRAM;
	cmd[1] = addr;
	w25qxx_write_enable();
	spi_handle->ctrlr0 = (0x01 << tmod_offset) | (0x07 << dfs_offset) | (0x02 << frf_offset);
	spi_handle->spi_ctrlr0 = (0x06 << 2) | (0x02 << 8);
	w25qxx_send_data_enhanced(cmd, 2, data_buf, length);
	return W25QXX_OK;
}

static enum w25qxx_status_t w25qxx_sector_program(uint32_t addr, uint8_t *data_buf)
{
	uint8_t index;

	for (index = 0; index < w25qxx_FLASH_PAGE_NUM_PER_SECTOR; index++) {
		w25qxx_page_program_fun(addr, data_buf, w25qxx_FLASH_PAGE_SIZE);
		while (w25qxx_is_busy() == W25QXX_BUSY)
			;
		addr += w25qxx_FLASH_PAGE_SIZE;
		data_buf += w25qxx_FLASH_PAGE_SIZE;
	}
	return W25QXX_OK;
}

enum w25qxx_status_t w25qxx_write_data(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
	uint32_t sector_addr, sector_offset, sector_remain, write_len, index;
	uint8_t swap_buf[w25qxx_FLASH_SECTOR_SIZE];
	uint8_t *pread, *pwrite;

	while (length) {
		sector_addr = addr & (~(w25qxx_FLASH_SECTOR_SIZE - 1));
		sector_offset = addr & (w25qxx_FLASH_SECTOR_SIZE - 1);
		sector_remain = w25qxx_FLASH_SECTOR_SIZE - sector_offset;
		write_len = length < sector_remain ? length : sector_remain;
		w25qxx_read_fun(sector_addr, swap_buf, w25qxx_FLASH_SECTOR_SIZE);
		pread = swap_buf + sector_offset;
		pwrite = data_buf;
		for (index = 0; index < write_len; index++) {
			if ((*pwrite) != ((*pwrite) & (*pread))) {
				w25qxx_sector_erase(sector_addr);
				while (w25qxx_is_busy() == W25QXX_BUSY)
					;
				break;
			}
			pwrite++;
			pread++;
		}
		if (write_len == w25qxx_FLASH_SECTOR_SIZE)
			w25qxx_sector_program(sector_addr, data_buf);
		else {
			pread = swap_buf + sector_offset;
			pwrite = data_buf;
			for (index = 0; index < write_len; index++)
				*pread++ = *pwrite++;
			w25qxx_sector_program(sector_addr, swap_buf);
		}
		length -= write_len;
		addr += write_len;
		data_buf += write_len;
	}
	return W25QXX_OK;
}

enum w25qxx_status_t w25qxx_write_data_direct(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
	uint32_t page_remain, write_len;

	while (length) {
		page_remain = w25qxx_FLASH_PAGE_SIZE - (addr & (w25qxx_FLASH_PAGE_SIZE - 1));
		write_len = length < page_remain ? length : page_remain;
		w25qxx_page_program_fun(addr, data_buf, write_len);
		while (w25qxx_is_busy() == W25QXX_BUSY)
			;
		length -= write_len;
		addr += write_len;
		data_buf += write_len;
	}
	return W25QXX_OK;
}

static enum w25qxx_status_t _w25qxx_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length, enum w25qxx_read_t mode)
{
	uint32_t cmd[2];

	switch (mode) {
	case W25QXX_STANDARD:
		*(((uint8_t *)cmd) + 0) = READ_DATA;
		*(((uint8_t *)cmd) + 1) = (uint8_t)(addr >> 16);
		*(((uint8_t *)cmd) + 2) = (uint8_t)(addr >> 8);
		*(((uint8_t *)cmd) + 3) = (uint8_t)(addr >> 0);
		w25qxx_receive_data((uint8_t *)cmd, 4, data_buf, length);
		break;
	case W25QXX_STANDARD_FAST:
		*(((uint8_t *)cmd) + 0) = FAST_READ;
		*(((uint8_t *)cmd) + 1) = (uint8_t)(addr >> 16);
		*(((uint8_t *)cmd) + 2) = (uint8_t)(addr >> 8);
		*(((uint8_t *)cmd) + 3) = (uint8_t)(addr >> 0);
		*(((uint8_t *)cmd) + 4) = 0xFF;
		w25qxx_receive_data((uint8_t *)cmd, 5, data_buf, length);
		break;
	case W25QXX_DUAL:
		cmd[0] = FAST_READ_DUAL_OUTPUT;
		cmd[1] = addr;
		spi_handle->ctrlr0 = (0x02 << tmod_offset) | (0x07 << dfs_offset) | (0x01 << frf_offset);
		spi_handle->spi_ctrlr0 = (0x06 << 2) | (0x02 << 8) | (0x08 << 11);
		w25qxx_receive_data_enhanced(cmd, 2, data_buf, length);
		break;
	case W25QXX_DUAL_FAST:
		cmd[0] = FAST_READ_DUAL_IO;
		cmd[1] = addr << 8;
		spi_handle->ctrlr0 = (0x02 << tmod_offset) | (0x07 << dfs_offset) | (0x01 << frf_offset);
		spi_handle->spi_ctrlr0 = (0x08 << 2) | (0x02 << 8) | 0x01;
		w25qxx_receive_data_enhanced(cmd, 2, data_buf, length);
		break;
	case W25QXX_QUAD:
		cmd[0] = FAST_READ_QUAL_OUTPUT;
		cmd[1] = addr;
		spi_handle->ctrlr0 = (0x02 << tmod_offset) | (0x07 << dfs_offset) | (0x02 << frf_offset);
		spi_handle->spi_ctrlr0 = (0x06 << 2) | (0x02 << 8) | (0x08 << 11);
		w25qxx_receive_data_enhanced(cmd, 2, data_buf, length);
		break;
	case W25QXX_QUAD_FAST:
		cmd[0] = FAST_READ_QUAL_IO;
		cmd[1] = addr << 8;
		spi_handle->ctrlr0 = (0x02 << tmod_offset) | (0x07 << dfs_offset) | (0x02 << frf_offset);
		spi_handle->spi_ctrlr0 = (0x08 << 2) | (0x02 << 8) | (0x04 << 11) | 0x01;
		w25qxx_receive_data_enhanced(cmd, 2, data_buf, length);
		break;
	}
	return W25QXX_OK;
}

enum w25qxx_status_t w25qxx_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length, enum w25qxx_read_t mode)
{
	uint32_t len;

	while (length) {
		len = length >= 0x010000 ? 0x010000 : length;
		_w25qxx_read_data(addr, data_buf, len, mode);
		addr += len;
		data_buf += len;
		length -= len;
	}
	return W25QXX_OK;
}

static enum w25qxx_status_t w25qxx_stand_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
	return w25qxx_read_data(addr, data_buf, length, W25QXX_STANDARD_FAST);
}

static enum w25qxx_status_t w25qxx_quad_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
	return w25qxx_read_data(addr, data_buf, length, W25QXX_QUAD_FAST);
}

enum w25qxx_status_t w25qxx_enable_xip_mode(void)
{
	if (spi_handle != spi[3])
		return W25QXX_ERROR;

	spi_handle->xip_ctrl = (0x01 << 29) | (0x02 << 26) | (0x01 << 23) | (0x01 << 22) | (0x04 << 13) |
			   (0x01 << 12) | (0x02 << 9) | (0x06 << 4) | (0x01 << 2) | 0x02;
	spi_handle->xip_incr_inst = 0xEB;
	spi_handle->xip_mode_bits = 0x00;
	spi_handle->xip_ser = 0x01;
	spi_handle->ssienr = 0x01;
	sysctl->peri.spi3_xip_en = 1;
	return W25QXX_OK;
}

enum w25qxx_status_t w25qxx_disable_xip_mode(void)
{
	sysctl->peri.spi3_xip_en = 0;
	return W25QXX_OK;
}

enum w25qxx_status_t w25qxx_read_data_dma(uint32_t addr, uint32_t *data_buf, uint32_t length)
{
	uint32_t len;

	if (spi_handle != spi[3])
		spi_handle->endian = 0x01;
	spi_handle->baudr = 0x02;
	spi_handle->ctrlr0 = (0x02 << tmod_offset) | (0x1F << dfs_offset) | (0x02 << frf_offset);
	spi_handle->spi_ctrlr0 = (0x08 << 2) | (0x02 << 8) | (0x04 << 11) | 0x01;
	dmac->channel[SPI_DMA_CHANNEL].sar = (uint64_t)(&spi_handle->dr[0]);
	dmac->channel[SPI_DMA_CHANNEL].ctl = (((uint64_t)1 << 47) | ((uint64_t)15 << 48) |
					      ((uint64_t)1 << 38) | ((uint64_t)15 << 39) |
					      ((uint64_t)3 << 18) | ((uint64_t)3 << 14) |
					      ((uint64_t)2 << 11) | ((uint64_t)2 << 8) |
					      ((uint64_t)1 << 4));
	dmac->channel[SPI_DMA_CHANNEL].cfg = (((uint64_t)4 << 49) | ((uint64_t)SPI_DMA_CHANNEL << 44) |
					      ((uint64_t)SPI_DMA_CHANNEL << 39) | ((uint64_t)2 << 32));
	sysctl_dma_select(SPI_DMA_CHANNEL, dma_rx_line);
	dmac->channel[SPI_DMA_CHANNEL].intstatus_en = 0xFFFFFFFF;
	while (length) {
		len = length >= 0x010000 ? 0x010000 : length;
		length -= len;
		spi_handle->dmacr = 0x01;
		spi_handle->ctrlr1 = len - 1;
		spi_handle->ssienr = 0x01;
		if (spi_handle != spi[3]) {
			spi_handle->dr[0] = LETOBE(FAST_READ_QUAL_IO);
			spi_handle->dr[0] = LETOBE(addr << 8);
		} else {
			spi_handle->dr[0] = (FAST_READ_QUAL_IO);
			spi_handle->dr[0] = (addr << 8);
		}
		addr += (len * 4);
		if (len > 0x0F) {
			dmac->channel[SPI_DMA_CHANNEL].dar = (uint64_t)data_buf;
			dmac->channel[SPI_DMA_CHANNEL].block_ts = (len & 0xFFFFFFF0) - 1;
			dmac->channel[SPI_DMA_CHANNEL].intclear = 0xFFFFFFFF;
			spi_handle->ser = SPI_SLAVE_SELECT;
			dmac->chen = 0x0101 << SPI_DMA_CHANNEL;
			while ((dmac->channel[SPI_DMA_CHANNEL].intstatus & 0x02) == 0)
				;
			data_buf += (len & 0xFFFFFFF0);
			len &= 0x0F;
		} else
			spi_handle->ser = SPI_SLAVE_SELECT;
		while ((spi_handle->sr & 0x05) != 0x04)
			;
		while (len--)
			*data_buf++ = spi_handle->dr[0];
		spi_handle->ser = 0x00;
		spi_handle->ssienr = 0x00;
		spi_handle->dmacr = 0x00;
	}
	spi_handle->baudr = 0x0A;
	if (spi_handle != spi[3])
		spi_handle->endian = 0x00;
	return W25QXX_OK;
}

enum w25qxx_status_t w25qxx_write_data_dma(uint32_t addr, uint32_t *data_buf, uint32_t length)
{
	uint32_t len;

	spi_handle->baudr = 0x02;
	dmac->channel[SPI_DMA_CHANNEL].dar = (uint64_t)(&spi_handle->dr[0]);
	dmac->channel[SPI_DMA_CHANNEL].ctl = (((uint64_t)1 << 47) | ((uint64_t)15 << 48) |
					      ((uint64_t)1 << 38) | ((uint64_t)15 << 39) |
					      ((uint64_t)3 << 18) | ((uint64_t)3 << 14) |
					      ((uint64_t)2 << 11) | ((uint64_t)2 << 8) |
					      ((uint64_t)1 << 6));
	dmac->channel[SPI_DMA_CHANNEL].cfg = (((uint64_t)4 << 49) | ((uint64_t)SPI_DMA_CHANNEL << 44) |
					      ((uint64_t)SPI_DMA_CHANNEL << 39) | ((uint64_t)1 << 32));
	sysctl_dma_select(SPI_DMA_CHANNEL, dma_tx_line);
	dmac->channel[SPI_DMA_CHANNEL].intstatus_en = 0xFFFFFFFF;
	while (length) {
		len = length >= w25qxx_FLASH_PAGE_SIZE / 4 ? w25qxx_FLASH_PAGE_SIZE / 4 : length;
		length -= len;
		w25qxx_write_enable();
		if (spi_handle != spi[3])
			spi_handle->endian = 0x01;
		spi_handle->ctrlr0 = (0x01 << tmod_offset) | (0x1F << dfs_offset) | (0x02 << frf_offset);
		spi_handle->spi_ctrlr0 = (0x06 << 2) | (0x02 << 8);
		spi_handle->dmacr = 0x02;
		spi_handle->ssienr = 0x01;
		if (spi_handle != spi[3]) {
			spi_handle->dr[0] = LETOBE(QUAD_PAGE_PROGRAM);
			spi_handle->dr[0] = LETOBE(addr);
		} else {
			spi_handle->dr[0] = (QUAD_PAGE_PROGRAM);
			spi_handle->dr[0] = (addr);
		}
		dmac->channel[SPI_DMA_CHANNEL].sar = (uint64_t)data_buf;
		dmac->channel[SPI_DMA_CHANNEL].block_ts = len - 1;
		dmac->channel[SPI_DMA_CHANNEL].intclear = 0xFFFFFFFF;
		dmac->chen = 0x0101 << SPI_DMA_CHANNEL;
		addr += (len * 4);
		data_buf += len;
		len = len >= 16 ? 16 : len;
		while (spi_handle->txflr < len)
			;
		spi_handle->ser = SPI_SLAVE_SELECT;
		while ((dmac->channel[SPI_DMA_CHANNEL].intstatus & 0x02) == 0)
			;
		while ((spi_handle->sr & 0x05) != 0x04)
			;
		spi_handle->ser = 0x00;
		spi_handle->ssienr = 0x00;
		spi_handle->dmacr = 0x00;
		if (spi_handle != spi[3])
			spi_handle->endian = 0x00;
		while (w25qxx_is_busy() == W25QXX_BUSY)
			;
	}
	spi_handle->baudr = 0x0A;
	return W25QXX_OK;
}
