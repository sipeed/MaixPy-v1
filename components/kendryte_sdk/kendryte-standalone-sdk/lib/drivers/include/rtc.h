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
 * @brief      A real-time clock (RTC) is a computer clock that keeps track of
 *             the current time.
 */

#ifndef _DRIVER_RTC_H
#define _DRIVER_RTC_H

#include <plic.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      RTC timer mode
 *
 *             Timer mode selector
 *             | Mode | Description            |
 *             |------|------------------------|
 *             | 0    | Timer pause            |
 *             | 1    | Timer time running     |
 *             | 2    | Timer time setting     |
 */
typedef enum _rtc_timer_mode_e
{
    RTC_TIMER_PAUSE,   /*!< 0: Timer pause */
    RTC_TIMER_RUNNING, /*!< 1: Timer time running */
    RTC_TIMER_SETTING, /*!< 2: Timer time setting */
    RTC_TIMER_MAX      /*!< Max count of this enum*/
} rtc_timer_mode_t;

/*
 * @brief      RTC tick interrupt mode
 *
 *             Tick interrupt mode selector
 *             | Mode | Description            |
 *             |------|------------------------|
 *             | 0    | Interrupt every second |
 *             | 1    | Interrupt every minute |
 *             | 2    | Interrupt every hour   |
 *             | 3    | Interrupt every day    |
 */
typedef enum _rtc_tick_interrupt_mode_e
{
    RTC_INT_SECOND, /*!< 0: Interrupt every second */
    RTC_INT_MINUTE, /*!< 1: Interrupt every minute */
    RTC_INT_HOUR,   /*!< 2: Interrupt every hour */
    RTC_INT_DAY,    /*!< 3: Interrupt every day */
    RTC_INT_MAX     /*!< Max count of this enum*/
} rtc_tick_interrupt_mode_t;

/**
 * @brief      RTC mask structure
 *
 *             RTC mask structure for common use
 */
typedef struct _rtc_mask
{
    uint32_t resv : 1;   /*!< Reserved */
    uint32_t second : 1; /*!< Second mask */
    uint32_t minute : 1; /*!< Minute mask */
    uint32_t hour : 1;   /*!< Hour mask */
    uint32_t week : 1;   /*!< Week mask */
    uint32_t day : 1;    /*!< Day mask */
    uint32_t month : 1;  /*!< Month mask */
    uint32_t year : 1;   /*!< Year mask */
} __attribute__((packed, aligned(1))) rtc_mask_t;

/**
 * @brief       RTC register
 *
 * @note        RTC register table
 *
 * | Offset    | Name           | Description                         |
 * |-----------|----------------|-------------------------------------|
 * | 0x00      | date           | Timer date information              |
 * | 0x04      | time           | Timer time information              |
 * | 0x08      | alarm_date     | Alarm date information              |
 * | 0x0c      | alarm_time     | Alarm time information              |
 * | 0x10      | initial_count  | Timer counter initial value         |
 * | 0x14      | current_count  | Timer counter current value         |
 * | 0x18      | interrupt_ctrl | RTC interrupt settings              |
 * | 0x1c      | register_ctrl  | RTC register settings               |
 * | 0x20      | reserved0      | Reserved                            |
 * | 0x24      | reserved1      | Reserved                            |
 * | 0x28      | extended       | Timer extended information          |
 *
 */

/**
 * @brief       Timer date information
 *
 *              No. 0 Register (0x00)
 */
typedef struct _rtc_date
{
    uint32_t week : 3;  /*!< Week. Range [0,6]. 0 is Sunday. */
    uint32_t resv0 : 5; /*!< Reserved */
    uint32_t day : 5;   /*!< Day. Range [1,31] or [1,30] or [1,29] or [1,28] */
    uint32_t resv1 : 3; /*!< Reserved */
    uint32_t month : 4; /*!< Month. Range [1,12] */
    uint32_t year : 12; /*!< Year. Range [0,99] */
} __attribute__((packed, aligned(4))) rtc_date_t;

/**
 * @brief       Timer time information
 *
 *              No. 1 Register (0x04)
 */
typedef struct _rtc_time
{
    uint32_t resv0 : 10; /*!< Reserved */
    uint32_t second : 6; /*!< Second. Range [0,59] */
    uint32_t minute : 6; /*!< Minute. Range [0,59] */
    uint32_t resv1 : 2;  /*!< Reserved */
    uint32_t hour : 5;   /*!< Hour. Range [0,23] */
    uint32_t resv2 : 3;  /*!< Reserved */
} __attribute__((packed, aligned(4))) rtc_time_t;

typedef struct _rtc_date_time
{
    uint32_t sec : 6;
    uint32_t min : 6;
    uint32_t hour : 5;
    uint32_t week : 3;
    uint32_t day : 5;
    uint32_t month : 4;
    uint16_t year;
} rtc_date_time_t;

/**
 * @brief       Alarm date information
 *
 *              No. 2 Register (0x08)
 */
typedef struct _rtc_alarm_date
{
    uint32_t week : 3;  /*!< Alarm Week. Range [0,6]. 0 is Sunday. */
    uint32_t resv0 : 5; /*!< Reserved */
    uint32_t day : 5;   /*!< Alarm Day. Range [1,31] or [1,30] or [1,29] or [1,28] */
    uint32_t resv1 : 3; /*!< Reserved */
    uint32_t month : 4; /*!< Alarm Month. Range [1,12] */
    uint32_t year : 12; /*!< Alarm Year. Range [0,99] */
} __attribute__((packed, aligned(4))) rtc_alarm_date_t;

/**
 * @brief       Alarm time information
 *
 *              No. 3 Register (0x0c)
 */
typedef struct _rtc_alarm_time
{
    uint32_t resv0 : 10; /*!< Reserved */
    uint32_t second : 6; /*!< Alarm Second. Range [0,59] */
    uint32_t minute : 6; /*!< Alarm Minute. Range [0,59] */
    uint32_t resv1 : 2;  /*!< Reserved */
    uint32_t hour : 5;   /*!< Alarm Hour. Range [0,23] */
    uint32_t resv2 : 3;  /*!< Reserved */
} __attribute__((packed, aligned(4))) rtc_alarm_time_t;

/**
 * @brief       Timer counter initial value
 *
 *              No. 4 Register (0x10)
 */
typedef struct _rtc_initial_count
{
    uint32_t count : 32; /*!< RTC counter initial value */
} __attribute__((packed, aligned(4))) rtc_initial_count_t;

/**
 * @brief       Timer counter current value
 *
 *              No. 5 Register (0x14)
 */
typedef struct _rtc_current_count
{
    uint32_t count : 32; /*!< RTC counter current value */
} __attribute__((packed, aligned(4))) rtc_current_count_t;

/**
 * @brief      RTC interrupt settings
 *
 *             No. 6 Register (0x18)
 */
typedef struct _rtc_interrupt_ctrl
{
    uint32_t tick_enable : 1;        /*!< Reserved */
    uint32_t alarm_enable : 1;       /*!< Alarm interrupt enable */
    uint32_t tick_int_mode : 2;      /*!< Tick interrupt enable */
    uint32_t resv : 20;              /*!< Reserved */
    uint32_t alarm_compare_mask : 8; /*!< Alarm compare mask for interrupt */
} __attribute__((packed, aligned(4))) rtc_interrupt_ctrl_t;

/**
 * @brief       RTC register settings
 *
 *              No. 7 Register (0x1c)
 */
typedef struct _rtc_register_ctrl
{
    uint32_t read_enable : 1;             /*!< RTC timer read enable */
    uint32_t write_enable : 1;            /*!< RTC timer write enable */
    uint32_t resv0 : 11;                  /*!< Reserved */
    uint32_t timer_mask : 8;              /*!< RTC timer mask */
    uint32_t alarm_mask : 8;              /*!< RTC alarm mask */
    uint32_t initial_count_mask : 1;      /*!< RTC counter initial count value mask */
    uint32_t interrupt_register_mask : 1; /*!< RTC interrupt register mask */
    uint32_t resv1 : 1;                   /*!< Reserved */
} __attribute__((packed, aligned(4))) rtc_register_ctrl_t;

/**
 * @brief       Reserved
 *
 *              No. 8 Register (0x20)
 */
typedef struct _rtc_reserved0
{
    uint32_t resv : 32; /*!< Reserved */
} __attribute__((packed, aligned(4))) rtc_reserved0_t;

/**
 * @brief      Reserved
 *
 *             No. 9 Register (0x24)
 */
typedef struct _rtc_reserved1
{
    uint32_t resv : 32; /*!< Reserved */
} __attribute__((packed, aligned(4))) rtc_reserved1_t;

/**
 * @brief      Timer extended information
 *
 *             No. 10 Register (0x28)
 */
typedef struct _rtc_extended
{
    uint32_t century : 5;   /*!< Century. Range [0,31] */
    uint32_t leap_year : 1; /*!< Is leap year. 1 is leap year, 0 is not leap year */
    uint32_t resv : 26;     /*!< Reserved */
    ;
} __attribute__((packed, aligned(4))) rtc_extended_t;

/**
 * @brief       Real-time clock struct
 *
 *              A real-time clock (RTC) is a computer clock that keeps track of
 *              the current time.
 */
typedef struct _rtc
{
    rtc_date_t date;                     /*!< No. 0 (0x00): Timer date information */
    rtc_time_t time;                     /*!< No. 1 (0x04): Timer time information */
    rtc_alarm_date_t alarm_date;         /*!< No. 2 (0x08): Alarm date information */
    rtc_alarm_time_t alarm_time;         /*!< No. 3 (0x0c): Alarm time information */
    rtc_initial_count_t initial_count;   /*!< No. 4 (0x10): Timer counter initial value */
    rtc_current_count_t current_count;   /*!< No. 5 (0x14): Timer counter current value */
    rtc_interrupt_ctrl_t interrupt_ctrl; /*!< No. 6 (0x18): RTC interrupt settings */
    rtc_register_ctrl_t register_ctrl;   /*!< No. 7 (0x1c): RTC register settings */
    rtc_reserved0_t reserved0;           /*!< No. 8 (0x20): Reserved */
    rtc_reserved1_t reserved1;           /*!< No. 9 (0x24): Reserved */
    rtc_extended_t extended;             /*!< No. 10 (0x28): Timer extended information */
} __attribute__((packed, aligned(4))) rtc_t;

/**
 * @brief       Real-time clock object
 */
extern volatile rtc_t *const rtc;
extern volatile uint32_t *const rtc_base;

/**
 * @brief      Set RTC timer mode
 *
 * @param[in]  timer_mode  The timer mode
 *
 * @return     Result
 *             - 0     Success
 *             - Other Fail
 */
int rtc_timer_set_mode(rtc_timer_mode_t timer_mode);

/**
 * @brief      Get RTC timer mode
 *
 * @return     The timer mode
 */
rtc_timer_mode_t rtc_timer_get_mode(void);

/**
 * @brief      Set date time to RTC
 *
 * @param[in]  tm    The Broken-down date time
 *
 * @return     Result
 *             - 0     Success
 *             - Other Fail
 */
int rtc_timer_set_tm(const struct tm *tm);

/**
 * @brief      Get date time from RTC
 *
 * @return     The Broken-down date time
 */
struct tm *rtc_timer_get_tm(void);

/**
 * @brief      Set date time to Alarm
 *
 * @param[in]  tm    The Broken-down date time
 *
 * @return     Result
 *             - 0     Success
 *             - Other Fail
 */
int rtc_timer_set_alarm_tm(const struct tm *tm);

/**
 * @brief      Get date time from Alarm
 *
 * @return     The Broken-down date time
 */
struct tm *rtc_timer_get_alarm_tm(void);

/**
 * @brief      Check if it is a leap year
 *
 * @param[in]  year  The year
 *
 * @return     Result
 *             - 0     Not leap year
 *             - Other Leap year
 */
int rtc_year_is_leap(int year);

/**
 * @brief      Get day of year from date
 *
 * @param[in]  year   The year
 * @param[in]  month  The month
 * @param[in]  day    The day
 *
 * @return     The day of year from date
 */
int rtc_get_yday(int year, int month, int day);

/**
 * @brief      Get the day of the week from date
 *
 * @param[in]  year   The year
 * @param[in]  month  The month
 * @param[in]  day    The day
 *
 * @return     The day of the week.
 *             Where Sunday = 0, Monday = 1, Tuesday = 2, Wednesday = 3,
 *             Thursday = 4, Friday = 5, Saturday = 6.
 */
int rtc_get_wday(int year, int month, int day);

/**
 * @brief      Set date time to RTC
 *
 * @param[in]  year    The year
 * @param[in]  month   The month
 * @param[in]  day     The day
 * @param[in]  hour    The hour
 * @param[in]  minute  The minute
 * @param[in]  second  The second
 *
 * @return     Result
 *             - 0     Success
 *             - Other Fail
 */
int rtc_timer_set(int year, int month, int day, int hour, int minute, int second);

/**
 * @brief      Get date time from RTC
 *
 * @param      year    The year
 * @param      month   The month
 * @param      day     The day
 * @param      hour    The hour
 * @param      minute  The minute
 * @param      second  The second
 *
 * @return     Result
 *             - 0     Success
 *             - Other Fail
 */
int rtc_timer_get(int *year, int *month, int *day, int *hour, int *minute, int *second);

/**
 * @brief      Set date time to Alarm
 *
 * @param[in]  year    The year
 * @param[in]  month   The month
 * @param[in]  day     The day
 * @param[in]  hour    The hour
 * @param[in]  minute  The minute
 * @param[in]  second  The second
 *
 * @return     Result
 *             - 0     Success
 *             - Other Fail
 */
int rtc_alarm_set(int year, int month, int day, int hour, int minute, int second);

/**
 * @brief      Get date time from Alarm
 *
 * @param      year    The year
 * @param      month   The month
 * @param      day     The day
 * @param      hour    The hour
 * @param      minute  The minute
 * @param      second  The second
 *
 * @return     Result
 *             - 0     Success
 *             - Other Fail
 */
int rtc_alarm_get(int *year, int *month, int *day, int *hour, int *minute, int *second);

/**
 * @brief     Get RTC timer clock count value
 *
 * @return    unsigned int, the value of counter
 */
unsigned int rtc_timer_get_clock_count_value(void);

/**
 * @brief     Enable or disable RTC tick interrupt
 *
 * @param     enable    Enable or disable RTC tick interrupt
 * @return    Result
 *            - 0     Success
 *            - Other Fail
 */
int rtc_tick_interrupt_set(int enable);

/**
 * @brief     Get the enable status of RTC tick interrupt
 *
 * @return    The enable status of RTC tick interrupt
 */
int rtc_tick_interrupt_get(void);

/**
 * @brief     Set the interrupt mode of RTC tick interrupt
 *
 * @param     mode    The mode of RTC tick interrupt
 *                    - RTC_INT_SECOND,  0: Interrupt every second
 *                    - RTC_INT_MINUTE,  1: Interrupt every minute
 *                    - RTC_INT_HOUR,    2: Interrupt every hour
 *                    - RTC_INT_DAY,     3: Interrupt every day
 * @return    Result
 *            - 0     Success
 *            - Other Fail
 */
int rtc_tick_set_interrupt_mode(rtc_tick_interrupt_mode_t mode);

/**
 * @brief     Get the interrupt mode of RTC tick interrupt
 *
 * @return    mode    The mode of RTC tick interrupt
 *                    - RTC_INT_SECOND,  0: Interrupt every second
 *                    - RTC_INT_MINUTE,  1: Interrupt every minute
 *                    - RTC_INT_HOUR,    2: Interrupt every hour
 *                    - RTC_INT_DAY,     3: Interrupt every day
 */
rtc_tick_interrupt_mode_t rtc_tick_get_interrupt_mode(void);

/**
 * @brief     Enable or disable RTC alarm interrupt
 *
 * @param     enable    Enable or disable RTC alarm interrupt
 * @return    Result
 *            - 0     Success
 *            - Other Fail
 */
int rtc_alarm_set_interrupt(int enable);

/**
 * @brief     Get the enable status of RTC alarm interrupt
 *
 * @return    The enable status of RTC alarm interrupt
 */
int rtc_alarm_get_interrupt(void);

/**
 * @brief     Set the compare mask for RTC alarm interrupt
 *
 * @param       mask    The alarm compare mask for RTC alarm interrupt
 *                      (rtc_mask_t) {
 *                          .second = 1, Set this mask to compare Second
 *                          .minute = 0, Set this mask to compare Minute
 *                          .hour = 0,   Set this mask to compare Hour
 *                          .week = 0,   Set this mask to compare Week
 *                          .day = 0,    Set this mask to compare Day
 *                          .month = 0,  Set this mask to compare Month
 *                          .year = 0,   Set this mask to compare Year
 *                      }
 * @return    Result
 *            - 0     Success
 *            - Other Fail
 */
int rtc_alarm_set_mask(rtc_mask_t mask);

/**
 * @brief    Get the compare mask for RTC alarm interrupt
 *
 * @return   mask    The alarm compare mask for RTC alarm interrupt
 *                   (rtc_mask_t) {
 *                       .second = 1, Set this mask to compare Second
 *                       .minute = 0, Set this mask to compare Minute
 *                       .hour = 0,   Set this mask to compare Hour
 *                       .week = 0,   Set this mask to compare Week
 *                       .day = 0,    Set this mask to compare Day
 *                       .month = 0,  Set this mask to compare Month
 *                       .year = 0,   Set this mask to compare Year
 *                   }
 */
rtc_mask_t rtc_alarm_get_mask(void);

/**
 * @brief       Initialize RTC
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
int rtc_init(void);

/**
 * @brief       Register callback of tick interrupt
 *
 * @param       is_single_shot          Indicates if single shot
 * @param       mode                    Tick interrupt mode
                                        0:second
                                        1:minute
                                        2:hour
                                        3:day
 * @param       callback                Callback of tick interrupt
 * @param       ctx                     Param of callback
 * @param       priority                Priority of tick interrupt
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int rtc_tick_irq_register(bool is_single_shot, rtc_tick_interrupt_mode_t mode, plic_irq_callback_t callback, void *ctx, uint8_t priority);

/**
 * @brief       Unregister tick interrupt
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
void rtc_tick_irq_unregister(void);

/**
 * @brief       Register callback of alarm interrupt
 *
 * @param       is_single_shot          Indicates if single shot
 * @param       mask                    The alarm compare mask for RTC alarm interrupt
 *                                      (rtc_mask_t) {
 *                                          .second = 1, Set this mask to compare Second
 *                                          .minute = 0, Set this mask to compare Minute
 *                                          .hour = 0,   Set this mask to compare Hour
 *                                          .week = 0,   Set this mask to compare Week
 *                                          .day = 0,    Set this mask to compare Day
 *                                          .month = 0,  Set this mask to compare Month
 *                                          .year = 0,   Set this mask to compare Year
 *                                      }
 * @param       callback                Callback of tick interrupt
 * @param       ctx                     Param of callback
 * @param       priority                Priority of tick interrupt
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int rtc_alarm_irq_register(bool is_single_shot, rtc_mask_t mask, plic_irq_callback_t callback, void *ctx, uint8_t priority);

/**
 * @brief       Unregister alarm interrupt
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
void rtc_alarm_irq_unregister(void);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_RTC_H */

