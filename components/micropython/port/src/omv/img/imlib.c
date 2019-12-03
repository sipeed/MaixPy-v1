/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Image library.
 *
 */
#include <stdlib.h>
#include <mp.h>
#include "font.h"
#include "array.h"
#include "vfs_wrapper.h"
#include "imlib.h"
#include "common.h"
#include "omv_boardconfig.h"
#include "sipeed_conv.h"
#include "picojpeg_util.h"
#include "framebuffer.h"

/////////////////
// Point Stuff //
/////////////////

void point_init(point_t *ptr, int x, int y)
{
    ptr->x = x;
    ptr->y = y;
}

void point_copy(point_t *dst, point_t *src)
{
    memcpy(dst, src, sizeof(point_t));
}

bool point_equal_fast(point_t *ptr0, point_t *ptr1)
{
    return !memcmp(ptr0, ptr1, sizeof(point_t));
}

int point_quadrance(point_t *ptr0, point_t *ptr1)
{
    int delta_x = ptr0->x - ptr1->x;
    int delta_y = ptr0->y - ptr1->y;
    return (delta_x * delta_x) + (delta_y * delta_y);
}

void point_rotate(int x, int y, float r, int center_x, int center_y, int16_t *new_x, int16_t *new_y)
{
    x -= center_x;
    y -= center_y;
    *new_x = (x * cosf(r)) - (y * sinf(r)) + center_x;
    *new_y = (x * sinf(r)) + (y * cosf(r)) + center_y;
}

////////////////
// Line Stuff //
////////////////

// http://www.skytopia.com/project/articles/compsci/clipping.html
bool lb_clip_line(line_t *l, int x, int y, int w, int h) // line is drawn if this returns true
{
    int xdelta = l->x2 - l->x1, ydelta = l->y2 - l->y1, p[4], q[4];
    float umin = 0, umax = 1;

    p[0] = -(xdelta);
    p[1] = +(xdelta);
    p[2] = -(ydelta);
    p[3] = +(ydelta);

    q[0] = l->x1 - (x);
    q[1] = (x + w - 1) - l->x1;
    q[2] = l->y1 - (y);
    q[3] = (y + h - 1) - l->y1;

    for (int i = 0; i < 4; i++) {
        if (p[i]) {
            float u = ((float) q[i]) / ((float) p[i]);

            if (p[i] < 0) { // outside to inside
                if (u > umax) return false;
                if (u > umin) umin = u;
            }

            if (p[i] > 0) { // inside to outside
                if (u < umin) return false;
                if (u < umax) umax = u;
            }

        } else if (q[i] < 0) {
            return false;
        }
    }

    if (umax < umin) return false;

    int x1_c = l->x1 + (xdelta * umin);
    int y1_c = l->y1 + (ydelta * umin);
    int x2_c = l->x1 + (xdelta * umax);
    int y2_c = l->y1 + (ydelta * umax);
    l->x1 = x1_c;
    l->y1 = y1_c;
    l->x2 = x2_c;
    l->y2 = y2_c;

    return true;
}

/////////////////////
// Rectangle Stuff //
/////////////////////

void rectangle_init(rectangle_t *ptr, int x, int y, int w, int h)
{
    ptr->x = x;
    ptr->y = y;
    ptr->w = w;
    ptr->h = h;
}

void rectangle_copy(rectangle_t *dst, rectangle_t *src)
{
    memcpy(dst, src, sizeof(rectangle_t));
}

bool rectangle_equal_fast(rectangle_t *ptr0, rectangle_t *ptr1)
{
    return !memcmp(ptr0, ptr1, sizeof(rectangle_t));
}

bool rectangle_overlap(rectangle_t *ptr0, rectangle_t *ptr1)
{
    int x0 = ptr0->x;
    int y0 = ptr0->y;
    int w0 = ptr0->w;
    int h0 = ptr0->h;
    int x1 = ptr1->x;
    int y1 = ptr1->y;
    int w1 = ptr1->w;
    int h1 = ptr1->h;
    return (x0 < (x1 + w1)) && (y0 < (y1 + h1)) && (x1 < (x0 + w0)) && (y1 < (y0 + h0));
}

void rectangle_intersected(rectangle_t *dst, rectangle_t *src)
{
    int leftX = IM_MAX(dst->x, src->x);
    int topY = IM_MAX(dst->y, src->y);
    int rightX = IM_MIN(dst->x + dst->w, src->x + src->w);
    int bottomY = IM_MIN(dst->y + dst->h, src->y + src->h);
    dst->x = leftX;
    dst->y = topY;
    dst->w = rightX - leftX;
    dst->h = bottomY - topY;
}

void rectangle_united(rectangle_t *dst, rectangle_t *src)
{
    int leftX = IM_MIN(dst->x, src->x);
    int topY = IM_MIN(dst->y, src->y);
    int rightX = IM_MAX(dst->x + dst->w, src->x + src->w);
    int bottomY = IM_MAX(dst->y + dst->h, src->y + src->h);
    dst->x = leftX;
    dst->y = topY;
    dst->w = rightX - leftX;
    dst->h = bottomY - topY;
}

/////////////////
// Image Stuff //
/////////////////

void image_init(image_t *ptr, int w, int h, int bpp, void *data)
{
    ptr->w = w;
    ptr->h = h;
    ptr->bpp = bpp;
    ptr->data = data;
}

void image_copy(image_t *dst, image_t *src)
{
    memcpy(dst, src, sizeof(image_t));
}

size_t image_size(image_t *ptr)
{
    switch (ptr->bpp) {
        case IMAGE_BPP_BINARY: {
            return IMAGE_BINARY_LINE_LEN_BYTES(ptr) * ptr->h;
        }
        case IMAGE_BPP_GRAYSCALE: {
            return IMAGE_GRAYSCALE_LINE_LEN_BYTES(ptr) * ptr->h;
        }
        case IMAGE_BPP_RGB565: {
            return IMAGE_RGB565_LINE_LEN_BYTES(ptr) * ptr->h;
        }
        case IMAGE_BPP_BAYER: {
            return ptr->w * ptr->h;
        }
        default: { // JPEG
            return ptr->bpp;
        }
    }
}

bool image_get_mask_pixel(image_t *ptr, int x, int y)
{
    if ((0 <= x) && (x < ptr->w) && (0 <= y) && (y < ptr->h)) {
        switch (ptr->bpp) {
            case IMAGE_BPP_BINARY: {
                return IMAGE_GET_BINARY_PIXEL(ptr, x, y);
            }
            case IMAGE_BPP_GRAYSCALE: {
                return COLOR_GRAYSCALE_TO_BINARY(IMAGE_GET_GRAYSCALE_PIXEL(ptr, x, y));
            }
            case IMAGE_BPP_RGB565: {
                return COLOR_RGB565_TO_BINARY(IMAGE_GET_RGB565_PIXEL(ptr, x, y));
            }
            default: {
                return false;
            }
        }
    }

    return false;
}

// Gamma uncompress
extern const float xyz_table[256];

const int8_t kernel_gauss_3[3*3] = {
     1, 2, 1,
     2, 4, 2,
     1, 2, 1,
};

const int8_t kernel_gauss_5[5*5] = {
    1,  4,  6,  4, 1,
    4, 16, 24, 16, 4,
    6, 24, 36, 24, 6,
    4, 16, 24, 16, 4,
    1,  4,  6,  4, 1
};

const int kernel_laplacian_3[3*3] = {
     -1, -1, -1,
     -1,  8, -1,
     -1, -1, -1
};

const int kernel_high_pass_3[3*3] = {
    -1, -1, -1,
    -1, +8, -1,
    -1, -1, -1
};

// USE THE LUT FOR RGB->LAB CONVERSION - NOT THIS FUNCTION!
void imlib_rgb_to_lab(simple_color_t *rgb, simple_color_t *lab)
{
    // https://en.wikipedia.org/wiki/SRGB -> Specification of the transformation
    // https://en.wikipedia.org/wiki/Lab_color_space -> CIELAB-CIEXYZ conversions

    float r_lin = xyz_table[rgb->red];
    float g_lin = xyz_table[rgb->green];
    float b_lin = xyz_table[rgb->blue];

    float x = ((r_lin * 0.4124f) + (g_lin * 0.3576f) + (b_lin * 0.1805f)) / 095.047f;
    float y = ((r_lin * 0.2126f) + (g_lin * 0.7152f) + (b_lin * 0.0722f)) / 100.000f;
    float z = ((r_lin * 0.0193f) + (g_lin * 0.1192f) + (b_lin * 0.9505f)) / 108.883f;

    x = (x>0.008856f) ? fast_cbrtf(x) : ((x * 7.787037f) + 0.137931f);
    y = (y>0.008856f) ? fast_cbrtf(y) : ((y * 7.787037f) + 0.137931f);
    z = (z>0.008856f) ? fast_cbrtf(z) : ((z * 7.787037f) + 0.137931f);

    lab->L = ((int8_t) fast_roundf(116 * y)) - 16;
    lab->A = ((int8_t) fast_roundf(500 * (x-y)));
    lab->B = ((int8_t) fast_roundf(200 * (y-z)));
}

void imlib_lab_to_rgb(simple_color_t *lab, simple_color_t *rgb)
{
    // https://en.wikipedia.org/wiki/Lab_color_space -> CIELAB-CIEXYZ conversions
    // https://en.wikipedia.org/wiki/SRGB -> Specification of the transformation

    float x = ((lab->L + 16) * 0.008621f) + (lab->A * 0.002f);
    float y = ((lab->L + 16) * 0.008621f);
    float z = ((lab->L + 16) * 0.008621f) - (lab->B * 0.005f);

    x = ((x>0.206897f) ? (x*x*x) : ((0.128419f * x) - 0.017713f)) * 095.047f;
    y = ((y>0.206897f) ? (y*y*y) : ((0.128419f * y) - 0.017713f)) * 100.000f;
    z = ((z>0.206897f) ? (z*z*z) : ((0.128419f * z) - 0.017713f)) * 108.883f;

    float r_lin = ((x * +3.2406f) + (y * -1.5372f) + (z * -0.4986f)) / 100.0f;
    float g_lin = ((x * -0.9689f) + (y * +1.8758f) + (z * +0.0415f)) / 100.0f;
    float b_lin = ((x * +0.0557f) + (y * -0.2040f) + (z * +1.0570f)) / 100.0f;

    r_lin = (r_lin>0.0031308f) ? ((1.055f*powf(r_lin, 0.416666f))-0.055f) : (r_lin*12.92f);
    g_lin = (g_lin>0.0031308f) ? ((1.055f*powf(g_lin, 0.416666f))-0.055f) : (g_lin*12.92f);
    b_lin = (b_lin>0.0031308f) ? ((1.055f*powf(b_lin, 0.416666f))-0.055f) : (b_lin*12.92f);

    rgb->red   = IM_MAX(IM_MIN(fast_roundf(r_lin * 255), 255), 0);
    rgb->green = IM_MAX(IM_MIN(fast_roundf(g_lin * 255), 255), 0);
    rgb->blue  = IM_MAX(IM_MIN(fast_roundf(b_lin * 255), 255), 0);
}

void imlib_rgb_to_grayscale(simple_color_t *rgb, simple_color_t *grayscale)
{
    float r_lin = xyz_table[rgb->red];
    float g_lin = xyz_table[rgb->green];
    float b_lin = xyz_table[rgb->blue];
    float y = ((r_lin * 0.2126f) + (g_lin * 0.7152f) + (b_lin * 0.0722f)) / 100.0f;
    y = (y>0.0031308f) ? ((1.055f*powf(y, 0.416666f))-0.055f) : (y*12.92f);
    grayscale->G = IM_MAX(IM_MIN(fast_roundf(y * 255), 255), 0);
}

// Just copy settings back.
void imlib_grayscale_to_rgb(simple_color_t *grayscale, simple_color_t *rgb)
{
    rgb->red   = grayscale->G;
    rgb->green = grayscale->G;
    rgb->blue  = grayscale->G;
}

ALWAYS_INLINE uint16_t imlib_yuv_to_rgb(uint8_t y, int8_t u, int8_t v)
{
    uint32_t r = IM_MAX(IM_MIN(y + ((91881*v)>>16), 255), 0);
    uint32_t g = IM_MAX(IM_MIN(y - (((22554*u)+(46802*v))>>16), 255), 0);
    uint32_t b = IM_MAX(IM_MIN(y + ((116130*u)>>16), 255), 0);
    return IM_RGB565(IM_R825(r), IM_G826(g), IM_B825(b));
}

void imlib_bayer_to_rgb565(image_t *img, int w, int h, int xoffs, int yoffs, uint16_t *rgbbuf)
{
    int r, g, b;
    for (int y=yoffs; y<yoffs+h; y++) {
        for (int x=xoffs; x<xoffs+w; x++) {
            if ((y % 2) == 0) { // Even row
                if ((x % 2) == 0) { // Even col
                    r = (IM_GET_RAW_PIXEL_CHECK_BOUNDS_XY(img, x-1, y-1)  +
                         IM_GET_RAW_PIXEL_CHECK_BOUNDS_XY(img, x+1, y-1)  +
                         IM_GET_RAW_PIXEL_CHECK_BOUNDS_XY(img, x-1, y+1)  +
                         IM_GET_RAW_PIXEL_CHECK_BOUNDS_XY(img, x+1, y+1)) >> 2;

                    g = (IM_GET_RAW_PIXEL_CHECK_BOUNDS_Y(img, x, y-1)  +
                         IM_GET_RAW_PIXEL_CHECK_BOUNDS_Y(img, x, y+1)  +
                         IM_GET_RAW_PIXEL_CHECK_BOUNDS_X(img, x-1, y)  +
                         IM_GET_RAW_PIXEL_CHECK_BOUNDS_X(img, x+1, y)) >> 2;

                    b = IM_GET_RAW_PIXEL(img,  x, y);
                } else { // Odd col
                    r = (IM_GET_RAW_PIXEL_CHECK_BOUNDS_Y(img, x, y-1)  +
                         IM_GET_RAW_PIXEL_CHECK_BOUNDS_Y(img, x, y+1)) >> 1;

                    b = (IM_GET_RAW_PIXEL_CHECK_BOUNDS_X(img, x-1, y)  +
                         IM_GET_RAW_PIXEL_CHECK_BOUNDS_X(img, x+1, y)) >> 1;

                    g =  IM_GET_RAW_PIXEL(img, x, y);
                }
            } else { // Odd row
                if ((x % 2) == 0) { // Even Col
                    r = (IM_GET_RAW_PIXEL_CHECK_BOUNDS_X(img, x-1, y)  +
                         IM_GET_RAW_PIXEL_CHECK_BOUNDS_X(img, x+1, y)) >> 1;

                    g =  IM_GET_RAW_PIXEL(img, x, y);

                    b = (IM_GET_RAW_PIXEL_CHECK_BOUNDS_Y(img, x, y-1)  +
                         IM_GET_RAW_PIXEL_CHECK_BOUNDS_Y(img, x, y+1)) >> 1;
                } else { // Odd col
                    r = IM_GET_RAW_PIXEL(img,  x, y);

                    g = (IM_GET_RAW_PIXEL_CHECK_BOUNDS_Y(img, x, y-1)  +
                         IM_GET_RAW_PIXEL_CHECK_BOUNDS_Y(img, x, y+1)  +
                         IM_GET_RAW_PIXEL_CHECK_BOUNDS_X(img, x-1, y)  +
                         IM_GET_RAW_PIXEL_CHECK_BOUNDS_X(img, x+1, y)) >> 2;

                    b = (IM_GET_RAW_PIXEL_CHECK_BOUNDS_XY(img, x-1, y-1)  +
                         IM_GET_RAW_PIXEL_CHECK_BOUNDS_XY(img, x+1, y-1)  +
                         IM_GET_RAW_PIXEL_CHECK_BOUNDS_XY(img, x-1, y+1)  +
                         IM_GET_RAW_PIXEL_CHECK_BOUNDS_XY(img, x+1, y+1)) >> 2;
                }

            }
            r = IM_R825(r);
            g = IM_G826(g);
            b = IM_B825(b);
            *rgbbuf++ = IM_RGB565(r, g, b);
        }
    }
}
////////////////////////////////////////////////////////////////////////////////

static save_image_format_t imblib_parse_extension(image_t *img, const char *path)
{
    size_t l = strlen(path);
    const char *p = path + l;
    if (l >= 5) {
               if (((p[-1] == 'g') || (p[-1] == 'G'))
               &&  ((p[-2] == 'e') || (p[-2] == 'E'))
               &&  ((p[-3] == 'p') || (p[-3] == 'P'))
               &&  ((p[-4] == 'j') || (p[-4] == 'J'))
               &&  ((p[-5] == '.') || (p[-5] == '.'))) {
                    // Will convert to JPG if not.
                    return FORMAT_JPG;
        }
    }
    if (l >= 4) {
               if (((p[-1] == 'g') || (p[-1] == 'G'))
               &&  ((p[-2] == 'p') || (p[-2] == 'P'))
               &&  ((p[-3] == 'j') || (p[-3] == 'J'))
               &&  ((p[-4] == '.') || (p[-4] == '.'))) {
                    // Will convert to JPG if not.
                    return FORMAT_JPG;
        } else if (((p[-1] == 'p') || (p[-1] == 'P'))
               &&  ((p[-2] == 'm') || (p[-2] == 'M'))
               &&  ((p[-3] == 'b') || (p[-3] == 'B'))
               &&  ((p[-4] == '.') || (p[-4] == '.'))) {
                    if (IM_IS_JPEG(img) || IM_IS_BAYER(img)) {
                        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Image is not BMP!"));
                    }
                    return FORMAT_BMP;
        } else if (((p[-1] == 'm') || (p[-1] == 'M'))
               &&  ((p[-2] == 'p') || (p[-2] == 'P'))
               &&  ((p[-3] == 'p') || (p[-3] == 'P'))
               &&  ((p[-4] == '.') || (p[-4] == '.'))) {
                    if (!IM_IS_RGB565(img)) {
                        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Image is not PPM!"));
                    }
                    return FORMAT_PNM;
        } else if (((p[-1] == 'm') || (p[-1] == 'M'))
               &&  ((p[-2] == 'g') || (p[-2] == 'G'))
               &&  ((p[-3] == 'p') || (p[-3] == 'P'))
               &&  ((p[-4] == '.') || (p[-4] == '.'))) {
                    if (!IM_IS_GS(img)) {
                        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Image is not PGM!"));
                    }
                    return FORMAT_PNM;
        } else if (((p[-1] == 'w') || (p[-1] == 'W'))
               &&  ((p[-2] == 'a') || (p[-2] == 'A'))
               &&  ((p[-3] == 'r') || (p[-3] == 'R'))
               &&  ((p[-4] == '.') || (p[-4] == '.'))) {
                    if (!IM_IS_BAYER(img)) {
                        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Image is not BAYER!"));
                    }
                    return FORMAT_RAW;
        }

    }
    return FORMAT_DONT_CARE;
}

bool imlib_read_geometry(mp_obj_t fp, image_t *img, const char *path, img_read_settings_t *rs)
{
    file_read_open_raise(fp, path);
    char magic[2];
    read_data(fp, &magic, 2);
    file_close(fp);

    bool vflipped = false;
    if ((magic[0]=='P')
    && ((magic[1]=='2') || (magic[1]=='3')
    ||  (magic[1]=='5') || (magic[1]=='6'))) { // PPM
        rs->format = FORMAT_PNM;
        file_read_open_raise(fp, path);
        file_buffer_on(fp); // REMEMBER TO TURN THIS OFF LATER!
        ppm_read_geometry(fp, img, path, &rs->ppm_rs);
    } else if ((magic[0]=='B') && (magic[1]=='M')) { // BMP
        rs->format = FORMAT_BMP;
        file_read_open_raise(fp, path);
        file_buffer_on(fp); // REMEMBER TO TURN THIS OFF LATER!
        vflipped = bmp_read_geometry(fp, img, &rs->bmp_rs);
    } else {
        fs_unsupported_format(NULL);
    }
    imblib_parse_extension(img, path); // Enforce extension!
    return vflipped;
}

static void imlib_read_pixels(FIL *fp, image_t *img, int line_start, int line_end, img_read_settings_t *rs)
{
    switch (rs->format) {
        case FORMAT_BMP:
            bmp_read_pixels(fp, img, line_start, line_end, &rs->bmp_rs);
            break;
        case FORMAT_PNM:
            ppm_read_pixels(fp, img, line_start, line_end, &rs->ppm_rs);
            break;
        default: // won't happen
            break;
    }
}

void imlib_image_operation(image_t *img, const char *path, image_t *other, int scalar, line_op_t op, void *data)
{
    if (path) {
        uint32_t size = fb_avail() / 2;
        void *alloc = fb_alloc(size); // We have to do this before the read.
        // This code reads a window of an image in at a time and then executes
        // the line operation on each line in that window before moving to the
        // next window. The vflipped part is here because BMP files can be saved
        // vertically flipped resulting in us reading the image backwards.
        FIL fp;
        image_t temp;
        img_read_settings_t rs;
        bool vflipped = imlib_read_geometry(&fp, &temp, path, &rs);
        if (!IM_EQUAL(img, &temp)) {
            fs_not_equal(&fp);
        }
        // When processing vertically flipped images the read function will fill
        // the window up from the bottom. The read function assumes that the
        // window is equal to an image in size. However, since this is not the
        // case we shrink the window size to how many lines we're buffering.
        temp.pixels = alloc;
        temp.h = (size / (temp.w * temp.bpp)); // round down
        // This should never happen unless someone forgot to free.
        if ((!temp.pixels) || (!temp.h)) {
            nlr_raise(mp_obj_new_exception_msg(&mp_type_MemoryError,
                                               "Not enough memory available!"));
        }
        for (int i=0; i<img->h; i+=temp.h) { // goes past end
            int can_do = IM_MIN(temp.h, img->h-i);
            imlib_read_pixels(&fp, &temp, 0, can_do, &rs);
            for (int j=0; j<can_do; j++) {
                if (!vflipped) {
                    op(img, i+j, temp.pixels+(temp.w*temp.bpp*j), data, false);
                } else {
                    op(img, (img->h-i-can_do)+j, temp.pixels+(temp.w*temp.bpp*j), data, true);
                }
            }
        }
        file_buffer_off(&fp);
        file_close(&fp);
        fb_free();
    } else if (other) {
        if (!IM_EQUAL(img, other)) {
            fs_not_equal(NULL);
        }
        switch (img->bpp) {
            case IMAGE_BPP_BINARY: {
                for (int i=0, ii=img->h; i<ii; i++) {
                    op(img, i, IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(other, i), data, false);
                }
                break;
            }
            case IMAGE_BPP_GRAYSCALE: {
                for (int i=0, ii=img->h; i<ii; i++) {
                    op(img, i, IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(other, i), data, false);
                }
                break;
            }
            case IMAGE_BPP_RGB565: {
                for (int i=0, ii=img->h; i<ii; i++) {
                    op(img, i, IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(other, i), data, false);
                }
                break;
            }
            default: {
                break;
            }
        }
    } else {
        switch(img->bpp) {
            case IMAGE_BPP_BINARY: {
                uint32_t *row_ptr = fb_alloc(IMAGE_BINARY_LINE_LEN_BYTES(img));

                for (int i=0, ii=img->w; i<ii; i++) {
                    IMAGE_PUT_BINARY_PIXEL_FAST(row_ptr, i, scalar);
                }

                for (int i=0, ii=img->h; i<ii; i++) {
                    op(img, i, row_ptr, data, false);
                }

                fb_free();
                break;
            }
            case IMAGE_BPP_GRAYSCALE: {
                uint8_t *row_ptr = fb_alloc(IMAGE_GRAYSCALE_LINE_LEN_BYTES(img));

                for (int i=0, ii=img->w; i<ii; i++) {
                    IMAGE_PUT_GRAYSCALE_PIXEL_FAST(row_ptr, i, scalar);
                }

                for (int i=0, ii=img->h; i<ii; i++) {
                    op(img, i, row_ptr, data, false);
                }

                fb_free();
                break;
            }
            case IMAGE_BPP_RGB565: {
                uint16_t *row_ptr = fb_alloc(IMAGE_RGB565_LINE_LEN_BYTES(img));

                for (int i=0, ii=img->w; i<ii; i++) {
                    IMAGE_PUT_RGB565_PIXEL_FAST(row_ptr, i, scalar);
                }

                for (int i=0, ii=img->h; i<ii; i++) {
                    op(img, i, row_ptr, data, false);
                }

                fb_free();
                break;
            }
            default: {
                break;
            }
        }
    }
}

void imlib_load_image(image_t *img, const char *path, mp_obj_t file, uint8_t* buf, uint32_t buf_len)
{
    int err = 0;
    char magic[2];
    uint8_t data_type = 0; // 0: file path, 1: file obj, 2: buffer(bytes)

    if(path)
        data_type = 0;
    else if(file)
        data_type = 1;
    else if(buf)
        data_type = 2;
    else
        mp_raise_OSError(EINVAL);
    if(data_type == 0)
    {
        file = vfs_internal_open(path,"rb", &err);
        if(err != 0)
            mp_raise_OSError(err);
    }
    if(data_type == 2)
        memcpy(magic, buf, 2);
    else
    {
        vfs_internal_read(file, magic, 2, &err);
        if( err != 0)
            mp_raise_OSError(err);
        vfs_internal_seek(file,-2, SEEK_CUR, &err);
        if( err != 0)
        {
            int tmp = err;
            vfs_internal_close(file, &err);
            mp_raise_OSError(tmp);
        }
    }
    /*if ((magic[0]=='P')
    && ((magic[1]=='2') || (magic[1]=='3')
    ||  (magic[1]=='5') || (magic[1]=='6'))) { // PPM
        ppm_read(img, path);
    } else*/ if ((magic[0]=='B') && (magic[1]=='M')) { // BMP
        if(data_type != 0)
        {
            mp_raise_msg(&mp_type_OSError, "Not support");
        }
        bmp_read(img, path);
    } else if ((magic[0]==0xFF) && (magic[1]==0xD8)) { // JPEG
        // jpeg_read(img, path);
        if(data_type == 2)
        {
            err = picojpeg_util_read(img, NULL, buf, buf_len, MAIN_FB()->w_max, MAIN_FB()->h_max);
        }
        else
        {
            err = picojpeg_util_read(img, file, NULL, 0, MAIN_FB()->w_max, MAIN_FB()->h_max);
        }
        if(data_type != 2)
        {
            int tmp;
            if(data_type == 0)
                vfs_internal_close(file, &tmp);
            if( err != 0)
            {
                if(data_type == 1)
                    vfs_internal_close(file, &tmp);
                mp_raise_OSError(err);
            }
        }
    } else {
        int tmp;
        if(data_type == 0)
            vfs_internal_close(file, &tmp);
        mp_raise_ValueError("format not supported");
    }
    if(data_type == 0)
    {
        imblib_parse_extension(img, path); // Enforce extension!
    }
}

void imlib_save_image(image_t *img, const char *path, rectangle_t *roi, int quality)
{
    switch (imblib_parse_extension(img, path)) {
        //TODO: 
        case FORMAT_BMP:
            bmp_write_subimg(img, path, roi);
            break;
        // case FORMAT_PNM:
        //     ppm_write_subimg(img, path, roi);
        //     break;
        // case FORMAT_RAW: {
        //     FIL fp;
        //     file_write_open_raise(&fp, path);
        //     write_data(&fp, img->pixels, img->w * img->h);
        //     file_close(&fp);
        //     break;
        // }
        case FORMAT_JPG:
            jpeg_write(img, path, quality);
            break;
        // case FORMAT_DONT_CARE:
        //     // Path doesn't have an extension.
        //     if (IM_IS_JPEG(img)) {
        //         char *new_path = strcat(strcpy(fb_alloc(strlen(path)+5), path), ".jpg");
        //         jpeg_write(img, new_path, quality);
        //         fb_free();
        //     } else if (IM_IS_BAYER(img)) {
        //         FIL fp;
        //         char *new_path = strcat(strcpy(fb_alloc(strlen(path)+5), path), ".raw");
        //         file_write_open_raise(&fp, new_path);
        //         write_data(&fp, img->pixels, img->w * img->h);
        //         file_close(&fp);
        //         fb_free();
        //     } else { // RGB or GS, save as BMP.
        //         char *new_path = strcat(strcpy(fb_alloc(strlen(path)+5), path), ".bmp");
        //         bmp_write_subimg(img, new_path, roi);
        //         fb_free();
        //     }
        //     break;
        default:
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////

// A simple algorithm for correcting lens distortion.
// See http://www.tannerhelland.com/4743/simple-algorithm-correcting-lens-distortion/
void imlib_lens_corr(image_t *img, float strength, float zoom)
{
    zoom = 1.0f / zoom;
    int halfWidth = img->w / 2;
    int halfHeight = img->h / 2;
    float lens_corr_radius = strength / fast_sqrtf((img->w * img->w) + (img->h * img->h));

    switch(img->bpp) {
        case IMAGE_BPP_BINARY: {
            // Create a temp copy of the image to pull pixels from.
            uint32_t *tmp = fb_alloc(((img->w + UINT32_T_MASK) >> UINT32_T_SHIFT) * img->h);
            memcpy(tmp, img->data, ((img->w + UINT32_T_MASK) >> UINT32_T_SHIFT) * img->h);
            memset(img->data, 0, ((img->w + UINT32_T_MASK) >> UINT32_T_SHIFT) * img->h);

            for (int y = 0, yy = img->h; y < yy; y++) {
                uint32_t *row_ptr = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(img, y);
                int newY = y - halfHeight;
                int newY2 = newY * newY;
                float zoomedY = newY * zoom;

                for (int x = 0, xx = img->w; x < xx; x++) {
                    int newX = x - halfWidth;
                    int newX2 = newX * newX;
                    float zoomedX = newX * zoom;

                    float r = lens_corr_radius * fast_sqrtf(newX2 + newY2);
                    float theta = (r < 0.0000001f) ? 1.0f : (fast_atanf(r) / r);
                    int sourceX = halfWidth + fast_roundf(theta * zoomedX);
                    int sourceY = halfHeight + fast_roundf(theta * zoomedY);

                    if ((0 <= sourceX) && (sourceX < img->w) && (0 <= sourceY) && (sourceY < img->h)) {
                        uint32_t *ptr = tmp + (((img->w + UINT32_T_MASK) >> UINT32_T_SHIFT) * sourceY);
                        int pixel = IMAGE_GET_BINARY_PIXEL_FAST(ptr, sourceX);
                        IMAGE_PUT_BINARY_PIXEL_FAST(row_ptr, x, pixel);
                    }
                }
            }

            fb_free();
            break;
        }
        case IMAGE_BPP_GRAYSCALE: {
            // Create a temp copy of the image to pull pixels from.
            uint8_t *tmp = fb_alloc(img->w * img->h * sizeof(uint8_t));
            memcpy(tmp, img->data, img->w * img->h * sizeof(uint8_t));
            memset(img->data, 0, img->w * img->h * sizeof(uint8_t));

            for (int y = 0, yy = img->h; y < yy; y++) {
                uint8_t *row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, y);
                int newY = y - halfHeight;
                int newY2 = newY * newY;
                float zoomedY = newY * zoom;

                for (int x = 0, xx = img->w; x < xx; x++) {
                    int newX = x - halfWidth;
                    int newX2 = newX * newX;
                    float zoomedX = newX * zoom;

                    float r = lens_corr_radius * fast_sqrtf(newX2 + newY2);
                    float theta = (r < 0.0000001f) ? 1.0f : (fast_atanf(r) / r);
                    int sourceX = halfWidth + fast_roundf(theta * zoomedX);
                    int sourceY = halfHeight + fast_roundf(theta * zoomedY);

                    if ((0 <= sourceX) && (sourceX < img->w) && (0 <= sourceY) && (sourceY < img->h)) {
                        uint8_t *ptr = tmp + (img->w * sourceY);
                        int pixel = IMAGE_GET_GRAYSCALE_PIXEL_FAST(ptr, sourceX);
                        IMAGE_PUT_GRAYSCALE_PIXEL_FAST(row_ptr, x, pixel);
                    }
                }
            }

            fb_free();
            break;
        }
        case IMAGE_BPP_RGB565: {
            // Create a temp copy of the image to pull pixels from.
            uint16_t *tmp = fb_alloc(img->w * img->h * sizeof(uint16_t));
            memcpy(tmp, img->data, img->w * img->h * sizeof(uint16_t));
            memset(img->data, 0, img->w * img->h * sizeof(uint16_t));

            for (int y = 0, yy = img->h; y < yy; y++) {
                uint16_t *row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(img, y);
                int newY = y - halfHeight;
                int newY2 = newY * newY;
                float zoomedY = newY * zoom;

                for (int x = 0, xx = img->w; x < xx; x++) {
                    int newX = x - halfWidth;
                    int newX2 = newX * newX;
                    float zoomedX = newX * zoom;

                    float r = lens_corr_radius * fast_sqrtf(newX2 + newY2);
                    float theta = (r < 0.0000001f) ? 1.0f : (fast_atanf(r) / r);
                    int sourceX = halfWidth + fast_roundf(theta * zoomedX);
                    int sourceY = halfHeight + fast_roundf(theta * zoomedY);

                    if ((0 <= sourceX) && (sourceX < img->w) && (0 <= sourceY) && (sourceY < img->h)) {
                        uint16_t *ptr = tmp + (img->w * sourceY);
                        int pixel = IMAGE_GET_RGB565_PIXEL_FAST(ptr, sourceX);
                        IMAGE_PUT_RGB565_PIXEL_FAST(row_ptr, x, pixel);
                    }
                }
            }

            fb_free();
            break;
        }
        default: {
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

int imlib_image_mean(image_t *src, int *r_mean, int *g_mean, int *b_mean)
{
    int r_s = 0;
    int g_s = 0;
    int b_s = 0;
    int n = src->w * src->h;

    switch(src->bpp) {
        case IMAGE_BPP_BINARY: {
            // Can't run this on a binary image.
            break;
        }
        case IMAGE_BPP_GRAYSCALE: {
            for (int i=0; i<n; i++) {
                r_s += src->pixels[i];
            }
            *r_mean = r_s/n;
            *g_mean = r_s/n;
            *b_mean = r_s/n;
            break;
        }
        case IMAGE_BPP_RGB565: {
            for (int i=0; i<n; i++) {
                uint16_t p = ((uint16_t*)src->pixels)[i];
                r_s += COLOR_RGB565_TO_R8(p);
                g_s += COLOR_RGB565_TO_G8(p);
                b_s += COLOR_RGB565_TO_B8(p);
            }
            *r_mean = r_s/n;
            *g_mean = g_s/n;
            *b_mean = b_s/n;
            break;
        }
        default: {
            break;
        }
    }

    return 0;
}

// One pass standard deviation.
int imlib_image_std(image_t *src)
{
    int w=src->w;
    int h=src->h;
    int n=w*h;
    uint8_t *data=src->pixels;

    uint32_t s=0, sq=0;
    for (int i=0; i<n; i+=2) {
        s += data[i+0]+data[i+1];
		sq += data[i+0]*data[i+0] + data[i+1]*data[i+1];  //TODO
    }

    if (n%2) {
        s += data[n-1];
        sq += data[n-1]*data[n-1];
    }

    /* mean */
    int m = s/n;

    /* variance */
    uint32_t v = sq/n-(m*m);

    /* std */
    return fast_sqrtf(v);
}

static volatile uint8_t _ai_done_flag;
static int kpu_done(void *ctx)
{
    _ai_done_flag = 1;
    return 0;
}

void imlib_sepconv3(image_t *img, const int8_t *krn, const float m, const int b)
{
	float krn_f[9];
	kpu_task_t task;
	
	_ai_done_flag = 0;
	krn_f[0]=(float)(krn[0]);krn_f[1]=(float)(krn[1]);krn_f[2]=(float)(krn[2]);
	krn_f[3]=(float)(krn[3]);krn_f[4]=(float)(krn[4]);krn_f[5]=(float)(krn[5]);
	krn_f[6]=(float)(krn[6]);krn_f[7]=(float)(krn[7]);krn_f[8]=(float)(krn[8]);
	sipeed_conv_init(&task, img->w, img->h, 1, 1, krn_f);
	sipeed_conv_run(&task, img->pixels, img->pixels, kpu_done);
	while(!_ai_done_flag);
    _ai_done_flag=0;
	return;
}
