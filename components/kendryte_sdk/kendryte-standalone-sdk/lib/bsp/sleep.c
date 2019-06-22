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
#include "sleep.h"
#include "sysctl.h"

int usleep(uint64_t usec)
{
    uint64_t cycle = read_cycle();
    uint64_t nop_all = usec * sysctl_clock_get_freq(SYSCTL_CLOCK_CPU) / 1000000UL;
    while (1)
    {
        if(read_cycle() - cycle >= nop_all)
            break;
    }
    return 0;
}

int msleep(uint64_t msec)
{
    return (unsigned int)usleep(msec * 1000);
}

unsigned int sleep(unsigned int seconds)
{
    return (unsigned int)msleep(seconds * 1000);
}

