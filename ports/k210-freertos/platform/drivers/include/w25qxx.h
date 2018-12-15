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
#ifndef _W25QXX_H
#define _W25QXX_H

#include <stdint.h>

/* clang-format off */
#define DATALENGTH                          8

#define SPI_SLAVE_SELECT                    (0x01)

#define w25qxx_FLASH_PAGE_SIZE              256
#define w25qxx_FLASH_SECTOR_SIZE            4096
#define w25qxx_FLASH_PAGE_NUM_PER_SECTOR    16
#define w25qxx_FLASH_CHIP_SIZE              (16777216 UL)

#define WRITE_ENABLE                        0x06
#define WRITE_DISABLE                       0x04
#define READ_REG1                           0x05
#define READ_REG2                           0x35
#define READ_REG3                           0x15
#define WRITE_REG1                          0x01
#define WRITE_REG2                          0x31
#define WRITE_REG3                          0x11
#define READ_DATA                           0x03
#define FAST_READ                           0x0B
#define FAST_READ_DUAL_OUTPUT               0x3B
#define FAST_READ_QUAL_OUTPUT               0x6B
#define FAST_READ_DUAL_IO                   0xBB
#define FAST_READ_QUAL_IO                   0xEB
#define DUAL_READ_RESET                     0xFFFF
#define QUAL_READ_RESET                     0xFF
#define PAGE_PROGRAM                        0x02
#define QUAD_PAGE_PROGRAM                   0x32
#define SECTOR_ERASE                        0x20
#define BLOCK_32K_ERASE                     0x52
#define BLOCK_64K_ERASE                     0xD8
#define CHIP_ERASE                          0x60
#define READ_ID                             0x90
#define ENABLE_QPI                          0x38
#define EXIT_QPI                            0xFF
#define ENABLE_RESET                        0x66
#define RESET_DEVICE                        0x99

#define REG1_BUSY_MASK                      0x01
#define REG2_QUAL_MASK                      0x02

#define LETOBE(x)     ((x >> 24) | ((x & 0x00FF0000) >> 8) | ((x & 0x0000FF00) << 8) | (x << 24))
/* clang-format on */

/**
 * @brief      w25qxx operating status enumerate
 */
typedef enum _w25qxx_status
{
    W25QXX_OK = 0,
    W25QXX_BUSY,
    W25QXX_ERROR,
} w25qxx_status_t;

/**
 * @brief      w25qxx read operating enumerate
 */
typedef enum _w25qxx_read
{
    W25QXX_STANDARD = 0,
    W25QXX_STANDARD_FAST,
    W25QXX_DUAL,
    W25QXX_DUAL_FAST,
    W25QXX_QUAD,
    W25QXX_QUAD_FAST,
} w25qxx_read_t;

w25qxx_status_t w25qxx_init(uint8_t spi_index, uint8_t spi_ss);
w25qxx_status_t w25qxx_is_busy(void);
w25qxx_status_t w25qxx_chip_erase(void);
w25qxx_status_t w25qxx_enable_quad_mode(void);
w25qxx_status_t w25qxx_disable_quad_mode(void);
w25qxx_status_t w25qxx_sector_erase(uint32_t addr);
w25qxx_status_t w25qxx_32k_block_erase(uint32_t addr);
w25qxx_status_t w25qxx_64k_block_erase(uint32_t addr);
w25qxx_status_t w25qxx_read_status_reg1(uint8_t *reg_data);
w25qxx_status_t w25qxx_read_status_reg2(uint8_t *reg_data);
w25qxx_status_t w25qxx_write_status_reg(uint8_t reg1_data, uint8_t reg2_data);
w25qxx_status_t w25qxx_read_id(uint8_t *manuf_id, uint8_t *device_id);
w25qxx_status_t w25qxx_write_data(uint32_t addr, uint8_t *data_buf, uint32_t length);
w25qxx_status_t w25qxx_write_data_direct(uint32_t addr, uint8_t *data_buf, uint32_t length);
w25qxx_status_t w25qxx_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length, w25qxx_read_t mode);
w25qxx_status_t w25qxx_enable_xip_mode(void);
w25qxx_status_t w25qxx_disable_xip_mode(void);
w25qxx_status_t w25qxx_read_id_dma(uint8_t *manuf_id, uint8_t *device_id);
w25qxx_status_t w25qxx_sector_erase_dma(uint32_t addr);
w25qxx_status_t w25qxx_init_dma(uint8_t spi_index, uint8_t spi_ss);
w25qxx_status_t w25qxx_write_data_dma(uint32_t addr, uint8_t *data_buf, uint32_t length);
w25qxx_status_t w25qxx_write_data_direct_dma(uint32_t addr, uint8_t *data_buf, uint32_t length);
w25qxx_status_t w25qxx_read_data_dma(uint32_t addr, uint8_t *data_buf, uint32_t length, w25qxx_read_t mode);
w25qxx_status_t w25qxx_is_busy_dma(void);
w25qxx_status_t w25qxx_enable_quad_mode_dma(void);

#endif

