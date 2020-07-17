/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Font data.
 *
 */
#ifndef __FONT_H__
#define __FONT_H__
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "fmath.h"
#include "imdefs.h"
#include "imlib.h"

enum FontIndex {
    ASCII,
    Unicode,
    UTF8,
    GBK,
    GB2312,
};

enum FontSource {
    BuildIn,
    FileIn,
    StringIO,
    ArrayIn,
};

void font_load(uint8_t index, uint8_t width, uint8_t high, uint8_t source_type, void *src_addr);
void font_free();

#endif // __FONT_H__
