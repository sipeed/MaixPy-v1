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
#ifndef __GC0328_H
#define __GC0328_H

#include <stdint.h>
#include "sensor.h"

#define GC0328_ID       (0x9d)
#define GC0328_ADDR     (0x21)
int gc0328_reset(sensor_t*);
uint8_t gc0328_scan(void);
int gc0328_init(sensor_t *sensor);

#endif /* __GC0328_H */
