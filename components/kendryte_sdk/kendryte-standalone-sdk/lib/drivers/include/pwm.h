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
#ifndef _DRIVER_PWM_H
#define _DRIVER_PWM_H

#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum _pwm_device_number
{
    PWM_DEVICE_0,
    PWM_DEVICE_1,
    PWM_DEVICE_2,
    PWM_DEVICE_MAX,
} pwm_device_number_t;

typedef enum _pwm_channel_number
{
    PWM_CHANNEL_0,
    PWM_CHANNEL_1,
    PWM_CHANNEL_2,
    PWM_CHANNEL_3,
    PWM_CHANNEL_MAX,
} pwm_channel_number_t;

/**
 * @brief       Init pwm timer
 *
 * @param[in]   timer       timer
 */
void pwm_init(pwm_device_number_t pwm_number);

/**
 * @brief       Enable timer
 *
 * @param[in]   timer       timer
 * @param[in]   channel     channel
 * @param[in]   enable      Enable or disable
 *
 */
void pwm_set_enable(pwm_device_number_t pwm_number, pwm_channel_number_t channel, int enable);

/**
 * @brief       Set pwm duty
 *
 * @param[in]   timer           timer
 * @param[in]   channel         channel
 * @param[in]   frequency       pwm frequency
 * @param[in]   duty            duty
 *
 */
double pwm_set_frequency(pwm_device_number_t pwm_number, pwm_channel_number_t channel, double frequency, double duty);


#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_PWM_H */
