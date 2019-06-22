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
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "i2s.h"
#include "sysctl.h"
#include "stdlib.h"
#include "utils.h"

volatile i2s_t *const i2s[3] =
{
    (volatile i2s_t *)I2S0_BASE_ADDR,
    (volatile i2s_t *)I2S1_BASE_ADDR,
    (volatile i2s_t *)I2S2_BASE_ADDR
};

typedef struct _i2s_instance
{
    i2s_device_number_t i2s_num;
    i2s_transfer_mode_t transfer_mode;
    dmac_channel_number_t dmac_channel;
    plic_instance_t i2s_int_instance;
} i2s_instance_t;

static i2s_instance_t g_i2s_send_instance[3];
static i2s_instance_t g_i2s_recv_instance[3];

static int i2s_recv_channel_enable(i2s_device_number_t device_num,
                            i2s_channel_num_t channel_num, uint32_t enable)
{
    rer_t u_rer;

    if (channel_num < I2S_CHANNEL_0 || channel_num > I2S_CHANNEL_3)
        return -1;
    u_rer.reg_data = readl(&i2s[device_num]->channel[channel_num].rer);
    u_rer.rer.rxchenx = enable;
    writel(u_rer.reg_data, &i2s[device_num]->channel[channel_num].rer);
    return 0;
}

static int i2s_transmit_channel_enable(i2s_device_number_t device_num,
    i2s_channel_num_t channel_num, uint32_t enable)
{
    ter_t u_ter;

    if (channel_num < I2S_CHANNEL_0 || channel_num > I2S_CHANNEL_3)
        return -1;

    u_ter.reg_data = readl(&i2s[device_num]->channel[channel_num].ter);
    u_ter.ter.txchenx = enable;
    writel(u_ter.reg_data, &i2s[device_num]->channel[channel_num].ter);
    return 0;
}

static void i2s_receive_enable(i2s_device_number_t device_num, i2s_channel_num_t channel_num)
{
    irer_t u_irer;

    u_irer.reg_data = readl(&i2s[device_num]->irer);
    u_irer.irer.rxen = 1;
    writel(u_irer.reg_data, &i2s[device_num]->irer);
    /* Receiver block enable */

    i2s_recv_channel_enable(device_num, channel_num, 1);
    /* Receive channel enable */
}

static void i2s_transimit_enable(i2s_device_number_t device_num, i2s_channel_num_t channel_num)
{
    iter_t u_iter;

    u_iter.reg_data = readl(&i2s[device_num]->iter);
    u_iter.iter.txen = 1;
    writel(u_iter.reg_data, &i2s[device_num]->iter);
    /* Transmitter block enable */

    i2s_transmit_channel_enable(device_num, channel_num, 1);
    /* Transmit channel enable */
}

static void i2s_set_enable(i2s_device_number_t device_num, uint32_t enable)
{
    ier_t u_ier;

    u_ier.reg_data = readl(&i2s[device_num]->ier);
    u_ier.ier.ien = enable;
    writel(u_ier.reg_data, &i2s[device_num]->ier);
}

static void i2s_disable_block(i2s_device_number_t device_num, i2s_transmit_t rxtx_mode)
{
    irer_t u_irer;
    iter_t u_iter;

    if (rxtx_mode == I2S_RECEIVER)
    {
        u_irer.reg_data = readl(&i2s[device_num]->irer);
        u_irer.irer.rxen = 0;
        writel(u_irer.reg_data, &i2s[device_num]->irer);
        /* Receiver block disable */
    }
    else
    {
        u_iter.reg_data = readl(&i2s[device_num]->iter);
        u_iter.iter.txen = 0;
        writel(u_iter.reg_data, &i2s[device_num]->iter);
        /* Transmitter block disable */
    }
}

static int i2s_set_rx_word_length(i2s_device_number_t device_num,
               i2s_word_length_t word_length,
               i2s_channel_num_t channel_num)
{
    rcr_tcr_t u_rcr;

    if (word_length > RESOLUTION_32_BIT || word_length < IGNORE_WORD_LENGTH)
        return -1;
    if (channel_num < I2S_CHANNEL_0 || channel_num > I2S_CHANNEL_3)
        return -1;

    u_rcr.reg_data = readl(&i2s[device_num]->channel[channel_num].rcr);
    u_rcr.rcr_tcr.wlen = word_length;
    writel(u_rcr.reg_data, &i2s[device_num]->channel[channel_num].rcr);
    return 0;
}

static int i2s_set_tx_word_length(i2s_device_number_t device_num,
               i2s_word_length_t word_length,
               i2s_channel_num_t channel_num)
{
    rcr_tcr_t u_tcr;

    if (word_length > RESOLUTION_32_BIT || word_length < IGNORE_WORD_LENGTH)
        return -1;
    if (channel_num < I2S_CHANNEL_0 || channel_num > I2S_CHANNEL_3)
        return -1;

    u_tcr.reg_data = readl(&i2s[device_num]->channel[channel_num].tcr);
    u_tcr.rcr_tcr.wlen = word_length;
    writel(u_tcr.reg_data, &i2s[device_num]->channel[channel_num].tcr);
    return 0;
}

static void i2s_master_configure(i2s_device_number_t device_num,
                          i2s_word_select_cycles_t word_select_size,
                          i2s_sclk_gating_cycles_t gating_cycles,
                          i2s_work_mode_t word_mode)
{
    configASSERT(!(word_select_size < SCLK_CYCLES_16 ||
                 word_select_size > SCLK_CYCLES_32));
    configASSERT(!(gating_cycles < NO_CLOCK_GATING ||
                  gating_cycles > CLOCK_CYCLES_24));

    ccr_t u_ccr;
    cer_t u_cer;

    u_ccr.reg_data = readl(&i2s[device_num]->ccr);
    u_ccr.ccr.clk_word_size = word_select_size;
    u_ccr.ccr.clk_gate = gating_cycles;
    u_ccr.ccr.align_mode = word_mode;
    writel(u_ccr.reg_data, &i2s[device_num]->ccr);

    u_cer.reg_data = readl(&i2s[device_num]->cer);
    u_cer.cer.clken = 1;
    writel(u_cer.reg_data, &i2s[device_num]->cer);
    /* Clock generation enable */

}

static int i2s_set_rx_threshold(i2s_device_number_t device_num,
             i2s_fifo_threshold_t threshold,
             i2s_channel_num_t channel_num)
{
    rfcr_t u_rfcr;

    if (threshold < TRIGGER_LEVEL_1 || threshold > TRIGGER_LEVEL_16)
        return -1;
    if (channel_num < I2S_CHANNEL_0 || channel_num > I2S_CHANNEL_3)
        return -1;

    u_rfcr.reg_data = readl(&i2s[device_num]->channel[channel_num].rfcr);
    u_rfcr.rfcr.rxchdt = threshold;
    writel(u_rfcr.reg_data, &i2s[device_num]->channel[channel_num].rfcr);

    return 0;
}

static int i2s_set_tx_threshold(i2s_device_number_t device_num,
             i2s_fifo_threshold_t threshold,
             i2s_channel_num_t channel_num)
{
    tfcr_t u_tfcr;

    if (threshold < TRIGGER_LEVEL_1 || threshold > TRIGGER_LEVEL_16)
        return -1;
    if (channel_num < I2S_CHANNEL_0 || channel_num > I2S_CHANNEL_3)
        return -1;

    u_tfcr.reg_data = readl(&i2s[device_num]->channel[channel_num].tfcr);
    u_tfcr.tfcr.txchet = threshold;
    writel(u_tfcr.reg_data, &i2s[device_num]->channel[channel_num].tfcr);
    return 0;
}

static int i2s_set_mask_interrupt(i2s_device_number_t device_num,
               i2s_channel_num_t channel_num,
               uint32_t rx_available_int, uint32_t rx_overrun_int,
               uint32_t tx_empty_int, uint32_t tx_overrun_int)
{
    imr_t u_imr;

    if (channel_num < I2S_CHANNEL_0 || channel_num > I2S_CHANNEL_3)
        return -1;
    u_imr.reg_data = readl(&i2s[device_num]->channel[channel_num].imr);

    if (rx_available_int == 1)
        u_imr.imr.rxdam = 1;
    else
        u_imr.imr.rxdam = 0;
    if (rx_overrun_int == 1)
        u_imr.imr.rxfom = 1;
    else
        u_imr.imr.rxfom = 0;

    if (tx_empty_int == 1)
        u_imr.imr.txfem = 1;
    else
        u_imr.imr.txfem = 0;
    if (tx_overrun_int == 1)
        u_imr.imr.txfom = 1;
    else
        u_imr.imr.txfom = 0;
    writel(u_imr.reg_data, &i2s[device_num]->channel[channel_num].imr);
    return 0;
}

static int i2s_transmit_dma_enable(i2s_device_number_t device_num, uint32_t enable)
{
    ccr_t u_ccr;

    if (device_num >= I2S_DEVICE_MAX)
        return -1;

    u_ccr.reg_data = readl(&i2s[device_num]->ccr);
    u_ccr.ccr.dma_tx_en = enable;
    writel(u_ccr.reg_data, &i2s[device_num]->ccr);

    return 0;
}

static int i2s_receive_dma_enable(i2s_device_number_t device_num, uint32_t enable)
{
    ccr_t u_ccr;

    if (device_num >= I2S_DEVICE_MAX)
        return -1;

    u_ccr.reg_data = readl(&i2s[device_num]->ccr);
    u_ccr.ccr.dma_rx_en = enable;
    writel(u_ccr.reg_data, &i2s[device_num]->ccr);

    return 0;
}

int i2s_set_dma_divide_16(i2s_device_number_t device_num, uint32_t enable)
{
    ccr_t u_ccr;

    if (device_num >= I2S_DEVICE_MAX)
        return -1;

    u_ccr.reg_data = readl(&i2s[device_num]->ccr);
    u_ccr.ccr.dma_divide_16 = enable;
    writel(u_ccr.reg_data, &i2s[device_num]->ccr);

    return 0;
}

int i2s_get_dma_divide_16(i2s_device_number_t device_num)
{
    if (device_num >= I2S_DEVICE_MAX)
        return -1;
    ccr_t u_ccr;
    u_ccr.reg_data = readl(&i2s[device_num]->ccr);
    return u_ccr.ccr.dma_divide_16;
}

int i2s_receive_data(i2s_device_number_t device_num, i2s_channel_num_t channel_num, uint64_t *buf, size_t buf_len)
{
    uint32_t i = 0;
    isr_t u_isr;

    readl(&i2s[device_num]->channel[channel_num].ror);
    /*clear over run*/

    for (i = 0; i < buf_len;)
    {
        u_isr.reg_data = readl(&i2s[device_num]->channel[channel_num].isr);
        if (u_isr.isr.rxda == 1)
        {
            buf[i] = readl(&i2s[device_num]->channel[channel_num].left_rxtx);
            buf[i] <<= 32;
            buf[i++] |= readl(&i2s[device_num]->channel[channel_num].right_rxtx);
        }
    }
    return 0;
}

void i2s_receive_data_dma(i2s_device_number_t device_num, uint32_t *buf,
                          size_t buf_len, dmac_channel_number_t channel_num)
{
    dmac_wait_idle(channel_num);
    sysctl_dma_select((sysctl_dma_channel_t)channel_num, SYSCTL_DMA_SELECT_I2S0_RX_REQ + device_num * 2);
    dmac_set_single_mode(channel_num, (void *)(&i2s[device_num]->rxdma), buf, DMAC_ADDR_NOCHANGE, DMAC_ADDR_INCREMENT,
                          DMAC_MSIZE_1, DMAC_TRANS_WIDTH_32, buf_len);
}

int i2s_rx_to_tx(i2s_device_number_t device_src_num, i2s_device_number_t device_dest_num,
    size_t buf_len, dmac_channel_number_t channel_num)
{
    static uint8_t dmac_recv_flag[6] = {0,0,0,0,0,0};
    if(dmac_recv_flag[channel_num])
        dmac_wait_done(channel_num);
    else
        dmac_recv_flag[channel_num] = 1;
    sysctl_dma_select((sysctl_dma_channel_t)channel_num, SYSCTL_DMA_SELECT_I2S0_RX_REQ + device_src_num * 2);
    dmac_set_single_mode(channel_num, (void *)(&i2s[device_src_num]->rxdma), (void *)(&i2s[device_dest_num]->txdma), DMAC_ADDR_NOCHANGE, DMAC_ADDR_NOCHANGE,
                          DMAC_MSIZE_1, DMAC_TRANS_WIDTH_32, buf_len);
    return 0;
}

int i2s_send_data(i2s_device_number_t device_num, i2s_channel_num_t channel_num, const uint8_t *pcm, size_t buf_len,
                  size_t single_length)
{
    isr_t u_isr;
    uint32_t left_buffer = 0;
    uint32_t right_buffer = 0;
    uint32_t i = 0;
    uint32_t j = 0;
    if (channel_num < I2S_CHANNEL_0 || channel_num > I2S_CHANNEL_3)
        return -1;

    buf_len = buf_len / (single_length / 8) / 2; /* sample num */
    readl(&i2s[device_num]->channel[channel_num].tor);
    /* read clear overrun flag */

    for (j = 0; j < buf_len;)
    {
        u_isr.reg_data = readl(&i2s[device_num]->channel[channel_num].isr);
        if (u_isr.isr.txfe == 1)
        {
            switch(single_length)
            {
                case 16:
                    left_buffer = ((uint16_t *)pcm)[i++];
                    right_buffer = ((uint16_t *)pcm)[i++];
                    break;
                case 24:
                    left_buffer = 0;
                    left_buffer |= pcm[i++];
                    left_buffer |= pcm[i++] << 8;
                    left_buffer |= pcm[i++] << 16;
                    right_buffer = 0;
                    right_buffer |= pcm[i++];
                    right_buffer |= pcm[i++] << 8;
                    right_buffer |= pcm[i++] << 16;
                    break;
                case 32:
                    left_buffer = ((uint32_t *)pcm)[i++];
                    right_buffer = ((uint32_t *)pcm)[i++];
                    break;
                default:
                    left_buffer = pcm[i++];
                    right_buffer = pcm[i++];
                    break;
            }
            writel(left_buffer, &i2s[device_num]->channel[channel_num].left_rxtx);
            writel(right_buffer, &i2s[device_num]->channel[channel_num].right_rxtx);
            j++;
        }
    }
    return 0;
}

void i2s_send_data_dma(i2s_device_number_t device_num, const void *buf, size_t buf_len, dmac_channel_number_t channel_num)
{

    dmac_wait_idle(channel_num);
    sysctl_dma_select((sysctl_dma_channel_t)channel_num, SYSCTL_DMA_SELECT_I2S0_TX_REQ + device_num * 2);
    dmac_set_single_mode(channel_num, buf, (void *)(&i2s[device_num]->txdma), DMAC_ADDR_INCREMENT,
                         DMAC_ADDR_NOCHANGE, DMAC_MSIZE_1, DMAC_TRANS_WIDTH_32, buf_len);
}

static void i2s_parse_voice(i2s_device_number_t device_num, uint32_t *buf, const uint8_t *pcm, size_t length,  size_t bits_per_sample,
    uint8_t track_num, size_t *send_len)
{
    uint32_t i,j=0;
    *send_len = length * 2;
    switch(bits_per_sample)
    {
        case 16:
            for(i = 0; i < length; i++)
            {
                buf[2*i] = ((uint16_t *)pcm)[i];
                buf[2*i+1] = 0;
            }
            break;
        case 24:
            for(i = 0; i < length; i++)
            {
                buf[2*i] = 0;
                buf[2*i] |= pcm[j++];
                buf[2*i] |= pcm[j++] << 8;
                buf[2*i] |= pcm[j++] << 16;
                buf[2*i+1] = 0;
                if(track_num == 2)
                {
                    buf[2*i+1] |= pcm[j++];
                    buf[2*i+1] |= pcm[j++] << 8;
                    buf[2*i+1] |= pcm[j++] << 16;
                }
            }
            break;
        case 32:
        default:
            for(i = 0; i < length; i++)
            {
                buf[2*i] = ((uint32_t *)pcm)[i];
                buf[2*i+1] = 0;
            }
            break;
    }
}


void i2s_play(i2s_device_number_t device_num, dmac_channel_number_t channel_num,
              const uint8_t *buf, size_t buf_len, size_t frame, size_t bits_per_sample, uint8_t track_num)
{
    const uint8_t *trans_buf;
    uint32_t i;
    size_t sample_cnt = buf_len / ( bits_per_sample / 8 ) / track_num;
    size_t frame_cnt = sample_cnt / frame;
    size_t frame_remain = sample_cnt % frame;
    i2s_set_dma_divide_16(device_num, 0);

    if (bits_per_sample == 16 && track_num == 2)
    {
        i2s_set_dma_divide_16(device_num, 1);
        for (i = 0; i < frame_cnt; i++)
        {
            trans_buf = buf + i * frame * (bits_per_sample / 8) * track_num;
            i2s_send_data_dma(device_num,trans_buf, frame, channel_num);
        }
        if(frame_remain)
        {
            trans_buf = buf + frame_cnt * frame * (bits_per_sample / 8) * track_num;
            i2s_send_data_dma(device_num, trans_buf, frame_remain, channel_num);
        }
    }
    else if (bits_per_sample == 32 && track_num == 2)
    {
        for (i = 0; i < frame_cnt; i++)
        {
            trans_buf = buf + i * frame * (bits_per_sample / 8) * track_num;
            i2s_send_data_dma(device_num,trans_buf, frame * 2, channel_num);
        }
        if(frame_remain)
        {
            trans_buf = buf + frame_cnt * frame * (bits_per_sample / 8) * track_num;
            i2s_send_data_dma(device_num, trans_buf, frame_remain * 2, channel_num);
        }
    }
    else
    {
        uint32_t *buff[2];
        buff[0] = malloc(frame * 2 * sizeof(uint32_t) * 2);
        buff[1] = buff[0] + frame * 2;
        uint8_t flag = 0;
        size_t send_len = 0;
        for (i = 0; i < frame_cnt; i++)
        {
            trans_buf = buf + i * frame * (bits_per_sample / 8) * track_num;
            i2s_parse_voice(device_num, buff[flag], trans_buf, frame, bits_per_sample, track_num, &send_len);
            i2s_send_data_dma(device_num,buff[flag], send_len, channel_num);
            flag = !flag;
        }
        if (frame_remain)
        {
            trans_buf = buf + frame_cnt * frame * (bits_per_sample / 8) * track_num;
            i2s_parse_voice(device_num, buff[flag], trans_buf, frame_remain, bits_per_sample, track_num, &send_len);
            i2s_send_data_dma(device_num, trans_buf, send_len, channel_num);
        }
        free(buff[0]);
    }
}

static inline void i2s_set_sign_expand_en(i2s_device_number_t device_num, uint32_t enable)
{
    ccr_t u_ccr;
    u_ccr.reg_data = readl(&i2s[device_num]->ccr);
    u_ccr.ccr.sign_expand_en = enable;
    writel(u_ccr.reg_data, &i2s[device_num]->ccr);
}

void i2s_rx_channel_config(i2s_device_number_t device_num,
    i2s_channel_num_t channel_num,
    i2s_word_length_t word_length,
    i2s_word_select_cycles_t word_select_size,
    i2s_fifo_threshold_t trigger_level,
    i2s_work_mode_t word_mode)
{
    i2s_recv_channel_enable(device_num, channel_num, 0);
    /* Receive channel disable */

    writel(0, &i2s[device_num]->channel[channel_num].ter);
    /* disable tx */

    writel(1, &i2s[device_num]->channel[channel_num].rff);
    /* flash individual fifo */

    writel(1, &i2s[device_num]->rxffr);
    /* flush tx fifo*/

    i2s_set_rx_word_length(device_num, word_length, channel_num);
    /* Word buf_len is RESOLUTION_32_BIT */

    i2s_master_configure(device_num,
        word_select_size, NO_CLOCK_GATING, word_mode);
    /* word select size is 32 bits,no clock gating */

    i2s_set_rx_threshold(device_num, trigger_level, channel_num);
    /* Interrupt trigger when FIFO level is 8 */

    readl(&i2s[device_num]->channel[channel_num].ror);
    readl(&i2s[device_num]->channel[channel_num].tor);

    i2s_recv_channel_enable(device_num, channel_num, 1);
}

void i2s_tx_channel_config(i2s_device_number_t device_num,
    i2s_channel_num_t channel_num,
    i2s_word_length_t word_length,
    i2s_word_select_cycles_t word_select_size,
    i2s_fifo_threshold_t trigger_level,
    i2s_work_mode_t word_mode)
{
    writel(0, &i2s[device_num]->channel[channel_num].rer);
    /* disable rx */

    i2s_transmit_channel_enable(device_num, channel_num, 0);
    /* Transmit channel disable */

    writel(1, &i2s[device_num]->txffr);
    /* flush tx fifo */
    writel(1, &i2s[device_num]->channel[channel_num].tff);
    /* flush individual fifo */

    i2s_set_tx_word_length(device_num, word_length, channel_num);
    /* Word buf_len is RESOLUTION_16_BIT */

    i2s_master_configure(device_num, word_select_size, NO_CLOCK_GATING, word_mode);
    /* word select size is 16 bits,gating after 16 bit */

    i2s_set_tx_threshold(device_num, trigger_level, channel_num);
    /* Interrupt trigger when FIFO level is 8 */

    i2s_transmit_channel_enable(device_num, channel_num, 1);
}

void i2s_init(i2s_device_number_t device_num, i2s_transmit_t rxtx_mode, uint32_t channel_mask)
{
    sysctl_clock_enable(SYSCTL_CLOCK_I2S0 + device_num);
    sysctl_reset(SYSCTL_RESET_I2S0  + device_num);
    sysctl_clock_set_threshold(SYSCTL_THRESHOLD_I2S0  + device_num, 7);
    /*96k:5,44k:12,24k:23,22k:25 16k:35 sampling*/
    /*sample rate*32bit*2 =75MHz/((N+1)*2) */
    i2s_set_enable(device_num, 1);
    i2s_disable_block(device_num, I2S_TRANSMITTER);
    i2s_disable_block(device_num, I2S_RECEIVER);

    if (rxtx_mode == I2S_TRANSMITTER)
    {
        for (int i=0; i<4; i++)
        {
            if ((channel_mask & 0x3) == 0x3)
            {
                i2s_set_mask_interrupt(device_num, I2S_CHANNEL_0 + i, 1, 1, 1, 1);
                i2s_transimit_enable(device_num, I2S_CHANNEL_0 + i);
            }
            else
            {
                i2s_transmit_channel_enable(device_num, I2S_CHANNEL_0 + i, 0);
            }
            channel_mask >>= 2;
        }
        i2s_transmit_dma_enable(device_num, 1);
    }
    else
    {
        for (int i=0; i<4; i++)
        {
            if ((channel_mask & 0x3) == 0x3)
            {
                i2s_set_mask_interrupt(device_num, I2S_CHANNEL_0 + i, 1, 1, 1, 1);
                i2s_receive_enable(device_num, I2S_CHANNEL_0 + i);
            }
            else
            {
                i2s_recv_channel_enable(device_num, I2S_CHANNEL_0 + i, 0);
            }
            channel_mask >>= 2;
        }
        /* Set expand_en when receive */
        i2s_set_sign_expand_en(device_num, 1);
        i2s_receive_dma_enable(device_num, 1);
    }
}

uint32_t i2s_set_sample_rate(i2s_device_number_t device_num, uint32_t sample_rate)
{
    ccr_t u_ccr;
    uint32_t pll2_clock = 0;
    pll2_clock = sysctl_pll_get_freq(SYSCTL_PLL2);

    u_ccr.reg_data = readl(&i2s[device_num]->ccr);
    /* 0x0 for 16sclk cycles, 0x1 for 24 sclk cycles 0x2 for 32 sclk */
    uint32_t v_clk_word_size = (u_ccr.ccr.clk_word_size + 2) * 8;
    uint32_t threshold = round(pll2_clock / (sample_rate * 2.0 * v_clk_word_size * 2.0) - 1);
    sysctl_clock_set_threshold(SYSCTL_THRESHOLD_I2S0 + device_num, threshold);
    return sysctl_clock_get_freq(SYSCTL_CLOCK_I2S0 + device_num);
}

int i2s_dmac_irq(void *ctx)
{
    i2s_instance_t *v_instance = (i2s_instance_t *)ctx;
    dmac_irq_unregister(v_instance->dmac_channel);
    if(v_instance->i2s_int_instance.callback)
    {
        v_instance->i2s_int_instance.callback(v_instance->i2s_int_instance.ctx);
    }
    return 0;
}

void i2s_handle_data_dma(i2s_device_number_t device_num, i2s_data_t data, plic_interrupt_t *cb)
{
    configASSERT(device_num < I2S_DEVICE_MAX);
    if(data.transfer_mode == I2S_SEND)
    {
        configASSERT(data.tx_buf && data.tx_len);
        if(!data.nowait_dma_idle)
        {
            dmac_wait_idle(data.tx_channel);
        }
        if(cb)
        {
            g_i2s_send_instance[device_num].i2s_int_instance.callback = cb->callback;
            g_i2s_send_instance[device_num].i2s_int_instance.ctx = cb->ctx;
            g_i2s_send_instance[device_num].dmac_channel = data.tx_channel;
            g_i2s_send_instance[device_num].transfer_mode = I2S_SEND;
            dmac_irq_register(data.tx_channel, i2s_dmac_irq, &g_i2s_send_instance[device_num], cb->priority);
        }
        sysctl_dma_select((sysctl_dma_channel_t)data.tx_channel, SYSCTL_DMA_SELECT_I2S0_TX_REQ + device_num * 2);
        dmac_set_single_mode(data.tx_channel, data.tx_buf, (void *)(&i2s[device_num]->txdma), DMAC_ADDR_INCREMENT,
                             DMAC_ADDR_NOCHANGE, DMAC_MSIZE_1, DMAC_TRANS_WIDTH_32, data.tx_len);
        if(!cb && data.wait_dma_done)
        {
            dmac_wait_done(data.tx_channel);
        }
    }
    else
    {
        configASSERT(data.rx_buf && data.rx_len);
        if(!data.nowait_dma_idle)
        {
            dmac_wait_idle(data.rx_channel);
        }
        if(cb)
        {
            g_i2s_recv_instance[device_num].i2s_int_instance.callback = cb->callback;
            g_i2s_recv_instance[device_num].i2s_int_instance.ctx = cb->ctx;
            g_i2s_recv_instance[device_num].dmac_channel = data.rx_channel;
            g_i2s_recv_instance[device_num].transfer_mode = I2S_RECEIVE;
            dmac_irq_register(data.rx_channel, i2s_dmac_irq, &g_i2s_recv_instance[device_num], cb->priority);
        }
        sysctl_dma_select((sysctl_dma_channel_t)data.rx_channel, SYSCTL_DMA_SELECT_I2S0_RX_REQ + device_num * 2);
        dmac_set_single_mode(data.rx_channel, (void *)(&i2s[device_num]->rxdma), data.rx_buf, DMAC_ADDR_NOCHANGE, DMAC_ADDR_INCREMENT,
                              DMAC_MSIZE_1, DMAC_TRANS_WIDTH_32, data.rx_len);
        if(!cb && data.wait_dma_done)
        {
            dmac_wait_done(data.rx_channel);
        }
    }
}

