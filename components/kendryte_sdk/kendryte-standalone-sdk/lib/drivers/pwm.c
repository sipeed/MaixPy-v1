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
#include "timer.h"
#include "pwm.h"
#include "sysctl.h"
#include <stddef.h>
#include "utils.h"
#include "plic.h"
#include "io.h"

void pwm_init(pwm_device_number_t pwm_number)
{
    sysctl_clock_enable(SYSCTL_CLOCK_TIMER0 + pwm_number);
}

void pwm_set_enable(pwm_device_number_t pwm_number, pwm_channel_number_t channel, int enable)
{
    if (enable)
    {
        if (timer[pwm_number]->channel[channel].load_count == 0)
            timer[pwm_number]->channel[channel].load_count = 1;
        if (timer[pwm_number]->load_count2[channel] == 0)
            timer[pwm_number]->load_count2[channel] = 1;
        timer[pwm_number]->channel[channel].control = TIMER_CR_INTERRUPT_MASK | TIMER_CR_PWM_ENABLE | TIMER_CR_USER_MODE | TIMER_CR_ENABLE;
    }
    else
    {
        timer[pwm_number]->channel[channel].control = TIMER_CR_INTERRUPT_MASK;
    }
}

double pwm_set_frequency(pwm_device_number_t pwm_number, pwm_channel_number_t channel, double frequency, double duty)
{
    uint32_t clk_freq = sysctl_clock_get_freq(SYSCTL_CLOCK_TIMER0 + pwm_number);

    int32_t periods = (int32_t)(clk_freq / frequency);
    configASSERT(periods > 0 && periods <= INT32_MAX);
    frequency = clk_freq / (double)periods;

    uint32_t percent = (uint32_t)(duty * periods);
    timer[pwm_number]->channel[channel].load_count = periods - percent;
    timer[pwm_number]->load_count2[channel] = percent;

    return frequency;
}

