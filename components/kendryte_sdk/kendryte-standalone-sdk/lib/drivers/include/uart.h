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

/**
 * @file
 * @brief       Universal Asynchronous Receiver/Transmitter (UART)
 *
 *              The UART peripheral supports the following features:
 *
 *              - 8-N-1 and 8-N-2 formats: 8 data bits, no parity bit, 1 start
 *                bit, 1 or 2 stop bits
 *
 *              - 8-entry transmit and receive FIFO buffers with programmable
 *                watermark interrupts
 *
 *              - 16× Rx oversampling with 2/3 majority voting per bit
 *
 *              The UART peripheral does not support hardware flow control or
 *              other modem control signals, or synchronous serial data
 *              tranfesrs.
 *
 *
 */

#ifndef _DRIVER_APBUART_H
#define _DRIVER_APBUART_H

#include <stdint.h>
#include "platform.h"
#include "plic.h"
#include "dmac.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _uart_dev
{
    UART_DEV1 = 0,
    UART_DEV2,
    UART_DEV3,
} uart_dev_t;

typedef struct _uart
{
    union
    {
        volatile uint32_t RBR;
        volatile uint32_t DLL;
        volatile uint32_t THR;
    };

    union
    {
        volatile uint32_t DLH;
        volatile uint32_t IER;
    };

    union
    {
        volatile uint32_t FCR;
        volatile uint32_t IIR;
    };

    volatile uint32_t LCR;
    volatile uint32_t MCR;
    volatile uint32_t LSR;
    volatile uint32_t MSR;

    volatile uint32_t SCR;
    volatile uint32_t LPDLL;
    volatile uint32_t LPDLH;

    volatile uint32_t reserved1[2];

    union
    {
        volatile uint32_t SRBR[16];
        volatile uint32_t STHR[16];
    };

    volatile uint32_t FAR;
    volatile uint32_t TFR;
    volatile uint32_t RFW;
    volatile uint32_t USR;
    volatile uint32_t TFL;
    volatile uint32_t RFL;
    volatile uint32_t SRR;
    volatile uint32_t SRTS;
    volatile uint32_t SBCR;
    volatile uint32_t SDMAM;
    volatile uint32_t SFE;
    volatile uint32_t SRT;
    volatile uint32_t STET;
    volatile uint32_t HTX;
    volatile uint32_t DMASA;
    volatile uint32_t TCR;
    volatile uint32_t DE_EN;
    volatile uint32_t RE_EN;
    volatile uint32_t DET;
    volatile uint32_t TAT;
    volatile uint32_t DLF;
    volatile uint32_t RAR;
    volatile uint32_t TAR;
    volatile uint32_t LCR_EXT;
    volatile uint32_t reserved2[9];
    volatile uint32_t CPR;
    volatile uint32_t UCV;
    volatile uint32_t CTR;
} uart_t;

typedef enum _uart_device_number
{
    UART_DEVICE_1,
    UART_DEVICE_2,
    UART_DEVICE_3,
    UART_DEVICE_MAX,
} uart_device_number_t;

typedef enum _uart_bitwidth
{
    UART_BITWIDTH_5BIT = 5,
    UART_BITWIDTH_6BIT,
    UART_BITWIDTH_7BIT,
    UART_BITWIDTH_8BIT,
} uart_bitwidth_t;

typedef enum _uart_stopbit
{
    UART_STOP_1,
    UART_STOP_1_5,
    UART_STOP_2
} uart_stopbit_t;

typedef enum _uart_rede_sel
{
    DISABLE = 0,
    ENABLE,
} uart_rede_sel_t;

typedef enum _uart_parity
{
    UART_PARITY_NONE,
    UART_PARITY_ODD,
    UART_PARITY_EVEN
} uart_parity_t;

typedef enum _uart_interrupt_mode
{
    UART_SEND = 1,
    UART_RECEIVE = 2,
} uart_interrupt_mode_t;

typedef enum _uart_send_trigger
{
    UART_SEND_FIFO_0,
    UART_SEND_FIFO_2,
    UART_SEND_FIFO_4,
    UART_SEND_FIFO_8,
} uart_send_trigger_t;

typedef enum _uart_receive_trigger
{
    UART_RECEIVE_FIFO_1,
    UART_RECEIVE_FIFO_4,
    UART_RECEIVE_FIFO_8,
    UART_RECEIVE_FIFO_14,
} uart_receive_trigger_t;

typedef struct _uart_data_t
{
    dmac_channel_number_t tx_channel;
    dmac_channel_number_t rx_channel;
    uint32_t *tx_buf;
    size_t tx_len;
    uint32_t *rx_buf;
    size_t rx_len;
    uart_interrupt_mode_t transfer_mode;
} uart_data_t;

typedef struct _uart_tcr
{
    uint32_t rs485_en : 1;
    uint32_t re_pol : 1;
    uint32_t de_pol : 1;
    uint32_t xfer_mode : 2;
    uint32_t reserve : 27;
} uart_tcr_t;

typedef enum _uart_work_mode
{
    UART_NORMAL,
    UART_IRDA,
    UART_RS485_FULL_DUPLEX,
    UART_RS485_HALF_DUPLEX,
} uart_work_mode_t;

typedef enum _uart_rs485_rede
{
    UART_RS485_DE,
    UART_RS485_RE,
    UART_RS485_REDE,
} uart_rs485_rede_t;

typedef enum _uart_polarity
{
    UART_LOW,
    UART_HIGH,
} uart_polarity_t;

typedef enum _uart_det_mode
{
    UART_DE_ASSERTION,
    UART_DE_DE_ASSERTION,
    UART_DE_ALL,
} uart_det_mode_t;

typedef struct _uart_det
{
    uint32_t de_assertion_time : 8;
    uint32_t reserve0 : 8;
    uint32_t de_de_assertion_time : 8;
    uint32_t reserve1 : 8;
} uart_det_t;

typedef enum _uart_tat_mode
{
    UART_DE_TO_RE,
    UART_RE_TO_DE,
    UART_TAT_ALL,
} uart_tat_mode_t;

typedef struct _uart_tat
{
    uint32_t de_to_re : 16;
    uint32_t re_to_de : 16;
} uart_tat_t;

/**
 * @brief       Send data from uart
 *
 * @param[in]   channel     Uart index
 * @param[in]   buffer      The data be transfer
 * @param[in]   len         The data length
 *
 * @return      Transfer length
 */
int uart_send_data(uart_device_number_t channel, const char *buffer, size_t buf_len);

/**
 * @brief       Read data from uart
 *
 * @param[in]   channel     Uart index
 * @param[in]   buffer      The Data received
 * @param[in]   len         Receive length
 *
 * @return      Receive length
 */
int uart_receive_data(uart_device_number_t channel, char *buffer, size_t buf_len);

/**
 * @brief       Init uart
 *
 * @param[in]   channel     Uart index
 *
 */
void uart_init(uart_device_number_t channel);

/**
 * @brief       Set uart param
 *
 * @param[in]   channel         Uart index
 * @param[in]   baud_rate       Baudrate
 * @param[in]   data_width      Data width
 * @param[in]   stopbit         Stop bit
 * @param[in]   parity          Odd Even parity
 *
 */
void uart_config(uart_device_number_t channel, uint32_t baud_rate, uart_bitwidth_t data_width, uart_stopbit_t stopbit, uart_parity_t parity);

/**
 * @brief       Set uart param
 *
 * @param[in]   channel         Uart index
 * @param[in]   baud_rate       Baudrate
 * @param[in]   data_width      Data width
 * @param[in]   stopbit         Stop bit
 * @param[in]   parity          Odd Even parity
 *
 */
void uart_configure(uart_device_number_t channel, uint32_t baud_rate, uart_bitwidth_t data_width, uart_stopbit_t stopbit, uart_parity_t parity);

/**
 * @brief       Register uart interrupt
 *
 * @param[in]   channel             Uart index
 * @param[in]   interrupt_mode      Interrupt Mode receive or send
 * @param[in]   uart_callback       Call back
 * @param[in]   ctx                 Param of call back
 * @param[in]   priority            Interrupt priority
 *
 */
void uart_irq_register(uart_device_number_t channel, uart_interrupt_mode_t interrupt_mode, plic_irq_callback_t uart_callback, void *ctx, uint32_t priority);

/**
 * @brief       Deregister uart interrupt
 *
 * @param[in]   channel             Uart index
 * @param[in]   interrupt_mode      Interrupt Mode receive or send
 *
 */
void uart_irq_unregister(uart_device_number_t channel, uart_interrupt_mode_t interrupt_mode);

/**
 * @brief       Set send interrupt threshold
 *
 * @param[in]   channel             Uart index
 * @param[in]   trigger             Threshold of send interrupt
 *
 */
void uart_set_send_trigger(uart_device_number_t channel, uart_send_trigger_t trigger);

/**
 * @brief       Set receive interrupt threshold
 *
 * @param[in]   channel             Uart index
 * @param[in]   trigger             Threshold of receive interrupt
 *
 */
void uart_set_receive_trigger(uart_device_number_t channel, uart_receive_trigger_t trigger);

/**
 * @brief       Send data by dma
 *
 * @param[in]   channel             Uart index
 * @param[in]   dmac_channel        Dmac channel
 * @param[in]   buffer              Send data
 * @param[in]   buf_len             Data length
 *
 */
void uart_send_data_dma(uart_device_number_t uart_channel, dmac_channel_number_t dmac_channel, const uint8_t *buffer, size_t buf_len);

/**
 * @brief       Receive data by dma
 *
 * @param[in]   channel             Uart index
 * @param[in]   dmac_channel        Dmac channel
 * @param[in]   buffer              Receive data
 * @param[in]   buf_len             Data length
 *
 */
void uart_receive_data_dma(uart_device_number_t uart_channel, dmac_channel_number_t dmac_channel, uint8_t *buffer, size_t buf_len);


/**
 * @brief       Send data by dma
 *
 * @param[in]   uart_channel        Uart index
 * @param[in]   dmac_channel        Dmac channel
 * @param[in]   buffer              Send data
 * @param[in]   buf_len             Data length
 * @param[in]   uart_callback       Call back
 * @param[in]   ctx                 Param of call back
 * @param[in]   priority            Interrupt priority
 *
 */
void uart_send_data_dma_irq(uart_device_number_t uart_channel, dmac_channel_number_t dmac_channel,
                            const uint8_t *buffer, size_t buf_len, plic_irq_callback_t uart_callback,
                            void *ctx, uint32_t priority);

/**
 * @brief       Receive data by dma
 *
 * @param[in]   uart_channel        Uart index
 * @param[in]   dmac_channel        Dmac channel
 * @param[in]   buffer              Receive data
 * @param[in]   buf_len             Data length
 * @param[in]   uart_callback       Call back
 * @param[in]   ctx                 Param of call back
 * @param[in]   priority            Interrupt priority
 *
 */
void uart_receive_data_dma_irq(uart_device_number_t uart_channel, dmac_channel_number_t dmac_channel,
                               uint8_t *buffer, size_t buf_len, plic_irq_callback_t uart_callback,
                               void *ctx, uint32_t priority);

/**
 * @brief       Uart handle transfer data operations
 *
 * @param[in]   uart_channel        Uart index
 * @param[in]   data                Uart data information
 * @param[in]   buffer              Uart DMA callback
 *
 */
void uart_handle_data_dma(uart_device_number_t uart_channel ,uart_data_t data, plic_interrupt_t *cb);

/**
 * @brief       Set uart work mode
 *
 * @param[in]   uart_channel        Uart index
 * @param[in]   work_mode           Work mode
                                    0:uart
                                    1: infrared
                                    2:full duplex rs485, control re and de manually
                                    3:half duplex rs485, control re and de automatically
 *
 */
void uart_set_work_mode(uart_device_number_t uart_channel, uart_work_mode_t work_mode);

/**
 * @brief       Set re or de driver enable polarity
 *
 * @param[in]   uart_channel        Uart index
 * @param[in]   rede                re or de
                                    0:de
                                    1:re
                                    2:de and re
 * @param[in]   polarity            Polarity
                                    0:signal is active low
                                    1:signal is active high
 *
 */
void uart_set_rede_polarity(uart_device_number_t uart_channel, uart_rs485_rede_t rede, uart_polarity_t polarity);

/**
 * @brief       Set rs485 de and re signal driver output enable
 *
 * @param[in]   uart_channel        Uart index
 * @param[in]   rede                0:de
                                    1:re
                                    2:de and re
 * @param[in]   enable              0:de-assert signal
                                    1:assert signal
 *
 */
void uart_set_rede_enable(uart_device_number_t uart_channel, uart_rs485_rede_t rede, bool enable);

/**
 * @brief       Set turn around time between switch of 're' and 'de' signals
 *
 * @param[in]   uart_channel        Uart index
 * @param[in]   tat_mode            0:de to re
                                    1:re to de
 * @param[in]   time                turn around time nanosecond
 *
 */
void uart_set_tat(uart_device_number_t uart_channel, uart_tat_mode_t tat_mode, size_t time);

/**
 * @brief       Set driver output enable time used to control de assertion and de-assertion timeing of 'de' signal
 *
 * @param[in]   uart_channel        Uart index
 * @param[in]   det_mode            0:de assertion
                                    1:de de-assertion
 * @param[in]   time                driver output enable time nanosecond
 *
 */
void uart_set_det(uart_device_number_t uart_channel, uart_det_mode_t det_mode, size_t time);

/**
 * @brief       Put a char to UART1
 *
 * @param[in]   c       The char to put
 *
 * @return      result
 *     - Byte   On success, returns the written character.
 *     - EOF    On failure, returns EOF and sets the error indicator (see ferror()) on stdout.
 */
int uart1_putchar(char c);

/**
 * @brief       Get a byte from UART1
 *
 * @return      byte as int type from UART
 *     - Byte   The character read as an unsigned char cast to an int
 *     - EOF    EOF on end of file or error, no enough byte to read
 */
int uart1_getchar(void);

/**
 * @brief       Put a char to UART2
 *
 * @param[in]   c       The char to put
 *
 * @return      result
 *     - Byte   On success, returns the written character.
 *     - EOF    On failure, returns EOF and sets the error indicator (see ferror()) on stdout.
 */
int uart2_putchar(char c);

/**
 * @brief       Get a byte from UART2
 *
 * @return      byte as int type from UART
 *     - Byte   The character read as an unsigned char cast to an int
 *     - EOF    EOF on end of file or error, no enough byte to read
 */
int uart2_getchar(void);

/**
 * @brief       Put a char to UART3
 *
 * @param[in]   c       The char to put
 *
 * @return      result
 *     - Byte   On success, returns the written character.
 *     - EOF    On failure, returns EOF and sets the error indicator (see ferror()) on stdout.
 */
int uart3_putchar(char c);

/**
 * @brief       Get a byte from UART3
 *
 * @return      byte as int type from UART
 *     - Byte   The character read as an unsigned char cast to an int
 *     - EOF    EOF on end of file or error, no enough byte to read
 */
int uart3_getchar(void);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_APBUART_H */
