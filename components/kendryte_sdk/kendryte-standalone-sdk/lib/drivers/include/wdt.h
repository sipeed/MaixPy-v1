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
#ifndef _DRIVER_WDT_H
#define _DRIVER_WDT_H

#include <stdint.h>
#include <stddef.h>
#include <plic.h>

#ifdef __cplusplus
extern "C" {
#endif

/* clang-format off */
typedef struct _wdt
{
    /* WDT Control Register                     (0x00) */
    volatile uint32_t cr;
    /* WDT Timeout Range Register               (0x04) */
    volatile uint32_t torr;
    /* WDT Current Counter Value Register       (0x08) */
    volatile uint32_t ccvr;
    /* WDT Counter Restart Register             (0x0c) */
    volatile uint32_t crr;
    /* WDT Interrupt Status Register            (0x10) */
    volatile uint32_t stat;
    /* WDT Interrupt Clear Register             (0x14) */
    volatile uint32_t eoi;
    /* reserverd                                (0x18) */
    volatile uint32_t resv1;
    /* WDT Protection level Register            (0x1c) */
    volatile uint32_t prot_level;
    /* reserved                                 (0x20-0xe0) */
    volatile uint32_t resv4[49];
    /* WDT Component Parameters Register 5      (0xe4) */
    volatile uint32_t comp_param_5;
    /* WDT Component Parameters Register 4      (0xe8) */
    volatile uint32_t comp_param_4;
    /* WDT Component Parameters Register 3      (0xec) */
    volatile uint32_t comp_param_3;
    /* WDT Component Parameters Register 2      (0xf0) */
    volatile uint32_t comp_param_2;
    /* WDT Component Parameters Register 1      (0xf4) */
    volatile uint32_t comp_param_1;
    /* WDT Component Version Register           (0xf8) */
    volatile uint32_t comp_version;
    /* WDT Component Type Register              (0xfc) */
    volatile uint32_t comp_type;
} __attribute__((packed, aligned(4))) wdt_t;

typedef enum _wdt_device_number
{
    WDT_DEVICE_0,
    WDT_DEVICE_1,
    WDT_DEVICE_MAX,
} wdt_device_number_t;


#define WDT_RESET_ALL                                       0x00000000U
#define WDT_RESET_CPU                                       0x00000001U

/* WDT Control Register */
#define WDT_CR_ENABLE                                       0x00000001U
#define WDT_CR_RMOD_MASK                                    0x00000002U
#define WDT_CR_RMOD_RESET                                   0x00000000U
#define WDT_CR_RMOD_INTERRUPT                               0x00000002U
#define WDT_CR_RPL_MASK                                     0x0000001CU
#define WDT_CR_RPL(x)                                       ((x) << 2)
/* WDT Timeout Range Register */
#define WDT_TORR_TOP_MASK                                   0x000000FFU
#define WDT_TORR_TOP(x)                                     ((x) << 4 | (x) << 0)
/* WDT Current Counter Value Register */
#define WDT_CCVR_MASK                                       0xFFFFFFFFU
/* WDT Counter Restart Register */
#define WDT_CRR_MASK                                        0x00000076U
/* WDT Interrupt Status Register */
#define WDT_STAT_MASK                                       0x00000001U
/* WDT Interrupt Clear Register */
#define WDT_EOI_MASK                                        0x00000001U
/* WDT Protection level Register */
#define WDT_PROT_LEVEL_MASK                                 0x00000007U
/* WDT Component Parameter Register 5 */
#define WDT_COMP_PARAM_5_CP_WDT_USER_TOP_MAX_MASK           0xFFFFFFFFU
/* WDT Component Parameter Register 4 */
#define WDT_COMP_PARAM_4_CP_WDT_USER_TOP_INIT_MAX_MASK      0xFFFFFFFFU
/* WDT Component Parameter Register 3 */
#define WDT_COMP_PARAM_3_CD_WDT_TOP_RST_MASK                0xFFFFFFFFU
/* WDT Component Parameter Register 2 */
#define WDT_COMP_PARAM_3_CP_WDT_CNT_RST_MASK                0xFFFFFFFFU
/* WDT Component Parameter Register 1 */
#define WDT_COMP_PARAM_1_WDT_ALWAYS_EN_MASK                 0x00000001U
#define WDT_COMP_PARAM_1_WDT_DFLT_RMOD_MASK                 0x00000002U
#define WDT_COMP_PARAM_1_WDT_DUAL_TOP_MASK                  0x00000004U
#define WDT_COMP_PARAM_1_WDT_HC_RMOD_MASK                   0x00000008U
#define WDT_COMP_PARAM_1_WDT_HC_RPL_MASK                    0x00000010U
#define WDT_COMP_PARAM_1_WDT_HC_TOP_MASK                    0x00000020U
#define WDT_COMP_PARAM_1_WDT_USE_FIX_TOP_MASK               0x00000040U
#define WDT_COMP_PARAM_1_WDT_PAUSE_MASK                     0x00000080U
#define WDT_COMP_PARAM_1_APB_DATA_WIDTH_MASK                0x00000300U
#define WDT_COMP_PARAM_1_WDT_DFLT_RPL_MASK                  0x00001C00U
#define WDT_COMP_PARAM_1_WDT_DFLT_TOP_MASK                  0x000F0000U
#define WDT_COMP_PARAM_1_WDT_DFLT_TOP_INIT_MASK             0x00F00000U
#define WDT_COMP_PARAM_1_WDT_CNT_WIDTH_MASK                 0x1F000000U
/* WDT Component Version Register */
#define WDT_COMP_VERSION_MASK                               0xFFFFFFFFU
/* WDT Component Type Register */
#define WDT_COMP_TYPE_MASK                                  0xFFFFFFFFU
/* clang-format on */

/**
 * @brief       Feed wdt
 */
void wdt_feed(wdt_device_number_t id);

/**
 * @brief       Start wdt
 *
 * @param[in]   id                  Wdt id 0 or 1
 * @param[in]   time_out_ms         Wdt trigger time
 * @param[in]   on_irq              Wdt interrupt callback
 *
 */
void wdt_start(wdt_device_number_t id, uint64_t time_out_ms, plic_irq_callback_t on_irq);

/**
 * @brief       Start wdt
 *
 * @param[in]   id                 Wdt id 0 or 1
 * @param[in]   time_out_ms        Wdt trigger time
 * @param[in]   on_irq             Wdt interrupt callback
 * @param[in]   ctx                Param of callback
 *
 * @return      Wdt time
 *
 */
uint32_t wdt_init(wdt_device_number_t id, uint64_t time_out_ms, plic_irq_callback_t on_irq, void *ctx);

/**
 * @brief       Stop wdt
 *
 * @param[in]   id      Wdt id 0 or 1
 *
 */
void wdt_stop(wdt_device_number_t id);

/**
 * @brief       Clear wdt interrupt
 *
 * @param[in]   id      Wdt id 0 or 1
 *
 */
void wdt_clear_interrupt(wdt_device_number_t id);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_WDT_H */
