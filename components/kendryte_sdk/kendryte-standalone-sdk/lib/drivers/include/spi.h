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
#ifndef _DRIVER_SPI_H
#define _DRIVER_SPI_H

#include <stdint.h>
#include <stddef.h>
#include "dmac.h"

#ifdef __cplusplus
extern "C" {
#endif

/* clang-format off */
typedef struct _spi
{
    /* SPI Control Register 0                                    (0x00)*/
    volatile uint32_t ctrlr0;
    /* SPI Control Register 1                                    (0x04)*/
    volatile uint32_t ctrlr1;
    /* SPI Enable Register                                       (0x08)*/
    volatile uint32_t ssienr;
    /* SPI Microwire Control Register                            (0x0c)*/
    volatile uint32_t mwcr;
    /* SPI Slave Enable Register                                 (0x10)*/
    volatile uint32_t ser;
    /* SPI Baud Rate Select                                      (0x14)*/
    volatile uint32_t baudr;
    /* SPI Transmit FIFO Threshold Level                         (0x18)*/
    volatile uint32_t txftlr;
    /* SPI Receive FIFO Threshold Level                          (0x1c)*/
    volatile uint32_t rxftlr;
    /* SPI Transmit FIFO Level Register                          (0x20)*/
    volatile uint32_t txflr;
    /* SPI Receive FIFO Level Register                           (0x24)*/
    volatile uint32_t rxflr;
    /* SPI Status Register                                       (0x28)*/
    volatile uint32_t sr;
    /* SPI Interrupt Mask Register                               (0x2c)*/
    volatile uint32_t imr;
    /* SPI Interrupt Status Register                             (0x30)*/
    volatile uint32_t isr;
    /* SPI Raw Interrupt Status Register                         (0x34)*/
    volatile uint32_t risr;
    /* SPI Transmit FIFO Overflow Interrupt Clear Register       (0x38)*/
    volatile uint32_t txoicr;
    /* SPI Receive FIFO Overflow Interrupt Clear Register        (0x3c)*/
    volatile uint32_t rxoicr;
    /* SPI Receive FIFO Underflow Interrupt Clear Register       (0x40)*/
    volatile uint32_t rxuicr;
    /* SPI Multi-Master Interrupt Clear Register                 (0x44)*/
    volatile uint32_t msticr;
    /* SPI Interrupt Clear Register                              (0x48)*/
    volatile uint32_t icr;
    /* SPI DMA Control Register                                  (0x4c)*/
    volatile uint32_t dmacr;
    /* SPI DMA Transmit Data Level                               (0x50)*/
    volatile uint32_t dmatdlr;
    /* SPI DMA Receive Data Level                                (0x54)*/
    volatile uint32_t dmardlr;
    /* SPI Identification Register                               (0x58)*/
    volatile uint32_t idr;
    /* SPI DWC_ssi component version                             (0x5c)*/
    volatile uint32_t ssic_version_id;
    /* SPI Data Register 0-36                                    (0x60 -- 0xec)*/
    volatile uint32_t dr[36];
    /* SPI RX Sample Delay Register                              (0xf0)*/
    volatile uint32_t rx_sample_delay;
    /* SPI SPI Control Register                                  (0xf4)*/
    volatile uint32_t spi_ctrlr0;
    /* reserved                                                  (0xf8)*/
    volatile uint32_t resv;
    /* SPI XIP Mode bits                                         (0xfc)*/
    volatile uint32_t xip_mode_bits;
    /* SPI XIP INCR transfer opcode                              (0x100)*/
    volatile uint32_t xip_incr_inst;
    /* SPI XIP WRAP transfer opcode                              (0x104)*/
    volatile uint32_t xip_wrap_inst;
    /* SPI XIP Control Register                                  (0x108)*/
    volatile uint32_t xip_ctrl;
    /* SPI XIP Slave Enable Register                             (0x10c)*/
    volatile uint32_t xip_ser;
    /* SPI XIP Receive FIFO Overflow Interrupt Clear Register    (0x110)*/
    volatile uint32_t xrxoicr;
    /* SPI XIP time out register for continuous transfers        (0x114)*/
    volatile uint32_t xip_cnt_time_out;
    volatile uint32_t endian;
} __attribute__((packed, aligned(4))) spi_t;
/* clang-format on */

typedef enum _spi_device_num
{
    SPI_DEVICE_0,
    SPI_DEVICE_1,
    SPI_DEVICE_2,
    SPI_DEVICE_3,
    SPI_DEVICE_MAX,
} spi_device_num_t;

typedef enum _spi_work_mode
{
    SPI_WORK_MODE_0,
    SPI_WORK_MODE_1,
    SPI_WORK_MODE_2,
    SPI_WORK_MODE_3,
} spi_work_mode_t;

typedef enum _spi_frame_format
{
    SPI_FF_STANDARD,
    SPI_FF_DUAL,
    SPI_FF_QUAD,
    SPI_FF_OCTAL
} spi_frame_format_t;

typedef enum _spi_instruction_address_trans_mode
{
    SPI_AITM_STANDARD,
    SPI_AITM_ADDR_STANDARD,
    SPI_AITM_AS_FRAME_FORMAT
} spi_instruction_address_trans_mode_t;

typedef enum _spi_transfer_mode
{
    SPI_TMOD_TRANS_RECV,
    SPI_TMOD_TRANS,
    SPI_TMOD_RECV,
    SPI_TMOD_EEROM
} spi_transfer_mode_t;


typedef enum _spi_transfer_width
{
    SPI_TRANS_CHAR  = 0x1,
    SPI_TRANS_SHORT = 0x2,
    SPI_TRANS_INT   = 0x4,
} spi_transfer_width_t;

typedef enum _spi_chip_select
{
    SPI_CHIP_SELECT_0,
    SPI_CHIP_SELECT_1,
    SPI_CHIP_SELECT_2,
    SPI_CHIP_SELECT_3,
    SPI_CHIP_SELECT_MAX,
} spi_chip_select_t;

typedef enum
{
    WRITE_CONFIG,
    READ_CONFIG,
    WRITE_DATA_BYTE,
    READ_DATA_BYTE,
    WRITE_DATA_BLOCK,
    READ_DATA_BLOCK,
} spi_slave_command_e;

typedef struct
{
    uint8_t cmd;
    uint8_t err;
    uint32_t addr;
    uint32_t len;
} spi_slave_command_t;

typedef enum
{
    IDLE,
    COMMAND,
    TRANSFER,
} spi_slave_status_e;

typedef int (*spi_slave_receive_callback_t)(void *ctx);

typedef struct _spi_slave_instance
{
    uint8_t int_pin;
    uint8_t ready_pin;
    dmac_channel_number_t dmac_channel;
    uint8_t dfs;
    uint8_t slv_oe;
    uint8_t work_mode;
    size_t data_bit_length;
    volatile spi_slave_status_e status;
    volatile spi_slave_command_t command;
    volatile uint8_t *config_ptr;
    uint32_t config_len;
    spi_slave_receive_callback_t callback;
} spi_slave_instance_t;

typedef struct _spi_data_t
{
    dmac_channel_number_t tx_channel;
    dmac_channel_number_t rx_channel;
    uint32_t *tx_buf;
    size_t tx_len;
    uint32_t *rx_buf;
    size_t rx_len;
    spi_transfer_mode_t transfer_mode;
    bool fill_mode;
} spi_data_t;

extern volatile spi_t *const spi[4];

/**
 * @brief       Set spi configuration
 *
 * @param[in]   spi_num             Spi bus number
 * @param[in]   mode                Spi mode
 * @param[in]   frame_format        Spi frame format
 * @param[in]   data_bit_length     Spi data bit length
 * @param[in]   endian              0:little-endian 1:big-endian
 *
 * @return      Void
 */
void spi_init(spi_device_num_t spi_num, spi_work_mode_t work_mode, spi_frame_format_t frame_format,
              size_t data_bit_length, uint32_t endian);

/**
 * @brief       Set multiline configuration
 *
 * @param[in]   spi_num                                 Spi bus number
 * @param[in]   instruction_length                      Instruction length
 * @param[in]   address_length                          Address length
 * @param[in]   wait_cycles                             Wait cycles
 * @param[in]   instruction_address_trans_mode          Spi transfer mode
 *
 */
void spi_init_non_standard(spi_device_num_t spi_num, uint32_t instruction_length, uint32_t address_length,
                           uint32_t wait_cycles, spi_instruction_address_trans_mode_t instruction_address_trans_mode);

/**
 * @brief       Spi send data
 *
 * @param[in]   spi_num         Spi bus number
 * @param[in]   chip_select     Spi chip select
 * @param[in]   cmd_buff        Spi command buffer point
 * @param[in]   cmd_len         Spi command length
 * @param[in]   tx_buff         Spi transmit buffer point
 * @param[in]   tx_len          Spi transmit buffer length
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
void spi_send_data_standard(spi_device_num_t spi_num, spi_chip_select_t chip_select, const uint8_t *cmd_buff,
                            size_t cmd_len, const uint8_t *tx_buff, size_t tx_len);

/**
 * @brief       Spi receive data
 *
 * @param[in]   spi_num             Spi bus number
 * @param[in]   chip_select         Spi chip select
 * @param[in]   cmd_buff            Spi command buffer point
 * @param[in]   cmd_len             Spi command length
 * @param[in]   rx_buff             Spi receive buffer point
 * @param[in]   rx_len              Spi receive buffer length
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
void spi_receive_data_standard(spi_device_num_t spi_num, spi_chip_select_t chip_select, const uint8_t *cmd_buff,
                               size_t cmd_len, uint8_t *rx_buff, size_t rx_len);

/**
 * @brief       Spi special receive data
 *
 * @param[in]   spi_num         Spi bus number
 * @param[in]   chip_select     Spi chip select
 * @param[in]   cmd_buff        Spi command buffer point
 * @param[in]   cmd_len         Spi command length
 * @param[in]   rx_buff         Spi receive buffer point
 * @param[in]   rx_len          Spi receive buffer length
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
void spi_receive_data_multiple(spi_device_num_t spi_num, spi_chip_select_t chip_select, const uint32_t *cmd_buff,
                               size_t cmd_len, uint8_t *rx_buff, size_t rx_len);

/**
 * @brief       Spi special send data
 *
 * @param[in]   spi_num         Spi bus number
 * @param[in]   chip_select     Spi chip select
 * @param[in]   cmd_buff        Spi command buffer point
 * @param[in]   cmd_len         Spi command length
 * @param[in]   tx_buff         Spi transmit buffer point
 * @param[in]   tx_len          Spi transmit buffer length
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
void spi_send_data_multiple(spi_device_num_t spi_num, spi_chip_select_t chip_select, const uint32_t *cmd_buff,
                            size_t cmd_len, const uint8_t *tx_buff, size_t tx_len);

/**
 * @brief       Spi send data by dma
 *
 * @param[in]   channel_num     Dmac channel number
 * @param[in]   spi_num         Spi bus number
 * @param[in]   chip_select     Spi chip select
 * @param[in]   cmd_buff        Spi command buffer point
 * @param[in]   cmd_len         Spi command length
 * @param[in]   tx_buff         Spi transmit buffer point
 * @param[in]   tx_len          Spi transmit buffer length
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
void spi_send_data_standard_dma(dmac_channel_number_t channel_num, spi_device_num_t spi_num,
                                spi_chip_select_t chip_select,
                                const uint8_t *cmd_buff, size_t cmd_len, const uint8_t *tx_buff, size_t tx_len);

/**
 * @brief       Spi receive data by dma
 *
 * @param[in]   w_channel_num       Dmac write channel number
 * @param[in]   r_channel_num       Dmac read channel number
 * @param[in]   spi_num             Spi bus number
 * @param[in]   chip_select         Spi chip select
 * @param[in]   cmd_buff            Spi command buffer point
 * @param[in]   cmd_len             Spi command length
 * @param[in]   rx_buff             Spi receive buffer point
 * @param[in]   rx_len              Spi receive buffer length
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
void spi_receive_data_standard_dma(dmac_channel_number_t dma_send_channel_num,
                                   dmac_channel_number_t dma_receive_channel_num,
                                   spi_device_num_t spi_num, spi_chip_select_t chip_select, const uint8_t *cmd_buff,
                                   size_t cmd_len, uint8_t *rx_buff, size_t rx_len);

/**
 * @brief       Spi special send data by dma
 *
 * @param[in]   channel_num     Dmac channel number
 * @param[in]   spi_num         Spi bus number
 * @param[in]   chip_select     Spi chip select
 * @param[in]   cmd_buff        Spi command buffer point
 * @param[in]   cmd_len         Spi command length
 * @param[in]   tx_buff         Spi transmit buffer point
 * @param[in]   tx_len          Spi transmit buffer length
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
void spi_send_data_multiple_dma(dmac_channel_number_t channel_num, spi_device_num_t spi_num,
                                spi_chip_select_t chip_select,
                                const uint32_t *cmd_buff, size_t cmd_len, const uint8_t *tx_buff, size_t tx_len);

/**
 * @brief       Spi special receive data by dma
 *
 * @param[in]   dma_send_channel_num        Dmac write channel number
 * @param[in]   dma_receive_channel_num     Dmac read channel number
 * @param[in]   spi_num                     Spi bus number
 * @param[in]   chip_select                 Spi chip select
 * @param[in]   cmd_buff                    Spi command buffer point
 * @param[in]   cmd_len                     Spi command length
 * @param[in]   rx_buff                     Spi receive buffer point
 * @param[in]   rx_len                      Spi receive buffer length
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
void spi_receive_data_multiple_dma(dmac_channel_number_t dma_send_channel_num,
                                   dmac_channel_number_t dma_receive_channel_num,
                                   spi_device_num_t spi_num, spi_chip_select_t chip_select, const uint32_t *cmd_buff,
                                   size_t cmd_len, uint8_t *rx_buff, size_t rx_len);

/**
 * @brief       Spi fill dma
 *
 * @param[in]   channel_num     Dmac channel number
 * @param[in]   spi_num         Spi bus number
 * @param[in]   chip_select     Spi chip select
 * @param[in]   tx_buff        Spi command buffer point
 * @param[in]   tx_len         Spi command length
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
void spi_fill_data_dma(dmac_channel_number_t channel_num, spi_device_num_t spi_num, spi_chip_select_t chip_select,
                       const uint32_t *tx_buff, size_t tx_len);

/**
 * @brief       Spi normal send by dma
 *
 * @param[in]   channel_num     Dmac channel number
 * @param[in]   spi_num         Spi bus number
 * @param[in]   chip_select     Spi chip select
 * @param[in]   tx_buff         Spi transmit buffer point
 * @param[in]   tx_len          Spi transmit buffer length
 * @param[in]   stw             Spi transfer width
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
void spi_send_data_normal_dma(dmac_channel_number_t channel_num, spi_device_num_t spi_num,
                              spi_chip_select_t chip_select,
                              const void *tx_buff, size_t tx_len, spi_transfer_width_t spi_transfer_width);

/**
 * @brief       Spi normal send by dma
 *
 * @param[in]   spi_num         Spi bus number
 * @param[in]   spi_clk         Spi clock rate
 *
 * @return      The real spi clock rate
 */
uint32_t spi_set_clk_rate(spi_device_num_t spi_num, uint32_t spi_clk);

/**
 * @brief       Spi full duplex send receive data by dma
 *
 * @param[in]   dma_send_channel_num          Dmac write channel number
 * @param[in]   dma_receive_channel_num       Dmac read channel number
 * @param[in]   spi_num                       Spi bus number
 * @param[in]   chip_select                   Spi chip select
 * @param[in]   tx_buf                        Spi send buffer
 * @param[in]   tx_len                        Spi send buffer length
 * @param[in]   rx_buf                        Spi receive buffer
 * @param[in]   rx_len                        Spi receive buffer length
 *
 */
void spi_dup_send_receive_data_dma(dmac_channel_number_t dma_send_channel_num,
                               dmac_channel_number_t dma_receive_channel_num,
                               spi_device_num_t spi_num, spi_chip_select_t chip_select,
                               const uint8_t *tx_buf, size_t tx_len, uint8_t *rx_buf, size_t rx_len);

/**
 * @brief       Set spi slave configuration
 *
 * @param[in]   int_pin             SPI master starts sending data interrupt.
 * @param[in]   ready_pin           SPI slave ready.
 * @param[in]   dmac_channel        Dmac channel number for block.
 * @param[in]   data_bit_length     Spi data bit length
 * @param[in]   data                SPI slave device data buffer.
 * @param[in]   len                 The length of SPI slave device data buffer.
 * @param[in]   callback            Callback of spi slave.
 *
 * @return      Void
 */
void spi_slave_config(uint8_t int_pin, uint8_t ready_pin, dmac_channel_number_t dmac_channel, size_t data_bit_length, uint8_t *data, uint32_t len, spi_slave_receive_callback_t callback);

/**
 * @brief       Spi handle transfer data operations
 *
 * @param[in]   spi_num         Spi bus number
 * @param[in]   chip_select     Spi chip select
 * @param[in]   data            Spi transfer data information
 * @param[in]   cb              Spi DMA callback
 *
 */
void spi_handle_data_dma(spi_device_num_t spi_num, spi_chip_select_t chip_select, spi_data_t data, plic_interrupt_t *cb);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_SPI_H */
