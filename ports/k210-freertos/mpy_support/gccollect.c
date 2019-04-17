/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
 * Copyright (c) 2017 Pycom Limited
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>

#include "py/mpconfig.h"
#include "py/mpstate.h"
#include "py/gc.h"
#include "py/mpthread.h"
#include "gccollect.h"
#include <sysctl.h>

#define K210_NUM_AREGS 32


uintptr_t get_sp(void) {
    uintptr_t result;
    __asm__ ("la %0, _sp0\n" : "=r" (result) );
    return result;
}

static void gc_collect_inner(int level)
{
    if (level < K210_NUM_AREGS) {
        gc_collect_inner(level + 1);
        if (level != 0) return;
    }

    if (level == K210_NUM_AREGS) {
        // collect on stack
        volatile void *stack_p = 0;
        volatile void *sp = &stack_p;
        gc_collect_root((void**)sp, ((mp_uint_t)MP_STATE_THREAD(stack_top) - (mp_uint_t)sp) / sizeof(void*));
        return;
    }
#if MICROPY_PY_THREAD
    // Trace root pointers from other threads
    int n_th = mp_thread_gc_others();
#endif
}


void gc_collect(void) {
    // start the GC
	//uint64_t cycle = read_cycle();
    gc_collect_start();
	gc_collect_inner(0);
#if MICROPY_PY_THREAD
    mp_thread_gc_others();
#endif
    // end the GC
    gc_collect_end();
	//mp_printf(&mp_plat_print, "#gc use: %d us\r\n",(read_cycle() - cycle)/(sysctl_clock_get_freq(SYSCTL_CLOCK_CPU) / 1000000UL));
}

