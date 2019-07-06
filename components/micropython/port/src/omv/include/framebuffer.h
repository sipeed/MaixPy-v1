/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Framebuffer stuff.
 *
 */
#ifndef __FRAMEBUFFER_H__
#define __FRAMEBUFFER_H__
#include <stdint.h>
#include "mutex.h"
#include "imlib_config.h"

typedef struct framebuffer {
    int x,y;
    int w,h;
    int w_max, h_max;
    int u,v;
    int bpp;
	uint8_t* pixels;
	uint8_t* pix_ai;
} __attribute__((aligned(8))) framebuffer_t;

typedef struct jpegbuffer {
    int w,h;
    int size;
    int enabled;
    int quality;
    mutex_t lock;
    uint8_t* pixels;
} __attribute__((aligned(8))) jpegbuffer_t;

extern framebuffer_t *fb_framebuffer;
// Use these macros to get a pointer to main or JPEG framebuffer.
#define MAIN_FB()           (fb_framebuffer)

#ifndef OMV_MINIMUM
extern jpegbuffer_t *jpeg_fb_framebuffer;
#define JPEG_FB()           (jpeg_fb_framebuffer)
#endif

// Use this macro to get a pointer to the free SRAM area located after the framebuffer.
#define MAIN_FB_PIXELS()    (MAIN_FB()->pixels + fb_buffer_size())

// Use this macro to get a pointer to the free SRAM area located after the framebuffer.
// #define JPEG_FB_PIXELS()    (JPEG_FB()->pixels + JPEG_FB()->size)

// Returns the main frame buffer size, factoring in pixel formats.
uint32_t fb_buffer_size();

// Transfers the frame buffer to the jpeg frame buffer if not locked.
void fb_update_jpeg_buffer();
#endif /* __FRAMEBUFFER_H__ */
