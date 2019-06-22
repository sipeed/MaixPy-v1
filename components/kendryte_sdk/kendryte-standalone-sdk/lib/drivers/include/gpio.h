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
#ifndef _DRIVER_GPIO_H
#define _DRIVER_GPIO_H

#include "platform.h"
#include <inttypes.h>
#include <stddef.h>
#include "gpio_common.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief       Structure for accessing GPIO registers by individual bit
 */
typedef struct _gpio_bits
{
    uint32_t b0 : 1;
    uint32_t b1 : 1;
    uint32_t b2 : 1;
    uint32_t b3 : 1;
    uint32_t b4 : 1;
    uint32_t b5 : 1;
    uint32_t b6 : 1;
    uint32_t b7 : 1;
    uint32_t b8 : 1;
    uint32_t b9 : 1;
    uint32_t b10 : 1;
    uint32_t b11 : 1;
    uint32_t b12 : 1;
    uint32_t b13 : 1;
    uint32_t b14 : 1;
    uint32_t b15 : 1;
    uint32_t b16 : 1;
    uint32_t b17 : 1;
    uint32_t b18 : 1;
    uint32_t b19 : 1;
    uint32_t b20 : 1;
    uint32_t b21 : 1;
    uint32_t b22 : 1;
    uint32_t b23 : 1;
    uint32_t b24 : 1;
    uint32_t b25 : 1;
    uint32_t b26 : 1;
    uint32_t b27 : 1;
    uint32_t b28 : 1;
    uint32_t b29 : 1;
    uint32_t b30 : 1;
    uint32_t b31 : 1;
} __attribute__((packed, aligned(4))) gpio_bits_t;

/**
 * @brief       Structure of templates for accessing GPIO registers
 */
typedef union _gpio_access_tp
{
    /* 32x1 bit mode */
    uint32_t u32[1];
    /* 16x2 bit mode */
    uint16_t u16[2];
    /* 8x4 bit mode */
    uint8_t u8[4];
    /* 1 bit mode */
    gpio_bits_t bits;
} __attribute__((packed, aligned(4))) gpio_access_tp_t;

/**
 * @brief       The GPIO address map
 */
typedef struct _gpio
{
    /* Offset 0x00: Data (output) registers */
    gpio_access_tp_t data_output;
    /* Offset 0x04: Data direction registers */
    gpio_access_tp_t direction;
    /* Offset 0x08: Data source registers */
    gpio_access_tp_t source;
    /* Offset 0x10 - 0x2f: Unused registers, 9x4 bytes */
    uint32_t unused_0[9];
    /* Offset 0x30: Interrupt enable/disable registers */
    gpio_access_tp_t interrupt_enable;
    /* Offset 0x34: Interrupt mask registers */
    gpio_access_tp_t interrupt_mask;
    /* Offset 0x38: Interrupt level registers */
    gpio_access_tp_t interrupt_level;
    /* Offset 0x3c: Interrupt polarity registers */
    gpio_access_tp_t interrupt_polarity;
    /* Offset 0x40: Interrupt status registers */
    gpio_access_tp_t interrupt_status;
    /* Offset 0x44: Raw interrupt status registers */
    gpio_access_tp_t interrupt_status_raw;
    /* Offset 0x48: Interrupt debounce registers */
    gpio_access_tp_t interrupt_debounce;
    /* Offset 0x4c: Registers for clearing interrupts */
    gpio_access_tp_t interrupt_clear;
    /* Offset 0x50: External port (data input) registers */
    gpio_access_tp_t data_input;
    /* Offset 0x54 - 0x5f: Unused registers, 3x4 bytes */
    uint32_t unused_1[3];
    /* Offset 0x60: Sync level registers */
    gpio_access_tp_t sync_level;
    /* Offset 0x64: ID code */
    gpio_access_tp_t id_code;
    /* Offset 0x68: Interrupt both edge type */
    gpio_access_tp_t interrupt_bothedge;

} __attribute__((packed, aligned(4))) gpio_t;

/**
 * @brief       Bus GPIO object instance
 */
extern volatile gpio_t *const gpio;

/**
 * @brief       Gpio initialize
 *
 * @return      Result
 *     - 0      Success
 *     - Other Fail
 */
int gpio_init(void);

/**
 * @brief       Set Gpio drive mode
 *
 * @param[in]   pin         Gpio pin
 * @param[in]   mode        Gpio pin drive mode
 */
void gpio_set_drive_mode(uint8_t pin, gpio_drive_mode_t mode);

/**
 * @brief       Get Gpio pin value
 *
 * @param[in]   pin      Gpio pin
 * @return      Pin value
 *
 *     - GPIO_PV_Low     Gpio pin low
 *     - GPIO_PV_High    Gpio pin high
 */
gpio_pin_value_t gpio_get_pin(uint8_t pin);

/**
 * @brief       Set Gpio pin value
 *
 * @param[in]   pin         Gpio pin
 * @param[in]   value       Gpio pin value
 */
void gpio_set_pin(uint8_t pin, gpio_pin_value_t value);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_GPIO_H */

