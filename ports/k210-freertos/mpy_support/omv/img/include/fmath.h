/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Fast approximate math functions.
 *
 */
#ifndef __FMATH_H
#define __FMATH_H
#include <stdint.h>

extern  float fast_sqrtf(float x);
extern  int fast_floorf(float x);
extern  int fast_ceilf(float x);
extern  int fast_roundf(float x);
extern  float fast_atanf(float x);
extern  float fast_atan2f(float y, float x);
extern  float fast_expf(float x);
extern  float fast_cbrtf(float d);
extern  float fast_fabsf(float d);
extern  float fast_log(float x);
extern  float fast_log2(float x);
extern  float fast_powf(float a, float b);

extern const float cos_table[360];
extern const float sin_table[360];
#endif // __FMATH_H
