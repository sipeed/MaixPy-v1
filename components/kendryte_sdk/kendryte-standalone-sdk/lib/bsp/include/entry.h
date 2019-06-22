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

#ifndef _BSP_ENTRY_H
#define _BSP_ENTRY_H

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*core_function)(void *ctx);

typedef struct _core_instance_t
{
    core_function callback;
    void *ctx;
} core_instance_t;

int register_core1(core_function func, void *ctx);

static inline void init_lma(void)
{
    extern unsigned int _data_lma;
    extern unsigned int _data;
    extern unsigned int _edata;
    unsigned int *src, *dst;

    src = &_data_lma;
    dst = &_data;
    while (dst < &_edata)
        *dst++ = *src++;
}

static inline void init_bss(void)
{
    extern unsigned int _bss;
    extern unsigned int _ebss;
    unsigned int *dst;

    dst = &_bss;
    while (dst < &_ebss)
        *dst++ = 0;
}

static inline void init_tls(void)
{
    register void *thread_pointer asm("tp");
    extern char _tls_data;

    extern __thread char _tdata_begin, _tdata_end, _tbss_end;

    size_t tdata_size = &_tdata_end - &_tdata_begin;

    memcpy(thread_pointer, &_tls_data, tdata_size);

    size_t tbss_size = &_tbss_end - &_tdata_end;

    memset(thread_pointer + tdata_size, 0, tbss_size);
}

#ifdef __cplusplus
}
#endif

#endif /* _BSP_ENTRY_H */

