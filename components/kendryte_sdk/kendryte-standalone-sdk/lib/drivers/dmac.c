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
#include <stddef.h>
#include <stdio.h>
#include "dmac.h"
#include "sysctl.h"
#include "fpioa.h"
#include "utils.h"
#include "plic.h"
#include "stdlib.h"

volatile dmac_t *const dmac = (dmac_t *)DMAC_BASE_ADDR;

typedef struct _dmac_context
{
    dmac_channel_number_t dmac_channel;
    plic_irq_callback_t callback;
    void *ctx;
} dmac_context_t;

dmac_context_t dmac_context[6];

static int is_memory(uintptr_t address)
{
    enum {
        mem_len = 6 * 1024 * 1024,
        mem_no_cache_len = 8 * 1024 * 1024,
    };
    return ((address >= 0x80000000) && (address < 0x80000000 + mem_len)) || ((address >= 0x40000000) && (address < 0x40000000 + mem_no_cache_len)) || (address == 0x50450040);
}

uint64_t dmac_read_id(void)
{
    return dmac->id;
}

uint64_t dmac_read_version(void)
{
    return dmac->compver;
}

uint64_t dmac_read_channel_id(dmac_channel_number_t channel_num)
{
    return dmac->channel[channel_num].axi_id;
}

static void dmac_enable(void)
{
    dmac_cfg_u_t  dmac_cfg;

    dmac_cfg.data = readq(&dmac->cfg);
    dmac_cfg.cfg.dmac_en = 1;
    dmac_cfg.cfg.int_en = 1;
    writeq(dmac_cfg.data, &dmac->cfg);
}

void dmac_disable(void)
{
    dmac_cfg_u_t  dmac_cfg;

    dmac_cfg.data = readq(&dmac->cfg);
    dmac_cfg.cfg.dmac_en = 0;
    dmac_cfg.cfg.int_en = 0;
    writeq(dmac_cfg.data, &dmac->cfg);
}

void src_transaction_complete_int_enable(dmac_channel_number_t channel_num)
{
    dmac_ch_intstatus_enable_u_t  ch_intstat;

    ch_intstat.data = readq(&dmac->channel[channel_num].intstatus_en);
    ch_intstat.ch_intstatus_enable.enable_src_transcomp_intstat = 1;

    writeq(ch_intstat.data, &dmac->channel[channel_num].intstatus_en);
}

void dmac_channel_enable(dmac_channel_number_t channel_num)
{
    dmac_chen_u_t chen;

    chen.data = readq(&dmac->chen);

    switch (channel_num) {
    case DMAC_CHANNEL0:
        chen.dmac_chen.ch1_en = 1;
        chen.dmac_chen.ch1_en_we = 1;
        break;
    case DMAC_CHANNEL1:
        chen.dmac_chen.ch2_en = 1;
        chen.dmac_chen.ch2_en_we = 1;
        break;
    case DMAC_CHANNEL2:
        chen.dmac_chen.ch3_en = 1;
        chen.dmac_chen.ch3_en_we = 1;
        break;
    case DMAC_CHANNEL3:
        chen.dmac_chen.ch4_en = 1;
        chen.dmac_chen.ch4_en_we = 1;
        break;
    case DMAC_CHANNEL4:
        chen.dmac_chen.ch5_en = 1;
        chen.dmac_chen.ch5_en_we = 1;
        break;
    case DMAC_CHANNEL5:
        chen.dmac_chen.ch6_en = 1;
        chen.dmac_chen.ch6_en_we = 1;
        break;
    default:
        break;
    }

    writeq(chen.data, &dmac->chen);
}

void dmac_channel_disable(dmac_channel_number_t channel_num)
{
    dmac_chen_u_t chen;

    chen.data = readq(&dmac->chen);

    switch (channel_num)
    {
    case DMAC_CHANNEL0:
        chen.dmac_chen.ch1_en = 0;
        chen.dmac_chen.ch1_en_we = 1;
        break;
    case DMAC_CHANNEL1:
        chen.dmac_chen.ch2_en = 0;
        chen.dmac_chen.ch2_en_we = 1;
        break;
    case DMAC_CHANNEL2:
        chen.dmac_chen.ch3_en = 0;
        chen.dmac_chen.ch3_en_we = 1;
        break;
    case DMAC_CHANNEL3:
        chen.dmac_chen.ch4_en = 0;
        chen.dmac_chen.ch4_en_we = 1;
        break;
    case DMAC_CHANNEL4:
        chen.dmac_chen.ch5_en = 0;
        chen.dmac_chen.ch5_en_we = 1;
        break;
    case DMAC_CHANNEL5:
        chen.dmac_chen.ch6_en = 0;
        chen.dmac_chen.ch6_en_we = 1;
        break;
    default:
        break;
    }

    writeq(chen.data, &dmac->chen);
}

int32_t dmac_check_channel_busy(dmac_channel_number_t channel_num)
{
    int32_t ret = 0;
    dmac_chen_u_t chen_u;

    chen_u.data = readq(&dmac->chen);
    switch (channel_num) {
    case DMAC_CHANNEL0:
        if (chen_u.dmac_chen.ch1_en == 1)
            ret = 1;
        break;
    case DMAC_CHANNEL1:
        if (chen_u.dmac_chen.ch2_en == 1)
            ret = 1;
        break;
    case DMAC_CHANNEL2:
        if (chen_u.dmac_chen.ch3_en == 1)
            ret = 1;
        break;
    case DMAC_CHANNEL3:
        if (chen_u.dmac_chen.ch4_en == 1)
            ret = 1;
        break;
    case DMAC_CHANNEL4:
        if (chen_u.dmac_chen.ch5_en == 1)
            ret = 1;
        break;
    case DMAC_CHANNEL5:
        if (chen_u.dmac_chen.ch6_en == 1)
            ret = 1;
        break;
    default:
        break;
    }

    writeq(chen_u.data, &dmac->chen);

    return ret;
}

int32_t dmac_set_list_master_select(dmac_channel_number_t channel_num,
    dmac_src_dst_select_t sd_sel, dmac_master_number_t  mst_num)
{
    int32_t ret = 0;
    uint64_t tmp = 0;
    dmac_ch_ctl_u_t ctl;

    ctl.data = readq(&dmac->channel[channel_num].ctl);
    ret = dmac_check_channel_busy(channel_num);
    if (ret == 0) {
        if (sd_sel == DMAC_SRC || sd_sel == DMAC_SRC_DST)
            ctl.ch_ctl.sms = mst_num;

        if (sd_sel == DMAC_DST || sd_sel == DMAC_SRC_DST)
            ctl.ch_ctl.dms = mst_num;
        tmp |= *(uint64_t *)&dmac->channel[channel_num].ctl;
        writeq(ctl.data, &dmac->channel[channel_num].ctl);
    }

    return ret;
}

void dmac_enable_common_interrupt_status(void)
{
    dmac_commonreg_intstatus_enable_u_t intstatus;

    intstatus.data = readq(&dmac->com_intstatus_en);
    intstatus.intstatus_enable.enable_slvif_dec_err_intstat = 1;
    intstatus.intstatus_enable.enable_slvif_wr2ro_err_intstat = 1;
    intstatus.intstatus_enable.enable_slvif_rd2wo_err_intstat = 1;
    intstatus.intstatus_enable.enable_slvif_wronhold_err_intstat = 1;
    intstatus.intstatus_enable.enable_slvif_undefinedreg_dec_err_intstat = 1;

    writeq(intstatus.data, &dmac->com_intstatus_en);
}

void dmac_enable_common_interrupt_signal(void)
{
    dmac_commonreg_intsignal_enable_u_t intsignal;

    intsignal.data = readq(&dmac->com_intsignal_en);
    intsignal.intsignal_enable.enable_slvif_dec_err_intsignal = 1;
    intsignal.intsignal_enable.enable_slvif_wr2ro_err_intsignal = 1;
    intsignal.intsignal_enable.enable_slvif_rd2wo_err_intsignal = 1;
    intsignal.intsignal_enable.enable_slvif_wronhold_err_intsignal = 1;
    intsignal.intsignal_enable.enable_slvif_undefinedreg_dec_err_intsignal = 1;

    writeq(intsignal.data, &dmac->com_intsignal_en);
}

static void dmac_enable_channel_interrupt(dmac_channel_number_t channel_num)
{
    writeq(0xffffffff, &dmac->channel[channel_num].intclear);
    writeq(0x2, &dmac->channel[channel_num].intstatus_en);
}

void dmac_disable_channel_interrupt(dmac_channel_number_t channel_num)
{
    writeq(0, &dmac->channel[channel_num].intstatus_en);
}

static void dmac_chanel_interrupt_clear(dmac_channel_number_t channel_num)
{
    writeq(0xffffffff, &dmac->channel[channel_num].intclear);
}

int dmac_set_channel_config(dmac_channel_number_t channel_num,
        dmac_channel_config_t *cfg_param)
{
    dmac_ch_ctl_u_t  ctl;
    dmac_ch_cfg_u_t cfg;
    dmac_ch_llp_u_t ch_llp;

    if (cfg_param->ctl_sms > DMAC_MASTER2)
        return -1;
    if (cfg_param->ctl_dms > DMAC_MASTER2)
        return -1;
    if (cfg_param->ctl_src_msize > DMAC_MSIZE_256)
        return -1;
    if (cfg_param->ctl_drc_msize > DMAC_MSIZE_256)
        return -1;

    /**
     * cfg register must configure before ts_block and
     * sar dar register
     */
    cfg.data = readq(&dmac->channel[channel_num].cfg);

    cfg.ch_cfg.hs_sel_src = cfg_param->cfg_hs_sel_src;
    cfg.ch_cfg.hs_sel_dst = cfg_param->cfg_hs_sel_dst;
    cfg.ch_cfg.src_hwhs_pol = cfg_param->cfg_src_hs_pol;
    cfg.ch_cfg.dst_hwhs_pol  = cfg_param->cfg_dst_hs_pol;
    cfg.ch_cfg.src_per = cfg_param->cfg_src_per;
    cfg.ch_cfg.dst_per = cfg_param->cfg_dst_per;
    cfg.ch_cfg.ch_prior = cfg_param->cfg_ch_prior;
    cfg.ch_cfg.tt_fc = cfg_param->ctl_tt_fc;

    cfg.ch_cfg.src_multblk_type = cfg_param->cfg_src_multblk_type;
    cfg.ch_cfg.dst_multblk_type = cfg_param->cfg_dst_multblk_type;

    writeq(cfg.data, &dmac->channel[channel_num].cfg);

    ctl.data = readq(&dmac->channel[channel_num].ctl);
    ctl.ch_ctl.sms = cfg_param->ctl_sms;
    ctl.ch_ctl.dms = cfg_param->ctl_dms;
    /* master select */
    ctl.ch_ctl.sinc = cfg_param->ctl_sinc;
    ctl.ch_ctl.dinc = cfg_param->ctl_dinc;
    /* address incrememt */
    ctl.ch_ctl.src_tr_width = cfg_param->ctl_src_tr_width;
    ctl.ch_ctl.dst_tr_width  = cfg_param->ctl_dst_tr_width;
    /* transfer width */
    ctl.ch_ctl.src_msize = cfg_param->ctl_src_msize;
    ctl.ch_ctl.dst_msize = cfg_param->ctl_drc_msize;
    /* Burst transaction length */
    ctl.ch_ctl.ioc_blktfr = cfg_param->ctl_ioc_blktfr;
    /* interrupt on completion of block transfer */
    /* 0x1 enable BLOCK_TFR_DONE_IntStat field */

    writeq(cfg_param->ctl_block_ts, &dmac->channel[channel_num].block_ts);
    /* the number of (blcok_ts +1) data of width SRC_TR_WIDTF to be */
    /* transferred in a dma block transfer */

    dmac->channel[channel_num].sar = cfg_param->sar;
    dmac->channel[channel_num].dar = cfg_param->dar;

    ch_llp.data = readq(&dmac->channel[channel_num].llp);
    ch_llp.llp.loc = cfg_param->llp_loc;
    ch_llp.llp.lms = cfg_param->llp_lms;
    writeq(ch_llp.data, &dmac->channel[channel_num].llp);
    writeq(ctl.data, &dmac->channel[channel_num].ctl);
    readq(&dmac->channel[channel_num].swhssrc);

    return 0;
}

int dmac_set_channel_param(dmac_channel_number_t channel_num,
    const void *src, void *dest, dmac_address_increment_t src_inc, dmac_address_increment_t dest_inc,
    dmac_burst_trans_length_t dmac_burst_size,
    dmac_transfer_width_t dmac_trans_width,
    uint32_t blockSize)
{
    dmac_ch_ctl_u_t  ctl;
    dmac_ch_cfg_u_t cfg_u;

    int mem_type_src = is_memory((uintptr_t)src), mem_type_dest = is_memory((uintptr_t)dest);
    dmac_transfer_flow_t flow_control;
    if (mem_type_src == 0 && mem_type_dest == 0)
    {
        flow_control = DMAC_PRF2PRF_DMA;
    }else if (mem_type_src == 1 && mem_type_dest == 0)
        flow_control = DMAC_MEM2PRF_DMA;
    else if (mem_type_src == 0 && mem_type_dest == 1)
        flow_control = DMAC_PRF2MEM_DMA;
    else
        flow_control = DMAC_MEM2MEM_DMA;

    /**
     * cfg register must configure before ts_block and
     * sar dar register
     */
    cfg_u.data = readq(&dmac->channel[channel_num].cfg);

    cfg_u.ch_cfg.tt_fc = flow_control;
    cfg_u.ch_cfg.hs_sel_src = mem_type_src ? DMAC_HS_SOFTWARE : DMAC_HS_HARDWARE;
    cfg_u.ch_cfg.hs_sel_dst = mem_type_dest ? DMAC_HS_SOFTWARE : DMAC_HS_HARDWARE;
    cfg_u.ch_cfg.src_per = channel_num;
    cfg_u.ch_cfg.dst_per = channel_num;
    cfg_u.ch_cfg.src_multblk_type = 0;
    cfg_u.ch_cfg.dst_multblk_type = 0;

    writeq(cfg_u.data, &dmac->channel[channel_num].cfg);

    dmac->channel[channel_num].sar = (uint64_t)src;
    dmac->channel[channel_num].dar = (uint64_t)dest;

    ctl.data = readq(&dmac->channel[channel_num].ctl);
    ctl.ch_ctl.sms = DMAC_MASTER1;
    ctl.ch_ctl.dms = DMAC_MASTER2;
    /* master select */
    ctl.ch_ctl.sinc = src_inc;
    ctl.ch_ctl.dinc = dest_inc;
    /* address incrememt */
    ctl.ch_ctl.src_tr_width = dmac_trans_width;
    ctl.ch_ctl.dst_tr_width  = dmac_trans_width;
    /* transfer width */
    ctl.ch_ctl.src_msize = dmac_burst_size;
    ctl.ch_ctl.dst_msize = dmac_burst_size;

    writeq(ctl.data, &dmac->channel[channel_num].ctl);

    writeq(blockSize - 1, &dmac->channel[channel_num].block_ts);
    /*the number of (blcok_ts +1) data of width SRC_TR_WIDTF to be */
    /* transferred in a dma block transfer */
    return 0;
}

int dmac_get_channel_config(dmac_channel_number_t channel_num,
        dmac_channel_config_t *cfg_param)
{
    dmac_ch_ctl_u_t  ctl;
    dmac_ch_cfg_u_t cfg;
    dmac_ch_llp_u_t ch_llp;

    if (cfg_param == 0)
        return -1;
    if (channel_num < DMAC_CHANNEL0 ||
        channel_num > DMAC_CHANNEL3)
        return -1;

    ctl.data = readq(&dmac->channel[channel_num].ctl);

    cfg_param->ctl_sms = ctl.ch_ctl.sms;
    cfg_param->ctl_dms = ctl.ch_ctl.dms;
    cfg_param->ctl_sinc = ctl.ch_ctl.sinc;
    cfg_param->ctl_dinc = ctl.ch_ctl.dinc;
    cfg_param->ctl_src_tr_width = ctl.ch_ctl.src_tr_width;
    cfg_param->ctl_dst_tr_width = ctl.ch_ctl.dst_tr_width;
    cfg_param->ctl_src_msize = ctl.ch_ctl.src_msize;
    cfg_param->ctl_drc_msize = ctl.ch_ctl.dst_msize;
    cfg_param->ctl_ioc_blktfr = ctl.ch_ctl.ioc_blktfr;

    cfg.data = readq(&dmac->channel[channel_num].cfg);
    cfg_param->cfg_hs_sel_src = cfg.ch_cfg.hs_sel_src;
    cfg_param->cfg_hs_sel_dst = cfg.ch_cfg.hs_sel_dst;
    cfg_param->cfg_src_hs_pol = cfg.ch_cfg.src_hwhs_pol;
    cfg_param->cfg_dst_hs_pol =  cfg.ch_cfg.dst_hwhs_pol;
    cfg_param->cfg_src_per = cfg.ch_cfg.src_per;
    cfg_param->cfg_dst_per = cfg.ch_cfg.dst_per;
    cfg_param->cfg_ch_prior = cfg.ch_cfg.ch_prior;
    cfg_param->cfg_src_multblk_type = cfg.ch_cfg.src_multblk_type;
    cfg_param->cfg_dst_multblk_type = cfg.ch_cfg.dst_multblk_type;

    cfg_param->sar = dmac->channel[channel_num].sar;
    cfg_param->dar = dmac->channel[channel_num].dar;

    ch_llp.data = readq(&dmac->channel[channel_num].llp);
    cfg_param->llp_loc = ch_llp.llp.loc;
    cfg_param->llp_lms = ch_llp.llp.lms;

    cfg_param->ctl_block_ts = readq(&dmac->channel[channel_num].block_ts);

    return 0;
}

void dmac_set_address(dmac_channel_number_t channel_num, uint64_t src_addr,
        uint64_t dst_addr)
{
    writeq(src_addr, &dmac->channel[channel_num].sar);
    writeq(dst_addr, &dmac->channel[channel_num].dar);
}

void dmac_set_block_ts(dmac_channel_number_t channel_num,
        uint32_t block_size)
{
    uint32_t block_ts;

    block_ts = block_size & 0x3fffff;
    writeq(block_ts, &dmac->channel[channel_num].block_ts);
}

void dmac_source_control(dmac_channel_number_t channel_num,
        dmac_master_number_t master_select,
        dmac_address_increment_t address_mode,
        dmac_transfer_width_t tr_width,
        dmac_burst_trans_length_t burst_length)
{
    dmac_ch_ctl_u_t ctl_u;

    ctl_u.data = readq(&dmac->channel[channel_num].ctl);
    ctl_u.ch_ctl.sms = master_select;
    ctl_u.ch_ctl.sinc = address_mode;
    ctl_u.ch_ctl.src_tr_width = tr_width;
    ctl_u.ch_ctl.src_msize = burst_length;

    writeq(ctl_u.data, &dmac->channel[channel_num].ctl);
}

void dmac_master_control(dmac_channel_number_t channel_num,
        dmac_master_number_t master_select,
        dmac_address_increment_t address_mode,
        dmac_transfer_width_t tr_width,
        dmac_burst_trans_length_t burst_length)
{
    dmac_ch_ctl_u_t ctl_u;

    ctl_u.data = readq(&dmac->channel[channel_num].ctl);
    ctl_u.ch_ctl.dms = master_select;
    ctl_u.ch_ctl.dinc = address_mode;
    ctl_u.ch_ctl.dst_tr_width = tr_width;
    ctl_u.ch_ctl.dst_msize = burst_length;

    writeq(ctl_u.data, &dmac->channel[channel_num].ctl);
}

void dmac_set_source_transfer_control(dmac_channel_number_t channel_num,
                dmac_multiblk_transfer_type_t transfer_type,
                dmac_sw_hw_hs_select_t handshak_select)
{
    dmac_ch_cfg_u_t cfg_u;

    cfg_u.data = readq(&dmac->channel[channel_num].cfg);
    cfg_u.ch_cfg.src_multblk_type = transfer_type;
    cfg_u.ch_cfg.hs_sel_src = handshak_select;

    writeq(cfg_u.data, &dmac->channel[channel_num].cfg);
}

void dmac_set_destination_transfer_control(dmac_channel_number_t channel_num,
                dmac_multiblk_transfer_type_t transfer_type,
                dmac_sw_hw_hs_select_t handshak_select)
{
    dmac_ch_cfg_u_t cfg_u;

    cfg_u.data = readq(&dmac->channel[channel_num].cfg);
    cfg_u.ch_cfg.dst_multblk_type = transfer_type;
    cfg_u.ch_cfg.hs_sel_dst = handshak_select;

    writeq(cfg_u.data, &dmac->channel[channel_num].cfg);
}

void dmac_set_flow_control(dmac_channel_number_t channel_num,
            dmac_transfer_flow_t flow_control)
{
    dmac_ch_cfg_u_t cfg_u;

    cfg_u.data = readq(&dmac->channel[channel_num].cfg);
    cfg_u.ch_cfg.tt_fc = flow_control;

    writeq(cfg_u.data, &dmac->channel[channel_num].cfg);
}

void dmac_set_linked_list_addr_point(dmac_channel_number_t channel_num,
            uint64_t *addr)
{
    dmac_ch_llp_u_t llp_u;

    llp_u.data = readq(&dmac->channel[channel_num].llp);
    /* Cast pointer to uint64_t */
    llp_u.llp.loc = (uint64_t)addr;
    writeq(llp_u.data, &dmac->channel[channel_num].llp);
}

void dmac_init(void)
{
    uint64_t tmp;
    dmac_commonreg_intclear_u_t intclear;
    dmac_cfg_u_t dmac_cfg;
    dmac_reset_u_t dmac_reset;

    sysctl_clock_enable(SYSCTL_CLOCK_DMA);

    dmac_reset.data = readq(&dmac->reset);
    dmac_reset.reset.rst = 1;
    writeq(dmac_reset.data, &dmac->reset);
    while (dmac_reset.reset.rst)
        dmac_reset.data = readq(&dmac->reset);

    /*reset dmac */

    intclear.data = readq(&dmac->com_intclear);
    intclear.com_intclear.cear_slvif_dec_err_intstat = 1;
    intclear.com_intclear.clear_slvif_wr2ro_err_intstat = 1;
    intclear.com_intclear.clear_slvif_rd2wo_err_intstat = 1;
    intclear.com_intclear.clear_slvif_wronhold_err_intstat = 1;
    intclear.com_intclear.clear_slvif_undefinedreg_dec_err_intstat = 1;
    writeq(intclear.data, &dmac->com_intclear);
    /* clear common register interrupt */

    dmac_cfg.data = readq(&dmac->cfg);
    dmac_cfg.cfg.dmac_en = 0;
    dmac_cfg.cfg.int_en = 0;
    writeq(dmac_cfg.data, &dmac->cfg);
    /* disable dmac and disable interrupt */

    while (readq(&dmac->cfg))
        ;
    tmp = readq(&dmac->chen);
    tmp &= ~0xf;
    writeq(tmp, &dmac->chen);
    /* disable all channel before configure */
    dmac_enable();
}

static void list_add(struct list_head_t *new, struct list_head_t *prev,
        struct list_head_t *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

void list_add_tail(struct list_head_t *new, struct list_head_t *head)
{
    list_add(new, head->prev, head);
}

void INIT_LIST_HEAD(struct list_head_t *list)
{
    list->next = list;
    list->prev = list;
}

void dmac_link_list_item(dmac_channel_number_t channel_num,
    uint8_t LLI_row_num, int8_t LLI_last_row,
    dmac_lli_item_t *lli_item,
    dmac_channel_config_t *cfg_param)
{
    dmac_ch_ctl_u_t ctl;
    dmac_ch_llp_u_t  llp_u;

    lli_item[LLI_row_num].sar = cfg_param->sar;
    lli_item[LLI_row_num].dar = cfg_param->dar;

    ctl.data = readq(&dmac->channel[channel_num].ctl);
    ctl.ch_ctl.sms = cfg_param->ctl_sms;
    ctl.ch_ctl.dms = cfg_param->ctl_dms;
    ctl.ch_ctl.sinc = cfg_param->ctl_sinc;
    ctl.ch_ctl.dinc = cfg_param->ctl_dinc;
    ctl.ch_ctl.src_tr_width = cfg_param->ctl_src_tr_width;
    ctl.ch_ctl.dst_tr_width = cfg_param->ctl_dst_tr_width;
    ctl.ch_ctl.src_msize = cfg_param->ctl_src_msize;
    ctl.ch_ctl.dst_msize = cfg_param->ctl_drc_msize;
    ctl.ch_ctl.src_stat_en = cfg_param->ctl_src_stat_en;
    ctl.ch_ctl.dst_stat_en = cfg_param->ctl_dst_stat_en;

    if (LLI_last_row != LAST_ROW) {
        ctl.ch_ctl.shadowreg_or_lli_valid = 1;
        ctl.ch_ctl.shadowreg_or_lli_last = 0;
    } else {
        ctl.ch_ctl.shadowreg_or_lli_valid = 1;
        ctl.ch_ctl.shadowreg_or_lli_last = 1;
    }

    lli_item[LLI_row_num].ctl = ctl.data;

    lli_item[LLI_row_num].ch_block_ts = cfg_param->ctl_block_ts;
    lli_item[LLI_row_num].sstat = 0;
    lli_item[LLI_row_num].dstat = 0;

    llp_u.data = readq(&dmac->channel[channel_num].llp);

    if (LLI_last_row != LAST_ROW)
        llp_u.llp.loc = ((uint64_t)&lli_item[LLI_row_num + 1]) >> 6;
    else
        llp_u.llp.loc = 0;

    lli_item[LLI_row_num].llp = llp_u.data;
}

void dmac_update_shandow_register(dmac_channel_number_t channel_num,
        int8_t last_block, dmac_channel_config_t *cfg_param)
{
    dmac_ch_ctl_u_t ctl_u;

    do {
        ctl_u.data = readq(&dmac->channel[channel_num].ctl);
    } while (ctl_u.ch_ctl.shadowreg_or_lli_valid);

    writeq(cfg_param->sar, &dmac->channel[channel_num].sar);
    writeq(cfg_param->dar, &dmac->channel[channel_num].dar);
    writeq(cfg_param->ctl_block_ts, &dmac->channel[channel_num].block_ts);

    ctl_u.ch_ctl.sms = cfg_param->ctl_sms;
    ctl_u.ch_ctl.dms = cfg_param->ctl_dms;
    ctl_u.ch_ctl.sinc = cfg_param->ctl_sinc;
    ctl_u.ch_ctl.dinc = cfg_param->ctl_dinc;
    ctl_u.ch_ctl.src_tr_width = cfg_param->ctl_src_tr_width;
    ctl_u.ch_ctl.dst_tr_width = cfg_param->ctl_dst_tr_width;
    ctl_u.ch_ctl.src_msize = cfg_param->ctl_src_msize;
    ctl_u.ch_ctl.dst_msize = cfg_param->ctl_drc_msize;
    ctl_u.ch_ctl.src_stat_en = cfg_param->ctl_src_stat_en;
    ctl_u.ch_ctl.dst_stat_en = cfg_param->ctl_dst_stat_en;
    if (last_block != LAST_ROW)
    {
        ctl_u.ch_ctl.shadowreg_or_lli_valid = 1;
        ctl_u.ch_ctl.shadowreg_or_lli_last = 0;
    } else {
        ctl_u.ch_ctl.shadowreg_or_lli_valid = 1;
        ctl_u.ch_ctl.shadowreg_or_lli_last = 1;
    }

    writeq(ctl_u.data, &dmac->channel[channel_num].ctl);
    writeq(0, &dmac->channel[channel_num].blk_tfr);
}

void dmac_set_shadow_invalid_flag(dmac_channel_number_t channel_num)
{
    dmac_ch_ctl_u_t ctl_u;

    ctl_u.data = readq(&dmac->channel[channel_num].ctl);
    ctl_u.ch_ctl.shadowreg_or_lli_valid = 1;
    ctl_u.ch_ctl.shadowreg_or_lli_last = 0;
    writeq(ctl_u.data, &dmac->channel[channel_num].ctl);
}

void dmac_set_single_mode(dmac_channel_number_t channel_num,
                          const void *src, void *dest, dmac_address_increment_t src_inc,
                          dmac_address_increment_t dest_inc,
                          dmac_burst_trans_length_t dmac_burst_size,
                          dmac_transfer_width_t dmac_trans_width,
                          size_t block_size) {
    dmac_chanel_interrupt_clear(channel_num);
    dmac_channel_disable(channel_num);
    dmac_wait_idle(channel_num);
    dmac_set_channel_param(channel_num, src, dest, src_inc, dest_inc,
                           dmac_burst_size, dmac_trans_width, block_size);
    dmac_enable();
    dmac_channel_enable(channel_num);
}

int dmac_is_done(dmac_channel_number_t channel_num)
{
    if(readq(&dmac->channel[channel_num].intstatus) & 0x2)
        return 1;
    else
        return 0;
}

void dmac_wait_done(dmac_channel_number_t channel_num)
{
    dmac_wait_idle(channel_num);
}

int dmac_is_idle(dmac_channel_number_t channel_num)
{
    dmac_chen_u_t chen;
    chen.data = readq(&dmac->chen);
    if((chen.data >> channel_num) & 0x1UL)
        return 0;
    else
        return 1;
}

void dmac_wait_idle(dmac_channel_number_t channel_num)
{
    while(!dmac_is_idle(channel_num));
    dmac_chanel_interrupt_clear(channel_num); /* clear interrupt */
}

void dmac_set_src_dest_length(dmac_channel_number_t channel_num, const void *src, void *dest, size_t len)
{
    if(src != NULL)
        dmac->channel[channel_num].sar = (uint64_t)src;
    if(dest != NULL)
        dmac->channel[channel_num].dar = (uint64_t)dest;
    if(len > 0)
        dmac_set_block_ts(channel_num, len - 1);
    dmac_channel_enable(channel_num);
}

static int dmac_irq_callback(void *ctx)
{
    dmac_context_t *v_dmac_context = (dmac_context_t *)(ctx);
    dmac_channel_number_t v_dmac_channel = v_dmac_context->dmac_channel;
    dmac_chanel_interrupt_clear(v_dmac_channel);
    if(v_dmac_context->callback != NULL)
        v_dmac_context->callback(v_dmac_context->ctx);

    return 0;
}

void dmac_irq_register(dmac_channel_number_t channel_num , plic_irq_callback_t dmac_callback, void *ctx, uint32_t priority)
{
    dmac_context[channel_num].dmac_channel = channel_num;
    dmac_context[channel_num].callback = dmac_callback;
    dmac_context[channel_num].ctx = ctx;
    dmac_enable_channel_interrupt(channel_num);
    plic_set_priority(IRQN_DMA0_INTERRUPT + channel_num, priority);
    plic_irq_enable(IRQN_DMA0_INTERRUPT + channel_num);
    plic_irq_register(IRQN_DMA0_INTERRUPT + channel_num, dmac_irq_callback, &dmac_context[channel_num]);
}

void __attribute__((weak, alias("dmac_irq_register"))) dmac_set_irq(dmac_channel_number_t channel_num , plic_irq_callback_t dmac_callback, void *ctx, uint32_t priority);

void dmac_irq_unregister(dmac_channel_number_t channel_num)
{
    dmac_context[channel_num].callback = NULL;
    dmac_context[channel_num].ctx = NULL;
    dmac_disable_channel_interrupt(channel_num);
    plic_irq_unregister(IRQN_DMA0_INTERRUPT + channel_num);
}

void __attribute__((weak, alias("dmac_irq_unregister"))) dmac_free_irq(dmac_channel_number_t channel_num);

