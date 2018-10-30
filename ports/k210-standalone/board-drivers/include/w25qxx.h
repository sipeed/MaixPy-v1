/**
 * @file
 * @brief      winbond w25qxx series driver
 */
#ifndef _W25QXX_LICHEE_H
#define _W25QXX_LICHEE_H

#include <stdint.h>

/**
 * @brief      w25qxx operating status enumerate
 */
enum w25qxx_status_t {
	W25QXX_OK = 0,
	W25QXX_BUSY,
	W25QXX_ERROR,
};

/**
 * @brief      w25qxx read operating enumerate
 */
enum w25qxx_read_t {
	W25QXX_STANDARD = 0,
	W25QXX_STANDARD_FAST,
	W25QXX_DUAL,
	W25QXX_DUAL_FAST,
	W25QXX_QUAD,
	W25QXX_QUAD_FAST,
};

extern enum w25qxx_status_t (*w25qxx_page_program_fun)(uint32_t addr, uint8_t *data_buf, uint32_t length);

enum w25qxx_status_t w25qxx_init(uint8_t spi_index);
enum w25qxx_status_t w25qxx_is_busy(void);
enum w25qxx_status_t w25qxx_chip_erase(void);
enum w25qxx_status_t w25qxx_enable_quad_mode(void);
enum w25qxx_status_t w25qxx_disable_quad_mode(void);
enum w25qxx_status_t w25qxx_sector_erase(uint32_t addr);
enum w25qxx_status_t w25qxx_32k_block_erase(uint32_t addr);
enum w25qxx_status_t w25qxx_64k_block_erase(uint32_t addr);
enum w25qxx_status_t w25qxx_read_status_reg1(uint8_t *reg_data);
enum w25qxx_status_t w25qxx_read_status_reg2(uint8_t *reg_data);
enum w25qxx_status_t w25qxx_write_status_reg(uint8_t reg1_data, uint8_t reg2_data);
enum w25qxx_status_t w25qxx_read_id(uint8_t *manuf_id, uint8_t *device_id);
enum w25qxx_status_t w25qxx_write_data(uint32_t addr, uint8_t *data_buf, uint32_t length);
enum w25qxx_status_t w25qxx_write_data_direct(uint32_t addr, uint8_t *data_buf, uint32_t length);
enum w25qxx_status_t w25qxx_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length, enum w25qxx_read_t mode);
enum w25qxx_status_t w25qxx_enable_xip_mode(void);
enum w25qxx_status_t w25qxx_disable_xip_mode(void);
enum w25qxx_status_t w25qxx_write_data_dma(uint32_t addr, uint32_t *data_buf, uint32_t length);
enum w25qxx_status_t w25qxx_read_data_dma(uint32_t addr, uint32_t *data_buf, uint32_t length);

#endif
