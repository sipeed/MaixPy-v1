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
#ifndef _DRIVER_GPIOHS_H
#define _DRIVER_GPIOHS_H

#include <stdint.h>
#include "platform.h"
#include <stddef.h>
#include "gpio_common.h"
#include "plic.h"
#ifdef __cplusplus
extern "C" {
#endif

/* clang-format off */
/* Register address offsets */
#define GPIOHS_INPUT_VAL  (0x00)
#define GPIOHS_INPUT_EN   (0x04)
#define GPIOHS_OUTPUT_EN  (0x08)
#define GPIOHS_OUTPUT_VAL (0x0C)
#define GPIOHS_PULLUP_EN  (0x10)
#define GPIOHS_DRIVE      (0x14)
#define GPIOHS_RISE_IE    (0x18)
#define GPIOHS_RISE_IP    (0x1C)
#define GPIOHS_FALL_IE    (0x20)
#define GPIOHS_FALL_IP    (0x24)
#define GPIOHS_HIGH_IE    (0x28)
#define GPIOHS_HIGH_IP    (0x2C)
#define GPIOHS_LOW_IE     (0x30)
#define GPIOHS_LOW_IP     (0x34)
#define GPIOHS_IOF_EN     (0x38)
#define GPIOHS_IOF_SEL    (0x3C)
#define GPIOHS_OUTPUT_XOR (0x40)
/* clang-format on */

/**
 * @brief      GPIO bits raw object
 */
typedef struct _gpiohs_raw
{
    /* Address offset 0x00 */
    uint32_t input_val;
    /* Address offset 0x04 */
    uint32_t input_en;
    /* Address offset 0x08 */
    uint32_t output_en;
    /* Address offset 0x0c */
    uint32_t output_val;
    /* Address offset 0x10 */
    uint32_t pullup_en;
    /* Address offset 0x14 */
    uint32_t drive;
    /* Address offset 0x18 */
    uint32_t rise_ie;
    /* Address offset 0x1c */
    uint32_t rise_ip;
    /* Address offset 0x20 */
    uint32_t fall_ie;
    /* Address offset 0x24 */
    uint32_t fall_ip;
    /* Address offset 0x28 */
    uint32_t high_ie;
    /* Address offset 0x2c */
    uint32_t high_ip;
    /* Address offset 0x30 */
    uint32_t low_ie;
    /* Address offset 0x34 */
    uint32_t low_ip;
    /* Address offset 0x38 */
    uint32_t iof_en;
    /* Address offset 0x3c */
    uint32_t iof_sel;
    /* Address offset 0x40 */
    uint32_t output_xor;
} __attribute__((packed, aligned(4))) gpiohs_raw_t;

/**
 * @brief       GPIO bits object
 */
typedef struct _gpiohs_bits
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
} __attribute__((packed, aligned(4))) gpiohs_bits_t;

/**
 * @brief       GPIO bits multi access union
 */
typedef union _gpiohs_u32
{
    /* 32x1 bit mode */
    uint32_t u32[1];
    /* 16x2 bit mode */
    uint16_t u16[2];
    /* 8x4 bit mode */
    uint8_t u8[4];
    /* 1 bit mode */
    gpiohs_bits_t bits;
} __attribute__((packed, aligned(4))) gpiohs_u32_t;

/**
 * @brief      GPIO object
 *
 *             The GPIO controller is a peripheral device mapped in the
 *             internal memory map, discoverable in the Configuration String.
 *             It is responsible for low-level configuration of the actual
 *             GPIO pads on the device (direction, pull up-enable, and drive
 *             value), as well as selecting between various sources of the
 *             controls for these signals. The GPIO controller allows seperate
 *             configuration of each of N GPIO bits.
 *
 *             Once the interrupt is pending, it will remain set until a 1 is
 *             written to the *_ip register at that bit.
 */

typedef struct _gpiohs
{
    /* Address offset 0x00, Input Values */
    gpiohs_u32_t input_val;
    /* Address offset 0x04, Input enable */
    gpiohs_u32_t input_en;
    /* Address offset 0x08, Output enable */
    gpiohs_u32_t output_en;
    /* Address offset 0x0c, Onput Values */
    gpiohs_u32_t output_val;
    /* Address offset 0x10, Internal Pull-Ups enable */
    gpiohs_u32_t pullup_en;
    /* Address offset 0x14, Drive Strength */
    gpiohs_u32_t drive;
    /* Address offset 0x18, Rise interrupt enable */
    gpiohs_u32_t rise_ie;
    /* Address offset 0x1c, Rise interrupt pending */
    gpiohs_u32_t rise_ip;
    /* Address offset 0x20, Fall interrupt enable */
    gpiohs_u32_t fall_ie;
    /* Address offset 0x24, Fall interrupt pending */
    gpiohs_u32_t fall_ip;
    /* Address offset 0x28, High interrupt enable */
    gpiohs_u32_t high_ie;
    /* Address offset 0x2c, High interrupt pending */
    gpiohs_u32_t high_ip;
    /* Address offset 0x30, Low interrupt enable */
    gpiohs_u32_t low_ie;
    /* Address offset 0x34, Low interrupt pending */
    gpiohs_u32_t low_ip;
    /* Address offset 0x38, HW I/O Function enable */
    gpiohs_u32_t iof_en;
    /* Address offset 0x3c, HW I/O Function select */
    gpiohs_u32_t iof_sel;
    /* Address offset 0x40, Output XOR (invert) */
    gpiohs_u32_t output_xor;
} __attribute__((packed, aligned(4))) gpiohs_t;

/**
 * @brief       GPIO High-speed object instanse
 */
extern volatile gpiohs_t *const gpiohs;

/**
 * @brief       Set Gpiohs drive mode
 *
 * @param[in]   pin         Gpiohs pin
 * @param[in]   mode        Gpiohs pin drive mode
 */
void gpiohs_set_drive_mode(uint8_t pin, gpio_drive_mode_t mode);

/**
 * @brief       Get Gpiohs pin value
 *
 * @param[in]   pin     Gpiohs pin
 * @return      Pin     value
 *
 *     - GPIO_PV_Low     Gpiohs pin low
 *     - GPIO_PV_High    Gpiohs pin high
 */
gpio_pin_value_t gpiohs_get_pin(uint8_t pin);

/**
 * @brief      Set Gpiohs pin value
 *
 * @param[in]   pin      Gpiohs pin
 * @param[in]   value    Gpiohs pin value
 */
void gpiohs_set_pin(uint8_t pin, gpio_pin_value_t value);

/**
 * @brief      Set Gpiohs pin edge for interrupt
 *
 * @param[in]   pin         Gpiohs pin
 * @param[in]   edge        Gpiohs pin edge type
 */
void gpiohs_set_pin_edge(uint8_t pin, gpio_pin_edge_t edge);

/**
 * @brief      Set Gpiohs pin interrupt
 *
 * @param[in]   pin             Gpiohs pin
 * @param[in]   priority        Gpiohs pin interrupt priority
 * @param[in]   func            Gpiohs pin interrupt service routine
 */
void gpiohs_set_irq(uint8_t pin, uint32_t priority, void(*func)());

/**
 * @brief      Set Gpiohs pin interrupt
 *
 * @param[in]   pin             Gpiohs pin
 * @param[in]   priority        Gpiohs pin interrupt priority
 * @param[in]   callback        Gpiohs pin interrupt service routine
 * @param[in]   ctx             Gpiohs interrupt param
 */
void gpiohs_irq_register(uint8_t pin, uint32_t priority, plic_irq_callback_t callback, void *ctx);

/**
 * @brief      Unregister Gpiohs pin interrupt
 *
 * @param[in]   pin             Gpiohs pin
 */
void gpiohs_irq_unregister(uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_GPIOHS_H */

