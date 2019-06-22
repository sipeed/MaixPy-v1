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
#include "gpio.h"
#include "utils.h"
#include "fpioa.h"
#include "sysctl.h"
#define GPIO_MAX_PINNO 8

volatile gpio_t* const gpio = (volatile gpio_t*)GPIO_BASE_ADDR;

int gpio_init(void)
{
    return sysctl_clock_enable(SYSCTL_CLOCK_GPIO);
}

void gpio_set_drive_mode(uint8_t pin, gpio_drive_mode_t mode)
{
    configASSERT(pin < GPIO_MAX_PINNO);
    int io_number = fpioa_get_io_by_function(FUNC_GPIO0 + pin);
    configASSERT(io_number >= 0);

    fpioa_pull_t pull;
    uint32_t dir;

    switch (mode)
    {
    case GPIO_DM_INPUT:
        pull = FPIOA_PULL_NONE;
        dir = 0;
        break;
    case GPIO_DM_INPUT_PULL_DOWN:
        pull = FPIOA_PULL_DOWN;
        dir = 0;
        break;
    case GPIO_DM_INPUT_PULL_UP:
        pull = FPIOA_PULL_UP;
        dir = 0;
        break;
    case GPIO_DM_OUTPUT:
        pull = FPIOA_PULL_DOWN;
        dir = 1;
        break;
    default:
        configASSERT(!"GPIO drive mode is not supported.") break;
    }

    fpioa_set_io_pull(io_number, pull);
    set_gpio_bit(gpio->direction.u32, pin, dir);
}

gpio_pin_value_t gpio_get_pin(uint8_t pin)
{
    configASSERT(pin < GPIO_MAX_PINNO);
    uint32_t dir = get_gpio_bit(gpio->direction.u32, pin);
    volatile uint32_t *reg = dir ? gpio->data_output.u32 : gpio->data_input.u32;
    return get_gpio_bit(reg, pin);
}

void gpio_set_pin(uint8_t pin, gpio_pin_value_t value)
{
    configASSERT(pin < GPIO_MAX_PINNO);
    uint32_t dir = get_gpio_bit(gpio->direction.u32, pin);
    volatile uint32_t *reg = dir ? gpio->data_output.u32 : gpio->data_input.u32;
    configASSERT(dir == 1);
    set_gpio_bit(reg, pin, value);
}

