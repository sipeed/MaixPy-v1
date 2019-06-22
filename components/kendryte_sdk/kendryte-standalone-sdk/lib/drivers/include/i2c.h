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
#ifndef _DRIVER_I2C_H
#define _DRIVER_I2C_H

#include <stdint.h>
#include <stddef.h>
#include "dmac.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_MAX_NUM 3

/* clang-format off */
typedef struct _i2c
{
    /* I2C Control Register                                 (0x00) */
    volatile uint32_t con;
    /* I2C Target Address Register                          (0x04) */
    volatile uint32_t tar;
    /* I2C Slave Address Register                           (0x08) */
    volatile uint32_t sar;
    /* reserved                                             (0x0c) */
    volatile uint32_t resv1;
    /* I2C Data Buffer and Command Register                 (0x10) */
    volatile uint32_t data_cmd;
    /* I2C Standard Speed Clock SCL High Count Register     (0x14) */
    volatile uint32_t ss_scl_hcnt;
    /* I2C Standard Speed Clock SCL Low Count Register      (0x18) */
    volatile uint32_t ss_scl_lcnt;
    /* reserverd                                            (0x1c-0x28) */
    volatile uint32_t resv2[4];
    /* I2C Interrupt Status Register                        (0x2c) */
    volatile uint32_t intr_stat;
    /* I2C Interrupt Mask Register                          (0x30) */
    volatile uint32_t intr_mask;
    /* I2C Raw Interrupt Status Register                    (0x34) */
    volatile uint32_t raw_intr_stat;
    /* I2C Receive FIFO Threshold Register                  (0x38) */
    volatile uint32_t rx_tl;
    /* I2C Transmit FIFO Threshold Register                 (0x3c) */
    volatile uint32_t tx_tl;
    /* I2C Clear Combined and Individual Interrupt Register (0x40) */
    volatile uint32_t clr_intr;
    /* I2C Clear RX_UNDER Interrupt Register                (0x44) */
    volatile uint32_t clr_rx_under;
    /* I2C Clear RX_OVER Interrupt Register                 (0x48) */
    volatile uint32_t clr_rx_over;
    /* I2C Clear TX_OVER Interrupt Register                 (0x4c) */
    volatile uint32_t clr_tx_over;
    /* I2C Clear RD_REQ Interrupt Register                  (0x50) */
    volatile uint32_t clr_rd_req;
    /* I2C Clear TX_ABRT Interrupt Register                 (0x54) */
    volatile uint32_t clr_tx_abrt;
    /* I2C Clear RX_DONE Interrupt Register                 (0x58) */
    volatile uint32_t clr_rx_done;
    /* I2C Clear ACTIVITY Interrupt Register                (0x5c) */
    volatile uint32_t clr_activity;
    /* I2C Clear STOP_DET Interrupt Register                (0x60) */
    volatile uint32_t clr_stop_det;
    /* I2C Clear START_DET Interrupt Register               (0x64) */
    volatile uint32_t clr_start_det;
    /* I2C Clear GEN_CALL Interrupt Register                (0x68) */
    volatile uint32_t clr_gen_call;
    /* I2C Enable Register                                  (0x6c) */
    volatile uint32_t enable;
    /* I2C Status Register                                  (0x70) */
    volatile uint32_t status;
    /* I2C Transmit FIFO Level Register                     (0x74) */
    volatile uint32_t txflr;
    /* I2C Receive FIFO Level Register                      (0x78) */
    volatile uint32_t rxflr;
    /* I2C SDA Hold Time Length Register                    (0x7c) */
    volatile uint32_t sda_hold;
    /* I2C Transmit Abort Source Register                   (0x80) */
    volatile uint32_t tx_abrt_source;
    /* reserved                                             (0x84) */
    volatile uint32_t resv3;
    /* I2C DMA Control Register                             (0x88) */
    volatile uint32_t dma_cr;
    /* I2C DMA Transmit Data Level Register                 (0x8c) */
    volatile uint32_t dma_tdlr;
    /* I2C DMA Receive Data Level Register                  (0x90) */
    volatile uint32_t dma_rdlr;
    /* I2C SDA Setup Register                               (0x94) */
    volatile uint32_t sda_setup;
    /* I2C ACK General Call Register                        (0x98) */
    volatile uint32_t general_call;
    /* I2C Enable Status Register                           (0x9c) */
    volatile uint32_t enable_status;
    /* I2C SS, FS or FM+ spike suppression limit            (0xa0) */
    volatile uint32_t fs_spklen;
    /* reserved                                             (0xa4-0xf0) */
    volatile uint32_t resv4[20];
    /* I2C Component Parameter Register 1                   (0xf4) */
    volatile uint32_t comp_param_1;
    /* I2C Component Version Register                       (0xf8) */
    volatile uint32_t comp_version;
    /* I2C Component Type Register                          (0xfc) */
    volatile uint32_t comp_type;
} __attribute__((packed, aligned(4))) i2c_t;

/* I2C Control Register*/
#define I2C_CON_MASTER_MODE                     0x00000001U
#define I2C_CON_SPEED_MASK                      0x00000006U
#define I2C_CON_SPEED(x)                        ((x) << 1)
#define I2C_CON_10BITADDR_SLAVE                 0x00000008U
#define I2C_CON_RESTART_EN                      0x00000020U
#define I2C_CON_SLAVE_DISABLE                   0x00000040U
#define I2C_CON_STOP_DET_IFADDRESSED            0x00000080U
#define I2C_CON_TX_EMPTY_CTRL                   0x00000100U

/* I2C Target Address Register*/
#define I2C_TAR_ADDRESS_MASK                    0x000003FFU
#define I2C_TAR_ADDRESS(x)                      ((x) << 0)
#define I2C_TAR_GC_OR_START                     0x00000400U
#define I2C_TAR_SPECIAL                         0x00000800U
#define I2C_TAR_10BITADDR_MASTER                0x00001000U

/* I2C Slave Address Register*/
#define I2C_SAR_ADDRESS_MASK                    0x000003FFU
#define I2C_SAR_ADDRESS(x)                  ((x) << 0)

/* I2C Rx/Tx Data Buffer and Command Register*/
#define I2C_DATA_CMD_CMD                        0x00000100U
#define I2C_DATA_CMD_DATA_MASK                  0x000000FFU
#define I2C_DATA_CMD_DATA(x)                    ((x) << 0)

/* Standard Speed I2C Clock SCL High Count Register*/
#define I2C_SS_SCL_HCNT_COUNT_MASK              0x0000FFFFU
#define I2C_SS_SCL_HCNT_COUNT(x)                ((x) << 0)

/* Standard Speed I2C Clock SCL Low Count Register*/
#define I2C_SS_SCL_LCNT_COUNT_MASK              0x0000FFFFU
#define I2C_SS_SCL_LCNT_COUNT(x)                ((x) << 0)

/* I2C Interrupt Status Register*/
#define I2C_INTR_STAT_RX_UNDER                  0x00000001U
#define I2C_INTR_STAT_RX_OVER                   0x00000002U
#define I2C_INTR_STAT_RX_FULL                   0x00000004U
#define I2C_INTR_STAT_TX_OVER                   0x00000008U
#define I2C_INTR_STAT_TX_EMPTY                  0x00000010U
#define I2C_INTR_STAT_RD_REQ                    0x00000020U
#define I2C_INTR_STAT_TX_ABRT                   0x00000040U
#define I2C_INTR_STAT_RX_DONE                   0x00000080U
#define I2C_INTR_STAT_ACTIVITY                  0x00000100U
#define I2C_INTR_STAT_STOP_DET                  0x00000200U
#define I2C_INTR_STAT_START_DET                 0x00000400U
#define I2C_INTR_STAT_GEN_CALL                  0x00000800U

/* I2C Interrupt Mask Register*/
#define I2C_INTR_MASK_RX_UNDER                  0x00000001U
#define I2C_INTR_MASK_RX_OVER                   0x00000002U
#define I2C_INTR_MASK_RX_FULL                   0x00000004U
#define I2C_INTR_MASK_TX_OVER                   0x00000008U
#define I2C_INTR_MASK_TX_EMPTY                  0x00000010U
#define I2C_INTR_MASK_RD_REQ                    0x00000020U
#define I2C_INTR_MASK_TX_ABRT                   0x00000040U
#define I2C_INTR_MASK_RX_DONE                   0x00000080U
#define I2C_INTR_MASK_ACTIVITY                  0x00000100U
#define I2C_INTR_MASK_STOP_DET                  0x00000200U
#define I2C_INTR_MASK_START_DET                 0x00000400U
#define I2C_INTR_MASK_GEN_CALL                  0x00000800U

/* I2C Raw Interrupt Status Register*/
#define I2C_RAW_INTR_MASK_RX_UNDER              0x00000001U
#define I2C_RAW_INTR_MASK_RX_OVER               0x00000002U
#define I2C_RAW_INTR_MASK_RX_FULL               0x00000004U
#define I2C_RAW_INTR_MASK_TX_OVER               0x00000008U
#define I2C_RAW_INTR_MASK_TX_EMPTY              0x00000010U
#define I2C_RAW_INTR_MASK_RD_REQ                0x00000020U
#define I2C_RAW_INTR_MASK_TX_ABRT               0x00000040U
#define I2C_RAW_INTR_MASK_RX_DONE               0x00000080U
#define I2C_RAW_INTR_MASK_ACTIVITY              0x00000100U
#define I2C_RAW_INTR_MASK_STOP_DET              0x00000200U
#define I2C_RAW_INTR_MASK_START_DET             0x00000400U
#define I2C_RAW_INTR_MASK_GEN_CALL              0x00000800U

/* I2C Receive FIFO Threshold Register*/
#define I2C_RX_TL_VALUE_MASK                    0x00000007U
#define I2C_RX_TL_VALUE(x)                  ((x) << 0)

/* I2C Transmit FIFO Threshold Register*/
#define I2C_TX_TL_VALUE_MASK                    0x00000007U
#define I2C_TX_TL_VALUE(x)                  ((x) << 0)

/* Clear Combined and Individual Interrupt Register*/
#define I2C_CLR_INTR_CLR                        0x00000001U

/* Clear RX_UNDER Interrupt Register*/
#define I2C_CLR_RX_UNDER_CLR                    0x00000001U

/* Clear RX_OVER Interrupt Register*/
#define I2C_CLR_RX_OVER_CLR                     0x00000001U

/* Clear TX_OVER Interrupt Register*/
#define I2C_CLR_TX_OVER_CLR                     0x00000001U

/* Clear RD_REQ Interrupt Register*/
#define I2C_CLR_RD_REQ_CLR                      0x00000001U

/* Clear TX_ABRT Interrupt Register*/
#define I2C_CLR_TX_ABRT_CLR                     0x00000001U

/* Clear RX_DONE Interrupt Register*/
#define I2C_CLR_RX_DONE_CLR                     0x00000001U

/* Clear ACTIVITY Interrupt Register*/
#define I2C_CLR_ACTIVITY_CLR                    0x00000001U

/* Clear STOP_DET Interrupt Register*/
#define I2C_CLR_STOP_DET_CLR                    0x00000001U

/* Clear START_DET Interrupt Register*/
#define I2C_CLR_START_DET_CLR                   0x00000001U

/* Clear GEN_CALL Interrupt Register*/
#define I2C_CLR_GEN_CALL_CLR                    0x00000001U

/* I2C Enable Register*/
#define I2C_ENABLE_ENABLE                       0x00000001U
#define I2C_ENABLE_ABORT                        0x00000002U
#define I2C_ENABLE_TX_CMD_BLOCK                 0x00000004U

/* I2C Status Register*/
#define I2C_STATUS_ACTIVITY                     0x00000001U
#define I2C_STATUS_TFNF                         0x00000002U
#define I2C_STATUS_TFE                          0x00000004U
#define I2C_STATUS_RFNE                         0x00000008U
#define I2C_STATUS_RFF                          0x00000010U
#define I2C_STATUS_MST_ACTIVITY                 0x00000020U
#define I2C_STATUS_SLV_ACTIVITY                 0x00000040U

/* I2C Transmit FIFO Level Register*/
#define I2C_TXFLR_VALUE_MASK                    0x00000007U
#define I2C_TXFLR_VALUE(x)                      ((x) << 0)

/* I2C Receive FIFO Level Register*/
#define I2C_RXFLR_VALUE_MASK                    0x00000007U
#define I2C_RXFLR_VALUE(x)                      ((x) << 0)

/* I2C SDA Hold Time Length Register*/
#define I2C_SDA_HOLD_TX_MASK                    0x0000FFFFU
#define I2C_SDA_HOLD_TX(x)                      ((x) << 0)
#define I2C_SDA_HOLD_RX_MASK                    0x00FF0000U
#define I2C_SDA_HOLD_RX(x)                      ((x) << 16)

/* I2C Transmit Abort Source Register*/
#define I2C_TX_ABRT_SOURCE_7B_ADDR_NOACK        0x00000001U
#define I2C_TX_ABRT_SOURCE_10B_ADDR1_NOACK      0x00000002U
#define I2C_TX_ABRT_SOURCE_10B_ADDR2_NOACK      0x00000004U
#define I2C_TX_ABRT_SOURCE_TXDATA_NOACK         0x00000008U
#define I2C_TX_ABRT_SOURCE_GCALL_NOACK          0x00000010U
#define I2C_TX_ABRT_SOURCE_GCALL_READ           0x00000020U
#define I2C_TX_ABRT_SOURCE_HS_ACKDET            0x00000040U
#define I2C_TX_ABRT_SOURCE_SBYTE_ACKDET         0x00000080U
#define I2C_TX_ABRT_SOURCE_HS_NORSTRT           0x00000100U
#define I2C_TX_ABRT_SOURCE_SBYTE_NORSTRT        0x00000200U
#define I2C_TX_ABRT_SOURCE_10B_RD_NORSTRT       0x00000400U
#define I2C_TX_ABRT_SOURCE_MASTER_DIS           0x00000800U
#define I2C_TX_ABRT_SOURCE_MST_ARBLOST          0x00001000U
#define I2C_TX_ABRT_SOURCE_SLVFLUSH_TXFIFO      0x00002000U
#define I2C_TX_ABRT_SOURCE_SLV_ARBLOST          0x00004000U
#define I2C_TX_ABRT_SOURCE_SLVRD_INTX           0x00008000U
#define I2C_TX_ABRT_SOURCE_USER_ABRT            0x00010000U

/* DMA Control Register*/
#define I2C_DMA_CR_RDMAE                        0x00000001U
#define I2C_DMA_CR_TDMAE                        0x00000002U

/* DMA Transmit Data Level Register*/
#define I2C_DMA_TDLR_VALUE_MASK                 0x00000007U
#define I2C_DMA_TDLR_VALUE(x)                   ((x) << 0)

/* DMA Receive Data Level Register*/
#define I2C_DMA_RDLR_VALUE_MASK                 0x00000007U
#define I2C_DMA_RDLR_VALUE(x)                   ((x) << 0)

/* I2C SDA Setup Register*/
#define I2C_SDA_SETUP_VALUE_MASK                0x000000FFU
#define I2C_SDA_SETUP_VALUE(x)                  ((x) << 0)

/* I2C ACK General Call Register*/
#define I2C_ACK_GENERAL_CALL_ENABLE             0x00000001U

/* I2C Enable Status Register*/
#define I2C_ENABLE_STATUS_IC_ENABLE             0x00000001U
#define I2C_ENABLE_STATUS_SLV_DIS_BUSY          0x00000002U
#define I2C_ENABLE_STATUS_SLV_RX_DATA_LOST      0x00000004U

/* I2C SS, FS or FM+ spike suppression limit*/
#define I2C_FS_SPKLEN_VALUE_MASK                0x000000FFU
#define I2C_FS_SPKLEN_VALUE(x)                  ((x) << 0)

/* Component Parameter Register 1*/
#define I2C_COMP_PARAM1_APB_DATA_WIDTH          0x00000003U
#define I2C_COMP_PARAM1_MAX_SPEED_MODE          0x0000000CU
#define I2C_COMP_PARAM1_HC_COUNT_VALUES         0x00000010U
#define I2C_COMP_PARAM1_INTR_IO                 0x00000020U
#define I2C_COMP_PARAM1_HAS_DMA                 0x00000040U
#define I2C_COMP_PARAM1_ENCODED_PARAMS          0x00000080U
#define I2C_COMP_PARAM1_RX_BUFFER_DEPTH         0x0000FF00U
#define I2C_COMP_PARAM1_TX_BUFFER_DEPTH         0x00FF0000U

/* I2C Component Version Register*/
#define I2C_COMP_VERSION_VALUE                  0xFFFFFFFFU

/* I2C Component Type Register*/
#define I2C_COMP_TYPE_VALUE                     0xFFFFFFFFU
/* clang-format on */

extern volatile i2c_t *const i2c[3];

typedef enum _i2c_device_number
{
    I2C_DEVICE_0,
    I2C_DEVICE_1,
    I2C_DEVICE_2,
    I2C_DEVICE_MAX,
} i2c_device_number_t;

typedef enum _i2c_bus_speed_mode
{
    I2C_BS_STANDARD,
    I2C_BS_FAST,
    I2C_BS_HIGHSPEED
} i2c_bus_speed_mode_t;

typedef enum _i2c_event
{
    I2C_EV_START,
    I2C_EV_RESTART,
    I2C_EV_STOP
} i2c_event_t;

typedef struct _i2c_slave_handler
{
    void(*on_receive)(uint32_t data);
    uint32_t(*on_transmit)();
    void(*on_event)(i2c_event_t event);
} i2c_slave_handler_t;

typedef enum _i2c_transfer_mode
{
    I2C_SEND,
    I2C_RECEIVE,
} i2c_transfer_mode_t;

typedef struct _i2c_data_t
{
    dmac_channel_number_t tx_channel;
    dmac_channel_number_t rx_channel;
    uint32_t *tx_buf;
    size_t tx_len;
    uint32_t *rx_buf;
    size_t rx_len;
    i2c_transfer_mode_t transfer_mode;
} i2c_data_t;

/**
 * @brief       Set i2c params
 *
 * @param[in]   i2c_num             i2c number
 * @param[in]   slave_address       i2c slave device address
 * @param[in]   address_width       address width 7bit or 10bit
 * @param[in]   i2c_clk             i2c clk rate
 */
void i2c_init(i2c_device_number_t i2c_num, uint32_t slave_address, uint32_t address_width,
              uint32_t i2c_clk);

/**
 * @brief       I2c send data
 *
 * @param[in]   i2c_num         i2c number
 * @param[in]   send_buf        send data
 * @param[in]   send_buf_len    send data length
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int i2c_send_data(i2c_device_number_t i2c_num, const uint8_t *send_buf, size_t send_buf_len);

/**
 * @brief       Init i2c as slave mode.
 *
 * @param[in]   i2c_num             i2c number
 * @param[in]   slave_address       i2c slave device address
 * @param[in]   address_width       address width 7bit or 10bit
 * @param[in]   handler             Handle of i2c slave interrupt function.
 */
void i2c_init_as_slave(i2c_device_number_t i2c_num, uint32_t slave_address, uint32_t address_width,
    const i2c_slave_handler_t *handler);

/**
 * @brief       I2c send data by dma
 *
 * @param[in]   dma_channel_num     dma channel
 * @param[in]   i2c_num             i2c number
 * @param[in]   send_buf            send data
 * @param[in]   send_buf_len        send data length
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
void i2c_send_data_dma(dmac_channel_number_t dma_channel_num, i2c_device_number_t i2c_num, const uint8_t *send_buf,
                       size_t send_buf_len);

/**
 * @brief       I2c receive data
 *
 * @param[in]   i2c_num             i2c number
 * @param[in]   send_buf            send data address
 * @param[in]   send_buf_len        length of send buf
 * @param[in]   receive_buf         receive buf address
 * @param[in]   receive_buf_len     length of receive buf
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
*/
int i2c_recv_data(i2c_device_number_t i2c_num, const uint8_t *send_buf, size_t send_buf_len, uint8_t *receive_buf,
                  size_t receive_buf_len);

/**
 * @brief       I2c receive data by dma
 *
 * @param[in]   dma_send_channel_num        send dma channel
 * @param[in]   dma_receive_channel_num     receive dma channel
 * @param[in]   i2c_num                     i2c number
 * @param[in]   send_buf                    send data address
 * @param[in]   send_buf_len                length of send buf
 * @param[in]   receive_buf                 receive buf address
 * @param[in]   receive_buf_len             length of receive buf
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
*/
void i2c_recv_data_dma(dmac_channel_number_t dma_send_channel_num, dmac_channel_number_t dma_receive_channel_num,
                       i2c_device_number_t i2c_num, const uint8_t *send_buf, size_t send_buf_len,
                       uint8_t *receive_buf, size_t receive_buf_len);
/**
 * @brief       I2c handle transfer data operations
 *
 * @param[in]   i2c_num             i2c number
 * @param[in]   data                i2c data information
 * @param[in]   cb                  i2c dma callback
 *
*/
void i2c_handle_data_dma(i2c_device_number_t i2c_num, i2c_data_t data, plic_interrupt_t *cb);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_I2C_H */
