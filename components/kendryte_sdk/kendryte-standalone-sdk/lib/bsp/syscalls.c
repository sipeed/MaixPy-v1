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

/* Enable kernel-mode log API */

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/unistd.h>
#include <machine/syscall.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "syscalls.h"
#include "atomic.h"
#include "clint.h"
#include "fpioa.h"
#include "interrupt.h"
#include "sysctl.h"
#include "util.h"
#include "syslog.h"
#include "dump.h"

/**
 * @note       System call list
 *
 * See also riscv-newlib/libgloss/riscv/syscalls.c
 *
 * | System call      | Number |
 * |------------------|--------|
 * | SYS_exit         | 93     |
 * | SYS_exit_group   | 94     |
 * | SYS_getpid       | 172    |
 * | SYS_kill         | 129    |
 * | SYS_read         | 63     |
 * | SYS_write        | 64     |
 * | SYS_open         | 1024   |
 * | SYS_openat       | 56     |
 * | SYS_close        | 57     |
 * | SYS_lseek        | 62     |
 * | SYS_brk          | 214    |
 * | SYS_link         | 1025   |
 * | SYS_unlink       | 1026   |
 * | SYS_mkdir        | 1030   |
 * | SYS_chdir        | 49     |
 * | SYS_getcwd       | 17     |
 * | SYS_stat         | 1038   |
 * | SYS_fstat        | 80     |
 * | SYS_lstat        | 1039   |
 * | SYS_fstatat      | 79     |
 * | SYS_access       | 1033   |
 * | SYS_faccessat    | 48     |
 * | SYS_pread        | 67     |
 * | SYS_pwrite       | 68     |
 * | SYS_uname        | 160    |
 * | SYS_getuid       | 174    |
 * | SYS_geteuid      | 175    |
 * | SYS_getgid       | 176    |
 * | SYS_getegid      | 177    |
 * | SYS_mmap         | 222    |
 * | SYS_munmap       | 215    |
 * | SYS_mremap       | 216    |
 * | SYS_time         | 1062   |
 * | SYS_getmainvars  | 2011   |
 * | SYS_rt_sigaction | 134    |
 * | SYS_writev       | 66     |
 * | SYS_gettimeofday | 169    |
 * | SYS_times        | 153    |
 * | SYS_fcntl        | 25     |
 * | SYS_getdents     | 61     |
 * | SYS_dup          | 23     |
 *
 */

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

static const char *TAG = "SYSCALL";

extern char _heap_start[];
extern char _heap_end[];
char *_heap_cur = &_heap_start[0];

sys_putchar_t sys_putchar;
sys_getchar_t sys_getchar;

void sys_register_putchar(sys_putchar_t putchar)
{
    sys_putchar = putchar;
}

void sys_register_getchar(sys_getchar_t getchar)
{
    sys_getchar = getchar;
}

void __attribute__((noreturn)) sys_exit(int code)
{
    /* Read core id */
    unsigned long core_id = current_coreid();
    /* First print some diagnostic information. */
    LOGW(TAG, "sys_exit called by core %ld with 0x%lx\r\n", core_id, (uint64_t)code);
    while (1)
        continue;
}

static int sys_nosys(long a0, long a1, long a2, long a3, long a4, long a5, unsigned long n)
{
    UNUSED(a3);
    UNUSED(a4);
    UNUSED(a5);

    LOGE(TAG, "Unsupported syscall %ld: a0=%lx, a1=%lx, a2=%lx!\r\n", n, a0, a1, a2);
    while (1)
        continue;
    return -ENOSYS;
}

static int sys_success(void)
{
    return 0;
}

static size_t sys_brk(size_t pos)
{
    uintptr_t res = 0;
    /**
     * brk() sets the end of the data segment to the value
     * specified by addr, when that value is reasonable, the system
     * has enough memory, and the process does not exceed its
     * maximum data size.
     *
     * sbrk() increments the program's data space by increment
     * bytes. Calling sbrk() with an increment of 0 can be used to
     * find the current location of the program break.
     *
     * uintptr_t brk(uintptr_t ptr);
     *
     * IN : regs[10] = ptr
     * OUT: regs[10] = ptr
     */

    /**
     * First call: Initialization brk pointer. newlib will pass
     * ptr = 0 when it is first called. In this case the address
     * _heap_start will be return.
     *
     * Call again: Adjust brk pointer. The ptr never equal with
     * 0. If ptr is below _heap_end, then allocate memory.
     * Otherwise throw out of memory error, return -1.
     */

    if (pos)
    {
        /* Call again */
        if ((uintptr_t)pos > (uintptr_t)&_heap_end[0])
        {
            /* Memory out, return -ENOMEM */
            LOGE(TAG, "Out of memory\r\n");
            res = -ENOMEM;
        }
        else
        {
            /* Adjust brk pointer. */
            _heap_cur = (char *)(uintptr_t)pos;
            /* Return current address. */
            res = (uintptr_t)_heap_cur;
        }
    }
    else
    {
        /* First call, return initial address */
        res = (uintptr_t)&_heap_start[0];
    }
    return (size_t)res;
}

static ssize_t sys_write(int file, const void *ptr, size_t len)
{
    ssize_t res = -EBADF;

    /**
     * Write to a file.
     *
     * ssize_t write(int file, const void *ptr, size_t len)
     *
     * IN : regs[10] = file, regs[11] = ptr, regs[12] = len
     * OUT: regs[10] = len
     */

    /* Get size to write */
    register size_t length = len;
    /* Get data pointer */
    register char *data = (char *)ptr;

    if (STDOUT_FILENO == file || STDERR_FILENO == file)
    {
        /* Write data */
        while (length-- > 0 && *data != 0) {
            if (sys_putchar)
                sys_putchar(*(data++));
        }

        /* Return the actual size written */
        res = len;
    }
    else
    {
        /* Not support yet */
        res = -ENOSYS;
    }

    return res;
}

static int sys_fstat(int file, struct stat *st)
{
    int res = -EBADF;

    /**
     * Status of an open file. The sys/stat.h header file required
     * is
     * distributed in the include subdirectory for this C library.
     *
     * int fstat(int file, struct stat* st)
     *
     * IN : regs[10] = file, regs[11] = st
     * OUT: regs[10] = Upon successful completion, 0 shall be
     * returned.
     * Otherwise, -1 shall be returned and errno set to indicate
     * the error.
     */

    UNUSED(file);

    if (st != NULL)
        memset(st, 0, sizeof(struct stat));
    /* Return the result */
    res = -ENOSYS;
    /**
     * Note: This value will return to syscall wrapper, syscall
     * wrapper will set errno to ENOSYS and return -1
     */

    return res;
}

static int sys_close(int file)
{
    int res = -EBADF;

    /**
     * Close a file.
     *
     * int close(int file)
     *
     * IN : regs[10] = file
     * OUT: regs[10] = Upon successful completion, 0 shall be
     * returned.
     * Otherwise, -1 shall be returned and errno set to indicate
     * the error.
     */

    UNUSED(file);
    /* Return the result */
    res = 0;
    return res;
}

static int sys_gettimeofday(struct timeval *tp, void *tzp)
{
    /**
     * Get the current time.  Only relatively correct.
     *
     * int gettimeofday(struct timeval *tp, void *tzp)
     *
     * IN : regs[10] = tp
     * OUT: regs[10] = Upon successful completion, 0 shall be
     * returned.
     * Otherwise, -1 shall be returned and errno set to indicate
     * the error.
     */
    UNUSED(tzp);

    if (tp != NULL)
    {
        uint64_t clint_usec = clint->mtime / (sysctl_clock_get_freq(SYSCTL_CLOCK_CPU) / CLINT_CLOCK_DIV / 1000000UL);

        tp->tv_sec  = clint_usec / 1000000UL;
        tp->tv_usec = clint_usec % 1000000UL;
    }
    /* Return the result */
    return 0;
}

uintptr_t __attribute__((weak))
handle_ecall(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32])
{
    UNUSED(cause);
    UNUSED(fregs);
    enum syscall_id_e
    {
        SYS_ID_NOSYS,
        SYS_ID_SUCCESS,
        SYS_ID_EXIT,
        SYS_ID_BRK,
        SYS_ID_WRITE,
        SYS_ID_FSTAT,
        SYS_ID_CLOSE,
        SYS_ID_GETTIMEOFDAY,
        SYS_ID_MAX
    };

    static uintptr_t (* const syscall_table[])(long a0, long a1, long a2, long a3, long a4, long a5, unsigned long n) =
    {
        [SYS_ID_NOSYS]         = (void *)sys_nosys,
        [SYS_ID_SUCCESS]       = (void *)sys_success,
        [SYS_ID_EXIT]          = (void *)sys_exit,
        [SYS_ID_BRK]           = (void *)sys_brk,
        [SYS_ID_WRITE]         = (void *)sys_write,
        [SYS_ID_FSTAT]         = (void *)sys_fstat,
        [SYS_ID_CLOSE]         = (void *)sys_close,
        [SYS_ID_GETTIMEOFDAY]  = (void *)sys_gettimeofday,
    };

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Woverride-init"
#endif
    static const uint8_t syscall_id_table[0x100] =
    {
        [0x00 ... 0xFF]            = SYS_ID_NOSYS,
        [0xFF & SYS_exit]          = SYS_ID_EXIT,
        [0xFF & SYS_exit_group]    = SYS_ID_EXIT,
        [0xFF & SYS_getpid]        = SYS_ID_NOSYS,
        [0xFF & SYS_kill]          = SYS_ID_NOSYS,
        [0xFF & SYS_read]          = SYS_ID_NOSYS,
        [0xFF & SYS_write]         = SYS_ID_WRITE,
        [0xFF & SYS_open]          = SYS_ID_NOSYS,
        [0xFF & SYS_openat]        = SYS_ID_NOSYS,
        [0xFF & SYS_close]         = SYS_ID_CLOSE,
        [0xFF & SYS_lseek]         = SYS_ID_NOSYS,
        [0xFF & SYS_brk]           = SYS_ID_BRK,
        [0xFF & SYS_link]          = SYS_ID_NOSYS,
        [0xFF & SYS_unlink]        = SYS_ID_NOSYS,
        [0xFF & SYS_mkdir]         = SYS_ID_NOSYS,
        [0xFF & SYS_chdir]         = SYS_ID_NOSYS,
        [0xFF & SYS_getcwd]        = SYS_ID_NOSYS,
        [0xFF & SYS_stat]          = SYS_ID_NOSYS,
        [0xFF & SYS_fstat]         = SYS_ID_FSTAT,
        [0xFF & SYS_lstat]         = SYS_ID_NOSYS,
        [0xFF & SYS_fstatat]       = SYS_ID_NOSYS,
        [0xFF & SYS_access]        = SYS_ID_NOSYS,
        [0xFF & SYS_faccessat]     = SYS_ID_NOSYS,
        [0xFF & SYS_pread]         = SYS_ID_NOSYS,
        [0xFF & SYS_pwrite]        = SYS_ID_NOSYS,
        [0xFF & SYS_uname]         = SYS_ID_NOSYS,
        [0xFF & SYS_getuid]        = SYS_ID_NOSYS,
        [0xFF & SYS_geteuid]       = SYS_ID_NOSYS,
        [0xFF & SYS_getgid]        = SYS_ID_NOSYS,
        [0xFF & SYS_getegid]       = SYS_ID_NOSYS,
        [0xFF & SYS_mmap]          = SYS_ID_NOSYS,
        [0xFF & SYS_munmap]        = SYS_ID_NOSYS,
        [0xFF & SYS_mremap]        = SYS_ID_NOSYS,
        [0xFF & SYS_time]          = SYS_ID_NOSYS,
        [0xFF & SYS_getmainvars]   = SYS_ID_NOSYS,
        [0xFF & SYS_rt_sigaction]  = SYS_ID_NOSYS,
        [0xFF & SYS_writev]        = SYS_ID_NOSYS,
        [0xFF & SYS_gettimeofday]  = SYS_ID_GETTIMEOFDAY,
        [0xFF & SYS_times]         = SYS_ID_NOSYS,
        [0xFF & SYS_fcntl]         = SYS_ID_NOSYS,
        [0xFF & SYS_getdents]      = SYS_ID_NOSYS,
        [0xFF & SYS_dup]           = SYS_ID_NOSYS,
    };
#if defined(__GNUC__)
#pragma GCC diagnostic warning "-Woverride-init"
#endif

    regs[10] = syscall_table[syscall_id_table[0xFF & regs[17]]]
    (
        regs[10], /* a0 */
        regs[11], /* a1 */
        regs[12], /* a2 */
        regs[13], /* a3 */
        regs[14], /* a4 */
        regs[15], /* a5 */
        regs[17]  /* n */
    );

    return epc + 4;
}

uintptr_t __attribute__((weak, alias("handle_ecall")))
handle_ecall_u(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32]);

uintptr_t __attribute__((weak, alias("handle_ecall")))
handle_ecall_h(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32]);

uintptr_t __attribute__((weak, alias("handle_ecall")))
handle_ecall_s(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32]);

uintptr_t __attribute__((weak, alias("handle_ecall")))
handle_ecall_m(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32]);

uintptr_t __attribute__((weak))
handle_misaligned_fetch(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32])
{
    dump_core("misaligned fetch", cause, epc, regs, fregs);
    sys_exit(1337);
    return epc;
}

uintptr_t __attribute__((weak))
handle_fault_fetch(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32])
{
    dump_core("fault fetch", cause, epc, regs, fregs);
    sys_exit(1337);
    return epc;
}

uintptr_t __attribute__((weak))
handle_illegal_instruction(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32])
{
    dump_core("illegal instruction", cause, epc, regs, fregs);
    sys_exit(1337);
    return epc;
}

uintptr_t __attribute__((weak))
handle_breakpoint(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32])
{
    dump_core("breakpoint", cause, epc, regs, fregs);
    sys_exit(1337);
    return epc;
}

uintptr_t __attribute__((weak))
handle_misaligned_load(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32])
{
    /* notice this function only support 16bit or 32bit instruction */

    bool compressed = (*(unsigned short *)epc & 3) != 3;
    bool fpu = 0;          /* load to fpu ? */
    uintptr_t addr = 0;    /* src addr */
    uint8_t src = 0;       /* src register */
    uint8_t dst = 0;       /* dst register */
    uint8_t len = 0;       /* data length */
    int offset = 0;        /* addr offset to addr in reg */
    bool unsigned_ = 0;    /* unsigned */
    uint64_t data_load = 0;/* real data load */

    if (compressed)
    {
        /* compressed instruction should not get this fault. */
        goto on_error;
    }
    else
    {
        uint32_t instruct = *(uint32_t *)epc;
        uint8_t opcode = instruct&0x7F;

        dst = (instruct >> 7)&0x1F;
        len = (instruct >> 12)&3;
        unsigned_ = (instruct >> 14)&1;
        src = (instruct >> 15)&0x1F;
        offset = (instruct >> 20);
        len = 1 << len;
        switch (opcode)
        {
            case 3:/* load */
                break;
            case 7:/* fpu load */
                fpu = 1;
                break;
            default:
                goto on_error;
        }
    }

    if (offset >> 11)
        offset = -((offset & 0x3FF) + 1);

    addr = (uint64_t)((uint64_t)regs[src] + offset);

    for (int i = 0; i < len; ++i)
        data_load |= ((uint64_t)*((uint8_t *)addr + i)) << (8 * i);


    if (!unsigned_ & !fpu)
    {
        /* adjust sign */
        switch (len)
        {
            case 1:
                data_load = (uint64_t)(int64_t)((int8_t)data_load);
                break;
            case 2:
                data_load = (uint64_t)(int64_t)((int16_t)data_load);
                break;
            case 4:
                data_load = (uint64_t)(int64_t)((int32_t)data_load);
                break;
            default:
                break;
        }
    }

    if (fpu)
        fregs[dst] = data_load;
    else
        regs[dst] = data_load;

    LOGV(TAG, "misaligned load recovered at %08lx. len:%02d,addr:%08lx,reg:%02d,data:%016lx,signed:%1d,float:%1d", (uint64_t)epc, len, (uint64_t)addr, dst, data_load, !unsigned_, fpu);

    return epc + (compressed ? 2 : 4);
on_error:
    dump_core("misaligned load", cause, epc, regs, fregs);
    sys_exit(1337);
    return epc;
}

uintptr_t __attribute__((weak))
handle_fault_load(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32])
{
    dump_core("fault load", cause, epc, regs, fregs);
    sys_exit(1337);
    return epc;
}

uintptr_t __attribute__((weak))
handle_misaligned_store(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32])
{
    /* notice this function only support 16bit or 32bit instruction */

    bool compressed = (*(unsigned short *)epc & 3) != 3;
    bool fpu = 0;           /* store to fpu*/
    uintptr_t addr = 0;     /* src addr*/
    uint8_t src = 0;        /* src register*/
    uint8_t dst = 0;        /* dst register*/
    uint8_t len = 0;        /* data length*/
    int offset = 0;         /* addr offset to addr in reg*/
    uint64_t data_store = 0;/* real data store*/

    if (compressed)
    {
        /* compressed instruction should not get this fault. */
        goto on_error;
    }
    else
    {
        uint32_t instruct = *(uint32_t *)epc;
        uint8_t opcode = instruct&0x7F;

        len = (instruct >> 12)&7;
        dst = (instruct >> 15)&0x1F;
        src = (instruct >> 20)&0x1F;
        offset = ((instruct >> 7)&0x1F) | ((instruct >> 20)&0xFE0);
        len = 1 << len;
        switch (opcode)
        {
            case 0x23:/* store */
                break;
            case 0x27:/* fpu store */
                fpu = 1;
                break;
            default:
                goto on_error;
        }
    }

    if (offset >> 11)
        offset = -((offset & 0x3FF) + 1);

    addr = (uint64_t)((uint64_t)regs[dst] + offset);


    if (fpu)
        data_store = fregs[src];
    else
        data_store = regs[src];

    for (int i = 0; i < len; ++i)
        *((uint8_t *)addr + i) = (data_store >> (i*8)) & 0xFF;

    LOGV(TAG, "misaligned store recovered at %08lx. len:%02d,addr:%08lx,reg:%02d,data:%016lx,float:%1d", (uint64_t)epc, len, (uint64_t)addr, src, data_store, fpu);

    return epc + (compressed ? 2 : 4);
on_error:
    dump_core("misaligned store", cause, epc, regs, fregs);
    sys_exit(1337);
    return epc;
}

uintptr_t __attribute__((weak))
handle_fault_store(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32])
{
    dump_core("fault store", cause, epc, regs, fregs);
    sys_exit(1337);
    return epc;
}

uintptr_t handle_syscall(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32])
{

    static uintptr_t (* const cause_table[])(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32]) =
    {
        [CAUSE_MISALIGNED_FETCH]      = handle_misaligned_fetch,
        [CAUSE_FAULT_FETCH]           = handle_fault_fetch,
        [CAUSE_ILLEGAL_INSTRUCTION]   = handle_illegal_instruction,
        [CAUSE_BREAKPOINT]            = handle_breakpoint,
        [CAUSE_MISALIGNED_LOAD]       = handle_misaligned_load,
        [CAUSE_FAULT_LOAD]            = handle_fault_load,
        [CAUSE_MISALIGNED_STORE]      = handle_misaligned_store,
        [CAUSE_FAULT_STORE]           = handle_fault_store,
        [CAUSE_USER_ECALL]            = handle_ecall_u,
        [CAUSE_SUPERVISOR_ECALL]      = handle_ecall_h,
        [CAUSE_HYPERVISOR_ECALL]      = handle_ecall_s,
        [CAUSE_MACHINE_ECALL]         = handle_ecall_m,
    };

    return cause_table[cause](cause, epc, regs, fregs);
}

size_t get_free_heap_size(void)
{
    return (size_t)(&_heap_end[0] - _heap_cur);
}

