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
#ifndef _SYSLOG_H
#define _SYSLOG_H

#include <stdint.h>
#include <stdio.h>
#include "printf.h"
#include "encoding.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Logging library
 *
 * Log library has two ways of managing log verbosity: compile time, set via
 * menuconfig
 *
 * At compile time, filtering is done using CONFIG_LOG_DEFAULT_LEVEL macro, set via
 * menuconfig. All logging statments for levels higher than CONFIG_LOG_DEFAULT_LEVEL
 * will be removed by the preprocessor.
 *
 *
 * How to use this library:
 *
 * In each C file which uses logging functionality, define TAG variable like this:
 *
 *      static const char *TAG = "MODULE_NAME";
 *
 * then use one of logging macros to produce output, e.g:
 *
 *      LOGW(TAG, "Interrupt error %d", error);
 *
 * Several macros are available for different verbosity levels:
 *
 *      LOGE - error
 *      LOGW - warning
 *      LOGI - info
 *      LOGD - debug
 *      LOGV - verbose
 *
 * To override default verbosity level at file or component scope, define LOG_LEVEL macro.
 * At file scope, define it before including esp_log.h, e.g.:
 *
 *      #define LOG_LEVEL LOG_VERBOSE
 *      #include "dxx_log.h"
 *
 * At component scope, define it in component makefile:
 *
 *      CFLAGS += -D LOG_LEVEL=LOG_DEBUG
 *
 *
 */

/* clang-format off */
typedef enum _kendryte_log_level
{
    LOG_NONE,       /*!< No log output */
    LOG_ERROR,      /*!< Critical errors, software module can not recover on its own */
    LOG_WARN,       /*!< Error conditions from which recovery measures have been taken */
    LOG_INFO,       /*!< Information messages which describe normal flow of events */
    LOG_DEBUG,      /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
    LOG_VERBOSE     /*!< Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
} kendryte_log_level_t ;
/* clang-format on */

/* clang-format off */
#if CONFIG_LOG_COLORS
#define LOG_COLOR_BLACK   "30"
#define LOG_COLOR_RED     "31"
#define LOG_COLOR_GREEN   "32"
#define LOG_COLOR_BROWN   "33"
#define LOG_COLOR_BLUE    "34"
#define LOG_COLOR_PURPLE  "35"
#define LOG_COLOR_CYAN    "36"
#define LOG_COLOR(COLOR)  "\033[0;" COLOR "m"
#define LOG_BOLD(COLOR)   "\033[1;" COLOR "m"
#define LOG_RESET_COLOR   "\033[0m"
#define LOG_COLOR_E       LOG_COLOR(LOG_COLOR_RED)
#define LOG_COLOR_W       LOG_COLOR(LOG_COLOR_BROWN)
#define LOG_COLOR_I       LOG_COLOR(LOG_COLOR_GREEN)
#define LOG_COLOR_D
#define LOG_COLOR_V
#else /* CONFIG_LOG_COLORS */
#define LOG_COLOR_E
#define LOG_COLOR_W
#define LOG_COLOR_I
#define LOG_COLOR_D
#define LOG_COLOR_V
#define LOG_RESET_COLOR
#endif /* CONFIG_LOG_COLORS */
/* clang-format on */

#define LOG_FORMAT(letter, format)  LOG_COLOR_ ## letter #letter " (%lu) %s: " format LOG_RESET_COLOR "\n"

#ifdef LOG_LEVEL
#undef CONFIG_LOG_LEVEL
#define CONFIG_LOG_LEVEL LOG_LEVEL
#endif

#ifdef LOG_KERNEL
#define LOG_PRINTF printk
#else
#define LOG_PRINTF printf
#endif

#ifdef CONFIG_LOG_ENABLE
#define LOGE(tag, format, ...)  do {if (CONFIG_LOG_LEVEL >= LOG_ERROR)   LOG_PRINTF(LOG_FORMAT(E, format), read_cycle(), tag, ##__VA_ARGS__); } while (0)
#define LOGW(tag, format, ...)  do {if (CONFIG_LOG_LEVEL >= LOG_WARN)    LOG_PRINTF(LOG_FORMAT(W, format), read_cycle(), tag, ##__VA_ARGS__); } while (0)
#define LOGI(tag, format, ...)  do {if (CONFIG_LOG_LEVEL >= LOG_INFO)    LOG_PRINTF(LOG_FORMAT(I, format), read_cycle(), tag, ##__VA_ARGS__); } while (0)
#define LOGD(tag, format, ...)  do {if (CONFIG_LOG_LEVEL >= LOG_DEBUG)   LOG_PRINTF(LOG_FORMAT(D, format), read_cycle(), tag, ##__VA_ARGS__); } while (0)
#define LOGV(tag, format, ...)  do {if (CONFIG_LOG_LEVEL >= LOG_VERBOSE) LOG_PRINTF(LOG_FORMAT(V, format), read_cycle(), tag, ##__VA_ARGS__); } while (0)
#else
#define LOGE(tag, format, ...)
#define LOGW(tag, format, ...)
#define LOGI(tag, format, ...)
#define LOGD(tag, format, ...)
#define LOGV(tag, format, ...)
#endif  /* LOG_ENABLE */

#ifdef __cplusplus
}
#endif


#endif /* _SYSLOG_H */

