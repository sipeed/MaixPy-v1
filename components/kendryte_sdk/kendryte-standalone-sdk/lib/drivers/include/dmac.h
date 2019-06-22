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
#ifndef _DRIVER_DMAC_H
#define _DRIVER_DMAC_H

#include <stdint.h>
#include "io.h"
#include "platform.h"
#include "stdbool.h"
#include "plic.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* DMAC */
#define DMAC_CHANNEL_COUNT (DMAC_CHANNEL_MAX)
#define LAST_ROW (-1)

typedef enum _dmac_channel_number
{
    DMAC_CHANNEL0 = 0,
    DMAC_CHANNEL1 = 1,
    DMAC_CHANNEL2 = 2,
    DMAC_CHANNEL3 = 3,
    DMAC_CHANNEL4 = 4,
    DMAC_CHANNEL5 = 5,
    DMAC_CHANNEL_MAX
} dmac_channel_number_t;

typedef enum _dmac_src_dst_select
{
    DMAC_SRC = 0x1,
    DMAC_DST = 0x2,
    DMAC_SRC_DST = 0x3
} dmac_src_dst_select_t;

typedef enum _state_value
{
    clear = 0,
    set = 1
} state_value_t;

typedef enum _dmac_lock_bus_ch
{
    DMAC_LOCK_BUS     = 0x1,
    DMAC_LOCK_CHANNEL = 0x2,
    DMAC_LOCK_BUS_CH  = 0x3
} dmac_lock_bus_ch_t;

typedef enum _dmac_sw_hw_hs_select
{
    DMAC_HS_HARDWARE = 0x0,
    DMAC_HS_SOFTWARE = 0x1
} dmac_sw_hw_hs_select_t;

typedef enum _dmac_scatter_gather_param
{
    DMAC_SG_COUNT = 0x0,
    DMAC_SG_INTERVAL = 0x1
} dmac_scatter_gather_param_t;

typedef enum _dmac_irq
{
    /* no interrupts */
    DMAC_IRQ_NONE    = 0x00,
    /* transfer complete */
    DMAC_IRQ_TFR     = 0x01,
    /* block transfer complete */
    DMAC_IRQ_BLOCK   = 0x02,
    /* source transaction complete */
    DMAC_IRQ_SRCTRAN = 0x04,
    /* destination transaction complete */
    DMAC_IRQ_DSTTRAN = 0x08,
    /* error */
    DMAC_IRQ_ERR     = 0x10,
    /* all interrupts */
    DMAC_IRQ_ALL     = 0x1f
} dmac_irq_t;

typedef enum _dmac_software_req
{
    /* ReqSrcReq/ReqDstReq */
    DMAC_REQUEST    = 0x1,
    /* SglReqSrcReq/SglReqDstReq */
    DMAC_SINGLE_REQUEST = 0x2,
    /* LstReqSrcReq/LstReqDstReq */
    DMAC_LAST_REQUEST   = 0x4
} dmac_software_req_t;

typedef enum _dmac_master_number
{
    DMAC_MASTER1 = 0x0,
    DMAC_MASTER2 = 0x1
} dmac_master_number_t;

typedef enum _dmac_transfer_flow
{
    /* mem to mem - DMAC   flow ctlr */
    DMAC_MEM2MEM_DMA    = 0x0,
    /* mem to prf - DMAC   flow ctlr */
    DMAC_MEM2PRF_DMA    = 0x1,
    /* prf to mem - DMAC   flow ctlr */
    DMAC_PRF2MEM_DMA    = 0x2,
    /* prf to prf - DMAC   flow ctlr */
    DMAC_PRF2PRF_DMA    = 0x3,
    /* prf to mem - periph flow ctlr */
    DMAC_PRF2MEM_PRF    = 0x4,
    /* prf to prf - source flow ctlr */
    DMAC_PRF2PRF_SRCPRF = 0x5,
    /* mem to prf - periph flow ctlr */
    DMAC_MEM2PRF_PRF    = 0x6,
    /* prf to prf - dest   flow ctlr */
    DMAC_PRF2PRF_DSTPRF = 0x7
} dmac_transfer_flow_t;

typedef enum _dmac_burst_trans_length
{
    DMAC_MSIZE_1   = 0x0,
    DMAC_MSIZE_4   = 0x1,
    DMAC_MSIZE_8   = 0x2,
    DMAC_MSIZE_16  = 0x3,
    DMAC_MSIZE_32  = 0x4,
    DMAC_MSIZE_64  = 0x5,
    DMAC_MSIZE_128 = 0x6,
    DMAC_MSIZE_256 = 0x7
} dmac_burst_trans_length_t;

typedef enum _dmac_address_increment
{
    DMAC_ADDR_INCREMENT = 0x0,
    DMAC_ADDR_NOCHANGE  = 0x1
} dmac_address_increment_t;

typedef enum _dmac_transfer_width
{
    DMAC_TRANS_WIDTH_8   = 0x0,
    DMAC_TRANS_WIDTH_16  = 0x1,
    DMAC_TRANS_WIDTH_32  = 0x2,
    DMAC_TRANS_WIDTH_64  = 0x3,
    DMAC_TRANS_WIDTH_128 = 0x4,
    DMAC_TRANS_WIDTH_256 = 0x5
} dmac_transfer_width_t;

typedef enum _dmac_hs_interface
{
    DMAC_HS_IF0  = 0x0,
    DMAC_HS_IF1  = 0x1,
    DMAC_HS_IF2  = 0x2,
    DMAC_HS_IF3  = 0x3,
    DMAC_HS_IF4  = 0x4,
    DMAC_HS_IF5  = 0x5,
    DMAC_HS_IF6  = 0x6,
    DMAC_HS_IF7  = 0x7,
    DMAC_HS_IF8  = 0x8,
    DMAC_HS_IF9  = 0x9,
    DMAC_HS_IF10 = 0xa,
    DMAC_HS_IF11 = 0xb,
    DMAC_HS_IF12 = 0xc,
    DMAC_HS_IF13 = 0xd,
    DMAC_HS_IF14 = 0xe,
    DMAC_HS_IF15 = 0xf
} dmac_hs_interface_t;

typedef enum _dmac_multiblk_transfer_type
{
    CONTIGUOUS     = 0,
    RELOAD   = 1,
    SHADOWREGISTER = 2,
    LINKEDLIST     = 3
} dmac_multiblk_transfer_type_t;

typedef enum _dmac_multiblk_type
{
    DMAC_SRC_DST_CONTINUE          = 0,
    DMAC_SRC_CONTINUE_DST_RELAOD       = 2,
    DMAC_SRC_CONTINUE_DST_LINKEDLIST   = 3,
    DMAC_SRC_RELOAD_DST_CONTINUE       = 4,
    DMAC_SRC_RELOAD_DST_RELOAD   = 5,
    DMAC_SRC_RELOAD_DST_LINKEDLIST     = 6,
    DMAC_SRC_LINKEDLIST_DST_CONTINUE   = 7,
    DMAC_SRC_LINKEDLIST_DST_RELOAD     = 8,
    DMAC_SRC_LINKEDLIST_DST_LINKEDLIST = 9,
    DMAC_SRC_SHADOWREG_DST_CONTINUE    = 10
} dmac_multiblk_type_t;

typedef enum _dmac_transfer_type
{
    DMAC_TRANSFER_ROW1  = 0x1,
    DMAC_TRANSFER_ROW2  = 0x2,
    DMAC_TRANSFER_ROW3  = 0x3,
    DMAC_TRANSFER_ROW4  = 0x4,
    DMAC_TRANSFER_ROW5  = 0x5,
    DMAC_TRANSFER_ROW6  = 0x6,
    DMAC_TRANSFER_ROW7  = 0x7,
    DMAC_TRANSFER_ROW8  = 0x8,
    DMAC_TRANSFER_ROW9  = 0x9,
    DMAC_TRANSFER_ROW10 = 0xa
} dmac_transfer_type_t;

typedef enum _dmac_prot_level
{
    /* default prot level */
    DMAC_NONCACHE_NONBUFF_NONPRIV_OPCODE = 0x0,
    DMAC_NONCACHE_NONBUFF_NONPRIV_DATA   = 0x1,
    DMAC_NONCACHE_NONBUFF_PRIV_OPCODE    = 0x2,
    DMAC_NONCACHE_NONBUFF_PRIV_DATA      = 0x3,
    DMAC_NONCACHE_BUFF_NONPRIV_OPCODE    = 0x4,
    DMAC_NONCACHE_BUFF_NONPRIV_DATA      = 0x5,
    DMAC_NONCACHE_BUFF_PRIV_OPCODE       = 0x6,
    DMAC_NONCACHE_BUFF_PRIV_DATA     = 0x7,
    DMAC_CACHE_NONBUFF_NONPRIV_OPCODE    = 0x8,
    DMAC_CACHE_NONBUFF_NONPRIV_DATA      = 0x9,
    DMAC_CACHE_NONBUFF_PRIV_OPCODE       = 0xa,
    DMAC_CACHE_NONBUFF_PRIV_DATA     = 0xb,
    DMAC_CACHE_BUFF_NONPRIV_OPCODE       = 0xc,
    DMAC_CACHE_BUFF_NONPRIV_DATA     = 0xd,
    DMAC_CACHE_BUFF_PRIV_OPCODE   = 0xe,
    DMAC_CACHE_BUFF_PRIV_DATA       = 0xf
} dmac_prot_level_t;

typedef enum _dmac_fifo_mode
{
    DMAC_FIFO_MODE_SINGLE = 0x0,
    DMAC_FIFO_MODE_HALF = 0x1
} dmac_fifo_mode_t;

typedef enum _dw_dmac_flow_ctl_mode
{
    DMAC_DATA_PREFETCH_ENABLED  = 0x0,
    DMAC_DATA_PREFETCH_DISABLED = 0x1
} dw_dmac_flow_ctl_mode_t;

typedef enum _dmac_polarity_level
{
    DMAC_ACTIVE_HIGH = 0x0,
    DMAC_ACTIVE_LOW = 0x1
} dmac_polarity_level_t;

typedef enum _dmac_lock_level
{
    DMAC_LOCK_LEVEL_DMA_TRANSFER   = 0x0,
    DMAC_LOCK_LEVEL_BLOCK_TRANSFER = 0x1,
    DMAC_LOCK_LEVEL_TRANSACTION    = 0x2
} dmac_lock_level_t;

typedef enum _dmac_channel_priority
{
    DMAC_PRIORITY_0 = 0x0,
    DMAC_PRIORITY_1 = 0x1,
    DMAC_PRIORITY_2 = 0x2,
    DMAC_PRIORITY_3 = 0x3,
    DMAC_PRIORITY_4 = 0x4,
    DMAC_PRIORITY_5 = 0x5,
    DMAC_PRIORITY_6 = 0x6,
    DMAC_PRIORITY_7 = 0x7
} dmac_channel_priority_t;

typedef enum _dmac_state
{
    ZERO,
    ONE
} dmac_state_t;

typedef enum _dmac_common_int
{
    SLVIF_COMMON_DEC_ERR       = 0,
    SLVIF_COMMON_WR2RO_ERR     = 1,
    SLVIF_COMMON_RD2WO_ERR     = 2,
    SLVIF_COMMON__WRONHOLD_ERR = 3,
    SLVIF_UNDEFINED_DEC_ERR    = 4,
    SLVIF_ALL_INT          = 5
} dmac_common_int_t;

typedef struct _dmac_cfg
{
    /**
     * Bit 0 is used to enable dmac
     * 0x1 for enable, 0x0 for disable
     */
    uint64_t dmac_en : 1;
    /**
     * Bit 1 is used to glabally enable interrupt generation
     * 0x1 for enable interrupt, 0x0 for disable interrupt
     */
    uint64_t int_en : 1;
    /* Bits [63:2] is reserved */
    uint64_t rsvd : 62;
} __attribute__((packed, aligned(8))) dmac_cfg_t;

typedef union _dmac_cfg_u
{
    dmac_cfg_t cfg;
    uint64_t data;
} dmac_cfg_u_t;

typedef struct _damc_chen
{
    /**
     * Bit 0 is used to enable channel 1
     * 0x1 for enable, 0x0 for disable
     */
    uint64_t ch1_en : 1;
    /**
     * Bit 1 is used to enable channel 2
     * 0x1 for enable, 0x0 for disable
     */
    uint64_t ch2_en : 1;
    /**
     * Bit 2 is used to enable channel 3
     * 0x1 for enable, 0x0 for disable
     */
    uint64_t ch3_en : 1;
    /**
     * Bit 3 is used to enable channel 4
     * 0x1 for enable, 0x0 for disable
     */
    uint64_t ch4_en : 1;
    /**
     * Bit 4 is used to enable channel 5
     * 0x1 for enable, 0x0 for disable
     */
    uint64_t ch5_en : 1;
    /**
     * Bit 5 is used to enable channel 6
     * 0x1 for enable, 0x0 for disable
     */
    uint64_t ch6_en : 1;
    /* Bits [7:6] is reserved */
    uint64_t rsvd1 : 2;
    /**
     * Bit 8 is write enable bit
     * 0x1 for enable, 0x0 for disable
     */
    uint64_t ch1_en_we : 1;
    /**
     * Bit 9 is write enable bit
     * 0x1 for enable, 0x0 for disable
     */
    uint64_t ch2_en_we : 1;
    /**
     * Bit 10 is write enable bit
     * 0x1 for enable, 0x0 for disable
     */
    uint64_t ch3_en_we : 1;
    /**
     * Bit 11 is write enable bit
     * 0x1 for enable, 0x0 for disable
     */
    uint64_t ch4_en_we : 1;
    /**
     * Bit 12 is write enable bit
     * 0x1 for enable, 0x0 for disable
     */
    uint64_t ch5_en_we : 1;
    /**
     * Bit 13 is write enable bit
     * 0x1 for enable, 0x0 for disable
     */
    uint64_t ch6_en_we : 1;
    /* Bits [15:14] is reserved */
    uint64_t rsvd2 : 2;
    /**
     * Bit 16 is susped reuest
     * 0x1 for request channel suspend
     * 0x0 for no channel suspend request
     */
    uint64_t ch1_susp : 1;
    /**
     * Bit 17 is susped reuest
     * 0x1 for request channel suspend
     * 0x0 for no channel suspend request
     */
    uint64_t ch2_susp : 1;
    /* Bit 18 is susped reuest
     * 0x1 for request channel suspend
     * 0x0 for no channel suspend request
     */
    uint64_t ch3_susp : 1;
    /**
     * Bit 19 is susped reuest
     * 0x1 for request channel suspend
     * 0x0 for no channel suspend request
     */
    uint64_t ch4_susp : 1;
    /**
     * Bit 20 is susped reuest
     * 0x1 for request channel suspend
     * 0x0 for no channel suspend request
     */
    uint64_t ch5_susp : 1;
    /**
     * Bit 21 is susped reuest
     * 0x1 for request channel suspend
     * 0x0 for no channel suspend request
     */
    uint64_t ch6_susp : 1;
    /* Bits [23:22] is reserved */
    uint64_t rsvd3 : 2;
    /**
     * Bit  24 is write enable to the channel suspend bit
     * 0x1 for enable write to CH1_SUSP bit
     * 0x0 for disable write to CH1_SUSP bit
     */
    uint64_t ch1_susp_we : 1;
    /**
     * Bit  25 is write enable to the channel suspend bit
     * 0x1 for enable write to CH2_SUSP bit
     * 0x0 for disable write to CH2_SUSP bit
     */
    uint64_t ch2_susp_we : 1;
    /**
     * Bit  26 is write enable to the channel suspend bit
     * 0x1 for enable write to CH3_SUSP bit
     * 0x0 for disable write to CH3_SUSP bit
     */
    uint64_t ch3_susp_we : 1;
    /**
     * Bit  27 is write enable to the channel suspend bit
     * 0x1 for enable write to CH4_SUSP bit
     * 0x0 for disable write to CH4_SUSP bit
     */
    uint64_t ch4_susp_we : 1;
    /**
     * Bit  28 is write enable to the channel suspend bit
     * 0x1 for enable write to CH5_SUSP bit
     * 0x0 for disable write to CH5_SUSP bit
     */
    uint64_t ch5_susp_we : 1;
    /**
     * Bit  29 is write enable to the channel suspend bit
     * 0x1 for enable write to CH6_SUSP bit
     * 0x0 for disable write to CH6_SUSP bit
     */
    uint64_t ch6_susp_we : 1;
    /* Bits [31:30] is reserved */
    uint64_t rsvd4 : 2;
    /**
     * Bit  32 is channel-1 abort requst bit
     * 0x1 for request for channnel abort
     * 0x0 for no channel abort request
     */
    uint64_t ch1_abort : 1;
    /**
     * Bit  33 is channel-2 abort requst bit
     * 0x1 for request for channnel abort
     * 0x0 for no channel abort request
     */
    uint64_t ch2_abort : 1;
    /**
     * Bit  34 is channel-3 abort requst bit
     * 0x1 for request for channnel abort
     * 0x0 for no channel abort request
     */
    uint64_t ch3_abort : 1;
    /**
     * Bit  35 is channel-4 abort requst bit
     * 0x1 for request for channnel abort
     * 0x0 for no channel abort request
     */
    uint64_t ch4_abort : 1;
    /**
     * Bit  36 is channel-5 abort requst bit
     * 0x1 for request for channnel abort
     * 0x0 for no channel abort request
     */
    uint64_t ch5_abort : 1;
    /**
     * Bit  37 is channel-6 abort requst bit
     * 0x1 for request for channnel abort
     * 0x0 for no channel abort request
     */
    uint64_t ch6_abort : 1;
    /* Bits [39:38] is reserved */
    uint64_t rsvd5 : 2;
    /**
     * Bit 40 is ued to write enable  channel-1 abort bit
     * 0x1 for enable write to CH1_ABORT bit
     * 0x0 for disable write to CH1_ABORT bit
     */
    uint64_t ch1_abort_we : 1;
    /**
     * Bit 41 is ued to write enable  channel-2 abort bit
     * 0x1 for enable write to CH2_ABORT bit
     * 0x0 for disable write to CH2_ABORT bit
     */
    uint64_t ch2_abort_we : 1;
    /**
     * Bit 42 is ued to write enable  channel-3 abort bit
     * 0x1 for enable write to CH3_ABORT bit
     * 0x0 for disable write to CH3_ABORT bit
     */
    uint64_t ch3_abort_we : 1;
    /**
     * Bit 43 is ued to write enable  channel-4 abort bit
     * 0x1 for enable write to CH4_ABORT bit
     * 0x0 for disable write to CH4_ABORT bit
     */
    uint64_t ch4_abort_we : 1;
    /**
     * Bit 44 is ued to write enable  channel-5 abort bit
     * 0x1 for enable write to CH5_ABORT bit
     * 0x0 for disable write to CH5_ABORT bit
     */
    uint64_t ch5_abort_we : 1;
    /**
     * Bit 45 is ued to write enable  channel-6 abort bit
     * 0x1 for enable write to CH6_ABORT bit
     * 0x0 for disable write to CH6_ABORT bit
     */
    uint64_t ch6_abort_we : 1;
    /* Bits [47:46] is reserved */
    uint64_t rsvd6 : 2;
    /* Bits [63:48] is reserved */
    uint64_t rsvd7 : 16;
} __attribute__((packed, aligned(8))) damc_chen_t;

typedef union _dmac_chen_u
{
    damc_chen_t dmac_chen;
    uint64_t data;
} dmac_chen_u_t;

typedef struct _dmac_intstatus
{
    /**
     * Bit 0 is channel 1 interrupt bit
     * 0x1 for channel 1 interrupt  active
     * 0x0 for channel 1 interrupt inactive
     */
    uint64_t ch1_intstat : 1;
    /**
     * Bit 1 is channel 1 interrupt bit
     * 0x1 for channel 2 interrupt  active
     * 0x0 for channel 2 interrupt inactive
     */
    uint64_t ch2_intstat : 1;
    /**
     * Bit 2 is channel 3 interrupt bit
     * 0x1 for channel 3 interrupt  active
     * 0x0 for channel 3 interrupt inactive
     */
    uint64_t ch3_intstat : 1;
    /**
     * Bit 3 is channel 4 interrupt bit
     * 0x1 for channel 4 interrupt  active
     * 0x0 for channel 4 interrupt inactive
     */
    uint64_t ch4_intstat : 1;
    /**
     * Bit 4 is channel 5 interrupt bit
     * 0x1 for channel 5 interrupt  active
     * 0x0 for channel 5 interrupt inactive
     */
    uint64_t ch5_intstat : 1;
    /**
     * Bit 5 is channel 6 interrupt bit
     * 0x1 for channel 6 interrupt  active
     * 0x0 for channel 6 interrupt inactive
     */
    uint64_t ch6_intstat : 1;
    /* Bits [15:6] is reserved */
    uint64_t rsvd1 : 10;
    /**
     * Bit 16 is commom register status bit
     * 0x1 for common register interrupt is active
     * 0x0 for common register interrupt inactive
     */
    uint64_t commonreg_intstat : 1;
    /* Bits [63:17] is reserved */
    uint64_t rsvd2 : 47;
} __attribute__((packed, aligned(8))) dmac_intstatus_t;

typedef union _dmac_intstatus_u
{
    dmac_intstatus_t intstatus;
    uint64_t data;
} dmac_intstatus_u_t;

typedef struct _dmac_commonreg_intclear
{
    /**
     * Bit 0 is slave nterface Common Register
     * Decode Error Interrupt clear Bit
     * x01 for clear SLVIF_CommonReg_DEC_ERR interrupt
     * in DMAC_COMMONREG_INTSTATUSREG
     * 0x0 for inactive signal
     */
    uint64_t cear_slvif_dec_err_intstat : 1;
    /**
     * Bit 1 is Slave Interface Common Register Write
     * to Read only Error Interrupt clear Bit
     * x01 for clear SLVIF_CommonReg_WR2RO_ERR interrupt
     * in DMAC_COMMONREG_INTSTATUSREG
     * 0x0 for inactive signal
     */
    uint64_t clear_slvif_wr2ro_err_intstat : 1;
    /**
     * Bit 2 is Slave Interface Common Register Read to
     * Write only Error Interrupt clear Bit
     * x01 for clear SLVIF_CommonReg_RD2WO_ERR  interrupt
     * in DMAC_COMMONREG_INTSTATUSREG
     * 0x0 for inactive signal
     */
    uint64_t clear_slvif_rd2wo_err_intstat : 1;
    /**
     * Bit 3 is Slave Interface Common Register Write
     * On Hold Error Interrupt clear Bit
     * x01 for clear SSLVIF_CommonReg_WrOnHold_ERR interrupt
     * in DMAC_COMMONREG_INTSTATUSREG
     * 0x0 for inactive signal
     */
    uint64_t clear_slvif_wronhold_err_intstat : 1;
    /* Bits [7:4] is reserved */
    uint64_t rsvd1 : 4;
    /**
     * Bit 8 is Slave Interface Undefined register
     * Decode Error Interrupt clear Bit
     * x01 for clear SLVIF_UndefinedReg_DEC_ERRinterrupt
     * in DMAC_COMMONREG_INTSTATUSREG
     * 0x0 for inactive signal
     */
    uint64_t clear_slvif_undefinedreg_dec_err_intstat : 1;
    /* Bits [63:9] is reserved */
    uint64_t rsvd2 : 55;
} __attribute__((packed, aligned(8))) dmac_commonreg_intclear_t;

typedef union _dmac_commonreg_intclear_u
{
    dmac_commonreg_intclear_t com_intclear;
    uint64_t data;
} dmac_commonreg_intclear_u_t;

typedef struct _dmac_commonreg_intstatus_enable
{
    /**
     * Bit 0 is Slave Interface Common Register Decode Error
     * Interrupt Status Enable Bit
     * 0x1 for SLVIF_CommonReg_DEC_ERR_IntStat bit enable
     * 0x0 for SLVIF_CommonReg_DEC_ERR_IntStat bit disable
     */
    uint64_t enable_slvif_dec_err_intstat : 1;
    /**
     * Bit 1 is Slave Interface Common Register Write to Read
     * only Error Interrupt Status Enable Bit
     * 0x1 for SLVIF_CommonReg_WR2RO_ERR_IntStat bit enable
     * 0x0 for SLVIF_CommonReg_WR2RO_ERR_IntStat bit disable
     */
    uint64_t enable_slvif_wr2ro_err_intstat : 1;
    /*!<
     * Bit 2 is Slave Interface Common Register Read to Write
     * only Error Interrupt Status Enable Bit
     * 0x1 for SLVIF_CommonReg_RD2WO_ERR_IntStat bit enable
     * 0x0 for SLVIF_CommonReg_RD2WO_ERR_IntStat bit disable
     */
    uint64_t enable_slvif_rd2wo_err_intstat : 1;
    /**
     * Bit 3 is Slave Interface Common Register Write On Hold
     * Error Interrupt Status Enable Bit
     * 0x1 for SLVIF_CommonReg_WrOnHold_ERR_IntStat bit enable
     * 0x0 for SLVIF_CommonReg_WrOnHold_ERR_IntStat bit disable
     */
    uint64_t enable_slvif_wronhold_err_intstat : 1;
    /* Bits [7:4] is reserved */
    uint64_t rsvd1 : 4;
    /**
     * Bit 8 is Slave Interface Undefined register Decode
     * Error Interrupt Status enable Bit
     * 0x1 for SLVIF_UndefinedReg_DEC_ERR_IntStat bit enable
     * 0x0 for SLVIF_UndefinedReg_DEC_ERR_IntStat disable
     */
    uint64_t enable_slvif_undefinedreg_dec_err_intstat : 1;
    /* Bits [63:9] is reserved */
    uint64_t rsvd2 : 55;
} __attribute__((packed, aligned(8))) dmac_commonreg_intstatus_enable_t;

typedef union _dmac_commonreg_intstatus_enable_u
{
    dmac_commonreg_intstatus_enable_t intstatus_enable;
    uint64_t data;
} dmac_commonreg_intstatus_enable_u_t;

typedef struct _dmac_commonreg_intsignal_enable
{
    /**
     * Bit 0 is Slave Interface Common Register Decode Error
     * Interrupt Signal Enable Bit
     * 0x1 for SLVIF_CommonReg_DEC_ERR_IntStat signal enable
     * 0x0 for SLVIF_CommonReg_DEC_ERR_IntStat signal disable
     */
    uint64_t enable_slvif_dec_err_intsignal : 1;
    /**
     * Bit 1 is Slave Interface Common Register Write to Read only
     *  Error Interrupt Signal Enable Bit
     * 0x1 for SLVIF_CommonReg_WR2RO_ERR_IntStat signal enable
     * 0x0 for SLVIF_CommonReg_WR2RO_ERR_IntStat signal disable
     */
    uint64_t enable_slvif_wr2ro_err_intsignal : 1;
    /**
     * Bit 2 is Slave Interface Common Register Read to
     *  Write only Error Interrupt Status Enable Bit
     * 0x1 for SLVIF_CommonReg_RD2WO_ERR_IntStat bit enable
     * 0x0 for SLVIF_CommonReg_RD2WO_ERR_IntStat bit disable
     */
    uint64_t enable_slvif_rd2wo_err_intsignal : 1;
    /**
     * Bit 3 is Slave Interface Common Register Write On Hold Error
     * Interrupt Signal Enable Bit
     * 0x1 for SLVIF_CommonReg_WrOnHold_ERR_IntStat signal enable
     * 0x0 for SLVIF_CommonReg_WrOnHold_ERR_IntStat signal disable
     */
    uint64_t enable_slvif_wronhold_err_intsignal : 1;
    /* Bits [7:4] is reserved */
    uint64_t rsvd1 : 4;
    /**
     * Bit 8 is Slave Interface Undefined register Decode Error
     * Interrupt Signal Enable Bit
     * 0x1 for SLVIF_UndefinedReg_DEC_ERR_IntStat signal enable
     * 0x0 for SLVIF_UndefinedReg_DEC_ERR_IntStat signal disable
     */
    uint64_t enable_slvif_undefinedreg_dec_err_intsignal : 1;
    /* Bits [63:9] is reserved */
    uint64_t rsvd2 : 55;
} __attribute__((packed, aligned(8))) dmac_commonreg_intsignal_enable_t;

typedef union _dmac_commonreg_intsignal_enable_u
{
    dmac_commonreg_intsignal_enable_t intsignal_enable;
    uint64_t data;
} dmac_commonreg_intsignal_enable_u_t;

typedef struct _dmac_commonreg_intstatus
{
    /**
     * Bit 0 is Slave Interface Common Register Decode
     * Error Interrupt Status Bit
     * 0x1 for Slave Interface Decode Error detected
     * 0x0 for No Slave Interface Decode Errors
     */
    uint64_t slvif_dec_err_intstat : 1;
    /**
     * Bit 1 is Slave Interface Common Register Write to Read Only
     * Error Interrupt Status bit
     * 0x1 for Slave Interface Write to Read Only Error detected
     * 0x0 No Slave Interface Write to Read Only Errors
     */
    uint64_t slvif_wr2ro_err_intstat : 1;
    /**
     * Bit 2 is Slave Interface Common Register Read to Write
     * only Error Interrupt Status bit
     * 0x1 for Slave Interface Read to Write Only Error detected
     * 0x0 for No Slave Interface Read to Write Only Errors
     */
    uint64_t slvif_rd2wo_err_intstat : 1;
    /**
     * Bit 3 is Slave Interface Common Register Write On
     * Hold Error Interrupt Status Bit
     * 0x1 for Slave Interface Common Register Write On Hold Error detected
     * 0x0 for No Slave Interface Common Register Write On Hold Errors
     */
    uint64_t slvif_wronhold_err_intstat : 1;
    /*!< Bits [7:4] is reserved */
    uint64_t rsvd1 : 4;
    /**
     * Bit 8 is Slave Interface Undefined register Decode
     * Error Interrupt Signal Enable Bit
     * 0x1 for  Slave Interface Decode Error detected
     * 0x0 for No Slave Interface Decode Errors
     */
    uint64_t slvif_undefinedreg_dec_err_intstat : 1;
    /* Bits [63:9] is reserved */
    uint64_t rsvd2 : 55;
} __attribute__((packed, aligned(8))) dmac_commonreg_intstatus_t;

typedef union _dmac_commonreg_intstatus_u
{
    dmac_commonreg_intstatus_t commonreg_intstatus;
    uint64_t data;
} dmac_commonreg_intstatus_u_t;

typedef struct _dmac_reset
{
    /* Bit 0 is DMAC reset request bit */
    uint64_t rst : 1;
    /* Bits [63:1] is reserved */
    uint64_t rsvd : 63;
} __attribute__((packed, aligned(8))) dmac_reset_t;

typedef union _dmac_reset_u
{
    dmac_reset_t reset;
    uint64_t data;
} dmac_reset_u_t;

typedef struct _dmac_ch_block_ts
{
    uint64_t block_ts : 22;
    /*!< Bit [21:0] is block transfer size*/
    uint64_t rsvd : 42;
    /*!< Bits [63:22] is reserved */
} __attribute__((packed, aligned(8))) dmac_ch_block_ts_t;

typedef union _dmac_ch_block_ts_u
{
    dmac_ch_block_ts_t block_ts;
    uint64_t data;
} dmac_ch_block_ts_u_t;

typedef struct _dmac_ch_ctl
{
    /**
     * Bit 0 is source master select
     * 1 for AXI master 2, 0 for AXI master 1
     */
    uint64_t sms : 1;
    /* Bit 1 is reserved */
    uint64_t rsvd1 : 1;
    /**
     * Bit 2 is destination master select
     * 0x1 for AXI master 2,0x0 for AXI master 1
     */
    uint64_t dms : 1;
    /* Bit 3 is reserved */
    uint64_t rsvd2 : 1;
    /**
     * Bit 4 is source address increment
     * 0x1 for no change, 0x0 for incremnet
     */
    uint64_t sinc : 1;
    /**
     * Bit 5 is reserved
     */
    uint64_t rsvd3 : 1;
    /**
     * Bit 6 is destination address incremnet
     * 0x1 for no change, 0x0 for increment
     */
    uint64_t dinc : 1;
    /* Bit 7 is reserved*/
    uint64_t rsvd4 : 1;
    /**
     * Bits [10:8] is source transfer width
     * 0x0 for source transfer width is 8 bits
     * 0x1 for source transfer width is 16 bits
     * 0x2 for source transfer width is 32 bits
     * 0x3 for source transfer width is 64 bits
     * 0x4 for source transfer width is 128 bits
     * 0x5  for source transfer width is 256 bits
     * 0x6 for source transfer width is 512 bits
     */
    uint64_t src_tr_width : 3;
    /**
     * Bits [13:11] is detination transfer width
     * 0x0 for detination transfer width is 8 bits
     * 0x1 for detination transfer width is 16 bits
     * 0x2 for detination transfer width is 32 bits
     * 0x3 for detination transfer width is 64 bits
     * 0x4 for detination transfer width is 128 bits
     * 0x5  for detination transfer width is 256 bits
     * 0x6 for detination transfer width is 512 bits
     */
    uint64_t dst_tr_width : 3;
    /**
     * Bits [17:14] is source burst transaction length
     * 0x0 for 1 data item read from rource in the burst transaction
     * 0x1 for 4 data item read from rource in the burst transaction
     * 0x2 for 8 data item read from rource in the burst transaction
     * 0x3 for 16 data item read from rource in the burst transaction
     * 0x4 for 32 data item read from rource in the burst transaction
     * 0x5 for 64 data item read from rource in the burst transaction
     * 0x6 for 128 data item read from rource in the burst transaction
     * 0x7 for 256 data item read from rource in the burst transaction
     * 0x8 for 512 data item read from rource in the burst transaction
     * 0x9 for 1024 data item read from rource in the burst transaction
     */
    uint64_t src_msize : 4;
    /**
     * Bits [17:14] is sdestination burst transaction length
     * 0x0 for 1 data item read from rource in the burst transaction
     * 0x1 for 4 data item read from rource in the burst transaction
     * 0x2 for 8 data item read from rource in the burst transaction
     * 0x3 for 16 data item read from rource in the burst transaction
     * 0x4 for 32 data item read from rource in the burst transaction
     * 0x5 for 64 data item read from rource in the burst transaction
     * 0x6 for 128 data item read from rource in the burst transaction
     * 0x7 for 256 data item read from rource in the burst transaction
     * 0x8 for 512 data item read from rource in the burst transaction
     * 0x9 for 1024 data item read from rource in the burst transaction
     */
    uint64_t dst_msize : 4;
    /**
     * Bits [25:22] is reserved
     */
    uint64_t rsvd5 : 4;
    /*!< Bits [29:26] is reserved */
    uint64_t rsvd6 : 4;
    /**
     * Bit 30 is Non Posted Last Write Enable
     * 0x1 for posted writes may be used till the end of the block
     * 0x 0 for posted writes may be used throughout the block transfer
     */
    uint64_t nonposted_lastwrite_en : 1;
    /* Bit 31 is resrved */
    uint64_t rsvd7 : 1;
    /* Bits [34:32] is reserved*/
    uint64_t rsvd8 : 3;
    /* Bits [37:35] is reserved*/
    uint64_t rsvd9 : 3;
    /**
     * Bit 38 is source burst length enable
     * 1 for enable, 0 for disable
     */
    uint64_t arlen_en : 1;
    /* Bits [46:39] is source burst length*/
    uint64_t arlen : 8;
    /**
     * Bit 47 is destination burst length enable
     * 1 for enable, 0 for disable
     */
    uint64_t awlen_en : 1;
    /* Bits [55:48] is destination burst length */
    uint64_t awlen : 8;
    /**
     * Bit 56 is source status enable
     * 0x1 for enable, 0x0 for disable
     */
    uint64_t src_stat_en : 1;
    /**
     * Bit 57 is destination status enable
     * 0x1 for enable, 0x0 for disable
     */
    uint64_t dst_stat_en : 1;
    /**
     * Bit 58 is interrupt completion of block transfer
     * 0x1 for enable CHx_IntStatusReg.BLOCK_TFR_DONE_IntStat field
     * 0x0 for dsiable CHx_IntStatusReg.BLOCK_TFR_DONE_IntStat field
     */
    uint64_t ioc_blktfr : 1;
    /**
     * Bits [61:59] is reserved
     */
    uint64_t rsvd10 : 3;
    /**
     * Bit 62 is last shadow linked list item
     * 0x1 for indicate shadowreg/LLI content is the last one
     * 0x0 for indicate  shadowreg/LLI content not the last one
     */
    uint64_t shadowreg_or_lli_last : 1;
    /**
     * Bit 63 is last shadow linked list item valid
     * 0x1 for indicate shadowreg/LLI content is  valid
     * 0x0 for indicate  shadowreg/LLI content is invalid
     */
    uint64_t shadowreg_or_lli_valid : 1;
} __attribute__((packed, aligned(8))) dmac_ch_ctl_t;

typedef union _dmac_ch_ctl_u
{
    dmac_ch_ctl_t ch_ctl;
    uint64_t data;
} dmac_ch_ctl_u_t;

typedef struct _dmac_ch_cfg
{
    /**
     * Bit[1:0] is source multi block transfer type
     * 0x0 for continuous multiblock type
     * 0x1 for reload multiblock type
     * 0x2 for shadow register based multiblock type
     * 0x3 for linked lisr bases multiblock type
     */
    uint64_t src_multblk_type : 2;
    /**
     * Bit[3:2] is source multi block transfer type
     * 0x0 for continuous multiblock type
     * 0x1 for reload multiblock type
     * 0x2 for shadow register based multiblock type
     * 0x3 for linked lisr bases multiblock type
     */
    uint64_t dst_multblk_type : 2;
    /* Bits [31:4] is reserved*/
    uint64_t rsvd1 : 28;
    /**
     * Bits [34:32] is transfer type and flow control
     * 0x0  transfer memory to memory and flow controler is dmac
     * 0x1  transfer memory to peripheral and flow controler is dmac
     * 0x2  transfer peripheral to memory and flow controler is dmac
     * 0x3  transfer peripheral to peripheral and flow controler is dmac
     * 0x4  transfer peripheral to memory and flow controler is
     * source peripheral
     * 0x5  transfer peripheral to peripheral and flow controler
     * is source peripheral
     * 0x6  transfer memory to peripheral and flow controler is
     * destination peripheral
     * 0x7  transfer peripheral to peripheral and flow controler
     * is destination peripheral
     */
    uint64_t tt_fc : 3;
    /**
     * Bit 35 is source software or hardware handshaking select
     * 0x1 for software handshaking is used
     * 0x0 for hardware handshaking is used
     */
    uint64_t hs_sel_src : 1;
    /**
     * Bit 36 is destination software or hardware handshaking select
     *0x1 for software handshaking is used
     *0x0 for hardware handshaking is used
     */
    uint64_t hs_sel_dst : 1;
    /**
     * Bit 37 is sorce hardware handshaking interface polarity
     * 0x1 active low, 0x0 active high
     */
    uint64_t src_hwhs_pol : 1;
    /**
     * Bit 38 is destination hardware handshaking interface polarity
     * 0x1 active low, 0x0 active high
     */
    uint64_t dst_hwhs_pol : 1;
    /**
     * Bits [41:39] is assign a hardware handshaking interface
     * to source of channel x
     */
    uint64_t src_per : 4;
    /* Bit 43 is reserved*/
    uint64_t rsvd3 : 1;
    /**
     * Bits [46:44] is assign a hardware handshaking interface
     * to destination of channel x
     */
    uint64_t dst_per : 4;
    /* Bit 48 is reserved*/
    uint64_t rsvd5 : 1;
    /* Bits [51:49] is channel priority,7 is highest, 0 is lowest*/
    uint64_t ch_prior : 3;
    /**
     * Bit 52 is channel lock bit
     * 0x0 for channel is not locked, 0x1 for channel is locked
     */
    uint64_t lock_ch : 1;
    /**
     * Bits [54:53] is chnannel lock level
     * 0x0 for  duration of channel is locked for entire DMA transfer
     * 0x1 for duration of channel is locked for current block transfer
     */
    uint64_t lock_ch_l : 2;
    uint64_t src_osr_lmt : 4;
    /* Bits [58:55] is source outstanding request limit */
    uint64_t dst_osr_lmt : 4;
    /* Bits [62:59] is destination outstanding request limit */
} __attribute__((packed, aligned(8))) dmac_ch_cfg_t;

typedef union _dmac_ch_cfg_u
{
    dmac_ch_cfg_t ch_cfg;
    uint64_t data;
} dmac_ch_cfg_u_t;

typedef struct _dmac_ch_llp
{
    /**
     * Bit 0 is LLI master select
     * 0x0 for next linked list item resides on AXI madster1 interface
     * 0x1 for next linked list item resides on AXI madster2 interface
     */
    uint64_t lms : 1;
    /* Bits [5:1] is reserved */
    uint64_t rsvd1 : 5;
    /* Bits [63:6] is starting address memeory of LLI block */
    uint64_t loc : 58;
} __attribute__((packed, aligned(8))) dmac_ch_llp_t;

typedef union _dmac_ch_llp_u
{
    dmac_ch_llp_t llp;
    uint64_t data;
} dmac_ch_llp_u_t;

typedef struct _dmac_ch_status
{
    /* Bits [21:0] is completed block transfer size */
    uint64_t cmpltd_blk_size : 22;
    /* Bits [46:32] is reserved */
    uint64_t rsvd1 : 15;
    /* Bits [63:47] is reserved */
    uint64_t rsvd2 : 17;
} __attribute__((packed, aligned(8))) dmac_ch_status_t;

typedef union _dmac_ch_status_u
{
    dmac_ch_status_t status;
    uint64_t data;
} dmac_ch_status_u_t;

typedef struct _dmac_ch_swhssrc
{
    /**
     * Bit 0 is software handshake request for channel source
     * 0x1 source periphraral request for a dma transfer
     * 0x0 source peripheral is not request for a burst transfer
     */
    uint64_t swhs_req_src : 1;
    /**
     * Bit 1 is write enable bit for software handshake request
     *0x1 for enable, 0x0 for disable
     */
    uint64_t swhs_req_src_we : 1;
    /**
     * Bit 2 is software handshake single request for channel source
     * 0x1 for source peripheral requesr for a single dma transfer
     *  0x0 for source peripheral is not requesting for a single transfer
     */
    uint64_t swhs_sglreq_src : 1;
    /**
     * Bit 3 is write enable bit for software handshake
     * single request for channle source
     * 0x1 for enable write, 0x0 for disable write
     */
    uint64_t swhs_sglreq_src_we : 1;
    /**
     * Bit 4 software handshake last request for channel source
     * 0x1 for current transfer is last transfer
     * 0x0 for current transfer is not the last transfer
     */
    uint64_t swhs_lst_src : 1;
    /**
     * Bit 5 is write enable bit for software
     * handshake last request
     * 0x1 for enable, 0x0 for disable
     */
    uint64_t swhs_lst_src_we : 1;
    /* Bits [63:6] is reserved */
    uint64_t rsvd : 58;
} __attribute__((packed, aligned(8))) dmac_ch_swhssrc_t;

typedef union _dmac_ch_swhssrc_u
{
    dmac_ch_swhssrc_t swhssrc;
    uint64_t data;
} dmac_ch_swhssrc_u_t;

typedef struct _dmac_ch_swhsdst
{
    /**
     * Bit 0 is software handshake request for channel destination
     * 0x1 destination periphraral request for a dma transfer
     * 0x0 destination peripheral is not request for a burst transfer
     */
    uint64_t swhs_req_dst : 1;
    /**
     * Bit 1 is write enable bit for software handshake request
     * 0x1 for enable, 0x0 for disable
     */
    uint64_t swhs_req_dst_we : 1;
    /**
     * Bit 2 is software handshake single request for channel destination
     * 0x1 for destination peripheral requesr for a single dma transfer
     * 0x0 for destination peripheral is not requesting
     * for a single transfer
     */
    uint64_t swhs_sglreq_dst : 1;
    /**
     * Bit 3 is write enable bit for software handshake
     * single request for channle destination
     * 0x1 for enable write, 0x0 for disable write
     */
    uint64_t swhs_sglreq_dst_we : 1;
    /**
     * Bit 4 software handshake last request for channel dstination
     * 0x1 for current transfer is last transfer
     * 0x0 for current transfer is not the last transfer
     */
    uint64_t swhs_lst_dst : 1;
    /**
     * Bit 5 is write enable bit for software handshake last request
     * 0x1 for enable, 0x0 for disable
     */
    uint64_t swhs_lst_dst_we : 1;
    /* Bits [63:6] is reserved */
    uint64_t rsvd : 58;
} __attribute__((packed, aligned(8))) dmac_ch_swhsdst_t;

typedef union _dmac_ch_swhsdst_u
{
    dmac_ch_swhsdst_t swhsdst;
    uint64_t data;
} dmac_ch_swhsdst_u_t;

typedef struct _dmac_ch_blk_tfr_resumereq
{
    /**
     * Bit 0 is block transfer resume request bit
     * 0x1 for request for resuming
     * 0x0 for no request to resume
     */
    uint64_t blk_tfr_resumereq : 1;
    /* Bits [63:1] is reserved */
    uint64_t rsvd : 63;
} __attribute__((packed, aligned(8))) dmac_ch_blk_tfr_resumereq_t;

typedef union _dmac_ch_blk_tfr_resumereq_u
{
    dmac_ch_blk_tfr_resumereq_t blk_tfr_resumereq;
    uint64_t data;
} dmac_ch_blk_tfr_resumereq_u_t;

typedef struct _dmac_ch_intstatus_enable
{
    /* Bit 0 is block transfer done interrupt status enable */
    uint64_t enable_block_tfr_done_intstatus : 1;
    /* DMA transfer done interrupt status enable */
    uint64_t enable_dma_tfr_done_intstat : 1;
    /* Bit 2 reserved */
    uint64_t rsvd1 : 1;
    /* Bit 3 source transaction complete status enable */
    uint64_t enable_src_transcomp_intstat : 1;
    /* Bit 4 destination transaction complete */
    uint64_t enable_dst_transcomp_intstat : 1;
    /* Bit 5 Source Decode Error Status Enable */
    uint64_t enable_src_dec_err_intstat : 1;
    /* Bit 6 Destination Decode Error Status Enable */
    uint64_t enable_dst_dec_err_intstat : 1;
    /* Bit 7 Source Slave Error Status Enable */
    uint64_t enable_src_slv_err_intstat : 1;
    /* Bit 8 Destination Slave Error Status Enable */
    uint64_t enable_dst_slv_err_intstat : 1;
    /* Bit 9 LLI Read Decode Error Status Enable */
    uint64_t enable_lli_rd_dec_err_intstat : 1;
    /* Bit 10 LLI WRITE Decode Error Status Enable */
    uint64_t enable_lli_wr_dec_err_intstat : 1;
    /* Bit 11 LLI Read Slave Error Status Enable */
    uint64_t enable_lli_rd_slv_err_intstat : 1;
    /* Bit 12 LLI WRITE Slave Error Status Enable */
    uint64_t enable_lli_wr_slv_err_intstat : 1;
    uint64_t rsvd2 : 51;
} dmac_ch_intstatus_enable_t;

typedef union _dmac_ch_intstatus_enable_u
{
    dmac_ch_intstatus_enable_t ch_intstatus_enable;
    uint64_t data;
} dmac_ch_intstatus_enable_u_t;

typedef struct _dmac_ch_intclear
{
    /* Bit 0 block transfer done interrupt clear bit.*/
    uint64_t blk_tfr_done_intstat : 1;
    /* Bit 1 DMA transfer done interrupt clear bit */
    uint64_t dma_tfr_done_intstat : 1;
    /* Bit 2 is reserved */
    uint64_t resv1 : 1;
    uint64_t resv2 : 61;
} __attribute__((packed, aligned(8))) dmac_ch_intclear_t;

typedef union _dmac_ch_intclear_u
{
    uint64_t data;
    dmac_ch_intclear_t intclear;
} dmac_ch_intclear_u_t;

typedef struct _dmac_channel
{
    /* (0x100) SAR Address Register */
    uint64_t sar;
    /* (0x108) DAR Address Register */
    uint64_t dar;
    /* (0x110) Block Transfer Size Register */
    uint64_t block_ts;
    /* (0x118) Control Register */
    uint64_t ctl;
    /* (0x120) Configure Register */
    uint64_t cfg;
    /* (0x128) Linked List Pointer register */
    uint64_t llp;
    /* (0x130) Channelx Status Register */
    uint64_t status;
    /* (0x138) Channelx Software handshake Source Register */
    uint64_t swhssrc;
    /* (0x140) Channelx Software handshake Destination Register */
    uint64_t swhsdst;
    /* (0x148) Channelx Block Transfer Resume Request Register */
    uint64_t blk_tfr;
    /* (0x150) Channelx AXI ID Register */
    uint64_t axi_id;
    /* (0x158) Channelx AXI QOS Register */
    uint64_t axi_qos;
    /* Reserved address */
    uint64_t reserved1[4];
    /* (0x180) Interrupt Status Enable Register */
    uint64_t intstatus_en;
    /* (0x188) Channelx Interrupt Status Register */
    uint64_t intstatus;
    /* (0x190) Interrupt  Siganl Enable Register */
    uint64_t intsignal_en;
    /* (0x198) Interrupt Clear Register */
    uint64_t intclear;
    uint64_t reserved2[12];
} __attribute__((packed, aligned(8))) dmac_channel_t;

typedef struct _dmac
{
    /* (0x00) DMAC ID Rgister */
    uint64_t id;
    /* (0x08) DMAC COMPVER Register */
    uint64_t compver;
    /* (0x10) DMAC Configure Register */
    uint64_t cfg;
    /* (0x18) Channel Enable Register */
    uint64_t chen;
    uint64_t reserved1[2];
    /* (0x30) DMAC Interrupt Status Register */
    uint64_t intstatus;
    /* (0x38) DMAC Common register Interrupt Status Register */
    uint64_t com_intclear;
    /* (0x40) DMAC Common Interrupt Enable Register */
    uint64_t com_intstatus_en;
    /* (0x48) DMAC Common Interrupt Signal Enable Register */
    uint64_t com_intsignal_en;
    /* (0x50) DMAC Common Interrupt Status */
    uint64_t com_intstatus;
    /* (0x58) DMAC Reset register */
    uint64_t reset;
    uint64_t reserved2[20];
    dmac_channel_t channel[DMAC_CHANNEL_COUNT];
} __attribute__((packed, aligned(8))) dmac_t;

typedef struct _dmac_channel_config
{
    uint64_t sar;
    uint64_t dar;
    uint8_t ctl_sms;
    uint8_t ctl_dms;
    uint8_t ctl_src_msize;
    uint8_t ctl_drc_msize;
    uint8_t ctl_sinc;
    uint8_t ctl_dinc;
    uint8_t ctl_src_tr_width;
    uint8_t ctl_dst_tr_width;
    uint8_t ctl_ioc_blktfr;
    uint8_t ctl_src_stat_en;
    uint8_t ctl_dst_stat_en;
    uint8_t cfg_dst_per;
    uint8_t cfg_src_per;
    uint8_t cfg_src_hs_pol;
    uint8_t cfg_dst_hs_pol;
    uint8_t cfg_hs_sel_src;
    uint8_t cfg_hs_sel_dst;
    uint64_t cfg_src_multblk_type;
    uint64_t cfg_dst_multblk_type;
    uint64_t llp_loc;
    uint8_t llp_lms;
    uint64_t ctl_block_ts;
    uint8_t ctl_tt_fc;
    uint8_t cfg_protctl;
    uint8_t cfg_fifo_mode;
    uint8_t cfg_fcmode;
    uint8_t cfg_lock_ch_l;
    uint8_t cfg_ch_prior;
} dmac_channel_config_t;

#define LIST_ENTRY(ptr, type, member)                                          \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

struct list_head_t
{
    struct list_head_t *next, *prev;
};

/**
 * @brief       Dmac.data/dw_dmac_lli_item
 *
 * @desc        This structure is used when creating Linked List Items.
 *
 * @see         dw_dmac_addLliItem()
 */
typedef struct _dmac_lli_item
{
    uint64_t sar;
    uint64_t dar;
    uint64_t ch_block_ts;
    uint64_t llp;
    uint64_t ctl;
    uint64_t sstat;
    uint64_t dstat;
    uint64_t resv;
} __attribute__((packed, aligned(64))) dmac_lli_item_t;

extern volatile dmac_t *const dmac;

/**
 * @brief       Dmac initialize
 */
void dmac_init(void);

/**
 * @brief       Set dmac param
 *
 * @param[in]   channel_num             Dmac channel
 * @param[in]   src                     Dmac source
 * @param[in]   dest                    Dmac dest
 * @param[in]   src_inc                 Source address increase or not
 * @param[in]   dest_inc                Dest address increase or not
 * @param[in]   dmac_burst_size         Dmac burst length
 * @param[in]   dmac_trans_width        Dmac transfer data width
 * @param[in]   block_size               Dmac transfer length
 *
 */
void dmac_set_single_mode(dmac_channel_number_t channel_num,
                          const void *src, void *dest, dmac_address_increment_t src_inc,
                          dmac_address_increment_t dest_inc,
                          dmac_burst_trans_length_t dmac_burst_size,
                          dmac_transfer_width_t dmac_trans_width,
                          size_t block_size);

/**
 * @brief       Determine the transfer is complete or not
 *
 * @param[in]   channel_num             Dmac channel
 *
 * @return      result
 *     - 0      uncompleted
 *     - 1  completed
*/
int dmac_is_done(dmac_channel_number_t channel_num);

/**
 * @brief       Wait for dmac work done
 *
 * @param[in]   channel_num  Dmac channel
 *
 */
void dmac_wait_done(dmac_channel_number_t channel_num);

/**
 * @brief       Determine the dma is idle or not
 *
 * @param[in]   channel_num             Dmac channel
 *
 * @return      result
 *     - 0      busy
 *     - 1      idel
*/
int dmac_is_idle(dmac_channel_number_t channel_num);

/**
 * @brief       Wait for dmac idle
 *
 * @param[in]   channel_num  Dmac channel
 *
 */
void dmac_wait_idle(dmac_channel_number_t channel_num);

/**
 * @brief       Set interrupt param
 *
 * @param[in]   channel_num             Dmac channel
 * @param[in]   dmac_callback           Dmac interrupt callback
 * @param[in]   ctx                     The param of callback
 * @param[in]   priority                Interrupt priority
 */
void dmac_set_irq(dmac_channel_number_t channel_num , plic_irq_callback_t dmac_callback, void *ctx, uint32_t priority);

/**
 * @brief       Set interrupt param
 *
 * @param[in]   channel_num             Dmac channel
 * @param[in]   dmac_callback           Dmac interrupt callback
 * @param[in]   ctx                     The param of callback
 * @param[in]   priority                Interrupt priority
 */
void dmac_irq_register(dmac_channel_number_t channel_num , plic_irq_callback_t dmac_callback, void *ctx, uint32_t priority);


/**
 * @brief       Unregister dmac interrupt
 *
 * @param[in]   channel_num             Dmac channel
 *
 */
void dmac_irq_unregister(dmac_channel_number_t channel_num);

/**
 * @brief       Disable dmac interrupt
 *
 * @param[in]   channel_num             Dmac channel
 *
 */
void dmac_free_irq(dmac_channel_number_t channel_num);

/**
 * @brief       Set source dest and length
 *
 * @param[in]   channel_num             Dmac channel
 * @param[in]   src                     Source
 * @param[in]   dest                    Dest
 * @param[in]   len                     The length of dmac transfer
 */
void dmac_set_src_dest_length(dmac_channel_number_t channel_num, const void *src, void *dest, size_t len);

/**
 * @brief       Disable dmac channel interrupt
 *
 * @param[in]   channel_num             Dmac channel
 *
*/
void dmac_disable_channel_interrupt(dmac_channel_number_t channel_num);

/**
 * @brief       Disable dmac channel
 *
 * @param[in]   channel_num             Dmac channel
 *
*/
void dmac_channel_disable(dmac_channel_number_t channel_num);

/**
 * @brief       Enable dmac channel
 *
 * @param[in]   channel_num             Dmac channel
 *
*/
void dmac_channel_enable(dmac_channel_number_t channel_num);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_DMAC_H */
