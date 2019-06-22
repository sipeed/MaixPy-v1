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

#ifndef _BSP_INTERRUPT_H
#define _BSP_INTERRUPT_H

#ifdef __cplusplus
extern "C" {
#endif
/* clang-format off */
/* Machine interrupt mask for 64 bit system, 0x8000 0000 0000 0000 */
#define CAUSE_MACHINE_IRQ_MASK            (0x1ULL << 63)

/* Machine interrupt reason mask for 64 bit system, 0x7FFF FFFF FFFF FFFF */
#define CAUSE_MACHINE_IRQ_REASON_MASK     (CAUSE_MACHINE_IRQ_MASK - 1)

/* Hypervisor interrupt mask for 64 bit system, 0x8000 0000 0000 0000 */
#define CAUSE_HYPERVISOR_IRQ_MASK         (0x1ULL << 63)

/* Hypervisor interrupt reason mask for 64 bit system, 0x7FFF FFFF FFFF FFFF */
#define CAUSE_HYPERVISOR_IRQ_REASON_MASK  (CAUSE_HYPERVISOR_IRQ_MASK - 1)

/* Supervisor interrupt mask for 64 bit system, 0x8000 0000 0000 0000 */
#define CAUSE_SUPERVISOR_IRQ_MASK         (0x1ULL << 63)

/* Supervisor interrupt reason mask for 64 bit system, 0x7FFF FFFF FFFF FFFF */
#define CAUSE_SUPERVISOR_IRQ_REASON_MASK  (CAUSE_SUPERVISOR_IRQ_MASK - 1)
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _BSP_INTERRUPT_H */

