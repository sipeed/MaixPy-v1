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
//
// Core Synchronization

#ifndef CORE_SYNC_H
#define CORE_SYNC_H

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    CORE_SYNC_NONE,
    CORE_SYNC_ADD_TCB
} core_sync_event_t;

void core_sync_request_context_switch(uint64_t core_id);
void core_sync_complete_context_switch(uint64_t core_id);
void core_sync_complete(uint64_t core_id);
int core_sync_is_in_progress(uint64_t core_id);

#ifdef __cplusplus
}
#endif

#endif /* CORE_SYNC_H */
