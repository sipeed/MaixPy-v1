/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * BMP reader/writer.
 *
 */
#include <math.h>
#include <stdlib.h>
#include <ff.h>
#include "vfs_wrapper.h"
#include "xalloc.h"
#include "imlib.h"
#include "omv_boardconfig.h"
#include "framebuffer.h"
#include "py/runtime.h"

// This function inits the geometry values of an image (opens file).
bool bmp_read_geometry(mp_obj_t fp, image_t *img, bmp_read_settings_t *rs)
{
    read_byte_expect(fp, 'B');
    read_byte_expect(fp, 'M');
    uint32_t file_size;
    read_long_raise(fp, &file_size);
    read_word_ignore(fp);
    read_word_ignore(fp);

    uint32_t header_size;
    read_long_raise(fp, &header_size);
    if (file_size <= header_size) file_corrupted_raise(fp);
    
    uint32_t data_size = file_size - header_size;
    // if (data_size % 4) file_corrupted_raise(fp);
    // if (file_size % 4) file_corrupted_raise(fp);
    uint32_t header2_size;
    read_long_raise(fp, &header2_size);
    read_long_raise(fp, (uint32_t*) &rs->bmp_w);
    read_long_raise(fp, (uint32_t*) &rs->bmp_h);
    if ((rs->bmp_w == 0) || (rs->bmp_h == 0)) file_corrupted_raise(fp);
    img->w = abs(rs->bmp_w);
    img->h = abs(rs->bmp_h);

    read_word_expect(fp, 1);
    read_word_raise(fp, &rs->bmp_bpp);
    if ((rs->bmp_bpp != 8) && (rs->bmp_bpp != 16) && (rs->bmp_bpp != 24)) fs_unsupported_format(fp);
    img->bpp = (rs->bmp_bpp == 8) ? 1 : 2;

    read_long_raise(fp, &rs->bmp_fmt);
    if ((rs->bmp_fmt != 0) && (rs->bmp_fmt != 3)) fs_unsupported_format(fp);

    read_long_expect(fp, data_size);
    read_long_ignore(fp);
    read_long_ignore(fp);
    read_long_ignore(fp);
    read_long_ignore(fp);

    if (rs->bmp_bpp == 8) {
        if (rs->bmp_fmt != 0) fs_unsupported_format(fp);
        // Color Table (1024 bytes)
        for (int i = 0; i < 256; i++) {
            read_long_expect(fp, ((i) << 16) | ((i) << 8) | i);
        }
    } else if (rs->bmp_bpp == 16) {
        if (rs->bmp_fmt != 3) fs_unsupported_format(fp);
        // Bit Masks (12 bytes)
        read_long_expect(fp, 0x1F << 11);
        read_long_expect(fp, 0x3F << 5);
        read_long_expect(fp, 0x1F);
    } else if (rs->bmp_bpp == 24) {
        if (rs->bmp_fmt == 3) {
            // Bit Masks (12 bytes)
            read_long_expect(fp, 0xFF << 16);
            read_long_expect(fp, 0xFF << 8);
            read_long_expect(fp, 0xFF);
        }
    }
    int err;
    vfs_internal_seek(fp, header_size, VFS_SEEK_SET, &err);
    if(err != 0)
        mp_raise_OSError(err);

    rs->bmp_row_bytes = (((img->w * rs->bmp_bpp) + 31) / 32) * 4;
    if (data_size < (rs->bmp_row_bytes * img->h)) file_corrupted_raise(fp);
    return (rs->bmp_h >= 0);
}

// This function reads the pixel values of an image.
bool bmp_read_pixels(mp_obj_t fp, image_t *img, int line_start, int line_end, bmp_read_settings_t *rs)
{
    if (rs->bmp_bpp == 8) {
        if ((rs->bmp_h < 0) && (rs->bmp_w >= 0) && (img->w == rs->bmp_row_bytes)) {
            if(read_data(fp, // Super Fast - Zoom, Zoom!
                      img->pixels + (line_start * img->w),
                      (line_end - line_start) * img->w))
            {
                return false;
            }
        } else {
            for (int i = line_start; i < line_end; i++) {
                for (int j = 0; j < rs->bmp_row_bytes; j++) {
                    uint8_t pixel;
                    if(!read_byte(fp, &pixel))
                        return false;
                    if (j < img->w) {
                        if (rs->bmp_h < 0) { // vertical flip (BMP file perspective)
                            if (rs->bmp_w < 0) { // horizontal flip (BMP file perspective)
                                IM_SET_GS_PIXEL(img, (img->w-j-1), i, pixel);
                            } else {
                                IM_SET_GS_PIXEL(img, j, i, pixel);
                            }
                        } else {
                            if (rs->bmp_w < 0) {
                                IM_SET_GS_PIXEL(img, (img->w-j-1), (img->h-i-1), pixel);
                            } else {
                                IM_SET_GS_PIXEL(img, j, (img->h-i-1), pixel);
                            }
                        }
                    }
                }
            }
        }
    } else if (rs->bmp_bpp == 16) {
        for (int i = line_start; i < line_end; i++) {
            for (int j = 0, jj = rs->bmp_row_bytes / 2; j < jj; j++) {
                uint16_t pixel;
                if(read_word(fp, &pixel) != 0)
                    return false;
                pixel = IM_SWAP16(pixel);
                if (j < img->w) {
                    if (rs->bmp_h < 0) { // vertical flip (BMP file perspective)
                        if (rs->bmp_w < 0) { // horizontal flip (BMP file perspective)
                            IM_SET_RGB565_PIXEL(img, (img->w-j-1), i, pixel);
                        } else {
                            IM_SET_RGB565_PIXEL(img, j, i, pixel);
                        }
                    } else {
                        if (rs->bmp_w < 0) {
                            IM_SET_RGB565_PIXEL(img, (img->w-j-1), (img->h-i-1), pixel);
                        } else {
                            IM_SET_RGB565_PIXEL(img, j, (img->h-i-1), pixel);
                        }
                    }
                }
            }
        }
    } else if (rs->bmp_bpp == 24) {
        for (int i = line_start; i < line_end; i++) {
            for (int j = 0, jj = rs->bmp_row_bytes / 3; j < jj; j++) {
                uint8_t r, g, b;
                if(!read_byte(fp, &r))
                    return false;
                if(!read_byte(fp, &g))
                    return false;
                if(!read_byte(fp, &b))
                    return false;
                uint16_t pixel = IM_RGB565(IM_R825(r), IM_G826(g), IM_B825(b));
                if (j < img->w) {
                    if (rs->bmp_h < 0) { // vertical flip
                        if (rs->bmp_w < 0) { // horizontal flip
                            IM_SET_RGB565_PIXEL(img, (img->w-j-1), i, pixel);
                        } else {
                            IM_SET_RGB565_PIXEL(img, j, i, pixel);
                        }
                    } else {
                        if (rs->bmp_w < 0) {
                            IM_SET_RGB565_PIXEL(img, (img->w-j-1), (img->h-i-1), pixel);
                        } else {
                            IM_SET_RGB565_PIXEL(img, j, (img->h-i-1), pixel);
                        }
                    }
                }
            }
            for (int j = 0, jj = rs->bmp_row_bytes % 3; j < jj; j++) {
                if(!read_byte_ignore(fp))
                    return false;
            }
        }
    }
    return true;
}

void bmp_read(image_t *img, const char *path)
{
    int err;
    bmp_read_settings_t rs;

    mp_obj_t file = vfs_internal_open(path, "rb", &err);
    if( file == MP_OBJ_NULL || err != 0)
    {
        mp_raise_OSError(err);
    }
    bmp_read_geometry(file, img, &rs);
    if (!img->pixels)
        img->pixels = xalloc(img->w * img->h * img->bpp);
    else
    {
        if( (img->w * img->h * img->bpp) > MAIN_FB()->w_max * MAIN_FB()->h_max * OMV_INIT_BPP )
        {
            mp_raise_OSError(MP_EINVAL);    
        }
    }
    if(!bmp_read_pixels(file, img, 0, img->h, &rs))
    {
        if(img->pixels != MAIN_FB()->pixels )
            xfree(img->pixels);
        vfs_internal_close(file, &err);
        mp_raise_OSError(MP_EIO);
    }
    vfs_internal_close(file, &err);
}

void bmp_write_subimg(image_t *img, const char *path, rectangle_t *r)
{
    rectangle_t rect;
    if (!rectangle_subimg(img, r, &rect)) fs_no_intersection(NULL);

    int err;    
    mp_obj_t file = vfs_internal_open(path, "wb", &err);
    if(file == MP_OBJ_NULL || err != 0)
        mp_raise_OSError(err);

    if (IM_IS_GS(img)) {
        const int row_bytes = (((rect.w * 8) + 31) / 32) * 4;
        const int data_size = (row_bytes * rect.h);
        const int waste = (row_bytes / sizeof(uint8_t)) - rect.w;
        // File Header (14 bytes)
        write_byte_raise(file, 'B');
        write_byte_raise(file, 'M');
        write_long_raise(file, 14 + 40 + 1024 + data_size);
        write_word_raise(file, 0);
        write_word_raise(file, 0);
        write_long_raise(file, 14 + 40 + 1024);
        // Info Header (40 bytes)
        write_long_raise(file, 40);
        write_long_raise(file, rect.w);
        write_long_raise(file, -rect.h); // store the image flipped (correctly)
        write_word_raise(file, 1);
        write_word_raise(file, 8);
        write_long_raise(file, 0);
        write_long_raise(file, data_size);
        write_long_raise(file, 0);
        write_long_raise(file, 0);
        write_long_raise(file, 0);
        write_long_raise(file, 0);
        // Color Table (1024 bytes)
        for (int i = 0; i < 256; i++) {
            write_long_raise(file, ((i) << 16) | ((i) << 8) | i);
        }
        if ((rect.x == 0) && (rect.w == img->w) && (img->w == row_bytes)) {
            write_data_raise(file, // Super Fast - Zoom, Zoom!
                       img->pixels + (rect.y * img->w),
                       rect.w * rect.h);
        } else {
            for (int i = 0; i < rect.h; i++) {
                write_data_raise(file, img->pixels+((rect.y+i)*img->w)+rect.x, rect.w);
                for (int j = 0; j < waste; j++) {
                    write_byte_raise(file, 0);
                }
            }
        }
    } else {
        const int row_bytes = (((rect.w * 16) + 31) / 32) * 4;
        const int data_size = (row_bytes * rect.h);
        const int waste = (row_bytes / sizeof(uint16_t)) - rect.w;
        // File Header (14 bytes)
        write_byte_raise(file, 'B');
        write_byte_raise(file, 'M');
        write_long_raise(file, 14 + 40 + 12 + data_size);
        write_word_raise(file, 0);
        write_word_raise(file, 0);
        write_long_raise(file, 14 + 40 + 12);
        // Info Header (40 bytes)
        write_long_raise(file, 40);
        write_long_raise(file, rect.w);
        write_long_raise(file, -rect.h); // store the image flipped (correctly)
        write_word_raise(file, 1);
        write_word_raise(file, 16);
        write_long_raise(file, 3);
        write_long_raise(file, data_size);
        write_long_raise(file, 0);
        write_long_raise(file, 0);
        write_long_raise(file, 0);
        write_long_raise(file, 0);
        // Bit Masks (12 bytes)
        write_long_raise(file, 0x1F << 11);
        write_long_raise(file, 0x3F << 5);
        write_long_raise(file, 0x1F);
        for (int i = 0; i < rect.h; i++) {
            for (int j = 0; j < rect.w; j++) {
                write_word_raise(file, IM_SWAP16(IM_GET_RGB565_PIXEL(img, (rect.x + j), (rect.y + i))));
            }
            for (int j = 0; j < waste; j++) {
                write_word_raise(file, 0);
            }
        }
    }
    vfs_internal_close(file, &err);
}
