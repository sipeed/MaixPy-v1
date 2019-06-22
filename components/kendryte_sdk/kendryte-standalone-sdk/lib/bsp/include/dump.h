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

#ifndef _BSP_DUMP_H
#define _BSP_DUMP_H

#include <stdlib.h>
#include <string.h>
#include "syslog.h"
#include "uarths.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DUMP_PRINTF printk

static inline void
dump_core(const char *reason, uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32])
{
    static const char *const reg_usage[][2] =
    {
        {"zero ", "Hard-wired zero"},
        {"ra   ", "Return address"},
        {"sp   ", "Stack pointer"},
        {"gp   ", "Global pointer"},
        {"tp   ", "Thread pointer"},
        {"t0   ", "Temporaries Caller"},
        {"t1   ", "Temporaries Caller"},
        {"t2   ", "Temporaries Caller"},
        {"s0/fp", "Saved register/frame pointer"},
        {"s1   ", "Saved register"},
        {"a0   ", "Function arguments/return values"},
        {"a1   ", "Function arguments/return values"},
        {"a2   ", "Function arguments values"},
        {"a3   ", "Function arguments values"},
        {"a4   ", "Function arguments values"},
        {"a5   ", "Function arguments values"},
        {"a6   ", "Function arguments values"},
        {"a7   ", "Function arguments values"},
        {"s2   ", "Saved registers"},
        {"s3   ", "Saved registers"},
        {"s4   ", "Saved registers"},
        {"s5   ", "Saved registers"},
        {"s6   ", "Saved registers"},
        {"s7   ", "Saved registers"},
        {"s8   ", "Saved registers"},
        {"s9   ", "Saved registers"},
        {"s10  ", "Saved registers"},
        {"s11  ", "Saved registers"},
        {"t3   ", "Temporaries Caller"},
        {"t4   ", "Temporaries Caller"},
        {"t5   ", "Temporaries Caller"},
        {"t6   ", "Temporaries Caller"},
    };

    static const char *const regf_usage[][2] =
    {
        {"ft0 ", "FP temporaries"},
        {"ft1 ", "FP temporaries"},
        {"ft2 ", "FP temporaries"},
        {"ft3 ", "FP temporaries"},
        {"ft4 ", "FP temporaries"},
        {"ft5 ", "FP temporaries"},
        {"ft6 ", "FP temporaries"},
        {"ft7 ", "FP temporaries"},
        {"fs0 ", "FP saved registers"},
        {"fs1 ", "FP saved registers"},
        {"fa0 ", "FP arguments/return values"},
        {"fa1 ", "FP arguments/return values"},
        {"fa2 ", "FP arguments values"},
        {"fa3 ", "FP arguments values"},
        {"fa4 ", "FP arguments values"},
        {"fa5 ", "FP arguments values"},
        {"fa6 ", "FP arguments values"},
        {"fa7 ", "FP arguments values"},
        {"fs2 ", "FP Saved registers"},
        {"fs3 ", "FP Saved registers"},
        {"fs4 ", "FP Saved registers"},
        {"fs5 ", "FP Saved registers"},
        {"fs6 ", "FP Saved registers"},
        {"fs7 ", "FP Saved registers"},
        {"fs8 ", "FP Saved registers"},
        {"fs9 ", "FP Saved registers"},
        {"fs10", "FP Saved registers"},
        {"fs11", "FP Saved registers"},
        {"ft8 ", "FP Temporaries Caller"},
        {"ft9 ", "FP Temporaries Caller"},
        {"ft10", "FP Temporaries Caller"},
        {"ft11", "FP Temporaries Caller"},
    };

    if (CONFIG_LOG_LEVEL >= LOG_ERROR)
    {
        const char unknown_reason[] = "unknown";

        if (!reason)
            reason = unknown_reason;

        DUMP_PRINTF("core dump: %s\r\n", reason);
        DUMP_PRINTF("Cause 0x%016lx, EPC 0x%016lx\r\n", cause, epc);

        int i = 0;
        for (i = 0; i < 32 / 2; i++)
        {
            DUMP_PRINTF(
                "reg[%02d](%s) = 0x%016lx, reg[%02d](%s) = 0x%016lx\r\n",
                i * 2, reg_usage[i * 2][0], regs[i * 2],
                i * 2 + 1, reg_usage[i * 2 + 1][0], regs[i * 2 + 1]);
        }

        for (i = 0; i < 32 / 2; i++)
        {
            DUMP_PRINTF(
                "freg[%02d](%s) = 0x%016lx(%f), freg[%02d](%s) = 0x%016lx(%f)\r\n",
                i * 2, regf_usage[i * 2][0], fregs[i * 2], (float)fregs[i * 2],
                i * 2 + 1, regf_usage[i * 2 + 1][0], fregs[i * 2 + 1], (float)fregs[i * 2 + 1]);
        }
    }
}

#undef DUMP_PRINTF

#ifdef __cplusplus
}
#endif

#endif /* _BSP_DUMP_H */

