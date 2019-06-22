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

#include "entry.h"

/**
 * @brief      Dummy function for __libc_init_array called
 */
void __attribute__((weak)) _init(void)
{
    /**
     * These don't have to do anything since we use init_array/fini_array.
     */
}

/**
 * @brief      Dummy function for __libc_fini_array called
 */
void __attribute__((weak)) _fini(void)
{
    /**
     * These don't have to do anything since we use init_array/fini_array.
     */
}

