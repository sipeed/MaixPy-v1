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
 * @brief      The PLIC complies with the RISC-V Privileged Architecture
 *             specification, and can support a maximum of 1023 external
 *             interrupt sources targeting up to 15,872 core contexts.
 *
 * @note       PLIC RAM Layout
 *
 * | Address   | Description                     |
 * |-----------|---------------------------------|
 * |0x0C000000 | Reserved                        |
 * |0x0C000004 | source 1 priority               |
 * |0x0C000008 | source 2 priority               |
 * |...        | ...                             |
 * |0x0C000FFC | source 1023 priority            |
 * |           |                                 |
 * |0x0C001000 | Start of pending array          |
 * |...        | (read-only)                     |
 * |0x0C00107C | End of pending array            |
 * |0x0C001080 | Reserved                        |
 * |...        | ...                             |
 * |0x0C001FFF | Reserved                        |
 * |           |                                 |
 * |0x0C002000 | target 0 enables                |
 * |0x0C002080 | target 1 enables                |
 * |...        | ...                             |
 * |0x0C1F1F80 | target 15871 enables            |
 * |0x0C1F2000 | Reserved                        |
 * |...        | ...                             |
 * |0x0C1FFFFC | Reserved                        |
 * |           |                                 |
 * |0x0C200000 | target 0 priority threshold     |
 * |0x0C200004 | target 0 claim/complete         |
 * |...        | ...                             |
 * |0x0C201000 | target 1 priority threshold     |
 * |0x0C201004 | target 1 claim/complete         |
 * |...        | ...                             |
 * |0x0FFFF000 | target 15871 priority threshold |
 * |0x0FFFF004 | target 15871 claim/complete     |
 *
 */

#ifndef _DRIVER_PLIC_H
#define _DRIVER_PLIC_H

#include <stdint.h>
#include "encoding.h"
#include "platform.h"

/* For c++ compatibility */
#ifdef __cplusplus
extern "C" {
#endif

/* clang-format off */
/* IRQ number settings */
#define PLIC_NUM_SOURCES    (IRQN_MAX - 1)
#define PLIC_NUM_PRIORITIES (7)

/* Real number of cores */
#define PLIC_NUM_CORES      (2)
/* clang-format on */

/**
 * @brief       PLIC External Interrupt Numbers
 *
 * @note        PLIC interrupt sources
 *
 * | Source | Name                     | Description                        |
 * |--------|--------------------------|------------------------------------|
 * | 0      | IRQN_NO_INTERRUPT        | The non-existent interrupt         |
 * | 1      | IRQN_SPI0_INTERRUPT      | SPI0 interrupt                     |
 * | 2      | IRQN_SPI1_INTERRUPT      | SPI1 interrupt                     |
 * | 3      | IRQN_SPI_SLAVE_INTERRUPT | SPI_SLAVE interrupt                |
 * | 4      | IRQN_SPI3_INTERRUPT      | SPI3 interrupt                     |
 * | 5      | IRQN_I2S0_INTERRUPT      | I2S0 interrupt                     |
 * | 6      | IRQN_I2S1_INTERRUPT      | I2S1 interrupt                     |
 * | 7      | IRQN_I2S2_INTERRUPT      | I2S2 interrupt                     |
 * | 8      | IRQN_I2C0_INTERRUPT      | I2C0 interrupt                     |
 * | 9      | IRQN_I2C1_INTERRUPT      | I2C1 interrupt                     |
 * | 10     | IRQN_I2C2_INTERRUPT      | I2C2 interrupt                     |
 * | 11     | IRQN_UART1_INTERRUPT     | UART1 interrupt                    |
 * | 12     | IRQN_UART2_INTERRUPT     | UART2 interrupt                    |
 * | 13     | IRQN_UART3_INTERRUPT     | UART3 interrupt                    |
 * | 14     | IRQN_TIMER0A_INTERRUPT   | TIMER0 channel 0 or 1 interrupt    |
 * | 15     | IRQN_TIMER0B_INTERRUPT   | TIMER0 channel 2 or 3 interrupt    |
 * | 16     | IRQN_TIMER1A_INTERRUPT   | TIMER1 channel 0 or 1 interrupt    |
 * | 17     | IRQN_TIMER1B_INTERRUPT   | TIMER1 channel 2 or 3 interrupt    |
 * | 18     | IRQN_TIMER2A_INTERRUPT   | TIMER2 channel 0 or 1 interrupt    |
 * | 19     | IRQN_TIMER2B_INTERRUPT   | TIMER2 channel 2 or 3 interrupt    |
 * | 20     | IRQN_RTC_INTERRUPT       | RTC tick and alarm interrupt       |
 * | 21     | IRQN_WDT0_INTERRUPT      | Watching dog timer0 interrupt      |
 * | 22     | IRQN_WDT1_INTERRUPT      | Watching dog timer1 interrupt      |
 * | 23     | IRQN_APB_GPIO_INTERRUPT  | APB GPIO interrupt                 |
 * | 24     | IRQN_DVP_INTERRUPT       | Digital video port interrupt       |
 * | 25     | IRQN_AI_INTERRUPT        | AI accelerator interrupt           |
 * | 26     | IRQN_FFT_INTERRUPT       | FFT accelerator interrupt          |
 * | 27     | IRQN_DMA0_INTERRUPT      | DMA channel0 interrupt             |
 * | 28     | IRQN_DMA1_INTERRUPT      | DMA channel1 interrupt             |
 * | 29     | IRQN_DMA2_INTERRUPT      | DMA channel2 interrupt             |
 * | 30     | IRQN_DMA3_INTERRUPT      | DMA channel3 interrupt             |
 * | 31     | IRQN_DMA4_INTERRUPT      | DMA channel4 interrupt             |
 * | 32     | IRQN_DMA5_INTERRUPT      | DMA channel5 interrupt             |
 * | 33     | IRQN_UARTHS_INTERRUPT    | Hi-speed UART0 interrupt           |
 * | 34     | IRQN_GPIOHS0_INTERRUPT   | Hi-speed GPIO0 interrupt           |
 * | 35     | IRQN_GPIOHS1_INTERRUPT   | Hi-speed GPIO1 interrupt           |
 * | 36     | IRQN_GPIOHS2_INTERRUPT   | Hi-speed GPIO2 interrupt           |
 * | 37     | IRQN_GPIOHS3_INTERRUPT   | Hi-speed GPIO3 interrupt           |
 * | 38     | IRQN_GPIOHS4_INTERRUPT   | Hi-speed GPIO4 interrupt           |
 * | 39     | IRQN_GPIOHS5_INTERRUPT   | Hi-speed GPIO5 interrupt           |
 * | 40     | IRQN_GPIOHS6_INTERRUPT   | Hi-speed GPIO6 interrupt           |
 * | 41     | IRQN_GPIOHS7_INTERRUPT   | Hi-speed GPIO7 interrupt           |
 * | 42     | IRQN_GPIOHS8_INTERRUPT   | Hi-speed GPIO8 interrupt           |
 * | 43     | IRQN_GPIOHS9_INTERRUPT   | Hi-speed GPIO9 interrupt           |
 * | 44     | IRQN_GPIOHS10_INTERRUPT  | Hi-speed GPIO10 interrupt          |
 * | 45     | IRQN_GPIOHS11_INTERRUPT  | Hi-speed GPIO11 interrupt          |
 * | 46     | IRQN_GPIOHS12_INTERRUPT  | Hi-speed GPIO12 interrupt          |
 * | 47     | IRQN_GPIOHS13_INTERRUPT  | Hi-speed GPIO13 interrupt          |
 * | 48     | IRQN_GPIOHS14_INTERRUPT  | Hi-speed GPIO14 interrupt          |
 * | 49     | IRQN_GPIOHS15_INTERRUPT  | Hi-speed GPIO15 interrupt          |
 * | 50     | IRQN_GPIOHS16_INTERRUPT  | Hi-speed GPIO16 interrupt          |
 * | 51     | IRQN_GPIOHS17_INTERRUPT  | Hi-speed GPIO17 interrupt          |
 * | 52     | IRQN_GPIOHS18_INTERRUPT  | Hi-speed GPIO18 interrupt          |
 * | 53     | IRQN_GPIOHS19_INTERRUPT  | Hi-speed GPIO19 interrupt          |
 * | 54     | IRQN_GPIOHS20_INTERRUPT  | Hi-speed GPIO20 interrupt          |
 * | 55     | IRQN_GPIOHS21_INTERRUPT  | Hi-speed GPIO21 interrupt          |
 * | 56     | IRQN_GPIOHS22_INTERRUPT  | Hi-speed GPIO22 interrupt          |
 * | 57     | IRQN_GPIOHS23_INTERRUPT  | Hi-speed GPIO23 interrupt          |
 * | 58     | IRQN_GPIOHS24_INTERRUPT  | Hi-speed GPIO24 interrupt          |
 * | 59     | IRQN_GPIOHS25_INTERRUPT  | Hi-speed GPIO25 interrupt          |
 * | 60     | IRQN_GPIOHS26_INTERRUPT  | Hi-speed GPIO26 interrupt          |
 * | 61     | IRQN_GPIOHS27_INTERRUPT  | Hi-speed GPIO27 interrupt          |
 * | 62     | IRQN_GPIOHS28_INTERRUPT  | Hi-speed GPIO28 interrupt          |
 * | 63     | IRQN_GPIOHS29_INTERRUPT  | Hi-speed GPIO29 interrupt          |
 * | 64     | IRQN_GPIOHS30_INTERRUPT  | Hi-speed GPIO30 interrupt          |
 * | 65     | IRQN_GPIOHS31_INTERRUPT  | Hi-speed GPIO31 interrupt          |
 *
 */
/* clang-format off */
typedef enum _plic_irq
{
    IRQN_NO_INTERRUPT        = 0, /*!< The non-existent interrupt */
    IRQN_SPI0_INTERRUPT      = 1, /*!< SPI0 interrupt */
    IRQN_SPI1_INTERRUPT      = 2, /*!< SPI1 interrupt */
    IRQN_SPI_SLAVE_INTERRUPT = 3, /*!< SPI_SLAVE interrupt */
    IRQN_SPI3_INTERRUPT      = 4, /*!< SPI3 interrupt */
    IRQN_I2S0_INTERRUPT      = 5, /*!< I2S0 interrupt */
    IRQN_I2S1_INTERRUPT      = 6, /*!< I2S1 interrupt */
    IRQN_I2S2_INTERRUPT      = 7, /*!< I2S2 interrupt */
    IRQN_I2C0_INTERRUPT      = 8, /*!< I2C0 interrupt */
    IRQN_I2C1_INTERRUPT      = 9, /*!< I2C1 interrupt */
    IRQN_I2C2_INTERRUPT      = 10, /*!< I2C2 interrupt */
    IRQN_UART1_INTERRUPT     = 11, /*!< UART1 interrupt */
    IRQN_UART2_INTERRUPT     = 12, /*!< UART2 interrupt */
    IRQN_UART3_INTERRUPT     = 13, /*!< UART3 interrupt */
    IRQN_TIMER0A_INTERRUPT   = 14, /*!< TIMER0 channel 0 or 1 interrupt */
    IRQN_TIMER0B_INTERRUPT   = 15, /*!< TIMER0 channel 2 or 3 interrupt */
    IRQN_TIMER1A_INTERRUPT   = 16, /*!< TIMER1 channel 0 or 1 interrupt */
    IRQN_TIMER1B_INTERRUPT   = 17, /*!< TIMER1 channel 2 or 3 interrupt */
    IRQN_TIMER2A_INTERRUPT   = 18, /*!< TIMER2 channel 0 or 1 interrupt */
    IRQN_TIMER2B_INTERRUPT   = 19, /*!< TIMER2 channel 2 or 3 interrupt */
    IRQN_RTC_INTERRUPT       = 20, /*!< RTC tick and alarm interrupt */
    IRQN_WDT0_INTERRUPT      = 21, /*!< Watching dog timer0 interrupt */
    IRQN_WDT1_INTERRUPT      = 22, /*!< Watching dog timer1 interrupt */
    IRQN_APB_GPIO_INTERRUPT  = 23, /*!< APB GPIO interrupt */
    IRQN_DVP_INTERRUPT       = 24, /*!< Digital video port interrupt */
    IRQN_AI_INTERRUPT        = 25, /*!< AI accelerator interrupt */
    IRQN_FFT_INTERRUPT       = 26, /*!< FFT accelerator interrupt */
    IRQN_DMA0_INTERRUPT      = 27, /*!< DMA channel0 interrupt */
    IRQN_DMA1_INTERRUPT      = 28, /*!< DMA channel1 interrupt */
    IRQN_DMA2_INTERRUPT      = 29, /*!< DMA channel2 interrupt */
    IRQN_DMA3_INTERRUPT      = 30, /*!< DMA channel3 interrupt */
    IRQN_DMA4_INTERRUPT      = 31, /*!< DMA channel4 interrupt */
    IRQN_DMA5_INTERRUPT      = 32, /*!< DMA channel5 interrupt */
    IRQN_UARTHS_INTERRUPT    = 33, /*!< Hi-speed UART0 interrupt */
    IRQN_GPIOHS0_INTERRUPT   = 34, /*!< Hi-speed GPIO0 interrupt */
    IRQN_GPIOHS1_INTERRUPT   = 35, /*!< Hi-speed GPIO1 interrupt */
    IRQN_GPIOHS2_INTERRUPT   = 36, /*!< Hi-speed GPIO2 interrupt */
    IRQN_GPIOHS3_INTERRUPT   = 37, /*!< Hi-speed GPIO3 interrupt */
    IRQN_GPIOHS4_INTERRUPT   = 38, /*!< Hi-speed GPIO4 interrupt */
    IRQN_GPIOHS5_INTERRUPT   = 39, /*!< Hi-speed GPIO5 interrupt */
    IRQN_GPIOHS6_INTERRUPT   = 40, /*!< Hi-speed GPIO6 interrupt */
    IRQN_GPIOHS7_INTERRUPT   = 41, /*!< Hi-speed GPIO7 interrupt */
    IRQN_GPIOHS8_INTERRUPT   = 42, /*!< Hi-speed GPIO8 interrupt */
    IRQN_GPIOHS9_INTERRUPT   = 43, /*!< Hi-speed GPIO9 interrupt */
    IRQN_GPIOHS10_INTERRUPT  = 44, /*!< Hi-speed GPIO10 interrupt */
    IRQN_GPIOHS11_INTERRUPT  = 45, /*!< Hi-speed GPIO11 interrupt */
    IRQN_GPIOHS12_INTERRUPT  = 46, /*!< Hi-speed GPIO12 interrupt */
    IRQN_GPIOHS13_INTERRUPT  = 47, /*!< Hi-speed GPIO13 interrupt */
    IRQN_GPIOHS14_INTERRUPT  = 48, /*!< Hi-speed GPIO14 interrupt */
    IRQN_GPIOHS15_INTERRUPT  = 49, /*!< Hi-speed GPIO15 interrupt */
    IRQN_GPIOHS16_INTERRUPT  = 50, /*!< Hi-speed GPIO16 interrupt */
    IRQN_GPIOHS17_INTERRUPT  = 51, /*!< Hi-speed GPIO17 interrupt */
    IRQN_GPIOHS18_INTERRUPT  = 52, /*!< Hi-speed GPIO18 interrupt */
    IRQN_GPIOHS19_INTERRUPT  = 53, /*!< Hi-speed GPIO19 interrupt */
    IRQN_GPIOHS20_INTERRUPT  = 54, /*!< Hi-speed GPIO20 interrupt */
    IRQN_GPIOHS21_INTERRUPT  = 55, /*!< Hi-speed GPIO21 interrupt */
    IRQN_GPIOHS22_INTERRUPT  = 56, /*!< Hi-speed GPIO22 interrupt */
    IRQN_GPIOHS23_INTERRUPT  = 57, /*!< Hi-speed GPIO23 interrupt */
    IRQN_GPIOHS24_INTERRUPT  = 58, /*!< Hi-speed GPIO24 interrupt */
    IRQN_GPIOHS25_INTERRUPT  = 59, /*!< Hi-speed GPIO25 interrupt */
    IRQN_GPIOHS26_INTERRUPT  = 60, /*!< Hi-speed GPIO26 interrupt */
    IRQN_GPIOHS27_INTERRUPT  = 61, /*!< Hi-speed GPIO27 interrupt */
    IRQN_GPIOHS28_INTERRUPT  = 62, /*!< Hi-speed GPIO28 interrupt */
    IRQN_GPIOHS29_INTERRUPT  = 63, /*!< Hi-speed GPIO29 interrupt */
    IRQN_GPIOHS30_INTERRUPT  = 64, /*!< Hi-speed GPIO30 interrupt */
    IRQN_GPIOHS31_INTERRUPT  = 65, /*!< Hi-speed GPIO31 interrupt */
    IRQN_MAX
} plic_irq_t;
/* clang-format on */

/**
 * @brief      Interrupt Source Priorities
 *
 *             Each external interrupt source can be assigned a priority by
 *             writing to its 32-bit memory-mapped priority register. The
 *             number and value of supported priority levels can vary by
 *             implementa- tion, with the simplest implementations having all
 *             devices hardwired at priority 1, in which case, interrupts with
 *             the lowest ID have the highest effective priority. The priority
 *             registers are all WARL.
 */
typedef struct _plic_source_priorities
{
    /* 0x0C000000: Reserved, 0x0C000004-0x0C000FFC: 1-1023 priorities */
    uint32_t priority[1024];
} __attribute__((packed, aligned(4))) plic_source_priorities_t;

/**
 * @brief       Interrupt Pending Bits
 *
 *              The current status of the interrupt source pending bits in the
 *              PLIC core can be read from the pending array, organized as 32
 *              words of 32 bits. The pending bit for interrupt ID N is stored
 *              in bit (N mod 32) of word (N/32). Bit 0 of word 0, which
 *              represents the non-existent interrupt source 0, is always
 *              hardwired to zero. The pending bits are read-only. A pending
 *              bit in the PLIC core can be cleared by setting enable bits to
 *              only enable the desired interrupt, then performing a claim. A
 *              pending bit can be set by instructing the associated gateway to
 *              send an interrupt service request.
 */
typedef struct _plic_pending_bits
{
    /* 0x0C001000-0x0C00107C: Bit 0 is zero, Bits 1-1023 is pending bits */
    uint32_t u32[32];
    /* 0x0C001080-0x0C001FFF: Reserved */
    uint8_t resv[0xF80];
} __attribute__((packed, aligned(4))) plic_pending_bits_t;

/**
 * @brief       Target Interrupt Enables
 *
 *              For each interrupt target, each device’s interrupt can be
 *              enabled by setting the corresponding bit in that target’s
 *              enables registers. The enables for a target are accessed as a
 *              contiguous array of 32×32-bit words, packed the same way as the
 *              pending bits. For each target, bit 0 of enable word 0
 *              represents the non-existent interrupt ID 0 and is hardwired to
 *              0. Unused interrupt IDs are also hardwired to zero. The enables
 *              arrays for different targets are packed contiguously in the
 *              address space. Only 32-bit word accesses are supported by the
 *              enables array in RV32 systems. Implementations can trap on
 *              accesses to enables for non-existent targets, but must allow
 *              access to the full enables array for any extant target,
 *              treating all non-existent interrupt source’s enables as
 *              hardwired to zero.
 */
typedef struct _plic_target_enables
{
    /* 0x0C002000-0x0C1F1F80: target 0-15871 enables */
    struct
    {
        uint32_t enable[32 * 2];/* Offset 0x00-0x7C: Bit 0 is zero, Bits 1-1023 is bits*/
    } target[15872 / 2];

    /* 0x0C1F2000-0x0C1FFFFC: Reserved, size 0xE000 */
    uint8_t resv[0xE000];
} __attribute__((packed, aligned(4))) plic_target_enables_t;

/**
 * @brief       PLIC Targets
 *
 *              Target Priority Thresholds The threshold for a pending
 *              interrupt priority that can interrupt each target can be set in
 *              the target’s threshold register. The threshold is a WARL field,
 *              where different implementations can support different numbers
 *              of thresholds. The simplest implementation has a threshold
 *              hardwired to zero.
 *
 *              Target Claim Each target can perform a claim by reading the
 *              claim/complete register, which returns the ID of the highest
 *              priority pending interrupt or zero if there is no pending
 *              interrupt for the target. A successful claim will also
 *              atomically clear the corresponding pending bit on the interrupt
 *              source. A target can perform a claim at any time, even if the
 *              EIP is not set. The claim operation is not affected by the
 *              setting of the target’s priority threshold register.
 *
 *              Target Completion A target signals it has completed running a
 *              handler by writing the interrupt ID it received from the claim
 *              to the claim/complete register. This is routed to the
 *              corresponding interrupt gateway, which can now send another
 *              interrupt request to the PLIC. The PLIC does not check whether
 *              the completion ID is the same as the last claim ID for that
 *              target. If the completion ID does not match an interrupt source
 *              that is currently enabled for the target, the completion is
 *              silently ignored.
 */
typedef struct _plic_target
{
    /* 0x0C200000-0x0FFFF004: target 0-15871 */
    struct {
        uint32_t priority_threshold;/* Offset 0x000 */
        uint32_t claim_complete;    /* Offset 0x004 */
        uint8_t resv[0x1FF8];        /* Offset 0x008, Size 0xFF8 */
    } target[15872 / 2];
} __attribute__((packed, aligned(4))) plic_target_t;

/**
 * @brief       Platform-Level Interrupt Controller
 *
 *              PLIC is Platform-Level Interrupt Controller. The PLIC complies
 *              with the RISC-V Privileged Architecture specification, and can
 *              support a maximum of 1023 external interrupt sources targeting
 *              up to 15,872 core contexts.
 */
typedef struct _plic
{
    /* 0x0C000000-0x0C000FFC */
    plic_source_priorities_t source_priorities;
    /* 0x0C001000-0x0C001FFF */
    const plic_pending_bits_t pending_bits;
    /* 0x0C002000-0x0C1FFFFC */
    plic_target_enables_t target_enables;
    /* 0x0C200000-0x0FFFF004 */
    plic_target_t targets;
} __attribute__((packed, aligned(4))) plic_t;

extern volatile plic_t *const plic;

/**
 * @brief       Definitions for the interrupt callbacks
 */
typedef int (*plic_irq_callback_t)(void *ctx);

/**
 * @brief       Definitions for IRQ table instance
 */
typedef struct _plic_instance_t
{
    plic_irq_callback_t callback;
    void *ctx;
} plic_instance_t;

typedef struct _plic_callback_t
{
    plic_irq_callback_t callback;
    void *ctx;
    uint32_t priority;
} plic_interrupt_t;

/**
 * @brief       Initialize PLIC external interrupt
 *
 * @note        This function will set MIP_MEIP. The MSTATUS_MIE must set by user.
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
void plic_init(void);

/**
 * @brief       Enable PLIC external interrupt
 *
 * @param[in]   irq_number      external interrupt number
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */

int plic_irq_enable(plic_irq_t irq_number);

/**
 * @brief       Disable PLIC external interrupt
 *
 * @param[in]   irq_number  The external interrupt number
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int plic_irq_disable(plic_irq_t irq_number);

/**
 * @brief       Set IRQ priority
 *
 * @param[in]   irq_number      The external interrupt number
 * @param[in]   priority        The priority of external interrupt number
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int plic_set_priority(plic_irq_t irq_number, uint32_t priority);

/**
 * @brief       Get IRQ priority
 *
 * @param[in]   irq_number          The external interrupt number
 *
 * @return      The priority of external interrupt number
 */
uint32_t plic_get_priority(plic_irq_t irq_number);

/**
 * @brief       Claim an IRQ
 *
 * @return      The current IRQ number
 */
uint32_t plic_irq_claim(void);

/**
 * @brief       Complete an IRQ
 *
 * @param[in]   source      The source IRQ number to complete
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int plic_irq_complete(uint32_t source);

/**
 * @brief       Register user callback function by IRQ number
 *
 * @param[in]   irq             The irq
 * @param[in]   callback        The callback
 * @param       ctx             The context
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
void plic_irq_register(plic_irq_t irq, plic_irq_callback_t callback, void *ctx);

/**
 * @brief       Deegister user callback function by IRQ number
 *
 * @param[in]   irq     The irq
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
void plic_irq_deregister(plic_irq_t irq);

/**
 * @brief       Deegister user callback function by IRQ number
 *
 * @param[in]   irq     The irq
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
void plic_irq_unregister(plic_irq_t irq);

/**
 * @brief       Get IRQ table, Usage:
 *              plic_instance_t (*plic_instance)[IRQN_MAX] = plic_get_instance();
 *              ... plic_instance[x][y] ...;
 *
 * @return      the point of IRQ table
 */
plic_instance_t (*plic_get_instance(void))[IRQN_MAX];

/* For c++ compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_PLIC_H */
