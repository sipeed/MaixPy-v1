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
#include <stdint.h>
#include "dvp.h"
#include "utils.h"
#include "fpioa.h"
#include "sysctl.h"
#include <math.h>

volatile dvp_t* const dvp = (volatile dvp_t*)DVP_BASE_ADDR;
static uint8_t g_sccb_reg_len = 8;

static void mdelay(uint32_t ms)
{
    uint32_t i;

    while (ms && ms--)
    {
        for (i = 0; i < 25000; i++)
            __asm__ __volatile__("nop");
    }
}

static void dvp_sccb_clk_init(void)
{
    uint32_t tmp;

    tmp = dvp->sccb_cfg & (~(DVP_SCCB_SCL_LCNT_MASK | DVP_SCCB_SCL_HCNT_MASK));
    tmp |= DVP_SCCB_SCL_LCNT(255) | DVP_SCCB_SCL_HCNT(255);

    dvp->sccb_cfg = tmp;
}

uint32_t dvp_sccb_set_clk_rate(uint32_t clk_rate)
{
    uint32_t tmp;
    uint32_t v_sccb_freq = sysctl_clock_get_freq(SYSCTL_CLOCK_APB1);
    uint16_t v_period_clk_cnt = round(v_sccb_freq / clk_rate / 2.0);
    if(v_period_clk_cnt > 255)
    {
        return 0;
    }
    tmp = dvp->sccb_cfg & (~(DVP_SCCB_SCL_LCNT_MASK | DVP_SCCB_SCL_HCNT_MASK));
    tmp |= DVP_SCCB_SCL_LCNT(v_period_clk_cnt) | DVP_SCCB_SCL_HCNT(v_period_clk_cnt);
    dvp->sccb_cfg = tmp;
    return sysctl_clock_get_freq(SYSCTL_CLOCK_DVP) / (v_period_clk_cnt * 2);
}

static void dvp_sccb_start_transfer(void)
{
    while (dvp->sts & DVP_STS_SCCB_EN)
        ;
    dvp->sts = DVP_STS_SCCB_EN | DVP_STS_SCCB_EN_WE;
    while (dvp->sts & DVP_STS_SCCB_EN)
        ;
}

void dvp_sccb_send_data(uint8_t dev_addr, uint16_t reg_addr, uint8_t reg_data)
{
    uint32_t tmp;

    tmp = dvp->sccb_cfg & (~DVP_SCCB_BYTE_NUM_MASK);

    (g_sccb_reg_len == 8) ? (tmp |= DVP_SCCB_BYTE_NUM_3) : (tmp |= DVP_SCCB_BYTE_NUM_4);

    dvp->sccb_cfg = tmp;

    if (g_sccb_reg_len == 8)
    {
        dvp->sccb_ctl = DVP_SCCB_WRITE_DATA_ENABLE | DVP_SCCB_DEVICE_ADDRESS(dev_addr) | DVP_SCCB_REG_ADDRESS(reg_addr) | DVP_SCCB_WDATA_BYTE0(reg_data);
    }
    else
    {
        dvp->sccb_ctl = DVP_SCCB_WRITE_DATA_ENABLE | DVP_SCCB_DEVICE_ADDRESS(dev_addr) | DVP_SCCB_REG_ADDRESS(reg_addr >> 8) | DVP_SCCB_WDATA_BYTE0(reg_addr & 0xff) | DVP_SCCB_WDATA_BYTE1(reg_data);
    }
    dvp_sccb_start_transfer();
}

uint8_t dvp_sccb_receive_data(uint8_t dev_addr, uint16_t reg_addr)
{
    uint32_t tmp;

    tmp = dvp->sccb_cfg & (~DVP_SCCB_BYTE_NUM_MASK);

    if (g_sccb_reg_len == 8)
        tmp |= DVP_SCCB_BYTE_NUM_2;
    else
        tmp |= DVP_SCCB_BYTE_NUM_3;

    dvp->sccb_cfg = tmp;

    if (g_sccb_reg_len == 8)
    {
        dvp->sccb_ctl = DVP_SCCB_WRITE_DATA_ENABLE | DVP_SCCB_DEVICE_ADDRESS(dev_addr) | DVP_SCCB_REG_ADDRESS(reg_addr);
    }
    else
    {
        dvp->sccb_ctl = DVP_SCCB_WRITE_DATA_ENABLE | DVP_SCCB_DEVICE_ADDRESS(dev_addr) | DVP_SCCB_REG_ADDRESS(reg_addr >> 8) | DVP_SCCB_WDATA_BYTE0(reg_addr & 0xff);
    }
    dvp_sccb_start_transfer();

    dvp->sccb_ctl = DVP_SCCB_DEVICE_ADDRESS(dev_addr);

    dvp_sccb_start_transfer();

    return (uint8_t) DVP_SCCB_RDATA_BYTE(dvp->sccb_cfg);
}

static void dvp_reset(void)
{
    /* First power down */
    dvp->cmos_cfg |= DVP_CMOS_POWER_DOWN;
    mdelay(200);
    dvp->cmos_cfg &= ~DVP_CMOS_POWER_DOWN;
    mdelay(200);

    /* Second reset */
    dvp->cmos_cfg &= ~DVP_CMOS_RESET;
    mdelay(200);
    dvp->cmos_cfg |= DVP_CMOS_RESET;
    mdelay(200);
}

void dvp_init(uint8_t reg_len)
{
    g_sccb_reg_len = reg_len;
    sysctl_clock_enable(SYSCTL_CLOCK_DVP);
    sysctl_reset(SYSCTL_RESET_DVP);
    dvp->cmos_cfg &= (~DVP_CMOS_CLK_DIV_MASK);
    dvp->cmos_cfg |= DVP_CMOS_CLK_DIV(3) | DVP_CMOS_CLK_ENABLE;
    dvp_sccb_clk_init();
    dvp_reset();
}

uint32_t dvp_set_xclk_rate(uint32_t xclk_rate)
{
    uint32_t v_apb1_clk = sysctl_clock_get_freq(SYSCTL_CLOCK_APB1);
    uint32_t v_period;
    if(v_apb1_clk > xclk_rate * 2)
        v_period = round(v_apb1_clk / (xclk_rate * 2.0)) - 1;
    else
        v_period = 0;
    if(v_period > 255)
        v_period = 255;
    dvp->cmos_cfg &= (~DVP_CMOS_CLK_DIV_MASK);
    dvp->cmos_cfg |= DVP_CMOS_CLK_DIV(v_period) | DVP_CMOS_CLK_ENABLE;
    dvp_reset();
    return v_apb1_clk / ((v_period + 1) * 2);
}

void dvp_set_image_format(uint32_t format)
{
    uint32_t tmp;

    tmp = dvp->dvp_cfg & (~DVP_CFG_FORMAT_MASK);
    dvp->dvp_cfg = tmp | format;
}

void dvp_enable_burst(void)
{
    dvp->dvp_cfg |= DVP_CFG_BURST_SIZE_4BEATS;

    dvp->axi &= (~DVP_AXI_GM_MLEN_MASK);
    dvp->axi |= DVP_AXI_GM_MLEN_4BYTE;
}

void dvp_disable_burst(void)
{
    dvp->dvp_cfg &= (~DVP_CFG_BURST_SIZE_4BEATS);

    dvp->axi &= (~DVP_AXI_GM_MLEN_MASK);
    dvp->axi |= DVP_AXI_GM_MLEN_1BYTE;
}

void dvp_set_image_size(uint32_t width, uint32_t height)
{
    uint32_t tmp;

    tmp = dvp->dvp_cfg & (~(DVP_CFG_HREF_BURST_NUM_MASK | DVP_CFG_LINE_NUM_MASK));

    tmp |= DVP_CFG_LINE_NUM(height);

    if (dvp->dvp_cfg & DVP_CFG_BURST_SIZE_4BEATS)
        tmp |= DVP_CFG_HREF_BURST_NUM(width / 8 / 4);
    else
        tmp |= DVP_CFG_HREF_BURST_NUM(width / 8 / 1);

    dvp->dvp_cfg = tmp;
}

void dvp_set_ai_addr(uint32_t r_addr, uint32_t g_addr, uint32_t b_addr)
{
    dvp->r_addr = r_addr;
    dvp->g_addr = g_addr;
    dvp->b_addr = b_addr;
}

void dvp_set_display_addr(uint32_t addr)
{
    dvp->rgb_addr = addr;
}

void dvp_start_frame(void)
{
    while (!(dvp->sts & DVP_STS_FRAME_START))
        ;
    dvp->sts = (DVP_STS_FRAME_START | DVP_STS_FRAME_START_WE);
}

void dvp_start_convert(void)
{
    dvp->sts = DVP_STS_DVP_EN | DVP_STS_DVP_EN_WE;
}

void dvp_finish_convert(void)
{
    while (!(dvp->sts & DVP_STS_FRAME_FINISH))
        ;
    dvp->sts = DVP_STS_FRAME_FINISH | DVP_STS_FRAME_FINISH_WE;
}

void dvp_get_image(void)
{
    while (!(dvp->sts & DVP_STS_FRAME_START))
        ;
    dvp->sts = DVP_STS_FRAME_START | DVP_STS_FRAME_START_WE;
    while (!(dvp->sts & DVP_STS_FRAME_START))
        ;
    dvp->sts = DVP_STS_FRAME_FINISH | DVP_STS_FRAME_FINISH_WE | DVP_STS_FRAME_START | DVP_STS_FRAME_START_WE | DVP_STS_DVP_EN | DVP_STS_DVP_EN_WE;
    while (!(dvp->sts & DVP_STS_FRAME_FINISH))
        ;
}

void dvp_config_interrupt(uint32_t interrupt, uint8_t enable)
{
    if (enable)
        dvp->dvp_cfg |= interrupt;
    else
        dvp->dvp_cfg &= (~interrupt);
}

int dvp_get_interrupt(uint32_t interrupt)
{
    if (dvp->sts & interrupt)
        return 1;
    return 0;
}

void dvp_clear_interrupt(uint32_t interrupt)
{
    interrupt |= (interrupt << 1);
    dvp->sts |= interrupt;
}

void dvp_enable_auto(void)
{
    dvp->dvp_cfg |= DVP_CFG_AUTO_ENABLE;
}

void dvp_disable_auto(void)
{
    dvp->dvp_cfg &= (~DVP_CFG_AUTO_ENABLE);
}

void dvp_set_output_enable(dvp_output_mode_t index, int enable)
{
    configASSERT(index < 2);

    if (index == 0)
    {
        if (enable)
            dvp->dvp_cfg |= DVP_CFG_AI_OUTPUT_ENABLE;
        else
            dvp->dvp_cfg &= ~DVP_CFG_AI_OUTPUT_ENABLE;
    }
    else
    {
        if (enable)
            dvp->dvp_cfg |= DVP_CFG_DISPLAY_OUTPUT_ENABLE;
        else
            dvp->dvp_cfg &= ~DVP_CFG_DISPLAY_OUTPUT_ENABLE;
    }
}

