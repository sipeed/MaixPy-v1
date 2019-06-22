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
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "encoding.h"
#include "printf.h"
#include "rtc.h"
#include "sysctl.h"

volatile rtc_t *const rtc = (volatile rtc_t *)RTC_BASE_ADDR;

struct tm rtc_timer_date_time;

struct tm rtc_alarm_date_time;

typedef struct _rtc_instance_t
{
    plic_irq_callback_t rtc_tick_callback;
    void *tick_ctx;
    plic_irq_callback_t rtc_alarm_callback;
    void *alarm_ctx;
    bool tick_is_single_shot;
    bool alarm_is_single_shot;
    bool tick_enable_by_alarm;
    bool tick_enable_by_user;
    rtc_tick_interrupt_mode_t tick_mode_by_user;
    rtc_tick_interrupt_mode_t tick_mode_by_alarm;
} rtc_instance_t;

rtc_instance_t rtc_instance = (rtc_instance_t){
    .tick_mode_by_user = -1,
    .tick_mode_by_alarm = RTC_INT_MAX,
};

int rtc_timer_set_mode(rtc_timer_mode_t timer_mode)
{
    rtc_register_ctrl_t register_ctrl = rtc->register_ctrl;

    switch(timer_mode)
    {
        case RTC_TIMER_PAUSE:
            register_ctrl.read_enable = 0;
            register_ctrl.write_enable = 0;
            break;
        case RTC_TIMER_RUNNING:
            register_ctrl.read_enable = 1;
            register_ctrl.write_enable = 0;
            break;
        case RTC_TIMER_SETTING:
            register_ctrl.read_enable = 0;
            register_ctrl.write_enable = 1;
            break;
        default:
            register_ctrl.read_enable = 0;
            register_ctrl.write_enable = 0;
            break;
    }

    /* Get CPU current freq */
    unsigned long freq = sysctl_clock_get_freq(SYSCTL_CLOCK_CPU);
    /* Set threshold to 1/26000000 s */
    freq = freq / 26000000;
    /* Get current CPU cycle */
    unsigned long start_cycle = read_csr(mcycle);
    /* Wait for 1/26000000 s to sync data */
    while(read_csr(mcycle) - start_cycle < freq)
        continue;

    rtc->register_ctrl = register_ctrl;

    return 0;
}

rtc_timer_mode_t rtc_timer_get_mode(void)
{
    rtc_register_ctrl_t register_ctrl = rtc->register_ctrl;
    rtc_timer_mode_t timer_mode = RTC_TIMER_PAUSE;

    if((!register_ctrl.read_enable) && (!register_ctrl.write_enable))
    {
        /* RTC_TIMER_PAUSE */
        timer_mode = RTC_TIMER_PAUSE;
    } else if((register_ctrl.read_enable) && (!register_ctrl.write_enable))
    {
        /* RTC_TIMER_RUNNING */
        timer_mode = RTC_TIMER_RUNNING;
    } else if((!register_ctrl.read_enable) && (register_ctrl.write_enable))
    {
        /* RTC_TIMER_SETTING */
        timer_mode = RTC_TIMER_RUNNING;
    } else
    {
        /* Something is error, reset timer mode */
        rtc_timer_set_mode(timer_mode);
    }

    return timer_mode;
}

static inline int rtc_in_range(int value, int min, int max)
{
    return ((value >= min) && (value <= max));
}

int rtc_timer_set_tm(const struct tm *tm)
{
    rtc_date_t timer_date;
    rtc_time_t timer_time;
    rtc_extended_t timer_extended;

    if(tm)
    {
        /*
         * Range of tm->tm_sec could be [0,61]
         *
         * Range of tm->tm_sec allows for a positive leap second. Two
         * leap seconds in the same minute are not allowed (the C90
         * range 0..61 was a defect)
         */
        if(rtc_in_range(tm->tm_sec, 0, 59))
            timer_time.second = tm->tm_sec;
        else
            return -1;

        /* Range of tm->tm_min could be [0,59] */
        if(rtc_in_range(tm->tm_min, 0, 59))
            timer_time.minute = tm->tm_min;
        else
            return -1;

        /* Range of tm->tm_hour could be [0, 23] */
        if(rtc_in_range(tm->tm_hour, 0, 23))
            timer_time.hour = tm->tm_hour;
        else
            return -1;

        /* Range of tm->tm_mday could be [1, 31] */
        if(rtc_in_range(tm->tm_mday, 1, 31))
            timer_date.day = tm->tm_mday;
        else
            return -1;

        /*
         * Range of tm->tm_mon could be [0, 11]
         * But in this RTC, date.month should be [1, 12]
         */
        if(rtc_in_range(tm->tm_mon, 0, 11))
            timer_date.month = tm->tm_mon + 1;
        else
            return -1;

        /*
         * Range of tm->tm_year is the years since 1900
         * But in this RTC, year is split into year and century
         * In this RTC, century range is [0,31], year range is [0,99]
         */
        int human_year = tm->tm_year + 1900;
        int rtc_year = human_year % 100;
        int rtc_century = human_year / 100;

        if(rtc_in_range(rtc_year, 0, 99) &&
           rtc_in_range(rtc_century, 0, 31))
        {
            timer_date.year = rtc_year;
            timer_extended.century = rtc_century;
        } else
            return -1;

        /* Range of tm->tm_wday could be [0, 6] */
        if(rtc_in_range(tm->tm_wday, 0, 6))
            timer_date.week = tm->tm_wday;
        else
            return -1;

        /* Set RTC mode to timer setting mode */
        rtc_timer_set_mode(RTC_TIMER_SETTING);
        /* Write value to RTC */
        rtc->date = timer_date;
        rtc->time = timer_time;
        rtc->extended = timer_extended;
        /* Set RTC mode to timer running mode */
        rtc_timer_set_mode(RTC_TIMER_RUNNING);
    }

    return 0;
}

int rtc_alarm_set_tm(const struct tm *tm)
{
    rtc_alarm_date_t alarm_date;
    rtc_alarm_time_t alarm_time;

    if(tm)
    {
        /*
         * Range of tm->tm_sec could be [0,61]
         *
         * Range of tm->tm_sec allows for a positive leap second. Two
         * leap seconds in the same minute are not allowed (the C90
         * range 0..61 was a defect)
         */
        if(rtc_in_range(tm->tm_sec, 0, 59))
            alarm_time.second = tm->tm_sec;
        else
            return -1;

        /* Range of tm->tm_min could be [0,59] */
        if(rtc_in_range(tm->tm_min, 0, 59))
            alarm_time.minute = tm->tm_min;
        else
            return -1;

        /* Range of tm->tm_hour could be [0, 23] */
        if(rtc_in_range(tm->tm_hour, 0, 23))
            alarm_time.hour = tm->tm_hour;
        else
            return -1;

        /* Range of tm->tm_mday could be [1, 31] */
        if(rtc_in_range(tm->tm_mday, 1, 31))
            alarm_date.day = tm->tm_mday;
        else
            return -1;

        /*
         * Range of tm->tm_mon could be [0, 11]
         * But in this RTC, date.month should be [1, 12]
         */
        if(rtc_in_range(tm->tm_mon, 0, 11))
            alarm_date.month = tm->tm_mon + 1;
        else
            return -1;

        /*
         * Range of tm->tm_year is the years since 1900
         * But in this RTC, year is split into year and century
         * In this RTC, century range is [0,31], year range is [0,99]
         */
        int human_year = tm->tm_year + 1900;
        int rtc_year = human_year % 100;
        int rtc_century = human_year / 100;

        if(rtc_in_range(rtc_year, 0, 99) &&
           rtc_in_range(rtc_century, 0, 31))
        {
            alarm_date.year = rtc_year;
        } else
            return -1;

        /* Range of tm->tm_wday could be [0, 6] */
        if(rtc_in_range(tm->tm_wday, 0, 6))
            alarm_date.week = tm->tm_wday;
        else
            return -1;

        /* Set RTC mode to timer setting mode */
        rtc_timer_set_mode(RTC_TIMER_SETTING);
        /* Write value to RTC */
        rtc->alarm_date = alarm_date;
        rtc->alarm_time = alarm_time;
        /* Set RTC mode to timer running mode */
        rtc_timer_set_mode(RTC_TIMER_RUNNING);
    }

    return 0;
}

int rtc_year_is_leap(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int rtc_get_yday(int year, int month, int day)
{
    static const int days[2][13] = {
        {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
        {0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}};
    int leap = rtc_year_is_leap(year);

    return days[leap][month] + day;
}

int rtc_get_wday(int year, int month, int day)
{
    /* Magic method to get weekday */
    int weekday = (day += month < 3 ? year-- : year - 2, 23 * month / 9 + day + 4 + year / 4 - year / 100 + year / 400) % 7;
    return weekday;
}

struct tm *rtc_timer_get_tm(void)
{
    if(rtc_timer_get_mode() != RTC_TIMER_RUNNING)
        return NULL;

    rtc_date_t timer_date = rtc->date;
    rtc_time_t timer_time = rtc->time;
    rtc_extended_t timer_extended = rtc->extended;

    struct tm *tm = &rtc_timer_date_time;

    tm->tm_sec = timer_time.second % 61;      /* 0-60, follow C99 */
    tm->tm_min = timer_time.minute % 60;      /* 0-59 */
    tm->tm_hour = timer_time.hour % 24;       /* 0-23 */
    tm->tm_mday = timer_date.day % 32;        /* 1-31 */
    tm->tm_mon = (timer_date.month - 1) % 12; /* 0-11 */
    tm->tm_year = (timer_date.year % 100) + (timer_extended.century * 100) - 1900;
    tm->tm_wday = timer_date.week % 7;                                                 /* 0-6 */
    tm->tm_yday = rtc_get_yday(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday) % 366; /* 0-365 */
    tm->tm_isdst = -1;

    return tm;
}

struct tm *rtc_alarm_get_tm(void)
{
    if(rtc_timer_get_mode() != RTC_TIMER_RUNNING)
        return NULL;

    rtc_alarm_date_t alarm_date = rtc->alarm_date;
    rtc_alarm_time_t alarm_time = rtc->alarm_time;
    rtc_extended_t timer_extended = rtc->extended;

    struct tm *tm = &rtc_alarm_date_time;

    tm->tm_sec = alarm_time.second % 61;      /* 0-60, follow C99  */
    tm->tm_min = alarm_time.minute % 60;      /* 0-59 */
    tm->tm_hour = alarm_time.hour % 24;       /* 0-23 */
    tm->tm_mday = alarm_date.day % 32;        /* 1-31 */
    tm->tm_mon = (alarm_date.month - 1) % 12; /* 0-11 */
    tm->tm_year = (alarm_date.year % 100) + (timer_extended.century * 100) - 1900;
    tm->tm_wday = alarm_date.week % 7;                                                 /* 0-6 */
    tm->tm_yday = rtc_get_yday(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday) % 366; /* 0-365 */
    tm->tm_isdst = -1;

    return tm;
}

int rtc_timer_set(int year, int month, int day, int hour, int minute, int second)
{
    struct tm date_time = {
        .tm_sec = second,
        .tm_min = minute,
        .tm_hour = hour,
        .tm_mday = day,
        .tm_mon = month - 1,
        .tm_year = year - 1900,
        .tm_wday = rtc_get_wday(year, month, day),
        .tm_yday = rtc_get_yday(year, month, day),
        .tm_isdst = -1,
    };

    return rtc_timer_set_tm(&date_time);
}

int rtc_timer_get(int *year, int *month, int *day, int *hour, int *minute, int *second)
{
    struct tm *tm = rtc_timer_get_tm();

    if(tm)
    {
        if(year)
            *year = tm->tm_year + 1900;
        if(month)
            *month = tm->tm_mon + 1;
        if(day)
            *day = tm->tm_mday;
        if(hour)
            *hour = tm->tm_hour;
        if(minute)
            *minute = tm->tm_min;
        if(second)
            *second = tm->tm_sec;
    } else
        return -1;

    return 0;
}

int rtc_alarm_set(int year, int month, int day, int hour, int minute, int second)
{
    struct tm date_time = {
        .tm_sec = second,
        .tm_min = minute,
        .tm_hour = hour,
        .tm_mday = day,
        .tm_mon = month - 1,
        .tm_year = year - 1900,
        .tm_wday = rtc_get_wday(year, month, day),
        .tm_yday = rtc_get_yday(year, month, day),
        .tm_isdst = -1,
    };

    return rtc_alarm_set_tm(&date_time);
}

int rtc_alarm_get(int *year, int *month, int *day, int *hour, int *minute, int *second)
{
    struct tm *tm = rtc_alarm_get_tm();

    if(tm)
    {
        if(year)
            *year = tm->tm_year + 1900;
        if(month)
            *month = tm->tm_mon + 1;
        if(day)
            *day = tm->tm_mday;
        if(hour)
            *hour = tm->tm_hour;
        if(minute)
            *minute = tm->tm_min;
        if(second)
            *second = tm->tm_sec;
    } else
        return -1;

    return 0;
}

int rtc_timer_set_clock_frequency(unsigned int frequency)
{
    rtc_initial_count_t initial_count;

    initial_count.count = frequency;
    /* Set RTC mode to timer setting mode */
    rtc_timer_set_mode(RTC_TIMER_SETTING);
    rtc->initial_count = initial_count;
    /* Set RTC mode to timer running mode */
    rtc_timer_set_mode(RTC_TIMER_RUNNING);
    return 0;
}

unsigned int rtc_timer_get_clock_frequency(void)
{
    return rtc->initial_count.count;
}

int rtc_timer_set_clock_count_value(unsigned int count)
{

    rtc_current_count_t current_count;

    current_count.count = count;
    /* Set RTC mode to timer setting mode */
    rtc_timer_set_mode(RTC_TIMER_SETTING);
    rtc->current_count = current_count;
    /* Set RTC mode to timer running mode */
    rtc_timer_set_mode(RTC_TIMER_RUNNING);
    return 0;
}

unsigned int rtc_timer_get_clock_count_value(void)
{
    return rtc->current_count.count;
}

int rtc_tick_set_interrupt(int enable)
{
    rtc_interrupt_ctrl_t interrupt_ctrl = rtc->interrupt_ctrl;
    interrupt_ctrl.tick_enable = enable;
    /* Set RTC mode to timer setting mode */
    rtc_timer_set_mode(RTC_TIMER_SETTING);
    rtc->interrupt_ctrl = interrupt_ctrl;

    rtc_timer_set_mode(RTC_TIMER_RUNNING);
    return 0;
}

int rtc_tick_get_interrupt(void)
{
    rtc_interrupt_ctrl_t interrupt_ctrl = rtc->interrupt_ctrl;

    return interrupt_ctrl.tick_enable;
}

int rtc_tick_set_interrupt_mode(rtc_tick_interrupt_mode_t mode)
{
    rtc_interrupt_ctrl_t interrupt_ctrl = rtc->interrupt_ctrl;

    interrupt_ctrl.tick_int_mode = mode;
    /* Set RTC mode to timer setting mode */
    rtc_timer_set_mode(RTC_TIMER_SETTING);
    rtc->interrupt_ctrl = interrupt_ctrl;
    /* Set RTC mode to timer running mode */
    rtc_timer_set_mode(RTC_TIMER_RUNNING);
    return 0;
}

rtc_tick_interrupt_mode_t rtc_tick_get_interrupt_mode(void)
{
    rtc_interrupt_ctrl_t interrupt_ctrl = rtc->interrupt_ctrl;

    return interrupt_ctrl.tick_int_mode;
}

int rtc_alarm_set_interrupt(int enable)
{
    rtc_interrupt_ctrl_t interrupt_ctrl = rtc->interrupt_ctrl;

    interrupt_ctrl.alarm_enable = enable;
    /* Set RTC mode to timer setting mode */
    rtc_timer_set_mode(RTC_TIMER_SETTING);
    rtc->interrupt_ctrl = interrupt_ctrl;
    /* Set RTC mode to timer running mode */
    rtc_timer_set_mode(RTC_TIMER_RUNNING);
    return 0;
}

int rtc_alarm_get_interrupt(void)
{
    rtc_interrupt_ctrl_t interrupt_ctrl = rtc->interrupt_ctrl;

    return interrupt_ctrl.alarm_enable;
}

int rtc_alarm_set_mask(rtc_mask_t mask)
{
    rtc_interrupt_ctrl_t interrupt_ctrl = rtc->interrupt_ctrl;

    interrupt_ctrl.alarm_compare_mask = *(uint8_t *)&mask;
    /* Set RTC mode to timer setting mode */
    rtc_timer_set_mode(RTC_TIMER_SETTING);
    rtc->interrupt_ctrl = interrupt_ctrl;
    /* Set RTC mode to timer running mode */
    rtc_timer_set_mode(RTC_TIMER_RUNNING);
    return 0;
}

rtc_mask_t rtc_alarm_get_mask(void)
{
    rtc_interrupt_ctrl_t interrupt_ctrl = rtc->interrupt_ctrl;

    uint8_t compare_mask = interrupt_ctrl.alarm_compare_mask;

    return *(rtc_mask_t *)&compare_mask;
}

int rtc_protect_set(int enable)
{
    rtc_register_ctrl_t register_ctrl = rtc->register_ctrl;

    rtc_mask_t mask = {
        .second = 1, /* Second mask */
        .minute = 1, /* Minute mask */
        .hour = 1,   /* Hour mask */
        .week = 1,   /* Week mask */
        .day = 1,    /* Day mask */
        .month = 1,  /* Month mask */
        .year = 1,   /* Year mask */
    };

    rtc_mask_t unmask = {
        .second = 0, /* Second mask */
        .minute = 0, /* Minute mask */
        .hour = 0,   /* Hour mask */
        .week = 0,   /* Week mask */
        .day = 0,    /* Day mask */
        .month = 0,  /* Month mask */
        .year = 0,   /* Year mask */
    };

    if(enable)
    {
        /* Turn RTC in protect mode, no one can write time */
        register_ctrl.timer_mask = *(uint8_t *)&unmask;
        register_ctrl.alarm_mask = *(uint8_t *)&unmask;
        register_ctrl.initial_count_mask = 0;
        register_ctrl.interrupt_register_mask = 0;
    } else
    {
        /* Turn RTC in unprotect mode, everyone can write time */
        register_ctrl.timer_mask = *(uint8_t *)&mask;
        register_ctrl.alarm_mask = *(uint8_t *)&mask;
        register_ctrl.initial_count_mask = 1;
        register_ctrl.interrupt_register_mask = 1;
    }

    rtc->register_ctrl = register_ctrl;
    return 0;
}

int rtc_init(void)
{
    /* Reset RTC */
    sysctl_reset(SYSCTL_RESET_RTC);
    /* Enable RTC */
    sysctl_clock_enable(SYSCTL_CLOCK_RTC);
    /* Unprotect RTC */
    rtc_protect_set(0);
    /* Set RTC clock frequency */
    rtc_timer_set_clock_frequency(
        sysctl_clock_get_freq(SYSCTL_CLOCK_IN0));
    rtc_timer_set_clock_count_value(1);
    /* Set RTC mode to timer running mode */
    rtc_timer_set_mode(RTC_TIMER_RUNNING);
    return 0;
}

int rtc_irq_callback(void *ctx)
{
    rtc_instance_t *instance = (rtc_instance_t *)ctx;
    struct tm *now_tm = rtc_timer_get_tm();
    if(rtc_alarm_get_interrupt())
    {
        struct tm *alarm_tm = rtc_alarm_get_tm();
        rtc_mask_t alarm_mask = rtc_alarm_get_mask();
        if((*((uint8_t *)&alarm_mask) & 0xFE) == 0)
        {
            goto tick;
        }
        if(alarm_mask.year)
        {
            if(now_tm->tm_year != alarm_tm->tm_year)
            {
                goto tick;
            }
        }
        if(alarm_mask.month)
        {
            if(now_tm->tm_mon != alarm_tm->tm_mon)
            {
                goto tick;
            }
        }
        if(alarm_mask.day)
        {
            if(now_tm->tm_mday != alarm_tm->tm_mday)
            {
                goto tick;
            }
        }
        if(alarm_mask.hour)
        {
            if(now_tm->tm_hour != alarm_tm->tm_hour)
            {
                goto tick;
            }
        }
        if(alarm_mask.minute)
        {
            if(now_tm->tm_min != alarm_tm->tm_min)
            {
                goto tick;
            }
        }
        if(alarm_mask.second)
        {
            if(now_tm->tm_sec != alarm_tm->tm_sec)
            {
                goto tick;
            }
        }
        if(instance->alarm_is_single_shot)
        {
            rtc_alarm_set_interrupt(0);
            instance->tick_enable_by_alarm = false;
            instance->tick_mode_by_alarm = RTC_INT_MAX;

            if(instance->tick_enable_by_user)
            {
                if(instance->tick_mode_by_user > rtc_tick_get_interrupt_mode())
                {
                    rtc_tick_set_interrupt(0);
                    rtc_tick_set_interrupt_mode(instance->tick_mode_by_user);
                    rtc_tick_set_interrupt(1);
                }
            } else
            {
                rtc_tick_set_interrupt(0);
            }
        }
        if(instance->rtc_alarm_callback)
            instance->rtc_alarm_callback(instance->alarm_ctx);
    }
tick:
    if(instance->tick_enable_by_user)
    {
        switch(instance->tick_mode_by_user)
        {
            case RTC_INT_MINUTE:
                if(now_tm->tm_sec != 0)
                    goto ret;
                break;
            case RTC_INT_HOUR:
                if(now_tm->tm_sec != 0 || now_tm->tm_min != 0)
                    goto ret;
                break;
            case RTC_INT_DAY:
                if(now_tm->tm_sec != 0 || now_tm->tm_min != 0 || now_tm->tm_hour != 0)
                    goto ret;
                break;
            case RTC_INT_SECOND:
            default:
                break;
        }

        if(instance->tick_is_single_shot)
        {
            rtc_tick_set_interrupt(0);
            instance->tick_enable_by_user = false;
            instance->tick_mode_by_user = -1;
        }
        if(instance->rtc_tick_callback)
            instance->rtc_tick_callback(instance->tick_ctx);
    }
ret:
    return 0;
}

int rtc_tick_irq_register(bool is_single_shot, rtc_tick_interrupt_mode_t mode, plic_irq_callback_t callback, void *ctx, uint8_t priority)
{
    plic_irq_disable(IRQN_RTC_INTERRUPT);
    rtc_tick_set_interrupt(0);

    rtc_instance.rtc_tick_callback = callback;
    rtc_instance.tick_ctx = ctx;
    rtc_instance.tick_is_single_shot = is_single_shot;
    rtc_instance.tick_enable_by_user = true;
    rtc_instance.tick_mode_by_user = mode;

    if(!rtc_instance.tick_enable_by_alarm || (rtc_instance.tick_enable_by_alarm && mode < rtc_tick_get_interrupt_mode()))
    {
        rtc_tick_set_interrupt_mode(mode);
    }

    plic_set_priority(IRQN_RTC_INTERRUPT, priority);
    plic_irq_register(IRQN_RTC_INTERRUPT, rtc_irq_callback, &rtc_instance);
    plic_irq_enable(IRQN_RTC_INTERRUPT);

    rtc_tick_set_interrupt(1);
    return 0;
}

void rtc_tick_irq_unregister(void)
{
    /* Resolve interrupt dependency */
    if(!rtc_alarm_get_interrupt())
    {
        rtc_tick_set_interrupt(0);
    }

    rtc_instance.tick_enable_by_user = false;
    rtc_instance.tick_mode_by_user = -1;

    rtc_instance.rtc_tick_callback = NULL;
    rtc_instance.tick_ctx = NULL;

    if((!rtc_instance.rtc_tick_callback) && (!rtc_instance.rtc_alarm_callback))
    {
        plic_irq_unregister(IRQN_RTC_INTERRUPT);
    }
}

int rtc_alarm_irq_register(bool is_single_shot, rtc_mask_t mask, plic_irq_callback_t callback, void *ctx, uint8_t priority)
{
    plic_irq_disable(IRQN_RTC_INTERRUPT);

    rtc_tick_set_interrupt(0);
    rtc_alarm_set_interrupt(0);
    rtc_instance.rtc_alarm_callback = callback;
    rtc_instance.alarm_ctx = ctx;
    rtc_instance.alarm_is_single_shot = is_single_shot;
    rtc_instance.tick_enable_by_alarm = true;

    if(mask.second)
    {
        rtc_instance.tick_mode_by_alarm = RTC_INT_SECOND;
        goto alarm_mode;
    }
    if(mask.minute)
    {
        rtc_instance.tick_mode_by_alarm = RTC_INT_MINUTE;
        goto alarm_mode;
    }
    if(mask.hour)
    {
        rtc_instance.tick_mode_by_alarm = RTC_INT_HOUR;
        goto alarm_mode;
    } else
    {
        rtc_instance.tick_mode_by_alarm = RTC_INT_DAY;
    }
alarm_mode:
    if((rtc_instance.tick_enable_by_user && rtc_instance.tick_mode_by_alarm < rtc_tick_get_interrupt_mode()) || !rtc_instance.tick_enable_by_user)
    {
        rtc_tick_set_interrupt_mode(rtc_instance.tick_mode_by_alarm);
    }

    rtc_alarm_set_mask(mask);

    plic_set_priority(IRQN_RTC_INTERRUPT, priority);
    plic_irq_register(IRQN_RTC_INTERRUPT, rtc_irq_callback, &rtc_instance);
    plic_irq_enable(IRQN_RTC_INTERRUPT);

    rtc_alarm_set_interrupt(1);
    /* Must enable tick hardware interrupt */
    rtc_tick_set_interrupt(1);
    return 0;
}

void rtc_alarm_irq_unregister(void)
{
    rtc_alarm_set_interrupt(0);
    rtc_instance.rtc_alarm_callback = NULL;
    rtc_instance.alarm_ctx = NULL;
    rtc_instance.tick_enable_by_alarm = false;
    rtc_instance.tick_mode_by_alarm = RTC_INT_MAX;

    if(rtc_instance.tick_enable_by_user)
    {
        if(rtc_instance.tick_mode_by_user > rtc_tick_get_interrupt_mode())
        {
            rtc_tick_set_interrupt(0);
            rtc_tick_set_interrupt_mode(rtc_instance.tick_mode_by_user);
            rtc_tick_set_interrupt(1);
        }
    } else
    {
        rtc_tick_set_interrupt(0);
    }

    if((!rtc_instance.rtc_tick_callback) && (!rtc_instance.rtc_alarm_callback))
    {
        plic_irq_unregister(IRQN_RTC_INTERRUPT);
    }
}