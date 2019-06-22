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

#ifndef _BSP_SYSCALLS_H
#define _BSP_SYSCALLS_H

#include <machine/syscall.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief       Definitions for syscall putchar function
 *
 * @param[in]   c       The char to put
 *
 * @return      result
 *     - Byte   On success, returns the written character.
 *     - EOF   On failure, returns EOF and sets the error indicator (see ferror()) on stdout.
 */
typedef int (*sys_putchar_t)(char c);

/**
 * @brief       Definitions for syscall getchar function
 *
 * @return      byte as int type to get
 *     - Byte   The character read as an unsigned char cast to an int
 *     - EOF    EOF on end of file or error, no enough byte to read
 */
typedef int (*sys_getchar_t)(void);

/**
 * @brief       Register putchar function when perform write syscall
 * 
 * @param[in]   putchar       The user-defined putchar function
 *
 * @return      None
 */
void sys_register_putchar(sys_putchar_t putchar);

/**
 * @brief       Register getchar function when perform read syscall
 * 
 * @param[in]   getchar       The user-defined getchar function
 *
 * @return      None
 */
void sys_register_getchar(sys_getchar_t getchar);

void __attribute__((noreturn)) sys_exit(int code);

void setStats(int enable);

#undef putchar
int putchar(int ch);
void printstr(const char *s);

void printhex(uint64_t x);

size_t get_free_heap_size(void);

#ifdef __cplusplus
}
#endif

#endif /* _BSP_SYSCALLS_H */

