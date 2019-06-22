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
#ifndef _DRIVER_SYSCTL_H
#define _DRIVER_SYSCTL_H

#include <stdint.h>
#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      System controller register
 *
 * @note       System controller register table
 *
 * | Offset    | Name           | Description                         |
 * |-----------|----------------|-------------------------------------|
 * | 0x00      | git_id         | Git short commit id                 |
 * | 0x04      | clk_freq       | System clock base frequency         |
 * | 0x08      | pll0           | PLL0 controller                     |
 * | 0x0c      | pll1           | PLL1 controller                     |
 * | 0x10      | pll2           | PLL2 controller                     |
 * | 0x14      | resv5          | Reserved                            |
 * | 0x18      | pll_lock       | PLL lock tester                     |
 * | 0x1c      | rom_error      | AXI ROM detector                    |
 * | 0x20      | clk_sel0       | Clock select controller0            |
 * | 0x24      | clk_sel1       | Clock select controller1            |
 * | 0x28      | clk_en_cent    | Central clock enable                |
 * | 0x2c      | clk_en_peri    | Peripheral clock enable             |
 * | 0x30      | soft_reset     | Soft reset ctrl                     |
 * | 0x34      | peri_reset     | Peripheral reset controller         |
 * | 0x38      | clk_th0        | Clock threshold controller 0        |
 * | 0x3c      | clk_th1        | Clock threshold controller 1        |
 * | 0x40      | clk_th2        | Clock threshold controller 2        |
 * | 0x44      | clk_th3        | Clock threshold controller 3        |
 * | 0x48      | clk_th4        | Clock threshold controller 4        |
 * | 0x4c      | clk_th5        | Clock threshold controller 5        |
 * | 0x50      | clk_th6        | Clock threshold controller 6        |
 * | 0x54      | misc           | Miscellaneous controller            |
 * | 0x58      | peri           | Peripheral controller               |
 * | 0x5c      | spi_sleep      | SPI sleep controller                |
 * | 0x60      | reset_status   | Reset source status                 |
 * | 0x64      | dma_sel0       | DMA handshake selector              |
 * | 0x68      | dma_sel1       | DMA handshake selector              |
 * | 0x6c      | power_sel      | IO Power Mode Select controller     |
 * | 0x70      | resv28         | Reserved                            |
 * | 0x74      | resv29         | Reserved                            |
 * | 0x78      | resv30         | Reserved                            |
 * | 0x7c      | resv31         | Reserved                            |
 *
 */

typedef enum _sysctl_pll_t
{
    SYSCTL_PLL0,
    SYSCTL_PLL1,
    SYSCTL_PLL2,
    SYSCTL_PLL_MAX
} sysctl_pll_t;

typedef enum _sysctl_clock_source_t
{
    SYSCTL_SOURCE_IN0,
    SYSCTL_SOURCE_PLL0,
    SYSCTL_SOURCE_PLL1,
    SYSCTL_SOURCE_PLL2,
    SYSCTL_SOURCE_ACLK,
    SYSCTL_SOURCE_MAX
} sysctl_clock_source_t;

typedef enum _sysctl_dma_channel_t
{
    SYSCTL_DMA_CHANNEL_0,
    SYSCTL_DMA_CHANNEL_1,
    SYSCTL_DMA_CHANNEL_2,
    SYSCTL_DMA_CHANNEL_3,
    SYSCTL_DMA_CHANNEL_4,
    SYSCTL_DMA_CHANNEL_5,
    SYSCTL_DMA_CHANNEL_MAX
} sysctl_dma_channel_t;

typedef enum _sysctl_dma_select_t
{
    SYSCTL_DMA_SELECT_SSI0_RX_REQ,
    SYSCTL_DMA_SELECT_SSI0_TX_REQ,
    SYSCTL_DMA_SELECT_SSI1_RX_REQ,
    SYSCTL_DMA_SELECT_SSI1_TX_REQ,
    SYSCTL_DMA_SELECT_SSI2_RX_REQ,
    SYSCTL_DMA_SELECT_SSI2_TX_REQ,
    SYSCTL_DMA_SELECT_SSI3_RX_REQ,
    SYSCTL_DMA_SELECT_SSI3_TX_REQ,
    SYSCTL_DMA_SELECT_I2C0_RX_REQ,
    SYSCTL_DMA_SELECT_I2C0_TX_REQ,
    SYSCTL_DMA_SELECT_I2C1_RX_REQ,
    SYSCTL_DMA_SELECT_I2C1_TX_REQ,
    SYSCTL_DMA_SELECT_I2C2_RX_REQ,
    SYSCTL_DMA_SELECT_I2C2_TX_REQ,
    SYSCTL_DMA_SELECT_UART1_RX_REQ,
    SYSCTL_DMA_SELECT_UART1_TX_REQ,
    SYSCTL_DMA_SELECT_UART2_RX_REQ,
    SYSCTL_DMA_SELECT_UART2_TX_REQ,
    SYSCTL_DMA_SELECT_UART3_RX_REQ,
    SYSCTL_DMA_SELECT_UART3_TX_REQ,
    SYSCTL_DMA_SELECT_AES_REQ,
    SYSCTL_DMA_SELECT_SHA_RX_REQ,
    SYSCTL_DMA_SELECT_AI_RX_REQ,
    SYSCTL_DMA_SELECT_FFT_RX_REQ,
    SYSCTL_DMA_SELECT_FFT_TX_REQ,
    SYSCTL_DMA_SELECT_I2S0_TX_REQ,
    SYSCTL_DMA_SELECT_I2S0_RX_REQ,
    SYSCTL_DMA_SELECT_I2S1_TX_REQ,
    SYSCTL_DMA_SELECT_I2S1_RX_REQ,
    SYSCTL_DMA_SELECT_I2S2_TX_REQ,
    SYSCTL_DMA_SELECT_I2S2_RX_REQ,
    SYSCTL_DMA_SELECT_I2S0_BF_DIR_REQ,
    SYSCTL_DMA_SELECT_I2S0_BF_VOICE_REQ,
    SYSCTL_DMA_SELECT_MAX
} sysctl_dma_select_t;

/**
 * @brief      System controller clock id
 */
typedef enum _sysctl_clock_t
{
    SYSCTL_CLOCK_PLL0,
    SYSCTL_CLOCK_PLL1,
    SYSCTL_CLOCK_PLL2,
    SYSCTL_CLOCK_CPU,
    SYSCTL_CLOCK_SRAM0,
    SYSCTL_CLOCK_SRAM1,
    SYSCTL_CLOCK_APB0,
    SYSCTL_CLOCK_APB1,
    SYSCTL_CLOCK_APB2,
    SYSCTL_CLOCK_ROM,
    SYSCTL_CLOCK_DMA,
    SYSCTL_CLOCK_AI,
    SYSCTL_CLOCK_DVP,
    SYSCTL_CLOCK_FFT,
    SYSCTL_CLOCK_GPIO,
    SYSCTL_CLOCK_SPI0,
    SYSCTL_CLOCK_SPI1,
    SYSCTL_CLOCK_SPI2,
    SYSCTL_CLOCK_SPI3,
    SYSCTL_CLOCK_I2S0,
    SYSCTL_CLOCK_I2S1,
    SYSCTL_CLOCK_I2S2,
    SYSCTL_CLOCK_I2C0,
    SYSCTL_CLOCK_I2C1,
    SYSCTL_CLOCK_I2C2,
    SYSCTL_CLOCK_UART1,
    SYSCTL_CLOCK_UART2,
    SYSCTL_CLOCK_UART3,
    SYSCTL_CLOCK_AES,
    SYSCTL_CLOCK_FPIOA,
    SYSCTL_CLOCK_TIMER0,
    SYSCTL_CLOCK_TIMER1,
    SYSCTL_CLOCK_TIMER2,
    SYSCTL_CLOCK_WDT0,
    SYSCTL_CLOCK_WDT1,
    SYSCTL_CLOCK_SHA,
    SYSCTL_CLOCK_OTP,
    SYSCTL_CLOCK_RTC,
    SYSCTL_CLOCK_ACLK = 40,
    SYSCTL_CLOCK_HCLK,
    SYSCTL_CLOCK_IN0,
    SYSCTL_CLOCK_MAX
} sysctl_clock_t;

/**
 * @brief      System controller clock select id
 */
typedef enum _sysctl_clock_select_t
{
    SYSCTL_CLOCK_SELECT_PLL0_BYPASS,
    SYSCTL_CLOCK_SELECT_PLL1_BYPASS,
    SYSCTL_CLOCK_SELECT_PLL2_BYPASS,
    SYSCTL_CLOCK_SELECT_PLL2,
    SYSCTL_CLOCK_SELECT_ACLK,
    SYSCTL_CLOCK_SELECT_SPI3,
    SYSCTL_CLOCK_SELECT_TIMER0,
    SYSCTL_CLOCK_SELECT_TIMER1,
    SYSCTL_CLOCK_SELECT_TIMER2,
    SYSCTL_CLOCK_SELECT_SPI3_SAMPLE,
    SYSCTL_CLOCK_SELECT_MAX = 11
} sysctl_clock_select_t;

/**
 * @brief      System controller clock threshold id
 */
typedef enum _sysctl_threshold_t
{
    SYSCTL_THRESHOLD_ACLK,
    SYSCTL_THRESHOLD_APB0,
    SYSCTL_THRESHOLD_APB1,
    SYSCTL_THRESHOLD_APB2,
    SYSCTL_THRESHOLD_SRAM0,
    SYSCTL_THRESHOLD_SRAM1,
    SYSCTL_THRESHOLD_AI,
    SYSCTL_THRESHOLD_DVP,
    SYSCTL_THRESHOLD_ROM,
    SYSCTL_THRESHOLD_SPI0,
    SYSCTL_THRESHOLD_SPI1,
    SYSCTL_THRESHOLD_SPI2,
    SYSCTL_THRESHOLD_SPI3,
    SYSCTL_THRESHOLD_TIMER0,
    SYSCTL_THRESHOLD_TIMER1,
    SYSCTL_THRESHOLD_TIMER2,
    SYSCTL_THRESHOLD_I2S0,
    SYSCTL_THRESHOLD_I2S1,
    SYSCTL_THRESHOLD_I2S2,
    SYSCTL_THRESHOLD_I2S0_M,
    SYSCTL_THRESHOLD_I2S1_M,
    SYSCTL_THRESHOLD_I2S2_M,
    SYSCTL_THRESHOLD_I2C0,
    SYSCTL_THRESHOLD_I2C1,
    SYSCTL_THRESHOLD_I2C2,
    SYSCTL_THRESHOLD_WDT0,
    SYSCTL_THRESHOLD_WDT1,
    SYSCTL_THRESHOLD_MAX = 28
} sysctl_threshold_t;

/**
 * @brief      System controller reset control id
 */
typedef enum _sysctl_reset_t
{
    SYSCTL_RESET_SOC,
    SYSCTL_RESET_ROM,
    SYSCTL_RESET_DMA,
    SYSCTL_RESET_AI,
    SYSCTL_RESET_DVP,
    SYSCTL_RESET_FFT,
    SYSCTL_RESET_GPIO,
    SYSCTL_RESET_SPI0,
    SYSCTL_RESET_SPI1,
    SYSCTL_RESET_SPI2,
    SYSCTL_RESET_SPI3,
    SYSCTL_RESET_I2S0,
    SYSCTL_RESET_I2S1,
    SYSCTL_RESET_I2S2,
    SYSCTL_RESET_I2C0,
    SYSCTL_RESET_I2C1,
    SYSCTL_RESET_I2C2,
    SYSCTL_RESET_UART1,
    SYSCTL_RESET_UART2,
    SYSCTL_RESET_UART3,
    SYSCTL_RESET_AES,
    SYSCTL_RESET_FPIOA,
    SYSCTL_RESET_TIMER0,
    SYSCTL_RESET_TIMER1,
    SYSCTL_RESET_TIMER2,
    SYSCTL_RESET_WDT0,
    SYSCTL_RESET_WDT1,
    SYSCTL_RESET_SHA,
    SYSCTL_RESET_RTC,
    SYSCTL_RESET_MAX = 31
} sysctl_reset_t;

/**
 * @brief      System controller power bank id
 */
typedef enum _sysctl_power_bank
{
    SYSCTL_POWER_BANK0,
    SYSCTL_POWER_BANK1,
    SYSCTL_POWER_BANK2,
    SYSCTL_POWER_BANK3,
    SYSCTL_POWER_BANK4,
    SYSCTL_POWER_BANK5,
    SYSCTL_POWER_BANK6,
    SYSCTL_POWER_BANK7,
    SYSCTL_POWER_BANK_MAX,
} sysctl_power_bank_t;

/**
 * @brief      System controller reset control id
 */
typedef enum _sysctl_io_power_mode
{
    SYSCTL_POWER_V33,
    SYSCTL_POWER_V18
} sysctl_io_power_mode_t;

/**
 * @brief      System reset status
 */
typedef enum _sysctl_reset_enum_status
{
    SYSCTL_RESET_STATUS_HARD,
    SYSCTL_RESET_STATUS_SOFT,
    SYSCTL_RESET_STATUS_WDT0,
    SYSCTL_RESET_STATUS_WDT1,
    SYSCTL_RESET_STATUS_MAX,
} sysctl_reset_enum_status_t;

/**
 * @brief       Git short commit id
 *
 *              No. 0 Register (0x00)
 */
typedef struct _sysctl_git_id
{
    uint32_t git_id : 32;
} __attribute__((packed, aligned(4))) sysctl_git_id_t;

/**
 * @brief       System clock base frequency
 *
 *              No. 1 Register (0x04)
 */
typedef struct _sysctl_clk_freq
{
    uint32_t clk_freq : 32;
} __attribute__((packed, aligned(4))) sysctl_clk_freq_t;

/**
 * @brief       PLL0 controller
 *
 *              No. 2 Register (0x08)
 */
typedef struct _sysctl_pll0
{
    uint32_t clkr0 : 4;
    uint32_t clkf0 : 6;
    uint32_t clkod0 : 4;
    uint32_t bwadj0 : 6;
    uint32_t pll_reset0 : 1;
    uint32_t pll_pwrd0 : 1;
    uint32_t pll_intfb0 : 1;
    uint32_t pll_bypass0 : 1;
    uint32_t pll_test0 : 1;
    uint32_t pll_out_en0 : 1;
    uint32_t pll_test_en : 1;
    uint32_t reserved : 5;
} __attribute__((packed, aligned(4))) sysctl_pll0_t;

/**
 * @brief       PLL1 controller
 *
 *              No. 3 Register (0x0c)
 */
typedef struct _sysctl_pll1
{
    uint32_t clkr1 : 4;
    uint32_t clkf1 : 6;
    uint32_t clkod1 : 4;
    uint32_t bwadj1 : 6;
    uint32_t pll_reset1 : 1;
    uint32_t pll_pwrd1 : 1;
    uint32_t pll_intfb1 : 1;
    uint32_t pll_bypass1 : 1;
    uint32_t pll_test1 : 1;
    uint32_t pll_out_en1 : 1;
    uint32_t reserved : 6;
} __attribute__((packed, aligned(4))) sysctl_pll1_t;

/**
 * @brief       PLL2 controller
 *
 *              No. 4 Register (0x10)
 */
typedef struct _sysctl_pll2
{
    uint32_t clkr2 : 4;
    uint32_t clkf2 : 6;
    uint32_t clkod2 : 4;
    uint32_t bwadj2 : 6;
    uint32_t pll_reset2 : 1;
    uint32_t pll_pwrd2 : 1;
    uint32_t pll_intfb2 : 1;
    uint32_t pll_bypass2 : 1;
    uint32_t pll_test2 : 1;
    uint32_t pll_out_en2 : 1;
    uint32_t pll_ckin_sel2 : 2;
    uint32_t reserved : 4;
} __attribute__((packed, aligned(4))) sysctl_pll2_t;

/**
 * @brief       PLL lock tester
 *
 *              No. 6 Register (0x18)
 */
typedef struct _sysctl_pll_lock
{
    uint32_t pll_lock0 : 2;
    uint32_t pll_slip_clear0 : 1;
    uint32_t test_clk_out0 : 1;
    uint32_t reserved0 : 4;
    uint32_t pll_lock1 : 2;
    uint32_t pll_slip_clear1 : 1;
    uint32_t test_clk_out1 : 1;
    uint32_t reserved1 : 4;
    uint32_t pll_lock2 : 2;
    uint32_t pll_slip_clear2 : 1;
    uint32_t test_clk_out2 : 1;
    uint32_t reserved2 : 12;
} __attribute__((packed, aligned(4))) sysctl_pll_lock_t;

/**
 * @brief       AXI ROM detector
 *
 *              No. 7 Register (0x1c)
 */
typedef struct _sysctl_rom_error
{
    uint32_t rom_mul_error : 1;
    uint32_t rom_one_error : 1;
    uint32_t reserved : 30;
} __attribute__((packed, aligned(4))) sysctl_rom_error_t;

/**
 * @brief       Clock select controller0
 *
 *              No. 8 Register (0x20)
 */
typedef struct _sysctl_clk_sel0
{
    uint32_t aclk_sel : 1;
    uint32_t aclk_divider_sel : 2;
    uint32_t apb0_clk_sel : 3;
    uint32_t apb1_clk_sel : 3;
    uint32_t apb2_clk_sel : 3;
    uint32_t spi3_clk_sel : 1;
    uint32_t timer0_clk_sel : 1;
    uint32_t timer1_clk_sel : 1;
    uint32_t timer2_clk_sel : 1;
    uint32_t reserved : 16;
} __attribute__((packed, aligned(4))) sysctl_clk_sel0_t;

/**
 * @brief       Clock select controller1
 *
 *              No. 9 Register (0x24)
 */
typedef struct _sysctl_clk_sel1
{
    uint32_t spi3_sample_clk_sel : 1;
    uint32_t reserved0 : 30;
    uint32_t reserved1 : 1;
} __attribute__((packed, aligned(4))) sysctl_clk_sel1_t;

/**
 * @brief       Central clock enable
 *
 *              No. 10 Register (0x28)
 */
typedef struct _sysctl_clk_en_cent
{
    uint32_t cpu_clk_en : 1;
    uint32_t sram0_clk_en : 1;
    uint32_t sram1_clk_en : 1;
    uint32_t apb0_clk_en : 1;
    uint32_t apb1_clk_en : 1;
    uint32_t apb2_clk_en : 1;
    uint32_t reserved : 26;
} __attribute__((packed, aligned(4))) sysctl_clk_en_cent_t;

/**
 * @brief       Peripheral clock enable
 *
 *              No. 11 Register (0x2c)
 */
typedef struct _sysctl_clk_en_peri
{
    uint32_t rom_clk_en : 1;
    uint32_t dma_clk_en : 1;
    uint32_t ai_clk_en : 1;
    uint32_t dvp_clk_en : 1;
    uint32_t fft_clk_en : 1;
    uint32_t gpio_clk_en : 1;
    uint32_t spi0_clk_en : 1;
    uint32_t spi1_clk_en : 1;
    uint32_t spi2_clk_en : 1;
    uint32_t spi3_clk_en : 1;
    uint32_t i2s0_clk_en : 1;
    uint32_t i2s1_clk_en : 1;
    uint32_t i2s2_clk_en : 1;
    uint32_t i2c0_clk_en : 1;
    uint32_t i2c1_clk_en : 1;
    uint32_t i2c2_clk_en : 1;
    uint32_t uart1_clk_en : 1;
    uint32_t uart2_clk_en : 1;
    uint32_t uart3_clk_en : 1;
    uint32_t aes_clk_en : 1;
    uint32_t fpioa_clk_en : 1;
    uint32_t timer0_clk_en : 1;
    uint32_t timer1_clk_en : 1;
    uint32_t timer2_clk_en : 1;
    uint32_t wdt0_clk_en : 1;
    uint32_t wdt1_clk_en : 1;
    uint32_t sha_clk_en : 1;
    uint32_t otp_clk_en : 1;
    uint32_t reserved : 1;
    uint32_t rtc_clk_en : 1;
    uint32_t reserved0 : 2;
} __attribute__((packed, aligned(4))) sysctl_clk_en_peri_t;

/**
 * @brief       Soft reset ctrl
 *
 *              No. 12 Register (0x30)
 */
typedef struct _sysctl_soft_reset
{
    uint32_t soft_reset : 1;
    uint32_t reserved : 31;
} __attribute__((packed, aligned(4))) sysctl_soft_reset_t;

/**
 * @brief       Peripheral reset controller
 *
 *              No. 13 Register (0x34)
 */
typedef struct _sysctl_peri_reset
{
    uint32_t rom_reset : 1;
    uint32_t dma_reset : 1;
    uint32_t ai_reset : 1;
    uint32_t dvp_reset : 1;
    uint32_t fft_reset : 1;
    uint32_t gpio_reset : 1;
    uint32_t spi0_reset : 1;
    uint32_t spi1_reset : 1;
    uint32_t spi2_reset : 1;
    uint32_t spi3_reset : 1;
    uint32_t i2s0_reset : 1;
    uint32_t i2s1_reset : 1;
    uint32_t i2s2_reset : 1;
    uint32_t i2c0_reset : 1;
    uint32_t i2c1_reset : 1;
    uint32_t i2c2_reset : 1;
    uint32_t uart1_reset : 1;
    uint32_t uart2_reset : 1;
    uint32_t uart3_reset : 1;
    uint32_t aes_reset : 1;
    uint32_t fpioa_reset : 1;
    uint32_t timer0_reset : 1;
    uint32_t timer1_reset : 1;
    uint32_t timer2_reset : 1;
    uint32_t wdt0_reset : 1;
    uint32_t wdt1_reset : 1;
    uint32_t sha_reset : 1;
    uint32_t reserved : 2;
    uint32_t rtc_reset : 1;
    uint32_t reserved0 : 2;
} __attribute__((packed, aligned(4))) sysctl_peri_reset_t;

/**
 * @brief       Clock threshold controller 0
 *
 *              No. 14 Register (0x38)
 */
typedef struct _sysctl_clk_th0
{
    uint32_t sram0_gclk_threshold : 4;
    uint32_t sram1_gclk_threshold : 4;
    uint32_t ai_gclk_threshold : 4;
    uint32_t dvp_gclk_threshold : 4;
    uint32_t rom_gclk_threshold : 4;
    uint32_t reserved : 12;
} __attribute__((packed, aligned(4))) sysctl_clk_th0_t;

/**
 * @brief       Clock threshold controller 1
 *
 *              No. 15 Register (0x3c)
 */
typedef struct _sysctl_clk_th1
{
    uint32_t spi0_clk_threshold : 8;
    uint32_t spi1_clk_threshold : 8;
    uint32_t spi2_clk_threshold : 8;
    uint32_t spi3_clk_threshold : 8;
} __attribute__((packed, aligned(4))) sysctl_clk_th1_t;

/**
 * @brief       Clock threshold controller 2
 *
 *              No. 16 Register (0x40)
 */
typedef struct _sysctl_clk_th2
{
    uint32_t timer0_clk_threshold : 8;
    uint32_t timer1_clk_threshold : 8;
    uint32_t timer2_clk_threshold : 8;
    uint32_t reserved : 8;
} __attribute__((packed, aligned(4))) sysctl_clk_th2_t;

/**
 * @brief       Clock threshold controller 3
 *
 *              No. 17 Register (0x44)
 */
typedef struct _sysctl_clk_th3
{
    uint32_t i2s0_clk_threshold : 16;
    uint32_t i2s1_clk_threshold : 16;
} __attribute__((packed, aligned(4))) sysctl_clk_th3_t;

/**
 * @brief       Clock threshold controller 4
 *
 *              No. 18 Register (0x48)
 */
typedef struct _sysctl_clk_th4
{
    uint32_t i2s2_clk_threshold : 16;
    uint32_t i2s0_mclk_threshold : 8;
    uint32_t i2s1_mclk_threshold : 8;
} __attribute__((packed, aligned(4))) sysctl_clk_th4_t;

/**
 * @brief       Clock threshold controller 5
 *
 *              No. 19 Register (0x4c)
 */
typedef struct _sysctl_clk_th5
{
    uint32_t i2s2_mclk_threshold : 8;
    uint32_t i2c0_clk_threshold : 8;
    uint32_t i2c1_clk_threshold : 8;
    uint32_t i2c2_clk_threshold : 8;
} __attribute__((packed, aligned(4))) sysctl_clk_th5_t;

/**
 * @brief       Clock threshold controller 6
 *
 *              No. 20 Register (0x50)
 */
typedef struct _sysctl_clk_th6
{
    uint32_t wdt0_clk_threshold : 8;
    uint32_t wdt1_clk_threshold : 8;
    uint32_t reserved0 : 8;
    uint32_t reserved1 : 8;
} __attribute__((packed, aligned(4))) sysctl_clk_th6_t;

/**
 * @brief       Miscellaneous controller
 *
 *              No. 21 Register (0x54)
 */
typedef struct _sysctl_misc
{
    uint32_t debug_sel : 6;
    uint32_t reserved0 : 4;
    uint32_t spi_dvp_data_enable: 1;
    uint32_t reserved1 : 21;
} __attribute__((packed, aligned(4))) sysctl_misc_t;

/**
 * @brief       Peripheral controller
 *
 *              No. 22 Register (0x58)
 */
typedef struct _sysctl_peri
{
    uint32_t timer0_pause : 1;
    uint32_t timer1_pause : 1;
    uint32_t timer2_pause : 1;
    uint32_t timer3_pause : 1;
    uint32_t timer4_pause : 1;
    uint32_t timer5_pause : 1;
    uint32_t timer6_pause : 1;
    uint32_t timer7_pause : 1;
    uint32_t timer8_pause : 1;
    uint32_t timer9_pause : 1;
    uint32_t timer10_pause : 1;
    uint32_t timer11_pause : 1;
    uint32_t spi0_xip_en : 1;
    uint32_t spi1_xip_en : 1;
    uint32_t spi2_xip_en : 1;
    uint32_t spi3_xip_en : 1;
    uint32_t spi0_clk_bypass : 1;
    uint32_t spi1_clk_bypass : 1;
    uint32_t spi2_clk_bypass : 1;
    uint32_t i2s0_clk_bypass : 1;
    uint32_t i2s1_clk_bypass : 1;
    uint32_t i2s2_clk_bypass : 1;
    uint32_t jtag_clk_bypass : 1;
    uint32_t dvp_clk_bypass : 1;
    uint32_t debug_clk_bypass : 1;
    uint32_t reserved0 : 1;
    uint32_t reserved1 : 6;
} __attribute__((packed, aligned(4))) sysctl_peri_t;

/**
 * @brief       SPI sleep controller
 *
 *              No. 23 Register (0x5c)
 */
typedef struct _sysctl_spi_sleep
{
    uint32_t ssi0_sleep : 1;
    uint32_t ssi1_sleep : 1;
    uint32_t ssi2_sleep : 1;
    uint32_t ssi3_sleep : 1;
    uint32_t reserved : 28;
} __attribute__((packed, aligned(4))) sysctl_spi_sleep_t;

/**
 * @brief       Reset source status
 *
 *              No. 24 Register (0x60)
 */
typedef struct _sysctl_reset_status
{
    uint32_t reset_sts_clr : 1;
    uint32_t pin_reset_sts : 1;
    uint32_t wdt0_reset_sts : 1;
    uint32_t wdt1_reset_sts : 1;
    uint32_t soft_reset_sts : 1;
    uint32_t reserved : 27;
} __attribute__((packed, aligned(4))) sysctl_reset_status_t;

/**
 * @brief       DMA handshake selector
 *
 *              No. 25 Register (0x64)
 */
typedef struct _sysctl_dma_sel0
{
    uint32_t dma_sel0 : 6;
    uint32_t dma_sel1 : 6;
    uint32_t dma_sel2 : 6;
    uint32_t dma_sel3 : 6;
    uint32_t dma_sel4 : 6;
    uint32_t reserved : 2;
} __attribute__((packed, aligned(4))) sysctl_dma_sel0_t;

/**
 * @brief       DMA handshake selector
 *
 *              No. 26 Register (0x68)
 */
typedef struct _sysctl_dma_sel1
{
    uint32_t dma_sel5 : 6;
    uint32_t reserved : 26;
} __attribute__((packed, aligned(4))) sysctl_dma_sel1_t;

/**
 * @brief       IO Power Mode Select controller
 *
 *              No. 27 Register (0x6c)
 */
typedef struct _sysctl_power_sel
{
    uint32_t power_mode_sel0 : 1;
    uint32_t power_mode_sel1 : 1;
    uint32_t power_mode_sel2 : 1;
    uint32_t power_mode_sel3 : 1;
    uint32_t power_mode_sel4 : 1;
    uint32_t power_mode_sel5 : 1;
    uint32_t power_mode_sel6 : 1;
    uint32_t power_mode_sel7 : 1;
    uint32_t reserved : 24;
} __attribute__((packed, aligned(4))) sysctl_power_sel_t;

/**
 * @brief       System controller object
 *
 *              The System controller is a peripheral device mapped in the
 *              internal memory map, discoverable in the Configuration String.
 *              It is responsible for low-level configuration of all system
 *              related peripheral device. It contain PLL controller, clock
 *              controller, reset controller, DMA handshake controller, SPI
 *              controller, timer controller, WDT controller and sleep
 *              controller.
 */
typedef struct _sysctl
{
    /* No. 0 (0x00): Git short commit id */
    sysctl_git_id_t git_id;
    /* No. 1 (0x04): System clock base frequency */
    sysctl_clk_freq_t clk_freq;
    /* No. 2 (0x08): PLL0 controller */
    sysctl_pll0_t pll0;
    /* No. 3 (0x0c): PLL1 controller */
    sysctl_pll1_t pll1;
    /* No. 4 (0x10): PLL2 controller */
    sysctl_pll2_t pll2;
    /* No. 5 (0x14): Reserved */
    uint32_t resv5;
    /* No. 6 (0x18): PLL lock tester */
    sysctl_pll_lock_t pll_lock;
    /* No. 7 (0x1c): AXI ROM detector */
    sysctl_rom_error_t rom_error;
    /* No. 8 (0x20): Clock select controller0 */
    sysctl_clk_sel0_t clk_sel0;
    /* No. 9 (0x24): Clock select controller1 */
    sysctl_clk_sel1_t clk_sel1;
    /* No. 10 (0x28): Central clock enable */
    sysctl_clk_en_cent_t clk_en_cent;
    /* No. 11 (0x2c): Peripheral clock enable */
    sysctl_clk_en_peri_t clk_en_peri;
    /* No. 12 (0x30): Soft reset ctrl */
    sysctl_soft_reset_t soft_reset;
    /* No. 13 (0x34): Peripheral reset controller */
    sysctl_peri_reset_t peri_reset;
    /* No. 14 (0x38): Clock threshold controller 0 */
    sysctl_clk_th0_t clk_th0;
    /* No. 15 (0x3c): Clock threshold controller 1 */
    sysctl_clk_th1_t clk_th1;
    /* No. 16 (0x40): Clock threshold controller 2 */
    sysctl_clk_th2_t clk_th2;
    /* No. 17 (0x44): Clock threshold controller 3 */
    sysctl_clk_th3_t clk_th3;
    /* No. 18 (0x48): Clock threshold controller 4 */
    sysctl_clk_th4_t clk_th4;
    /* No. 19 (0x4c): Clock threshold controller 5 */
    sysctl_clk_th5_t clk_th5;
    /* No. 20 (0x50): Clock threshold controller 6 */
    sysctl_clk_th6_t clk_th6;
    /* No. 21 (0x54): Miscellaneous controller */
    sysctl_misc_t misc;
    /* No. 22 (0x58): Peripheral controller */
    sysctl_peri_t peri;
    /* No. 23 (0x5c): SPI sleep controller */
    sysctl_spi_sleep_t spi_sleep;
    /* No. 24 (0x60): Reset source status */
    sysctl_reset_status_t reset_status;
    /* No. 25 (0x64): DMA handshake selector */
    sysctl_dma_sel0_t dma_sel0;
    /* No. 26 (0x68): DMA handshake selector */
    sysctl_dma_sel1_t dma_sel1;
    /* No. 27 (0x6c): IO Power Mode Select controller */
    sysctl_power_sel_t power_sel;
    /* No. 28 (0x70): Reserved */
    uint32_t resv28;
    /* No. 29 (0x74): Reserved */
    uint32_t resv29;
    /* No. 30 (0x78): Reserved */
    uint32_t resv30;
    /* No. 31 (0x7c): Reserved */
    uint32_t resv31;
} __attribute__((packed, aligned(4))) sysctl_t;

/**
 * @brief       Abstruct PLL struct
 */
typedef struct _sysctl_general_pll
{
    uint32_t clkr : 4;
    uint32_t clkf : 6;
    uint32_t clkod : 4;
    uint32_t bwadj : 6;
    uint32_t pll_reset : 1;
    uint32_t pll_pwrd : 1;
    uint32_t pll_intfb : 1;
    uint32_t pll_bypass : 1;
    uint32_t pll_test : 1;
    uint32_t pll_out_en : 1;
    uint32_t pll_ckin_sel : 2;
    uint32_t reserved : 4;
} __attribute__((packed, aligned(4))) sysctl_general_pll_t;

/**
 * @brief       System controller object instanse
 */
extern volatile sysctl_t *const sysctl;

/**
 * @brief       Enable clock for peripheral
 *
 * @param[in]   clock       The clock to be enable
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int sysctl_clock_enable(sysctl_clock_t clock);

/**
 * @brief       Enable clock for peripheral
 *
 * @param[in]   clock       The clock to be disable
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int sysctl_clock_disable(sysctl_clock_t clock);

/**
 * @brief       Sysctl clock set threshold
 *
 * @param[in]   which           Which threshold to set
 * @param[in]   threshold       The threshold value
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int sysctl_clock_set_threshold(sysctl_threshold_t which, int threshold);

/**
 * @brief       Sysctl clock get threshold
 *
 * @param[in]   which       Which threshold to get
 *
 * @return      The threshold value
 *     - Other  Value of threshold
 *     - -1     Fail
 */
int sysctl_clock_get_threshold(sysctl_threshold_t which);

/**
 * @brief       Sysctl clock set clock select
 *
 * @param[in]   which       Which clock select to set
 * @param[in]   select      The clock select value
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int sysctl_clock_set_clock_select(sysctl_clock_select_t which, int select);

/**
 * @brief       Sysctl clock get clock select
 *
 * @param[in]   which  Which clock select to get
 *
 * @return      The clock select value
 *     - Other  Value of clock select
 *     - -1     Fail
 */
int sysctl_clock_get_clock_select(sysctl_clock_select_t which);

/**
 * @brief       Get PLL frequency
 *
 * @param[in]   pll     The PLL id
 *
 * @return      The frequency of PLL
 */
uint32_t sysctl_pll_get_freq(sysctl_pll_t pll);

/**
 * @brief       Get base clock frequency by clock id
 *
 * @param[in]   clock       The clock id
 *
 * @return      The clock frequency
 */
uint32_t sysctl_clock_get_freq(sysctl_clock_t clock);

/**
 * @brief       Reset device by reset controller
 *
 * @param[in]   reset       The reset signal
 */
void sysctl_reset(sysctl_reset_t reset);

/**
 * @brief       Enable the PLL and power on with reset
 *
 * @param[in]   pll     The pll id
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
int sysctl_pll_enable(sysctl_pll_t pll);

/**
 * @brief       Disable the PLL and power off
 *
 * @param[in]   pll     The pll id
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
int sysctl_pll_disable(sysctl_pll_t pll);

/**
 * @brief       Select DMA channel handshake peripheral signal
 *
 * @param[in]   channel     The DMA channel
 * @param[in]   select      The peripheral select
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
int sysctl_dma_select(sysctl_dma_channel_t channel, sysctl_dma_select_t select);

/**
 * @brief       Set SPI0_D0-D7 DVP_D0-D7 as spi and dvp data pin
 *
 * @param[in]   en     Enable or not
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
uint32_t sysctl_set_spi0_dvp_data(uint8_t en);

/**
 * @brief       Set io power mode
 *
 * @param[in]   power_bank          IO power bank
 * @param[in]   io_power_mode       Set power mode 3.3v or 1.8
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
void sysctl_set_power_mode(sysctl_power_bank_t power_bank, sysctl_io_power_mode_t io_power_mode);

/**
 * @brief       get the frequency of CPU
 * 
 * @return      The frequency of CPU
 */
uint32_t sysctl_cpu_get_freq(void);

/**
 * @brief       Set frequency of CPU
 * @param[in]   freq       The desired frequency in Hz
 *
 * @return      The actual frequency of CPU after set
 */
uint32_t sysctl_cpu_set_freq(uint32_t freq);

/**
 * @brief       Init PLL freqency
 * @param[in]   pll            The PLL id
 * @param[in]   pll_freq       The desired frequency in Hz

 */
uint32_t sysctl_pll_set_freq(sysctl_pll_t pll, uint32_t pll_freq);

/**
 * @brief       Enable interrupt
 */
void sysctl_enable_irq(void);

/**
 * @brief       Disable interrupt
 */
void sysctl_disable_irq(void);

/**
 * @brief       Get the time start up to now
 *
 * @return      The time of microsecond
 */
uint64_t sysctl_get_time_us(void);

/**
 * @brief       Get reset status
 *
 * @return      The status of reset
 */
sysctl_reset_enum_status_t sysctl_get_reset_status(void);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_SYSCTL_H */
