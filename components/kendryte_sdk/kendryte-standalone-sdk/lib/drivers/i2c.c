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
#include <stddef.h>
#include "i2c.h"
#include "utils.h"
#include "fpioa.h"
#include "platform.h"
#include "stdlib.h"
#include "string.h"
#include "sysctl.h"
#include "bsp.h"

typedef struct _i2c_slave_instance
{
    uint32_t i2c_num;
    const i2c_slave_handler_t *slave_handler;
} i2c_slave_instance_t;

static i2c_slave_instance_t slave_instance[I2C_MAX_NUM];

typedef struct _i2c_instance
{
    i2c_device_number_t i2c_num;
    i2c_transfer_mode_t transfer_mode;
    dmac_channel_number_t dmac_channel;
    plic_instance_t i2c_int_instance;
    spinlock_t lock;
} i2c_instance_t;

static i2c_instance_t g_i2c_instance[3];

volatile i2c_t* const i2c[3] =
{
    (volatile i2c_t*)I2C0_BASE_ADDR,
    (volatile i2c_t*)I2C1_BASE_ADDR,
    (volatile i2c_t*)I2C2_BASE_ADDR
};

static void i2c_clk_init(i2c_device_number_t i2c_num)
{
    configASSERT(i2c_num < I2C_MAX_NUM);
    sysctl_clock_enable(SYSCTL_CLOCK_I2C0 + i2c_num);
    sysctl_clock_set_threshold(SYSCTL_THRESHOLD_I2C0 + i2c_num, 3);
}

void i2c_init(i2c_device_number_t i2c_num, uint32_t slave_address, uint32_t address_width,
              uint32_t i2c_clk)
{
    configASSERT(i2c_num < I2C_MAX_NUM);
    configASSERT(address_width == 7 || address_width == 10);

    volatile i2c_t *i2c_adapter = i2c[i2c_num];

    i2c_clk_init(i2c_num);

    uint32_t v_i2c_freq = sysctl_clock_get_freq(SYSCTL_CLOCK_I2C0 + i2c_num);
    uint16_t v_period_clk_cnt = v_i2c_freq / i2c_clk / 2;

    if(v_period_clk_cnt == 0)
        v_period_clk_cnt = 1;

    i2c_adapter->enable = 0;
    i2c_adapter->con = I2C_CON_MASTER_MODE | I2C_CON_SLAVE_DISABLE | I2C_CON_RESTART_EN |
                       (address_width == 10 ? I2C_CON_10BITADDR_SLAVE : 0) | I2C_CON_SPEED(1);
    i2c_adapter->ss_scl_hcnt = I2C_SS_SCL_HCNT_COUNT(v_period_clk_cnt);
    i2c_adapter->ss_scl_lcnt = I2C_SS_SCL_LCNT_COUNT(v_period_clk_cnt);

    i2c_adapter->tar = I2C_TAR_ADDRESS(slave_address);
    i2c_adapter->intr_mask = 0;
    i2c_adapter->dma_cr = 0x3;
    i2c_adapter->dma_rdlr = 0;
    i2c_adapter->dma_tdlr = 4;
    i2c_adapter->enable = I2C_ENABLE_ENABLE;
}

static int i2c_slave_irq(void *userdata)
{
    i2c_slave_instance_t *instance = (i2c_slave_instance_t *)userdata;
    volatile i2c_t *i2c_adapter = i2c[instance->i2c_num];
    uint32_t status = i2c_adapter->intr_stat;
    if (status & I2C_INTR_STAT_START_DET)
    {
        instance->slave_handler->on_event(I2C_EV_START);
        readl(&i2c_adapter->clr_start_det);
    }
    if (status & I2C_INTR_STAT_STOP_DET)
    {
        instance->slave_handler->on_event(I2C_EV_STOP);
        readl(&i2c_adapter->clr_stop_det);
    }
    if (status & I2C_INTR_STAT_RX_FULL)
    {
        instance->slave_handler->on_receive(i2c_adapter->data_cmd);
    }
    if (status & I2C_INTR_STAT_RD_REQ)
    {
        i2c_adapter->data_cmd = instance->slave_handler->on_transmit();
        readl(&i2c_adapter->clr_rd_req);
    }
    return 0;
}

void i2c_init_as_slave(i2c_device_number_t i2c_num, uint32_t slave_address, uint32_t address_width,
    const i2c_slave_handler_t *handler)
{
    configASSERT(address_width == 7 || address_width == 10);
    volatile i2c_t *i2c_adapter = i2c[i2c_num];
    slave_instance[i2c_num].i2c_num = i2c_num;
    slave_instance[i2c_num].slave_handler = handler;

    i2c_clk_init(i2c_num);
    i2c_adapter->enable = 0;
    i2c_adapter->con = (address_width == 10 ? I2C_CON_10BITADDR_SLAVE : 0) | I2C_CON_SPEED(1) | I2C_CON_STOP_DET_IFADDRESSED;
    i2c_adapter->ss_scl_hcnt = I2C_SS_SCL_HCNT_COUNT(37);
    i2c_adapter->ss_scl_lcnt = I2C_SS_SCL_LCNT_COUNT(40);
    i2c_adapter->sar = I2C_SAR_ADDRESS(slave_address);
    i2c_adapter->rx_tl = I2C_RX_TL_VALUE(0);
    i2c_adapter->tx_tl = I2C_TX_TL_VALUE(0);
    i2c_adapter->intr_mask = I2C_INTR_MASK_RX_FULL | I2C_INTR_MASK_START_DET | I2C_INTR_MASK_STOP_DET | I2C_INTR_MASK_RD_REQ;

    plic_set_priority(IRQN_I2C0_INTERRUPT + i2c_num, 1);
    plic_irq_register(IRQN_I2C0_INTERRUPT + i2c_num, i2c_slave_irq, slave_instance + i2c_num);
    plic_irq_enable(IRQN_I2C0_INTERRUPT + i2c_num);

    i2c_adapter->enable = I2C_ENABLE_ENABLE;
}

int i2c_send_data(i2c_device_number_t i2c_num, const uint8_t *send_buf, size_t send_buf_len)
{
    configASSERT(i2c_num < I2C_MAX_NUM);
    volatile i2c_t* i2c_adapter = i2c[i2c_num];
    size_t fifo_len, index;
    i2c_adapter->clr_tx_abrt = i2c_adapter->clr_tx_abrt;
    while (send_buf_len)
    {
        fifo_len = 8 - i2c_adapter->txflr;
        fifo_len = send_buf_len < fifo_len ? send_buf_len : fifo_len;
        for (index = 0; index < fifo_len; index++)
            i2c_adapter->data_cmd = I2C_DATA_CMD_DATA(*send_buf++);
        if (i2c_adapter->tx_abrt_source != 0)
            return 1;
        send_buf_len -= fifo_len;
    }
    while ((i2c_adapter->status & I2C_STATUS_ACTIVITY) || !(i2c_adapter->status & I2C_STATUS_TFE))
        ;

    if (i2c_adapter->tx_abrt_source != 0)
        return 1;

    return 0;
}

void i2c_send_data_dma(dmac_channel_number_t dma_channel_num, i2c_device_number_t i2c_num, const uint8_t *send_buf,
                       size_t send_buf_len)
{
    configASSERT(i2c_num < I2C_MAX_NUM);
    volatile i2c_t* i2c_adapter = i2c[i2c_num];
    i2c_adapter->clr_tx_abrt = i2c_adapter->clr_tx_abrt;
    uint32_t *buf = malloc(send_buf_len * sizeof(uint32_t));
    int i;
    for (i = 0; i < send_buf_len; i++)
    {
        buf[i] = send_buf[i];
    }

    sysctl_dma_select((sysctl_dma_channel_t)dma_channel_num, SYSCTL_DMA_SELECT_I2C0_TX_REQ + i2c_num * 2);
    dmac_set_single_mode(dma_channel_num, buf, (void *)(&i2c_adapter->data_cmd), DMAC_ADDR_INCREMENT, DMAC_ADDR_NOCHANGE,
        DMAC_MSIZE_4, DMAC_TRANS_WIDTH_32, send_buf_len);

    dmac_wait_done(dma_channel_num);
    free((void *)buf);

    while ((i2c_adapter->status & I2C_STATUS_ACTIVITY) || !(i2c_adapter->status & I2C_STATUS_TFE))
    {
        if (i2c_adapter->tx_abrt_source != 0)
            return;
    }
}

int i2c_recv_data(i2c_device_number_t i2c_num, const uint8_t *send_buf, size_t send_buf_len, uint8_t *receive_buf,
                  size_t receive_buf_len)
{
    configASSERT(i2c_num < I2C_MAX_NUM);

    size_t fifo_len, index;
    size_t rx_len = receive_buf_len;
    volatile i2c_t* i2c_adapter = i2c[i2c_num];

    while (send_buf_len)
    {
        fifo_len = 8 - i2c_adapter->txflr;
        fifo_len = send_buf_len < fifo_len ? send_buf_len : fifo_len;
        for (index = 0; index < fifo_len; index++)
            i2c_adapter->data_cmd = I2C_DATA_CMD_DATA(*send_buf++);
        if (i2c_adapter->tx_abrt_source != 0)
            return 1;
        send_buf_len -= fifo_len;
    }

    while (receive_buf_len || rx_len)
    {
        fifo_len = i2c_adapter->rxflr;
        fifo_len = rx_len < fifo_len ? rx_len : fifo_len;
        for (index = 0; index < fifo_len; index++)
            *receive_buf++ = (uint8_t)i2c_adapter->data_cmd;
        rx_len -= fifo_len;
        fifo_len = 8 - i2c_adapter->txflr;
        fifo_len = receive_buf_len < fifo_len ? receive_buf_len : fifo_len;
        for (index = 0; index < fifo_len; index++)
            i2c_adapter->data_cmd = I2C_DATA_CMD_CMD;
        if (i2c_adapter->tx_abrt_source != 0)
            return 1;
        receive_buf_len -= fifo_len;
    }
    return 0;
}

void i2c_recv_data_dma(dmac_channel_number_t dma_send_channel_num, dmac_channel_number_t dma_receive_channel_num,
                       i2c_device_number_t i2c_num, const uint8_t *send_buf, size_t send_buf_len,
                       uint8_t *receive_buf, size_t receive_buf_len)
{
    configASSERT(i2c_num < I2C_MAX_NUM);

    volatile i2c_t* i2c_adapter = i2c[i2c_num];

    uint32_t *write_cmd = malloc(sizeof(uint32_t) * (send_buf_len + receive_buf_len));
    size_t i;
    for(i = 0; i < send_buf_len; i++)
        write_cmd[i] = *send_buf++;
    for (i = 0; i < receive_buf_len; i++)
        write_cmd[i + send_buf_len] = I2C_DATA_CMD_CMD;

    sysctl_dma_select((sysctl_dma_channel_t)dma_send_channel_num, SYSCTL_DMA_SELECT_I2C0_TX_REQ + i2c_num * 2);
    sysctl_dma_select((sysctl_dma_channel_t)dma_receive_channel_num, SYSCTL_DMA_SELECT_I2C0_RX_REQ + i2c_num * 2);

    dmac_set_single_mode(dma_receive_channel_num, (void *)(&i2c_adapter->data_cmd), write_cmd, DMAC_ADDR_NOCHANGE,
         DMAC_ADDR_INCREMENT,DMAC_MSIZE_1, DMAC_TRANS_WIDTH_32, receive_buf_len);

    dmac_set_single_mode(dma_send_channel_num, write_cmd, (void *)(&i2c_adapter->data_cmd), DMAC_ADDR_INCREMENT,
         DMAC_ADDR_NOCHANGE,DMAC_MSIZE_4, DMAC_TRANS_WIDTH_32, receive_buf_len + send_buf_len);

    dmac_wait_done(dma_send_channel_num);
    dmac_wait_done(dma_receive_channel_num);

    for (i = 0; i < receive_buf_len; i++)
    {
        receive_buf[i] = (uint8_t)write_cmd[i];
    }

    free(write_cmd);
}

static int i2c_dma_irq(void *ctx)
{
    i2c_instance_t *v_instance = (i2c_instance_t *)ctx;
    volatile i2c_t* i2c_adapter = i2c[v_instance->i2c_num];
    dmac_irq_unregister(v_instance->dmac_channel);
    if(v_instance->transfer_mode == I2C_SEND)
    {
        while ((i2c_adapter->status & I2C_STATUS_ACTIVITY) || !(i2c_adapter->status & I2C_STATUS_TFE))
        {
            if (i2c_adapter->tx_abrt_source != 0)
            {
                spinlock_unlock(&v_instance->lock);
                return -1;
            }
        }
    }
    spinlock_unlock(&v_instance->lock);
    if(v_instance->i2c_int_instance.callback)
    {
        v_instance->i2c_int_instance.callback(v_instance->i2c_int_instance.ctx);
    }
    return 0;
}

void i2c_handle_data_dma(i2c_device_number_t i2c_num, i2c_data_t data, plic_interrupt_t *cb)
{
    configASSERT(i2c_num < I2C_MAX_NUM);
    configASSERT(data.tx_channel < DMAC_CHANNEL_MAX && data.rx_channel < DMAC_CHANNEL_MAX);

    spinlock_lock(&g_i2c_instance[i2c_num].lock);
    if(cb)
    {
        g_i2c_instance[i2c_num].i2c_int_instance.callback = cb->callback;
        g_i2c_instance[i2c_num].i2c_int_instance.ctx = cb->ctx;
    }

    volatile i2c_t* i2c_adapter = i2c[i2c_num];
    if(data.transfer_mode == I2C_SEND)
    {
        configASSERT(data.tx_buf && data.tx_len);

        i2c_adapter->clr_tx_abrt = i2c_adapter->clr_tx_abrt;
        if(cb)
        {
            g_i2c_instance[i2c_num].dmac_channel = data.tx_channel;
            g_i2c_instance[i2c_num].transfer_mode = I2C_SEND;
            dmac_irq_register(data.tx_channel, i2c_dma_irq, &g_i2c_instance[i2c_num], cb->priority);
        }
        sysctl_dma_select((sysctl_dma_channel_t)data.tx_channel, SYSCTL_DMA_SELECT_I2C0_TX_REQ + i2c_num * 2);
        dmac_set_single_mode(data.tx_channel, data.tx_buf, (void *)(&i2c_adapter->data_cmd), DMAC_ADDR_INCREMENT, DMAC_ADDR_NOCHANGE,
            DMAC_MSIZE_4, DMAC_TRANS_WIDTH_32, data.tx_len);
        if(!cb)
        {
            dmac_wait_done(data.tx_channel);
            while ((i2c_adapter->status & I2C_STATUS_ACTIVITY) || !(i2c_adapter->status & I2C_STATUS_TFE))
            {
                if (i2c_adapter->tx_abrt_source != 0)
                    configASSERT(!"source abort");
            }
        }
    }
    else
    {
        configASSERT(data.rx_buf && data.rx_len);
        if(data.tx_len)
            configASSERT(data.tx_buf);
        if(cb)
        {
            g_i2c_instance[i2c_num].dmac_channel = data.rx_channel;
            g_i2c_instance[i2c_num].transfer_mode = I2C_RECEIVE;
            dmac_irq_register(data.rx_channel, i2c_dma_irq, &g_i2c_instance[i2c_num], cb->priority);
        }
        sysctl_dma_select((sysctl_dma_channel_t)data.rx_channel, SYSCTL_DMA_SELECT_I2C0_RX_REQ + i2c_num * 2);
        dmac_set_single_mode(data.rx_channel, (void *)(&i2c_adapter->data_cmd), data.rx_buf, DMAC_ADDR_NOCHANGE,
             DMAC_ADDR_INCREMENT,DMAC_MSIZE_1, DMAC_TRANS_WIDTH_32, data.rx_len);

        sysctl_dma_select((sysctl_dma_channel_t)data.tx_channel, SYSCTL_DMA_SELECT_I2C0_TX_REQ + i2c_num * 2);
        if(data.tx_len)
        {
            configASSERT(data.tx_buf);
            dmac_set_single_mode(data.tx_channel, data.tx_buf, (void *)(&i2c_adapter->data_cmd), DMAC_ADDR_INCREMENT,
                 DMAC_ADDR_NOCHANGE,DMAC_MSIZE_4, DMAC_TRANS_WIDTH_32, data.tx_len);
            dmac_wait_done(data.tx_channel);
        }
        static uint32_t s_read_cmd = I2C_DATA_CMD_CMD;
        dmac_set_single_mode(data.tx_channel, &s_read_cmd, (void *)(&i2c_adapter->data_cmd), DMAC_ADDR_NOCHANGE,
                     DMAC_ADDR_NOCHANGE,DMAC_MSIZE_1, DMAC_TRANS_WIDTH_32, data.rx_len);

        if(!cb)
        {
            dmac_wait_done(data.tx_channel);
            dmac_wait_done(data.rx_channel);
        }
    }
    if(!cb)
        spinlock_unlock(&g_i2c_instance[i2c_num].lock);
}
