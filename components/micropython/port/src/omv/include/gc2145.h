/*
* Copyright 2020 Sipeed Co.,Ltd.

* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef __GC2145_H__
#define __GC2145_H__

#include <stdint.h>
#include "sensor.h"
#include "i2c.h"

#define GC2145_ID       0x45 // (0x2145)
#define GC2145_ADDR     (0x78>>1)

int gc2145_reset(sensor_t *sensor);
int gc2145_init(sensor_t *sensor);

#endif /* __GC2145_H__ */
