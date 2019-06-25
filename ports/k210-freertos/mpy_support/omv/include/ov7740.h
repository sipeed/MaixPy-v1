#pragma once

#include "sensor.h"
#include "stdint.h"

#define OV7740_SET_MIRROR(r, x)   ((r&0xBF)|((x&1)<<6))
#define OV7740_SET_FLIP(r, x)     ((r&0x7F)|((x&1)<<7))


int ov7740_init(sensor_t *sensor);



