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
#include <stdlib.h>
#include <stdint.h>
#include "plic.h"
#include "sysctl.h"
#include "uart.h"
#include "utils.h"
#include "atomic.h"

#define __UART_BRATE_CONST  16

volatile uart_t* const  uart[3] =
{
    (volatile uart_t*)UART1_BASE_ADDR,
    (volatile uart_t*)UART2_BASE_ADDR,
    (volatile uart_t*)UART3_BASE_ADDR
};

#define UART_INTERRUPT_SEND                 0x02U
#define UART_INTERRUPT_RECEIVE              0x04U
#define UART_INTERRUPT_CHARACTER_TIMEOUT    0x0CU

typedef struct _uart_interrupt_instance
{
    plic_irq_callback_t callback;
    void *ctx;
} uart_interrupt_instance_t;

typedef struct _uart_instance
{
    uart_interrupt_instance_t uart_receive_instance;
    uart_interrupt_instance_t uart_send_instance;
    uint32_t uart_num;
} uart_instance_t;

uart_instance_t g_uart_instance[3];

typedef struct _uart_dma_instance
{
    uint8_t *buffer;
    size_t buf_len;
    uint32_t *malloc_buffer;
    uart_interrupt_mode_t int_mode;
    dmac_channel_number_t dmac_channel;
    uart_device_number_t uart_num;
    uart_interrupt_instance_t uart_int_instance;
} uart_dma_instance_t;

uart_dma_instance_t uart_send_dma_instance[3];
uart_dma_instance_t uart_recv_dma_instance[3];

typedef struct _uart_instance_dma
{
    uart_device_number_t uart_num;
    uart_interrupt_mode_t transfer_mode;
    dmac_channel_number_t dmac_channel;
    plic_instance_t uart_int_instance;
    spinlock_t lock;
} uart_instance_dma_t;

static uart_instance_dma_t g_uart_send_instance_dma[3];
static uart_instance_dma_t g_uart_recv_instance_dma[3];

volatile int g_write_count = 0;
volatile uint32_t tmp_;
static int uart_irq_callback(void *param)
{
    uart_instance_t *uart_instance = (uart_instance_t *)param;
    uint32_t v_channel = uart_instance->uart_num;
    uint8_t v_int_status = uart[v_channel]->IIR & 0xF;

    if(v_int_status == UART_INTERRUPT_SEND && g_write_count != 0)
    {
        if(uart_instance->uart_send_instance.callback != NULL)
            uart_instance->uart_send_instance.callback(uart_instance->uart_send_instance.ctx);
    }
    else if(v_int_status == UART_INTERRUPT_RECEIVE || v_int_status == UART_INTERRUPT_CHARACTER_TIMEOUT)
    {
        if(uart_instance->uart_receive_instance.callback != NULL)
            uart_instance->uart_receive_instance.callback(uart_instance->uart_receive_instance.ctx);
    }
    else
    {
        tmp_ = uart[v_channel]->LSR;
        tmp_ = uart[v_channel]->USR;
        tmp_ = uart[v_channel]->MSR;
    }
    return 0;
}

int uart_channel_putc(char c, uart_device_number_t channel)
{
    while (uart[channel]->LSR & (1u << 5))
        continue;
    uart[channel]->THR = c;
    return c & 0xff;
}

int uart_channel_getc(uart_device_number_t channel)
{
    /* If received empty */
    if (!(uart[channel]->LSR & 1))
        return EOF;
    else
        return (char)(uart[channel]->RBR & 0xff);
}

int uart1_putchar(char c)
{
    return uart_channel_putc(c, UART_DEVICE_1);
}

int uart1_getchar(void)
{
    return uart_channel_getc(UART_DEVICE_1);
}

int uart2_putchar(char c)
{
    return uart_channel_putc(c, UART_DEVICE_2);
}

int uart2_getchar(void)
{
    return uart_channel_getc(UART_DEVICE_2);
}

int uart3_putchar(char c)
{
    return uart_channel_putc(c, UART_DEVICE_3);
}

int uart3_getchar(void)
{
    return uart_channel_getc(UART_DEVICE_3);
}

static int uart_dma_callback(void *ctx)
{
    uart_dma_instance_t *v_uart_dma_instance = (uart_dma_instance_t *)ctx;
    dmac_channel_number_t dmac_channel = v_uart_dma_instance->dmac_channel;
    dmac_irq_unregister(dmac_channel);

    if(v_uart_dma_instance->int_mode == UART_RECEIVE)
    {
        size_t v_buf_len = v_uart_dma_instance->buf_len;
        uint8_t *v_buffer = v_uart_dma_instance->buffer;
        uint32_t *v_recv_buffer = v_uart_dma_instance->malloc_buffer;
        for(size_t i = 0; i < v_buf_len; i++)
        {
            v_buffer[i] = v_recv_buffer[i];
        }
    }
    free(v_uart_dma_instance->malloc_buffer);
    if(v_uart_dma_instance->uart_int_instance.callback)
        v_uart_dma_instance->uart_int_instance.callback(v_uart_dma_instance->uart_int_instance.ctx);
    return 0;
}

int uart_receive_data(uart_device_number_t channel, char *buffer, size_t buf_len)
{
    size_t i = 0;
    for(i = 0;i < buf_len; i++)
    {
        if(uart[channel]->LSR & 1)
            buffer[i] = (char)(uart[channel]->RBR & 0xff);
        else
            break;
    }
    return i;
}

void uart_receive_data_dma(uart_device_number_t uart_channel, dmac_channel_number_t dmac_channel, uint8_t *buffer, size_t buf_len)
{
    uint32_t *v_recv_buf = malloc(buf_len * sizeof(uint32_t));
        configASSERT(v_recv_buf!=NULL);

    sysctl_dma_select((sysctl_dma_channel_t)dmac_channel, SYSCTL_DMA_SELECT_UART1_RX_REQ + uart_channel * 2);
    dmac_set_single_mode(dmac_channel, (void *)(&uart[uart_channel]->RBR), v_recv_buf, DMAC_ADDR_NOCHANGE, DMAC_ADDR_INCREMENT,
        DMAC_MSIZE_1, DMAC_TRANS_WIDTH_32, buf_len);
    dmac_wait_done(dmac_channel);
    for(uint32_t i = 0; i < buf_len; i++)
    {
        buffer[i] = (uint8_t)(v_recv_buf[i] & 0xff);
    }
    free(v_recv_buf);
}

void uart_receive_data_dma_irq(uart_device_number_t uart_channel, dmac_channel_number_t dmac_channel,
                                     uint8_t *buffer, size_t buf_len, plic_irq_callback_t uart_callback,
                                     void *ctx, uint32_t priority)
{
    uint32_t *v_recv_buf = malloc(buf_len * sizeof(uint32_t));
        configASSERT(v_recv_buf!=NULL);

    uart_recv_dma_instance[uart_channel].dmac_channel = dmac_channel;
    uart_recv_dma_instance[uart_channel].uart_num = uart_channel;
    uart_recv_dma_instance[uart_channel].malloc_buffer = v_recv_buf;
    uart_recv_dma_instance[uart_channel].buffer = buffer;
    uart_recv_dma_instance[uart_channel].buf_len = buf_len;
    uart_recv_dma_instance[uart_channel].int_mode = UART_RECEIVE;
    uart_recv_dma_instance[uart_channel].uart_int_instance.callback = uart_callback;
    uart_recv_dma_instance[uart_channel].uart_int_instance.ctx = ctx;

    dmac_irq_register(dmac_channel, uart_dma_callback, &uart_recv_dma_instance[uart_channel], priority);
    sysctl_dma_select((sysctl_dma_channel_t)dmac_channel, SYSCTL_DMA_SELECT_UART1_RX_REQ + uart_channel * 2);
    dmac_set_single_mode(dmac_channel, (void *)(&uart[uart_channel]->RBR), v_recv_buf, DMAC_ADDR_NOCHANGE, DMAC_ADDR_INCREMENT,
        DMAC_MSIZE_1, DMAC_TRANS_WIDTH_32, buf_len);
}

int uart_send_data(uart_device_number_t channel, const char *buffer, size_t buf_len)
{
    g_write_count = 0;
    while (g_write_count < buf_len)
    {
        uart_channel_putc(*buffer++, channel);
        g_write_count++;
    }
    return g_write_count;
}

void uart_send_data_dma(uart_device_number_t uart_channel, dmac_channel_number_t dmac_channel, const uint8_t *buffer, size_t buf_len)
{
    uint32_t *v_send_buf = malloc(buf_len * sizeof(uint32_t));
    configASSERT(v_send_buf!=NULL);
    for(uint32_t i = 0; i < buf_len; i++)
        v_send_buf[i] = buffer[i];
    sysctl_dma_select((sysctl_dma_channel_t)dmac_channel, SYSCTL_DMA_SELECT_UART1_TX_REQ + uart_channel * 2);
    dmac_set_single_mode(dmac_channel, v_send_buf, (void *)(&uart[uart_channel]->THR), DMAC_ADDR_INCREMENT, DMAC_ADDR_NOCHANGE,
        DMAC_MSIZE_1, DMAC_TRANS_WIDTH_32, buf_len);
    dmac_wait_done(dmac_channel);
    free((void *)v_send_buf);
}

void uart_send_data_dma_irq(uart_device_number_t uart_channel, dmac_channel_number_t dmac_channel,
                            const uint8_t *buffer, size_t buf_len, plic_irq_callback_t uart_callback,
                            void *ctx, uint32_t priority)
{
    uint32_t *v_send_buf = malloc(buf_len * sizeof(uint32_t));
    configASSERT(v_send_buf!=NULL);

    uart_send_dma_instance[uart_channel] = (uart_dma_instance_t) {
        .dmac_channel = dmac_channel,
        .uart_num = uart_channel,
        .malloc_buffer = v_send_buf,
        .buffer = (uint8_t *)buffer,
        .buf_len = buf_len,
        .int_mode = UART_SEND,
        .uart_int_instance.callback = uart_callback,
        .uart_int_instance.ctx = ctx,
    };

    for(uint32_t i = 0; i < buf_len; i++)
        v_send_buf[i] = buffer[i];
    dmac_irq_register(dmac_channel, uart_dma_callback, &uart_send_dma_instance[uart_channel], priority);
    sysctl_dma_select((sysctl_dma_channel_t)dmac_channel, SYSCTL_DMA_SELECT_UART1_TX_REQ + uart_channel * 2);
    dmac_set_single_mode(dmac_channel, v_send_buf, (void *)(&uart[uart_channel]->THR), DMAC_ADDR_INCREMENT, DMAC_ADDR_NOCHANGE,
        DMAC_MSIZE_1, DMAC_TRANS_WIDTH_32, buf_len);

}

void uart_configure(uart_device_number_t channel, uint32_t baud_rate, uart_bitwidth_t data_width, uart_stopbit_t stopbit, uart_parity_t parity)
{
    configASSERT(data_width >= 5 && data_width <= 8);
    if (data_width == 5)
    {
        configASSERT(stopbit != UART_STOP_2);
    }
    else
    {
        configASSERT(stopbit != UART_STOP_1_5);
    }

    uint32_t stopbit_val = stopbit == UART_STOP_1 ? 0 : 1;
    uint32_t parity_val;
    switch (parity)
    {
        case UART_PARITY_NONE:
            parity_val = 0;
            break;
        case UART_PARITY_ODD:
            parity_val = 1;
            break;
        case UART_PARITY_EVEN:
            parity_val = 3;
            break;
        default:
            configASSERT(!"Invalid parity");
            break;
    }

    uint32_t freq = sysctl_clock_get_freq(SYSCTL_CLOCK_APB0);
    uint32_t divisor = freq / baud_rate;
    uint8_t dlh = divisor >> 12;
    uint8_t dll = (divisor - (dlh << 12)) / __UART_BRATE_CONST;
    uint8_t dlf = divisor - (dlh << 12) - dll * __UART_BRATE_CONST;

    /* Set UART registers */

    uart[channel]->LCR |= 1u << 7;
    uart[channel]->DLH = dlh;
    uart[channel]->DLL = dll;
    uart[channel]->DLF = dlf;
    uart[channel]->LCR = 0;
    uart[channel]->LCR = (data_width - 5) | (stopbit_val << 2) | (parity_val << 3);
    uart[channel]->LCR &= ~(1u << 7);
    uart[channel]->IER |= 0x80; /* THRE */
    uart[channel]->FCR = UART_RECEIVE_FIFO_1 << 6 | UART_SEND_FIFO_8 << 4 | 0x1 << 3 | 0x1;
}

void __attribute__((weak, alias("uart_configure")))
uart_config(uart_device_number_t channel, uint32_t baud_rate, uart_bitwidth_t data_width, uart_stopbit_t stopbit, uart_parity_t parity);

void uart_init(uart_device_number_t channel)
{
    sysctl_clock_enable(SYSCTL_CLOCK_UART1 + channel);
}

void uart_set_send_trigger(uart_device_number_t channel, uart_send_trigger_t trigger)
{
    uart[channel]->STET = trigger;
}

void uart_set_receive_trigger(uart_device_number_t channel, uart_receive_trigger_t trigger)
{
    uart[channel]->SRT = trigger;
}

void uart_irq_register(uart_device_number_t channel, uart_interrupt_mode_t interrupt_mode, plic_irq_callback_t uart_callback, void *ctx, uint32_t priority)
{
    if(interrupt_mode == UART_SEND)
    {
        uart[channel]->IER |= 0x2;
        g_uart_instance[channel].uart_send_instance.callback = uart_callback;
        g_uart_instance[channel].uart_send_instance.ctx = ctx;
    }
    else if(interrupt_mode == UART_RECEIVE)
    {
        uart[channel]->IER |= 0x1;
        g_uart_instance[channel].uart_receive_instance.callback = uart_callback;
        g_uart_instance[channel].uart_receive_instance.ctx = ctx;
    }
    g_uart_instance[channel].uart_num = channel;
    plic_set_priority(IRQN_UART1_INTERRUPT + channel, priority);
    plic_irq_register(IRQN_UART1_INTERRUPT + channel, uart_irq_callback, &g_uart_instance[channel]);
    plic_irq_enable(IRQN_UART1_INTERRUPT + channel);
}

void uart_irq_unregister(uart_device_number_t channel, uart_interrupt_mode_t interrupt_mode)
{
    if(interrupt_mode == UART_SEND)
    {
        uart[channel]->IER &= ~(0x2);
        g_uart_instance[channel].uart_send_instance.callback = NULL;
        g_uart_instance[channel].uart_send_instance.ctx = NULL;
    }
    else if(interrupt_mode == UART_RECEIVE)
    {
        uart[channel]->IER &= ~(0x1);
        g_uart_instance[channel].uart_receive_instance.callback = NULL;
        g_uart_instance[channel].uart_receive_instance.ctx = NULL;
    }
    if(uart[channel]->IER == 0)
    {
        plic_irq_unregister(IRQN_UART1_INTERRUPT + channel);
    }
}

int uart_dma_irq(void *ctx)
{
    uart_instance_dma_t *v_instance = (uart_instance_dma_t *)ctx;
    dmac_irq_unregister(v_instance->dmac_channel);
    if(v_instance->transfer_mode == UART_SEND)
    {
        while(!(uart[v_instance->uart_num]->LSR & (1u << 6)));
    }
    spinlock_unlock(&v_instance->lock);
    if(v_instance->uart_int_instance.callback)
    {
        v_instance->uart_int_instance.callback(v_instance->uart_int_instance.ctx);
    }
    return 0;
}

void uart_handle_data_dma(uart_device_number_t uart_channel ,uart_data_t data, plic_interrupt_t *cb)
{
    configASSERT(uart_channel < UART_DEVICE_MAX);
    if(data.transfer_mode == UART_SEND)
    {
        configASSERT(data.tx_buf && data.tx_len && data.tx_channel < DMAC_CHANNEL_MAX);
        spinlock_lock(&g_uart_send_instance_dma[uart_channel].lock);
        if(cb)
        {
            g_uart_send_instance_dma[uart_channel].uart_int_instance.callback = cb->callback;
            g_uart_send_instance_dma[uart_channel].uart_int_instance.ctx = cb->ctx;
            g_uart_send_instance_dma[uart_channel].dmac_channel = data.tx_channel;
            g_uart_send_instance_dma[uart_channel].transfer_mode = UART_SEND;
            dmac_irq_register(data.tx_channel, uart_dma_irq, &g_uart_send_instance_dma[uart_channel], cb->priority);
        }
        sysctl_dma_select((sysctl_dma_channel_t)data.tx_channel, SYSCTL_DMA_SELECT_UART1_TX_REQ + uart_channel * 2);
        dmac_set_single_mode(data.tx_channel, data.tx_buf, (void *)(&uart[uart_channel]->THR), DMAC_ADDR_INCREMENT, DMAC_ADDR_NOCHANGE,
            DMAC_MSIZE_1, DMAC_TRANS_WIDTH_32, data.tx_len);
        if(!cb)
        {
            dmac_wait_done(data.tx_channel);
            while(!(uart[uart_channel]->LSR & (1u << 6)));
            spinlock_unlock(&g_uart_send_instance_dma[uart_channel].lock);
        }
    }
    else
    {
        configASSERT(data.rx_buf && data.rx_len && data.rx_channel < DMAC_CHANNEL_MAX);
        spinlock_lock(&g_uart_recv_instance_dma[uart_channel].lock);
        if(cb)
        {
            g_uart_recv_instance_dma[uart_channel].uart_int_instance.callback = cb->callback;
            g_uart_recv_instance_dma[uart_channel].uart_int_instance.ctx = cb->ctx;
            g_uart_recv_instance_dma[uart_channel].dmac_channel = data.rx_channel;
            g_uart_recv_instance_dma[uart_channel].transfer_mode = UART_RECEIVE;
            dmac_irq_register(data.rx_channel, uart_dma_irq, &g_uart_recv_instance_dma[uart_channel], cb->priority);
        }
        sysctl_dma_select((sysctl_dma_channel_t)data.rx_channel, SYSCTL_DMA_SELECT_UART1_RX_REQ + uart_channel * 2);
        dmac_set_single_mode(data.rx_channel, (void *)(&uart[uart_channel]->RBR), data.rx_buf, DMAC_ADDR_NOCHANGE, DMAC_ADDR_INCREMENT,
                DMAC_MSIZE_1, DMAC_TRANS_WIDTH_32, data.rx_len);
        if(!cb)
        {
            dmac_wait_done(data.rx_channel);
            spinlock_unlock(&g_uart_recv_instance_dma[uart_channel].lock);
        }
    }
}

void uart_set_work_mode(uart_device_number_t uart_channel, uart_work_mode_t work_mode)
{
    volatile uart_tcr_t *tcr;
    switch(work_mode)
    {
        case UART_IRDA:
            uart[uart_channel]->MCR |= (1 << 6);
            break;
        case UART_RS485_FULL_DUPLEX:
            tcr = (uart_tcr_t *)&uart[uart_channel]->TCR;
            tcr->xfer_mode = 0;
            tcr->rs485_en = 1;
            break;
        case UART_RS485_HALF_DUPLEX:
            tcr = (uart_tcr_t *)&uart[uart_channel]->TCR;
            tcr->xfer_mode = 2;
            tcr->rs485_en = 1;
            uart[uart_channel]->DE_EN = 1;
            uart[uart_channel]->RE_EN = 1;
            break;
        default:
            uart[uart_channel]->MCR &= ~(1 << 6);
            uart[uart_channel]->TCR &= ~(1 << 0);
            break;
    }
}

void uart_set_rede_polarity(uart_device_number_t uart_channel, uart_rs485_rede_t rede, uart_polarity_t polarity)
{
    volatile uart_tcr_t *tcr = (uart_tcr_t *)&uart[uart_channel]->TCR;
    switch(rede)
    {
        case UART_RS485_DE:
            tcr->de_pol = polarity;
            break;
        case UART_RS485_RE:
            tcr->re_pol = polarity;
            break;
        default:
            tcr->de_pol = polarity;
            tcr->re_pol = polarity;
            break;
    }
}

void uart_set_rede_enable(uart_device_number_t uart_channel, uart_rs485_rede_t rede, bool enable)
{
    switch(rede)
    {
        case UART_RS485_DE:
            uart[uart_channel]->DE_EN = enable;
            break;
        case UART_RS485_RE:
            uart[uart_channel]->RE_EN = enable;
            break;
        default:
            uart[uart_channel]->DE_EN = enable;
            uart[uart_channel]->RE_EN = enable;
            break;
    }
}

void uart_set_det(uart_device_number_t uart_channel, uart_det_mode_t det_mode, size_t time)
{
    volatile uart_det_t *det = (uart_det_t *)&uart[uart_channel]->DET;
    uint32_t freq = sysctl_clock_get_freq(SYSCTL_CLOCK_APB0);
    uint32_t v_clk_cnt = time * freq / 1e9 + 1;
    if(v_clk_cnt > 255)
        v_clk_cnt = 255;
    switch(det_mode)
    {
        case UART_DE_ASSERTION:
            det ->de_assertion_time = v_clk_cnt;
            break;
        case UART_DE_DE_ASSERTION:
            det->de_de_assertion_time = v_clk_cnt;
            break;
        default:
            det ->de_assertion_time = v_clk_cnt;
            det->de_de_assertion_time = v_clk_cnt;
            break;
    }
}

void uart_set_tat(uart_device_number_t uart_channel, uart_tat_mode_t tat_mode, size_t time)
{
    volatile uart_tat_t *tat = (uart_tat_t *)&uart[uart_channel]->TAT;
    uint32_t freq = sysctl_clock_get_freq(SYSCTL_CLOCK_APB0);
    uint32_t v_clk_cnt = time * freq / 1e9 + 1;
    if(v_clk_cnt > 65536)
        v_clk_cnt = 65536;
    switch(tat_mode)
    {
        case UART_DE_TO_RE:
            tat->de_to_re = v_clk_cnt - 1;
            break;
        case UART_RE_TO_DE:
            tat->re_to_de = v_clk_cnt - 1;
            break;
        default:
            tat->de_to_re = v_clk_cnt - 1;
            tat->re_to_de = v_clk_cnt - 1;
            break;
    }
}

