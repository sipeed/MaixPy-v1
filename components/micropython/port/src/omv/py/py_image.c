/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Image Python module.
 *
 */

// #include <arm_math.h>
#include <mp.h>
#include "imlib.h"
#include "array.h"
#include "sensor.h"
#include "vfs_wrapper.h"
#include "xalloc.h"
#include "fb_alloc.h"
#include "framebuffer.h"
#include "py_assert.h"
#include "py_helper.h"
#include "py_image.h"
#include "omv_boardconfig.h"
#include "py/runtime0.h"
#include "py/runtime.h"
#include "py/objstr.h"
#include "py/objarray.h"
#include "py/binary.h"
//#include "sipeed_sys.h"

#include "font.h"

static const mp_obj_type_t py_image_type;

//extern const char *ffs_strerror(FRESULT res);
extern uint32_t systick_current_millis(void);

#if CONFIG_MAIXPY_OMV_DOUBLE_BUFF
extern volatile uint8_t g_sensor_buff_index_out;
#endif

#ifndef OMV_MINIMUM

static const mp_obj_type_t py_cascade_type;

// Haar Cascade ///////////////////////////////////////////////////////////////

typedef struct _py_cascade_obj_t {
    mp_obj_base_t base;
    struct cascade _cobj;
} py_cascade_obj_t;

void *py_cascade_cobj(mp_obj_t cascade)
{
    PY_ASSERT_TYPE(cascade, &py_cascade_type);
    return &((py_cascade_obj_t *)cascade)->_cobj;
}

static void py_cascade_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_cascade_obj_t *self = self_in;
    mp_printf(print, "{\"width\":%d, \"height\":%d, \"n_stages\":%d, \"n_features\":%d, \"n_rectangles\":%d}",
            self->_cobj.window.w, self->_cobj.window.h, self->_cobj.n_stages,
            self->_cobj.n_features, self->_cobj.n_rectangles);
}

static const mp_obj_type_t py_cascade_type = {
    { &mp_type_type },
    .name  = MP_QSTR_Cascade,
    .print = py_cascade_print,
};

// Keypoints object ///////////////////////////////////////////////////////////

typedef struct _py_kp_obj_t {
    mp_obj_base_t base;
    array_t *kpts;
    int threshold;
    bool normalized;
} py_kp_obj_t;

static void py_kp_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_kp_obj_t *self = self_in;
    mp_printf(print, "{\"size\":%d, \"threshold\":%d, \"normalized\":%d}", array_length(self->kpts), self->threshold, self->normalized);
}

mp_obj_t py_kp_unary_op(mp_unary_op_t op, mp_obj_t self_in){
    py_kp_obj_t *self = MP_OBJ_TO_PTR(self_in);
    switch ((mp_uint_t)op) {
        case MP_UNARY_OP_LEN:
            return MP_OBJ_NEW_SMALL_INT(array_length(self->kpts));

        default:
            return MP_OBJ_NULL; // op not supported
    }
}

static mp_obj_t py_kp_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    if (value == MP_OBJ_SENTINEL) { // load
        py_kp_obj_t *self = self_in;
        int size = array_length(self->kpts);
        int i = mp_get_index(self->base.type, size, index, false);
        kp_t *kp = array_at(self->kpts, i);
        return mp_obj_new_tuple(5, (mp_obj_t []) {mp_obj_new_int(kp->x),
                                                  mp_obj_new_int(kp->y),
                                                  mp_obj_new_int(kp->score),
                                                  mp_obj_new_int(kp->octave),
                                                  mp_obj_new_int(kp->angle)});
    }

    return MP_OBJ_NULL; // op not supported
}

static const mp_obj_type_t py_kp_type = {
    { &mp_type_type },
    .name  = MP_QSTR_kp_desc,
    .print = py_kp_print,
    .subscr = py_kp_subscr,
    .unary_op = py_kp_unary_op,
};

py_kp_obj_t *py_kpts_obj(mp_obj_t kpts_obj)
{
    PY_ASSERT_TYPE(kpts_obj, &py_kp_type);
    return kpts_obj;
}

// LBP descriptor /////////////////////////////////////////////////////////////

typedef struct _py_lbp_obj_t {
    mp_obj_base_t base;
    uint8_t *hist;
} py_lbp_obj_t;

static void py_lbp_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    mp_printf(print, "{}");
}

static const mp_obj_type_t py_lbp_type = {
    { &mp_type_type },
    .name  = MP_QSTR_lbp_desc,
    .print = py_lbp_print,
};

// Keypoints match object /////////////////////////////////////////////////////
#define kptmatch_obj_size 9
typedef struct _py_kptmatch_obj_t {
    mp_obj_base_t base;
    mp_obj_t cx, cy;
    mp_obj_t x, y, w, h;
    mp_obj_t count;
    mp_obj_t theta;
    mp_obj_t match;
} py_kptmatch_obj_t;

static void py_kptmatch_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_kptmatch_obj_t *self = self_in;
    mp_printf(print, "{\"cx\":%d, \"cy\":%d, \"x\":%d, \"y\":%d, \"w\":%d, \"h\":%d, \"count\":%d, \"theta\":%d}",
              mp_obj_get_int(self->cx), mp_obj_get_int(self->cy), mp_obj_get_int(self->x), mp_obj_get_int(self->y),
              mp_obj_get_int(self->w),  mp_obj_get_int(self->h),  mp_obj_get_int(self->count), mp_obj_get_int(self->theta));
}

static mp_obj_t py_kptmatch_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    if (value == MP_OBJ_SENTINEL) { // load
        py_kptmatch_obj_t *self = self_in;
        if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
            mp_bound_slice_t slice;
            if (!mp_seq_get_fast_slice_indexes(kptmatch_obj_size, index, &slice)) {
                nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
            }
            mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
            mp_seq_copy(result->items, &(self->x) + slice.start, result->len, mp_obj_t);
            return result;
        }
        switch (mp_get_index(self->base.type, kptmatch_obj_size, index, false)) {
            case 0: return self->cx;
            case 1: return self->cy;
            case 2: return self->x;
            case 3: return self->y;
            case 4: return self->w;
            case 5: return self->h;
            case 6: return self->count;
            case 7: return self->theta;
            case 8: return self->match;
        }
    }
    return MP_OBJ_NULL; // op not supported
}

mp_obj_t py_kptmatch_cx(mp_obj_t self_in)     { return ((py_kptmatch_obj_t *) self_in)->cx;    }
mp_obj_t py_kptmatch_cy(mp_obj_t self_in)     { return ((py_kptmatch_obj_t *) self_in)->cy;    }
mp_obj_t py_kptmatch_x (mp_obj_t self_in)     { return ((py_kptmatch_obj_t *) self_in)->x;     }
mp_obj_t py_kptmatch_y (mp_obj_t self_in)     { return ((py_kptmatch_obj_t *) self_in)->y;     }
mp_obj_t py_kptmatch_w (mp_obj_t self_in)     { return ((py_kptmatch_obj_t *) self_in)->w;     }
mp_obj_t py_kptmatch_h (mp_obj_t self_in)     { return ((py_kptmatch_obj_t *) self_in)->h;     }
mp_obj_t py_kptmatch_count(mp_obj_t self_in)  { return ((py_kptmatch_obj_t *) self_in)->count; }
mp_obj_t py_kptmatch_theta(mp_obj_t self_in)  { return ((py_kptmatch_obj_t *) self_in)->theta; }
mp_obj_t py_kptmatch_match(mp_obj_t self_in)  { return ((py_kptmatch_obj_t *) self_in)->match; }
mp_obj_t py_kptmatch_rect(mp_obj_t  self_in)  {
    return mp_obj_new_tuple(4, (mp_obj_t []) {((py_kptmatch_obj_t *) self_in)->x,
                                              ((py_kptmatch_obj_t *) self_in)->y,
                                              ((py_kptmatch_obj_t *) self_in)->w,
                                              ((py_kptmatch_obj_t *) self_in)->h});
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_kptmatch_cx_obj,    py_kptmatch_cx);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_kptmatch_cy_obj,    py_kptmatch_cy);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_kptmatch_x_obj,     py_kptmatch_x);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_kptmatch_y_obj,     py_kptmatch_y);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_kptmatch_w_obj,     py_kptmatch_w);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_kptmatch_h_obj,     py_kptmatch_h);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_kptmatch_count_obj, py_kptmatch_count);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_kptmatch_theta_obj, py_kptmatch_theta);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_kptmatch_match_obj, py_kptmatch_match);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_kptmatch_rect_obj,  py_kptmatch_rect);

STATIC const mp_rom_map_elem_t py_kptmatch_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_cx),      MP_ROM_PTR(&py_kptmatch_cx_obj)      },
    { MP_ROM_QSTR(MP_QSTR_cy),      MP_ROM_PTR(&py_kptmatch_cy_obj)      },
    { MP_ROM_QSTR(MP_QSTR_x),       MP_ROM_PTR(&py_kptmatch_x_obj)       },
    { MP_ROM_QSTR(MP_QSTR_y),       MP_ROM_PTR(&py_kptmatch_y_obj)       },
    { MP_ROM_QSTR(MP_QSTR_w),       MP_ROM_PTR(&py_kptmatch_w_obj)       },
    { MP_ROM_QSTR(MP_QSTR_h),       MP_ROM_PTR(&py_kptmatch_h_obj)       },
    { MP_ROM_QSTR(MP_QSTR_count),   MP_ROM_PTR(&py_kptmatch_count_obj)   },
    { MP_ROM_QSTR(MP_QSTR_theta),   MP_ROM_PTR(&py_kptmatch_theta_obj)   },
    { MP_ROM_QSTR(MP_QSTR_match),   MP_ROM_PTR(&py_kptmatch_match_obj)   },
    { MP_ROM_QSTR(MP_QSTR_rect),    MP_ROM_PTR(&py_kptmatch_rect_obj)    }
};

STATIC MP_DEFINE_CONST_DICT(py_kptmatch_locals_dict, py_kptmatch_locals_dict_table);

static const mp_obj_type_t py_kptmatch_type = {
    { &mp_type_type },
    .name   = MP_QSTR_kptmatch,
    .print  = py_kptmatch_print,
    .subscr = py_kptmatch_subscr,
    .locals_dict = (mp_obj_t) &py_kptmatch_locals_dict
};

#endif //OMV_MINIMUM

// Image //////////////////////////////////////////////////////////////////////

typedef struct _py_image_obj_t {
    mp_obj_base_t base;
    image_t _cobj;
} py_image_obj_t MICROPY_OBJ_BASE_ALIGNMENT;

void *py_image_cobj(mp_obj_t img_obj)
{
    PY_ASSERT_TYPE(img_obj, &py_image_type);
    return &((py_image_obj_t *)img_obj)->_cobj;
}

bool py_image_obj_is_image(mp_obj_t obj)
{
    if( ((mp_obj_base_t*)obj)->type == &py_image_type )
        return true;
    return false;
}

static void py_image_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_image_obj_t *self = self_in;
    switch(self->_cobj.bpp) {
        case IMAGE_BPP_BINARY: {
            mp_printf(print, "{\"w\":%d, \"h\":%d, \"type\"=\"binary\", \"size\":%d}",
                      self->_cobj.w, self->_cobj.h,
                      ((self->_cobj.w + UINT32_T_MASK) >> UINT32_T_SHIFT) * self->_cobj.h);
            break;
        }
        case IMAGE_BPP_GRAYSCALE: {
            mp_printf(print, "{\"w\":%d, \"h\":%d, \"type\"=\"grayscale\", \"size\":%d}",
                      self->_cobj.w, self->_cobj.h,
                      (self->_cobj.w * self->_cobj.h) * sizeof(uint8_t));
            break;
        }
        case IMAGE_BPP_RGB565: {
            mp_printf(print, "{\"w\":%d, \"h\":%d, \"type\"=\"rgb565\", \"size\":%d}",
                      self->_cobj.w, self->_cobj.h,
                      (self->_cobj.w * self->_cobj.h) * sizeof(uint16_t));
            break;
        }
        default: {
            if((self->_cobj.data[0] == 0xFE) && (self->_cobj.data[self->_cobj.bpp-1] == 0xFE)) { // for ide
                print->print_strn(print->data, (const char *) self->_cobj.data, self->_cobj.bpp);
            } else { // not for ide
                mp_printf(print, "{\"w\":%d, \"h\":%d, \"type\"=\"jpeg\", \"size\":%d}",
                          self->_cobj.w, self->_cobj.h,
                          self->_cobj.bpp);
            }
            break;
        }
    }
}

static mp_obj_t py_image_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    py_image_obj_t *self = self_in;
    if (value == MP_OBJ_NULL) { // delete
    } else if (value == MP_OBJ_SENTINEL) { // load
        switch (self->_cobj.bpp) {
            case IMAGE_BPP_BINARY: {
                if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
                    mp_bound_slice_t slice;
                    if (!mp_seq_get_fast_slice_indexes(self->_cobj.w * self->_cobj.h, index, &slice)) {
                        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
                    }
                    mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
                    for (mp_uint_t i = 0; i < result->len; i++) {
                        result->items[i] = mp_obj_new_int(IMAGE_GET_BINARY_PIXEL(&(self->_cobj), (slice.start + i) % self->_cobj.w, (slice.start + i) / self->_cobj.w));
                    }
                    return result;
                }
                mp_uint_t i = mp_get_index(self->base.type, self->_cobj.w * self->_cobj.h, index, false);
                return mp_obj_new_int(IMAGE_GET_BINARY_PIXEL(&(self->_cobj), i % self->_cobj.w, i / self->_cobj.w));
            }
            case IMAGE_BPP_BAYER:
            case IMAGE_BPP_GRAYSCALE: {
                if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
                    mp_bound_slice_t slice;
                    if (!mp_seq_get_fast_slice_indexes(self->_cobj.w * self->_cobj.h, index, &slice)) {
                        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
                    }
                    mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
                    for (mp_uint_t i = 0; i < result->len; i++) {
                        uint8_t p = IMAGE_GET_GRAYSCALE_PIXEL(&(self->_cobj), (slice.start + i) % self->_cobj.w, (slice.start + i) / self->_cobj.w);
                        result->items[i] = mp_obj_new_int(p);
                    }
                    return result;
                }
                mp_uint_t i = mp_get_index(self->base.type, self->_cobj.w * self->_cobj.h, index, false);
                uint8_t p = IMAGE_GET_GRAYSCALE_PIXEL(&(self->_cobj), i % self->_cobj.w, i / self->_cobj.w);
                return mp_obj_new_int(p);
            }
            case IMAGE_BPP_RGB565: {
                if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
                    mp_bound_slice_t slice;
                    if (!mp_seq_get_fast_slice_indexes(self->_cobj.w * self->_cobj.h, index, &slice)) {
                        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
                    }
                    mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
                    for (mp_uint_t i = 0; i < result->len; i++) {
                        uint16_t p = IMAGE_GET_RGB565_PIXEL(&(self->_cobj), (slice.start + i) % self->_cobj.w, (slice.start + i) / self->_cobj.w);
                        result->items[i] = mp_obj_new_tuple(3, (mp_obj_t []) {mp_obj_new_int(COLOR_RGB565_TO_R8(p)),
                                                                              mp_obj_new_int(COLOR_RGB565_TO_G8(p)),
                                                                              mp_obj_new_int(COLOR_RGB565_TO_B8(p))});
                    }
                    return result;
                }
                mp_uint_t i = mp_get_index(self->base.type, self->_cobj.w * self->_cobj.h, index, false);
                uint16_t p = IMAGE_GET_RGB565_PIXEL(&(self->_cobj), i % self->_cobj.w, i / self->_cobj.w);
                return mp_obj_new_tuple(3, (mp_obj_t []) {mp_obj_new_int(COLOR_RGB565_TO_R8(p)),
                                                          mp_obj_new_int(COLOR_RGB565_TO_G8(p)),
                                                          mp_obj_new_int(COLOR_RGB565_TO_B8(p))});
            }
            default: {
                if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
                    mp_bound_slice_t slice;
                    if (!mp_seq_get_fast_slice_indexes(self->_cobj.bpp, index, &slice)) {
                        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
                    }
                    mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
                    for (mp_uint_t i = 0; i < result->len; i++) {
                        result->items[i] = mp_obj_new_int(self->_cobj.data[slice.start + i]);
                    }
                    return result;
                }
                mp_uint_t i = mp_get_index(self->base.type, self->_cobj.bpp, index, false);
                return mp_obj_new_int(self->_cobj.data[i]);
            }
        }
    } else { // store
        switch (self->_cobj.bpp) {
            case IMAGE_BPP_BINARY: {
                if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
                    mp_bound_slice_t slice;
                    if (!mp_seq_get_fast_slice_indexes(self->_cobj.w * self->_cobj.h, index, &slice)) {
                        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
                    }
                    if (MP_OBJ_IS_TYPE(value, &mp_type_list)) {
                        mp_uint_t value_l_len;
                        mp_obj_t *value_l;
                        mp_obj_get_array(value, &value_l_len, &value_l);
                        PY_ASSERT_TRUE_MSG(value_l_len == (slice.stop - slice.start), "cannot grow or shrink image");
                        for (mp_uint_t i = 0; i < (slice.stop - slice.start); i++) {
                            IMAGE_PUT_BINARY_PIXEL(&(self->_cobj), (slice.start + i) % self->_cobj.w, (slice.start + i) / self->_cobj.w, mp_obj_get_int(value_l[i]));
                        }
                    } else {
                        mp_int_t v = mp_obj_get_int(value);
                        for (mp_uint_t i = 0; i < (slice.stop - slice.start); i++) {
                            IMAGE_PUT_BINARY_PIXEL(&(self->_cobj), (slice.start + i) % self->_cobj.w, (slice.start + i) / self->_cobj.w, v);
                        }
                    }
                    return mp_const_none;
                }
                mp_uint_t i = mp_get_index(self->base.type, self->_cobj.w * self->_cobj.h, index, false);
                IMAGE_PUT_BINARY_PIXEL(&(self->_cobj), i % self->_cobj.w, i / self->_cobj.w, mp_obj_get_int(value));
                return mp_const_none;
            }
            case IMAGE_BPP_BAYER:
            case IMAGE_BPP_GRAYSCALE: {
                if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
                    mp_bound_slice_t slice;
                    if (!mp_seq_get_fast_slice_indexes(self->_cobj.w * self->_cobj.h, index, &slice)) {
                        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
                    }
                    if (MP_OBJ_IS_TYPE(value, &mp_type_list)) {
                        mp_uint_t value_l_len;
                        mp_obj_t *value_l;
                        mp_obj_get_array(value, &value_l_len, &value_l);
                        PY_ASSERT_TRUE_MSG(value_l_len == (slice.stop - slice.start), "cannot grow or shrink image");
                        for (mp_uint_t i = 0; i < (slice.stop - slice.start); i++) {
                            uint8_t p = mp_obj_get_int(value_l[i]);
                            IMAGE_PUT_GRAYSCALE_PIXEL(&(self->_cobj), (slice.start + i) % self->_cobj.w, (slice.start + i) / self->_cobj.w, p);
                        }
                    } else {
                        uint8_t p = mp_obj_get_int(value);
                        for (mp_uint_t i = 0; i < (slice.stop - slice.start); i++) {
                            IMAGE_PUT_GRAYSCALE_PIXEL(&(self->_cobj), (slice.start + i) % self->_cobj.w, (slice.start + i) / self->_cobj.w, p);
                        }
                    }
                    return mp_const_none;
                }
                mp_uint_t i = mp_get_index(self->base.type, self->_cobj.w * self->_cobj.h, index, false);
                uint8_t p = mp_obj_get_int(value);
                IMAGE_PUT_GRAYSCALE_PIXEL(&(self->_cobj), i % self->_cobj.w, i / self->_cobj.w, p);
                return mp_const_none;
            }
            case IMAGE_BPP_RGB565: {
                if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
                    mp_bound_slice_t slice;
                    if (!mp_seq_get_fast_slice_indexes(self->_cobj.w * self->_cobj.h, index, &slice)) {
                        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
                    }
                    if (MP_OBJ_IS_TYPE(value, &mp_type_list)) {
                        mp_uint_t value_l_len;
                        mp_obj_t *value_l;
                        mp_obj_get_array(value, &value_l_len, &value_l);
                        PY_ASSERT_TRUE_MSG(value_l_len == (slice.stop - slice.start), "cannot grow or shrink image");
                        for (mp_uint_t i = 0; i < (slice.stop - slice.start); i++) {
                            mp_obj_t *value_2;
                            mp_obj_get_array_fixed_n(value_l[i], 3, &value_2);
                            uint16_t p = COLOR_R8_G8_B8_TO_RGB565(mp_obj_get_int(value_2[0]), mp_obj_get_int(value_2[1]), mp_obj_get_int(value_2[2]));
                            IMAGE_PUT_RGB565_PIXEL(&(self->_cobj), (slice.start + i) % self->_cobj.w, (slice.start + i) / self->_cobj.w, p);
                        }
                    } else {
                        mp_obj_t *value_2;
                        mp_obj_get_array_fixed_n(value, 3, &value_2);
                        uint16_t p = COLOR_R8_G8_B8_TO_RGB565(mp_obj_get_int(value_2[0]), mp_obj_get_int(value_2[1]), mp_obj_get_int(value_2[2]));
                        for (mp_uint_t i = 0; i < (slice.stop - slice.start); i++) {
                            IMAGE_PUT_RGB565_PIXEL(&(self->_cobj), (slice.start + i) % self->_cobj.w, (slice.start + i) / self->_cobj.w, p);
                        }
                    }
                    return mp_const_none;
                }
                mp_uint_t i = mp_get_index(self->base.type, self->_cobj.w * self->_cobj.h, index, false);
                mp_obj_t *value_2;
                mp_obj_get_array_fixed_n(value, 3, &value_2);
                uint16_t p = COLOR_R8_G8_B8_TO_RGB565(mp_obj_get_int(value_2[0]), mp_obj_get_int(value_2[1]), mp_obj_get_int(value_2[2]));
                IMAGE_PUT_RGB565_PIXEL(&(self->_cobj), i % self->_cobj.w, i / self->_cobj.w, p);
                return mp_const_none;
            }
            default: {
                if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
                    mp_bound_slice_t slice;
                    if (!mp_seq_get_fast_slice_indexes(self->_cobj.bpp, index, &slice)) {
                        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
                    }
                    if (MP_OBJ_IS_TYPE(value, &mp_type_list)) {
                        mp_uint_t value_l_len;
                        mp_obj_t *value_l;
                        mp_obj_get_array(value, &value_l_len, &value_l);
                        PY_ASSERT_TRUE_MSG(value_l_len == (slice.stop - slice.start), "cannot grow or shrink image");
                        for (mp_uint_t i = 0; i < (slice.stop - slice.start); i++) {
                            self->_cobj.data[slice.start + i] = mp_obj_get_int(value_l[i]);
                        }
                    } else {
                        mp_int_t v = mp_obj_get_int(value);
                        for (mp_uint_t i = 0; i < (slice.stop - slice.start); i++) {
                            self->_cobj.data[slice.start + i] = v;
                        }
                    }
                    return mp_const_none;
                }
                mp_uint_t i = mp_get_index(self->base.type, self->_cobj.bpp, index, false);
                self->_cobj.data[i] = mp_obj_get_int(value);
                return mp_const_none;
            }
        }
    }
    return MP_OBJ_NULL; // op not supported
}

static mp_int_t py_image_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags)
{
    py_image_obj_t *self = self_in;
    if (flags == MP_BUFFER_READ) {
        bufinfo->buf = self->_cobj.data;
        bufinfo->len = image_size(&self->_cobj);
        bufinfo->typecode = 'b';
        return 0;
    } else { // Can't write to an image!
        bufinfo->buf = NULL;
        bufinfo->len = 0;
        bufinfo->typecode = -1;
        return 1;
    }
}

// ////////////////
// // Basic Methods
// ////////////////

static mp_obj_t py_image_del(mp_obj_t img_obj)
{
    //TODO: optimize or delete this function
    image_t* img = (image_t *) py_image_cobj(img_obj);
    if( img->data )
    {
        if(is_img_data_in_main_fb(img->data))
        {
           return mp_const_none;
        }
        xfree(img->data);
        img->data = NULL;
    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_image_del_obj, py_image_del);

static mp_obj_t py_image_width(mp_obj_t img_obj)
{
    return mp_obj_new_int(((image_t *) py_image_cobj(img_obj))->w);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_image_width_obj, py_image_width);

static mp_obj_t py_image_height(mp_obj_t img_obj)
{
    return mp_obj_new_int(((image_t *) py_image_cobj(img_obj))->h);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_image_height_obj, py_image_height);

static mp_obj_t py_image_format(mp_obj_t img_obj)
{
    switch (((image_t *) py_image_cobj(img_obj))->bpp) {
        case IMAGE_BPP_BINARY: return mp_const_none; // TODO: FIXME!
        case IMAGE_BPP_GRAYSCALE: return mp_obj_new_int(PIXFORMAT_GRAYSCALE);
        case IMAGE_BPP_RGB565: return mp_obj_new_int(PIXFORMAT_RGB565);
        case IMAGE_BPP_BAYER: return mp_obj_new_int(PIXFORMAT_BAYER);
        default: return mp_obj_new_int(PIXFORMAT_JPEG);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_image_format_obj, py_image_format);

static mp_obj_t py_image_size(mp_obj_t img_obj)
{
    return mp_obj_new_int(image_size((image_t *) py_image_cobj(img_obj)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_image_size_obj, py_image_size);

STATIC mp_obj_t py_image_get_pixel(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable_bayer(args[0]);

    const mp_obj_t *arg_vec;
    uint offset = py_helper_consume_array(n_args, args, 1, 2, &arg_vec);
    int arg_x = mp_obj_get_int(arg_vec[0]);
    int arg_y = mp_obj_get_int(arg_vec[1]);

    bool arg_rgbtuple =
        py_helper_keyword_int(n_args, args, offset, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_rgbtuple), arg_img->bpp == IMAGE_BPP_RGB565);

    if ((!IM_X_INSIDE(arg_img, arg_x)) || (!IM_Y_INSIDE(arg_img, arg_y))) {
        return mp_const_none;
    }

    switch (arg_img->bpp) {
        case IMAGE_BPP_BINARY: {
            if (arg_rgbtuple) {
                int pixel = IMAGE_GET_BINARY_PIXEL(arg_img, arg_x, arg_y);
                mp_obj_t pixel_tuple[3];
                pixel_tuple[0] = mp_obj_new_int(COLOR_RGB565_TO_R8(COLOR_BINARY_TO_RGB565(pixel)));
                pixel_tuple[1] = mp_obj_new_int(COLOR_RGB565_TO_G8(COLOR_BINARY_TO_RGB565(pixel)));
                pixel_tuple[2] = mp_obj_new_int(COLOR_RGB565_TO_B8(COLOR_BINARY_TO_RGB565(pixel)));
                return mp_obj_new_tuple(3, pixel_tuple);
            } else {
                return mp_obj_new_int(IMAGE_GET_BINARY_PIXEL(arg_img, arg_x, arg_y));
            }
        }
        case IMAGE_BPP_GRAYSCALE: {
            if (arg_rgbtuple) {
                int pixel = IMAGE_GET_GRAYSCALE_PIXEL(arg_img, arg_x, arg_y);
                mp_obj_t pixel_tuple[3];
                pixel_tuple[0] = mp_obj_new_int(COLOR_RGB565_TO_R8(COLOR_GRAYSCALE_TO_RGB565(pixel)));
                pixel_tuple[1] = mp_obj_new_int(COLOR_RGB565_TO_G8(COLOR_GRAYSCALE_TO_RGB565(pixel)));
                pixel_tuple[2] = mp_obj_new_int(COLOR_RGB565_TO_B8(COLOR_GRAYSCALE_TO_RGB565(pixel)));
                return mp_obj_new_tuple(3, pixel_tuple);
            } else {
                return mp_obj_new_int(IMAGE_GET_GRAYSCALE_PIXEL(arg_img, arg_x, arg_y));
            }
        }
        case IMAGE_BPP_RGB565: {
            if (arg_rgbtuple) {
                int pixel = IMAGE_GET_RGB565_PIXEL(arg_img, arg_x, arg_y);
                mp_obj_t pixel_tuple[3];
                pixel_tuple[0] = mp_obj_new_int(COLOR_R5_TO_R8(COLOR_RGB565_TO_R5(pixel)));
                pixel_tuple[1] = mp_obj_new_int(COLOR_G6_TO_G8(COLOR_RGB565_TO_G6(pixel)));
                pixel_tuple[2] = mp_obj_new_int(COLOR_B5_TO_B8(COLOR_RGB565_TO_B5(pixel)));
                return mp_obj_new_tuple(3, pixel_tuple);
            } else {
                return mp_obj_new_int(IMAGE_GET_RGB565_PIXEL(arg_img, arg_x, arg_y));
            }
        }
        case IMAGE_BPP_BAYER:
            if (arg_rgbtuple) {
                int pixel = IMAGE_GET_GRAYSCALE_PIXEL(arg_img, arg_x, arg_y); // Correct!
                mp_obj_t pixel_tuple[3];
                pixel_tuple[0] = mp_obj_new_int(COLOR_RGB565_TO_R8(COLOR_GRAYSCALE_TO_RGB565(pixel)));
                pixel_tuple[1] = mp_obj_new_int(COLOR_RGB565_TO_G8(COLOR_GRAYSCALE_TO_RGB565(pixel)));
                pixel_tuple[2] = mp_obj_new_int(COLOR_RGB565_TO_B8(COLOR_GRAYSCALE_TO_RGB565(pixel)));
                return mp_obj_new_tuple(3, pixel_tuple);
            } else {
                return mp_obj_new_int(IMAGE_GET_GRAYSCALE_PIXEL(arg_img, arg_x, arg_y)); // Correct!
            }
        default: return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_get_pixel_obj, 2, py_image_get_pixel);

STATIC mp_obj_t py_image_set_pixel(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable_bayer(args[0]);

    const mp_obj_t *arg_vec;
    uint offset = py_helper_consume_array(n_args, args, 1, 2, &arg_vec);
    int arg_x = mp_obj_get_int(arg_vec[0]);
    int arg_y = mp_obj_get_int(arg_vec[1]);

    int arg_c =
        py_helper_keyword_color(arg_img, n_args, args, offset, kw_args, -1); // White.

    if ((!IM_X_INSIDE(arg_img, arg_x)) || (!IM_Y_INSIDE(arg_img, arg_y))) {
        return args[0];
    }

    switch (arg_img->bpp) {
        case IMAGE_BPP_BINARY: {
            IMAGE_PUT_BINARY_PIXEL(arg_img, arg_x, arg_y, arg_c);
            return args[0];
        }
        case IMAGE_BPP_GRAYSCALE: {
            IMAGE_PUT_GRAYSCALE_PIXEL(arg_img, arg_x, arg_y, arg_c);
            return args[0];
        }
        case IMAGE_BPP_RGB565: {
            IMAGE_PUT_RGB565_PIXEL(arg_img, arg_x, arg_y, arg_c);
            return args[0];
        }
        case IMAGE_BPP_BAYER: {
            IMAGE_PUT_GRAYSCALE_PIXEL(arg_img, arg_x, arg_y, arg_c); // Correct!
            return args[0];
        }
        default: return args[0];
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_set_pixel_obj, 2, py_image_set_pixel);

static mp_obj_t py_image_mean_pool(mp_obj_t img_obj, mp_obj_t x_div_obj, mp_obj_t y_div_obj)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(img_obj);

    int x_div = mp_obj_get_int(x_div_obj);
    PY_ASSERT_TRUE_MSG(x_div >= 1, "Width divisor must be greater than >= 1");
    PY_ASSERT_TRUE_MSG(x_div <= arg_img->w, "Width divisor must be less than <= img width");
    int y_div = mp_obj_get_int(y_div_obj);
    PY_ASSERT_TRUE_MSG(y_div >= 1, "Height divisor must be greater than >= 1");
    PY_ASSERT_TRUE_MSG(y_div <= arg_img->h, "Height divisor must be less than <= img height");

    image_t out_img;
    out_img.w = arg_img->w / x_div;
    out_img.h = arg_img->h / y_div;
    out_img.bpp = arg_img->bpp;
    out_img.pixels = arg_img->pixels;

    imlib_mean_pool(arg_img, &out_img, x_div, y_div);
    arg_img->w = out_img.w;
    arg_img->h = out_img.h;

    if(is_img_data_in_main_fb(arg_img->data))
    {
        MAIN_FB()->w = out_img.w;
        MAIN_FB()->h = out_img.h;
    }

    return img_obj;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(py_image_mean_pool_obj, py_image_mean_pool);

static mp_obj_t py_image_mean_pooled(mp_obj_t img_obj, mp_obj_t x_div_obj, mp_obj_t y_div_obj)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(img_obj);

    int x_div = mp_obj_get_int(x_div_obj);
    PY_ASSERT_TRUE_MSG(x_div >= 1, "Width divisor must be greater than >= 1");
    PY_ASSERT_TRUE_MSG(x_div <= arg_img->w, "Width divisor must be less than <= img width");
    int y_div = mp_obj_get_int(y_div_obj);
    PY_ASSERT_TRUE_MSG(y_div >= 1, "Height divisor must be greater than >= 1");
    PY_ASSERT_TRUE_MSG(y_div <= arg_img->h, "Height divisor must be less than <= img height");

    image_t out_img;
    out_img.w = arg_img->w / x_div;
    out_img.h = arg_img->h / y_div;
    out_img.bpp = arg_img->bpp;
    out_img.pixels = xalloc(image_size(&out_img));

    imlib_mean_pool(arg_img, &out_img, x_div, y_div);
    return py_image_from_struct(&out_img);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(py_image_mean_pooled_obj, py_image_mean_pooled);

static mp_obj_t py_image_midpoint_pool(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    int x_div = mp_obj_get_int(args[1]);
    PY_ASSERT_TRUE_MSG(x_div >= 1, "Width divisor must be greater than >= 1");
    PY_ASSERT_TRUE_MSG(x_div <= arg_img->w, "Width divisor must be less than <= img width");
    int y_div = mp_obj_get_int(args[2]);
    PY_ASSERT_TRUE_MSG(y_div >= 1, "Height divisor must be greater than >= 1");
    PY_ASSERT_TRUE_MSG(y_div <= arg_img->h, "Height divisor must be less than <= img height");

    int bias = IM_MAX(IM_MIN(py_helper_keyword_float(n_args, args, 3, kw_args,
                                                     MP_OBJ_NEW_QSTR(MP_QSTR_bias), 0.5) * 256, 256), 0);

    image_t out_img;
    out_img.w = arg_img->w / x_div;
    out_img.h = arg_img->h / y_div;
    out_img.bpp = arg_img->bpp;
    out_img.pixels = arg_img->pixels;

    imlib_midpoint_pool(arg_img, &out_img, x_div, y_div, bias);
    arg_img->w = out_img.w;
    arg_img->h = out_img.h;
    if(is_img_data_in_main_fb(arg_img->data))
    {
        MAIN_FB()->w = out_img.w;
        MAIN_FB()->h = out_img.h;
    }

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_midpoint_pool_obj, 3, py_image_midpoint_pool);

static mp_obj_t py_image_midpoint_pooled(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    int x_div = mp_obj_get_int(args[1]);
    PY_ASSERT_TRUE_MSG(x_div >= 1, "Width divisor must be greater than >= 1");
    PY_ASSERT_TRUE_MSG(x_div <= arg_img->w, "Width divisor must be less than <= img width");
    int y_div = mp_obj_get_int(args[2]);
    PY_ASSERT_TRUE_MSG(y_div >= 1, "Height divisor must be greater than >= 1");
    PY_ASSERT_TRUE_MSG(y_div <= arg_img->h, "Height divisor must be less than <= img height");

    int bias = IM_MAX(IM_MIN(py_helper_keyword_float(n_args, args, 3, kw_args,
                                                     MP_OBJ_NEW_QSTR(MP_QSTR_bias), 0.5) * 256, 256), 0);

    image_t out_img;
    out_img.w = arg_img->w / x_div;
    out_img.h = arg_img->h / y_div;
    out_img.bpp = arg_img->bpp;
    out_img.pixels = xalloc(image_size(&out_img));

    imlib_midpoint_pool(arg_img, &out_img, x_div, y_div, bias);
    return py_image_from_struct(&out_img);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_midpoint_pooled_obj, 3, py_image_midpoint_pooled);

static mp_obj_t py_image_to_bitmap(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);
    bool copy = py_helper_keyword_int(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_copy), false);

    image_t out;
    out.w = arg_img->w;
    out.h = arg_img->h;
    out.bpp = IMAGE_BPP_BINARY;
    out.data = copy ? xalloc(image_size(&out)) : arg_img->data;
    out.pix_ai = copy ? xalloc(out.w*out.h*3) : arg_img->pix_ai;

    switch(arg_img->bpp) {
        case IMAGE_BPP_BINARY: {
            if (copy) memcpy(out.data, arg_img->data, image_size(&out));
            break;
        }
        case IMAGE_BPP_GRAYSCALE: {
            PY_ASSERT_TRUE_MSG((out.w >= (sizeof(uint32_t)/sizeof(uint8_t))) || copy,
                               "Can't convert to bitmap in place!");
            fb_alloc_mark();
            uint32_t *out_lin_ptr = fb_alloc(IMAGE_BINARY_LINE_LEN_BYTES(&out));
            for (int y = 0, yy = out.h; y < yy; y++) {
                uint8_t *row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(arg_img, y);
                for (int x = 0, xx = out.w; x < xx; x++) {
                    IMAGE_PUT_BINARY_PIXEL_FAST(out_lin_ptr, x,
                        COLOR_GRAYSCALE_TO_BINARY(IMAGE_GET_GRAYSCALE_PIXEL_FAST(row_ptr, x)));
                }
                memcpy(IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(&out, y),
                       out_lin_ptr, IMAGE_BINARY_LINE_LEN_BYTES(&out));
            }
            fb_free();
            fb_alloc_free_till_mark();
            break;
        }
        case IMAGE_BPP_RGB565: {
            PY_ASSERT_TRUE_MSG((out.w >= (sizeof(uint32_t)/sizeof(uint16_t))) || copy,
                               "Can't convert to bitmap in place!");
            fb_alloc_mark();
            uint32_t *out_lin_ptr = fb_alloc(IMAGE_BINARY_LINE_LEN_BYTES(&out));
            for (int y = 0, yy = out.h; y < yy; y++) {
                uint16_t *row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(arg_img, y);
                for (int x = 0, xx = out.w; x < xx; x++) {
                    IMAGE_PUT_BINARY_PIXEL_FAST(out_lin_ptr, x,
                        COLOR_RGB565_TO_BINARY(IMAGE_GET_RGB565_PIXEL_FAST(row_ptr, x)));
                }
                memcpy(IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(&out, y),
                       out_lin_ptr, IMAGE_BINARY_LINE_LEN_BYTES(&out));
            }
            fb_free();
            fb_alloc_free_till_mark();
            break;
        }
        default: {
            break;
        }
    }

    if ((!copy) && is_img_data_in_main_fb(out.data)) {
        MAIN_FB()->bpp = out.bpp;
    }

    return py_image_from_struct(&out);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_to_bitmap_obj, 1, py_image_to_bitmap);

static mp_obj_t py_image_to_grayscale(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);
    bool copy = py_helper_keyword_int(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_copy), false);

    image_t out;
    out.w = arg_img->w;
    out.h = arg_img->h;
    out.bpp = IMAGE_BPP_GRAYSCALE;
    out.data = copy ? xalloc(image_size(&out)) : arg_img->data;
    out.pix_ai = copy ? xalloc(out.w*out.h*3) : arg_img->pix_ai;

    switch(arg_img->bpp) {
        case IMAGE_BPP_BINARY: {
            PY_ASSERT_TRUE_MSG((out.w == 1) || copy,
                               "Can't convert to grayscale in place!");
            for (int y = 0, yy = out.h; y < yy; y++) {
                uint32_t *row_ptr = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(arg_img, y);
                uint8_t *out_row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(&out, y);
                for (int x = 0, xx = out.w; x < xx; x++) {
                    IMAGE_PUT_GRAYSCALE_PIXEL_FAST(out_row_ptr, x,
                        COLOR_BINARY_TO_GRAYSCALE(IMAGE_GET_BINARY_PIXEL_FAST(row_ptr, x)));
                }
            }
            break;
        }
        case IMAGE_BPP_GRAYSCALE: {
            if (copy) memcpy(out.data, arg_img->data, image_size(&out));
            break;
        }
        case IMAGE_BPP_RGB565: {
            for (int y = 0, yy = out.h; y < yy; y++) {
                uint16_t *row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(arg_img, y);
                uint8_t *out_row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(&out, y);
                for (int x = 0, xx = out.w; x < xx; x++) {
                    IMAGE_PUT_GRAYSCALE_PIXEL_FAST(out_row_ptr, x,
                        COLOR_RGB565_TO_GRAYSCALE(IMAGE_GET_RGB565_PIXEL_FAST(row_ptr, x)));
                }
            }
            break;
        }
        default: {
            break;
        }
    }

    if (!copy)
    {
        arg_img->bpp = out.bpp;
        if(is_img_data_in_main_fb(out.data))
        {
            MAIN_FB()->bpp = out.bpp;
        }
    }

    return py_image_from_struct(&out);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_to_grayscale_obj, 1, py_image_to_grayscale);

static mp_obj_t py_image_to_rgb565(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);
    bool copy = py_helper_keyword_int(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_copy), false);

    image_t out;
    out.w = arg_img->w;
    out.h = arg_img->h;
    out.bpp = IMAGE_BPP_RGB565;
    out.data = copy ? xalloc(image_size(&out)) : arg_img->data;
    out.pix_ai = copy ? xalloc(out.w*out.h*3) : arg_img->pix_ai;

    switch(arg_img->bpp) {
        case IMAGE_BPP_BINARY: {
            PY_ASSERT_TRUE_MSG((out.w == 1) || copy,
                "Can't convert to grayscale in place!");
            for (int y = 0, yy = out.h; y < yy; y++) {
                uint32_t *row_ptr = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(arg_img, y);
                uint16_t *out_row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(&out, y);
                for (int x = 0, xx = out.w; x < xx; x++) {
                    IMAGE_PUT_RGB565_PIXEL_FAST(out_row_ptr, x,
                        imlib_yuv_to_rgb(IMAGE_GET_BINARY_PIXEL_FAST(row_ptr, x) * COLOR_GRAYSCALE_MAX, 0, 0));
                }
            }
            break;
        }
        case IMAGE_BPP_GRAYSCALE: {
            PY_ASSERT_TRUE_MSG(copy,
                "Can't convert to rgb565 in place!");
            for (int y = 0, yy = out.h; y < yy; y++) {
                uint8_t *row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(arg_img, y);
                uint16_t *out_row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(&out, y);
                for (int x = 0, xx = out.w; x < xx; x++) {
                    IMAGE_PUT_RGB565_PIXEL_FAST(out_row_ptr, x,
                        imlib_yuv_to_rgb(IMAGE_GET_GRAYSCALE_PIXEL_FAST(row_ptr, x), 0, 0));
                }
            }
            break;
        }
        case IMAGE_BPP_RGB565: {
            if (copy) memcpy(out.data, arg_img->data, image_size(&out));
            break;
        }
        default: {
            break;
        }
    }

    if ((!copy) && is_img_data_in_main_fb(out.data)) {
        MAIN_FB()->bpp = out.bpp;
    }

    return py_image_from_struct(&out);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_to_rgb565_obj, 1, py_image_to_rgb565);

extern const uint16_t rainbow_table[256];

static mp_obj_t py_image_to_rainbow(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);
    bool copy = py_helper_keyword_int(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_copy), false);

    image_t out;
    out.w = arg_img->w;
    out.h = arg_img->h;
    out.bpp = IMAGE_BPP_RGB565;
    out.data = copy ? xalloc(image_size(&out)) : arg_img->data;
    out.pix_ai = copy ? xalloc(out.w*out.h*3) : arg_img->pix_ai;

    switch(arg_img->bpp) {
        case IMAGE_BPP_BINARY: {
            PY_ASSERT_TRUE_MSG((out.w == 1) || copy,
                "Can't convert to rainbow in place!");
            for (int y = 0, yy = out.h; y < yy; y++) {
                uint32_t *row_ptr = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(arg_img, y);
                uint16_t *out_row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(&out, y);
                for (int x = 0, xx = out.w; x < xx; x++) {
                    IMAGE_PUT_RGB565_PIXEL_FAST(out_row_ptr, x,
                        rainbow_table[IMAGE_GET_BINARY_PIXEL_FAST(row_ptr, x) * COLOR_GRAYSCALE_MAX]);
                }
            }
            break;
        }
        case IMAGE_BPP_GRAYSCALE: {
            PY_ASSERT_TRUE_MSG(copy,
                "Can't convert to rainbow in place!");
            for (int y = 0, yy = out.h; y < yy; y++) {
                uint8_t *row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(arg_img, y);
                uint16_t *out_row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(&out, y);
                for (int x = 0, xx = out.w; x < xx; x++) {
                    IMAGE_PUT_RGB565_PIXEL_FAST(out_row_ptr, x,
                        rainbow_table[IMAGE_GET_GRAYSCALE_PIXEL_FAST(row_ptr, x)]);
                }
            }
            break;
        }
        case IMAGE_BPP_RGB565: {
            for (int y = 0, yy = out.h; y < yy; y++) {
                uint16_t *row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(arg_img, y);
                uint16_t *out_row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(&out, y);
                for (int x = 0, xx = out.w; x < xx; x++) {
                    IMAGE_PUT_RGB565_PIXEL_FAST(out_row_ptr, x,
                        rainbow_table[COLOR_RGB565_TO_GRAYSCALE(IMAGE_GET_RGB565_PIXEL_FAST(row_ptr, x))]);
                }
            }
            break;
        }
        default: {
            break;
        }
    }

    if ((!copy) && is_img_data_in_main_fb(out.data)) {
        MAIN_FB()->bpp = out.bpp;
    }

    return py_image_from_struct(&out);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_to_rainbow_obj, 1, py_image_to_rainbow);

static mp_obj_t py_image_compress(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable_bayer(args[0]);
    int arg_q = py_helper_keyword_int(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_quality), 50);
    PY_ASSERT_TRUE_MSG((1 <= arg_q) && (arg_q <= 100), "Error: 1 <= quality <= 100!");

    uint64_t size;
    fb_alloc_mark();
    uint8_t *buffer = fb_alloc_all(&size);
    image_t out = { .w=arg_img->w, .h=arg_img->h, .bpp=size, .data=buffer };
    PY_ASSERT_FALSE_MSG(jpeg_compress(arg_img, &out, arg_q, false), "Out of Memory!");
    PY_ASSERT_TRUE_MSG(out.bpp <= image_size(arg_img), "Can't compress in place!");
    memcpy(arg_img->data, out.data, out.bpp);
    arg_img->bpp = out.bpp;
    fb_free();
    fb_alloc_free_till_mark();

    if (is_img_data_in_main_fb(arg_img->data)) {
        MAIN_FB()->bpp = arg_img->bpp;
    }

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_compress_obj, 1, py_image_compress);

static mp_obj_t py_image_to_bytes(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_image_cobj(args[0]);
    mp_uint_t size = 0;
    switch(arg_img->bpp) {
        case IMAGE_BPP_BINARY: {
            size = ((arg_img->w + UINT32_T_MASK) >> UINT32_T_SHIFT) * arg_img->h;
            break;
        }
        case IMAGE_BPP_GRAYSCALE: {
            size = (arg_img->w * arg_img->h) * sizeof(uint8_t);
            break;
        }
        case IMAGE_BPP_RGB565: {
            size = (arg_img->w * arg_img->h) * sizeof(uint16_t);
            break;
        }
        default: {
            size = arg_img->bpp;
            }
            break;
    }
    mp_obj_array_t *o = m_new_obj(mp_obj_array_t);
    o->base.type = &mp_type_bytearray;
    o->typecode = BYTEARRAY_TYPECODE;
    o->free = 0;
    o->len = size;
    o->items = (void*)arg_img->pixels;
    return o;
    // return mp_obj_new_bytearray(size, arg_img->pixels);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_to_bytes_obj, 1, py_image_to_bytes);

static mp_obj_t py_image_compress_for_ide(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable_bayer(args[0]);
    int arg_q = py_helper_keyword_int(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_quality), 50);
    PY_ASSERT_TRUE_MSG((1 <= arg_q) && (arg_q <= 100), "Error: 1 <= quality <= 100!");

    uint64_t size;
    fb_alloc_mark();
    uint8_t *buffer = fb_alloc_all(&size);
    image_t out = { .w=arg_img->w, .h=arg_img->h, .bpp=size, .data=buffer };
    PY_ASSERT_FALSE_MSG(jpeg_compress(arg_img, &out, arg_q, false), "Out of Memory!");
    PY_ASSERT_TRUE_MSG(out.bpp <= image_size(arg_img), "Can't compress in place!");
    uint8_t *ptr = arg_img->data;

    *ptr++ = 0xFE;

    for(int i = 0, j = (out.bpp / 3) * 3; i < j; i += 3) {
        int x = 0;
        x |= out.data[i + 0] << 0;
        x |= out.data[i + 1] << 8;
        x |= out.data[i + 2] << 16;
        *ptr++ = 0x80 | ((x >> 0) & 0x3F);
        *ptr++ = 0x80 | ((x >> 6) & 0x3F);
        *ptr++ = 0x80 | ((x >> 12) & 0x3F);
        *ptr++ = 0x80 | ((x >> 18) & 0x3F);
    }

    if((out.bpp % 3) == 2) { // 2 bytes -> 16-bits -> 24-bits sent
        int x = 0;
        x |= out.data[out.bpp - 2] << 0;
        x |= out.data[out.bpp - 1] << 8;
        *ptr++ = 0x80 | ((x >> 0) & 0x3F);
        *ptr++ = 0x80 | ((x >> 6) & 0x3F);
        *ptr++ = 0x80 | ((x >> 12) & 0x3F);
    }

    if((out.bpp % 3) == 1) { // 1 byte -> 8-bits -> 16-bits sent
        int x = 0;
        x |= out.data[out.bpp - 1] << 0;
        *ptr++ = 0x80 | ((x >> 0) & 0x3F);
        *ptr++ = 0x80 | ((x >> 6) & 0x3F);
    }

    *ptr++ = 0xFE;

    out.bpp = (((out.bpp * 8) + 5) / 6) + 2;
    arg_img->bpp = out.bpp;
    fb_free();
    fb_alloc_free_till_mark();

    if (is_img_data_in_main_fb(arg_img->data)) {
        MAIN_FB()->bpp = arg_img->bpp;
    }

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_compress_for_ide_obj, 1, py_image_compress_for_ide);

static mp_obj_t py_image_compressed(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable_bayer(args[0]);
    int arg_q = py_helper_keyword_int(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_quality), 50);
    PY_ASSERT_TRUE_MSG((1 <= arg_q) && (arg_q <= 100), "Error: 1 <= quality <= 100!");

    uint64_t size;
    fb_alloc_mark();
    uint8_t *buffer = fb_alloc_all(&size);
    image_t out = { .w=arg_img->w, .h=arg_img->h, .bpp=size, .data=buffer };
    PY_ASSERT_FALSE_MSG(jpeg_compress(arg_img, &out, arg_q, false), "Out of Memory!");
    uint8_t *temp = xalloc(out.bpp);
    memcpy(temp, out.data, out.bpp);
    out.data = temp;
    fb_free();
    fb_alloc_free_till_mark();

    return py_image_from_struct(&out);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_compressed_obj, 1, py_image_compressed);

static mp_obj_t py_image_compressed_for_ide(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable_bayer(args[0]);
    int arg_q = py_helper_keyword_int(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_quality), 50);
    PY_ASSERT_TRUE_MSG((1 <= arg_q) && (arg_q <= 100), "Error: 1 <= quality <= 100!");

    uint64_t size;
    fb_alloc_mark();
    uint8_t *buffer = fb_alloc_all(&size);
    image_t out = { .w=arg_img->w, .h=arg_img->h, .bpp=size, .data=buffer };
    PY_ASSERT_FALSE_MSG(jpeg_compress(arg_img, &out, arg_q, false), "Out of Memory!");
    uint8_t *temp = xalloc((((out.bpp * 8) + 5) / 6) + 2);
    uint8_t *ptr = temp;

    *ptr++ = 0xFE;

    for(int i = 0, j = (out.bpp / 3) * 3; i < j; i += 3) {
        int x = 0;
        x |= out.data[i + 0] << 0;
        x |= out.data[i + 1] << 8;
        x |= out.data[i + 2] << 16;
        *ptr++ = 0x80 | ((x >> 0) & 0x3F);
        *ptr++ = 0x80 | ((x >> 6) & 0x3F);
        *ptr++ = 0x80 | ((x >> 12) & 0x3F);
        *ptr++ = 0x80 | ((x >> 18) & 0x3F);
    }

    if((out.bpp % 3) == 2) { // 2 bytes -> 16-bits -> 24-bits sent
        int x = 0;
        x |= out.data[out.bpp - 2] << 0;
        x |= out.data[out.bpp - 1] << 8;
        *ptr++ = 0x80 | ((x >> 0) & 0x3F);
        *ptr++ = 0x80 | ((x >> 6) & 0x3F);
        *ptr++ = 0x80 | ((x >> 12) & 0x0F);
    }

    if((out.bpp % 3) == 1) { // 1 byte -> 8-bits -> 16-bits sent
        int x = 0;
        x |= out.data[out.bpp - 1] << 0;
        *ptr++ = 0x80 | ((x >> 0) & 0x3F);
        *ptr++ = 0x80 | ((x >> 6) & 0x03);
    }

    *ptr++ = 0xFE;

    out.bpp = (((out.bpp * 8) + 5) / 6) + 2;
    out.data = temp;
    fb_free();
    fb_alloc_free_till_mark();

    return py_image_from_struct(&out);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_compressed_for_ide_obj, 1, py_image_compressed_for_ide);

static mp_obj_t py_image_copy(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_image_cobj(args[0]);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 1, kw_args, &roi);

    bool copy_to_fb = py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_copy_to_fb), false);
    bool update_jb = py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_update_jb), false);
    if (copy_to_fb && update_jb) fb_update_jpeg_buffer();

    image_t image;
    image.w = roi.w;
    image.h = roi.h;
    image.bpp = arg_img->bpp;
    image.data = NULL;
    image.pix_ai = NULL;

    if (copy_to_fb) {
       if(roi.x!=0 || roi.y!=0)
       {
            PY_ASSERT_TRUE_MSG(!is_img_data_in_main_fb(arg_img->data), "Cannot copy to fb!");
       }
       PY_ASSERT_TRUE_MSG((image_size(&image) <= OMV_RAW_BUF_SIZE), "FB Overflow!");
       MAIN_FB()->w = image.w;
       MAIN_FB()->h = image.h;
       MAIN_FB()->bpp = image.bpp;
#if CONFIG_MAIXPY_OMV_DOUBLE_BUFF
       image.data = MAIN_FB()->pixels[g_sensor_buff_index_out];
       image.pix_ai = MAIN_FB()->pix_ai[g_sensor_buff_index_out];
#else
       image.data = MAIN_FB()->pixels;
       image.pix_ai = MAIN_FB()->pix_ai;
#endif
    } else {
       image.data = xalloc(image_size(&image));
       image.pix_ai = xalloc(image.w*image.h*3);
    }

    switch(arg_img->bpp) {
        case IMAGE_BPP_BINARY: {
            for (int y = roi.y, yy = roi.y + roi.h; y < yy; y++) {
                uint32_t *row_ptr = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(arg_img, y);
                uint32_t *row_ptr_2 = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(&image, y - roi.y);
                for (int x = roi.x, xx = roi.x + roi.w; x < xx; x++) {
                    IMAGE_PUT_BINARY_PIXEL_FAST(row_ptr_2, x - roi.x, IMAGE_GET_BINARY_PIXEL_FAST(row_ptr, x));
                }
            }
            break;
        }
        case IMAGE_BPP_GRAYSCALE: {
            for (int y = roi.y, yy = roi.y + roi.h; y < yy; y++) {
                uint8_t *row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(arg_img, y);
                uint8_t *row_ptr_2 = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(&image, y - roi.y);
                for (int x = roi.x, xx = roi.x + roi.w; x < xx; x++) {
                    IMAGE_PUT_GRAYSCALE_PIXEL_FAST(row_ptr_2, x - roi.x, IMAGE_GET_GRAYSCALE_PIXEL_FAST(row_ptr, x));
                }
            }
            break;
        }
        case IMAGE_BPP_RGB565: {
            for (int y = roi.y, yy = roi.y + roi.h; y < yy; y++) {
                uint16_t *row_ptr = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(arg_img, y);
                uint16_t *row_ptr_2 = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(&image, y - roi.y);
                for (int x = roi.x, xx = roi.x + roi.w; x < xx; x++) {
                    IMAGE_PUT_RGB565_PIXEL_FAST(row_ptr_2, x - roi.x, IMAGE_GET_RGB565_PIXEL_FAST(row_ptr, x));
                }
            }
            break;
        }
        case IMAGE_BPP_BAYER:  { // Correct!
            for (int y = roi.y, yy = roi.y + roi.h; y < yy; y++) {
                uint8_t *row_ptr = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(arg_img, y);
                uint8_t *row_ptr_2 = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(&image, y - roi.y);
                for (int x = roi.x, xx = roi.x + roi.w; x < xx; x++) {
                    IMAGE_PUT_GRAYSCALE_PIXEL_FAST(row_ptr_2, x - roi.x, IMAGE_GET_GRAYSCALE_PIXEL_FAST(row_ptr, x));
                }
            }
            break;
        }
        default: { // JPEG
            memcpy(image.data, arg_img->data, image_size(&image));
            break;
        }
    }

    return py_image_from_struct(&image); 
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_copy_obj, 1, py_image_copy);

static mp_obj_t py_image_save(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_image_cobj(args[0]);
    const char *path = mp_obj_str_get_str(args[1]);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 2, kw_args, &roi);

    int arg_q = py_helper_keyword_int(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_quality), 50);
    PY_ASSERT_TRUE_MSG((1 <= arg_q) && (arg_q <= 100), "Error: 1 <= quality <= 100!");

    fb_alloc_mark();
    imlib_save_image(arg_img, path, &roi, arg_q);
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_save_obj, 2, py_image_save);

// //////////////////
// // Drawing Methods
// //////////////////

STATIC mp_obj_t py_image_clear(mp_obj_t img_obj)
{
    image_t *arg_img = py_helper_arg_to_image_mutable_bayer(img_obj);
    memset(arg_img->data, 0, image_size(arg_img));
    return img_obj;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_image_clear_obj, py_image_clear);

STATIC mp_obj_t py_image_draw_line(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    const mp_obj_t *arg_vec;
    uint offset = py_helper_consume_array(n_args, args, 1, 4, &arg_vec);
    int arg_x0 = mp_obj_get_int(arg_vec[0]);
    int arg_y0 = mp_obj_get_int(arg_vec[1]);
    int arg_x1 = mp_obj_get_int(arg_vec[2]);
    int arg_y1 = mp_obj_get_int(arg_vec[3]);

    int arg_c =
        py_helper_keyword_color(arg_img, n_args, args, offset + 0, kw_args, -1); // White.
    int arg_thickness =
        py_helper_keyword_int(n_args, args, offset + 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_thickness), 1);

    imlib_draw_line(arg_img, arg_x0, arg_y0, arg_x1, arg_y1, arg_c, arg_thickness);
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_draw_line_obj, 2, py_image_draw_line);

STATIC mp_obj_t py_image_draw_rectangle(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    const mp_obj_t *arg_vec;
    uint offset = py_helper_consume_array(n_args, args, 1, 4, &arg_vec);
    int arg_rx = mp_obj_get_int(arg_vec[0]);
    int arg_ry = mp_obj_get_int(arg_vec[1]);
    int arg_rw = mp_obj_get_int(arg_vec[2]);
    int arg_rh = mp_obj_get_int(arg_vec[3]);

    int arg_c =
        py_helper_keyword_color(arg_img, n_args, args, offset + 0, kw_args, -1); // White.
    int arg_thickness =
        py_helper_keyword_int(n_args, args, offset + 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_thickness), 1);
    bool arg_fill =
        py_helper_keyword_int(n_args, args, offset + 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_fill), false);

    imlib_draw_rectangle(arg_img, arg_rx, arg_ry, arg_rw, arg_rh, arg_c, arg_thickness, arg_fill);
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_draw_rectangle_obj, 2, py_image_draw_rectangle);

STATIC mp_obj_t py_image_draw_circle(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    const mp_obj_t *arg_vec;
    uint offset = py_helper_consume_array(n_args, args, 1, 3, &arg_vec);
    int arg_cx = mp_obj_get_int(arg_vec[0]);
    int arg_cy = mp_obj_get_int(arg_vec[1]);
    int arg_cr = mp_obj_get_int(arg_vec[2]);

    int arg_c =
        py_helper_keyword_color(arg_img, n_args, args, offset + 0, kw_args, -1); // White.
    int arg_thickness =
        py_helper_keyword_int(n_args, args, offset + 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_thickness), 1);
    bool arg_fill =
        py_helper_keyword_int(n_args, args, offset + 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_fill), false);

    imlib_draw_circle(arg_img, arg_cx, arg_cy, arg_cr, arg_c, arg_thickness, arg_fill);
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_draw_circle_obj, 2, py_image_draw_circle);

STATIC mp_obj_t py_image_draw_ellipse(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    const mp_obj_t *arg_vec;
    uint offset = py_helper_consume_array(n_args, args, 1, 4, &arg_vec);
    int arg_cx = mp_obj_get_int(arg_vec[0]);
    int arg_cy = mp_obj_get_int(arg_vec[1]);
    int arg_rx = mp_obj_get_int(arg_vec[2]);
    int arg_ry = mp_obj_get_int(arg_vec[3]);

    int arg_rotation =
        py_helper_keyword_int(n_args, args, offset + 0, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_rotation), 0);
    int arg_c =
        py_helper_keyword_color(arg_img, n_args, args, offset + 1, kw_args, -1); // White.
    int arg_thickness =
        py_helper_keyword_int(n_args, args, offset + 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_thickness), 1);
    bool arg_fill =
        py_helper_keyword_int(n_args, args, offset + 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_fill), false);

    imlib_draw_ellipse(arg_img, arg_cx, arg_cy, arg_rx, arg_ry, arg_rotation, arg_c, arg_thickness, arg_fill);
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_draw_ellipse_obj, 2, py_image_draw_ellipse);

STATIC mp_obj_t py_image_draw_font(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    const mp_obj_t *arg_vec;
    uint offset = py_helper_consume_array(n_args, args, 1, 5, &arg_vec);
    int arg_x_off = mp_obj_get_int(arg_vec[0]);
    int arg_y_off = mp_obj_get_int(arg_vec[1]);
    int arg_w_font = mp_obj_get_int(arg_vec[2]);
    int arg_h_font = mp_obj_get_int(arg_vec[3]);
    const uint8_t *arg_font = mp_obj_str_get_str(arg_vec[4]);

    mp_int_t font_len = mp_obj_get_int(mp_obj_len(arg_vec[4]));

    PY_ASSERT_TRUE_MSG(arg_w_font % 8 == 0 && arg_w_font <= 32, "Error: font arg_w_font %% 8 == 0 && arg_w_font <= 32!");

    PY_ASSERT_TRUE_MSG((arg_w_font / 8) * (arg_h_font / 8) * 8 == font_len, "Error: font (arg_w_font / 8) * (arg_h_font / 8) * 8 == font_len!");

    int arg_c =
        py_helper_keyword_color(arg_img, n_args, args, offset + 0, kw_args, -1); // White.
    float arg_scale =
        py_helper_keyword_float(n_args, args, offset + 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_scale), 1.0);
    PY_ASSERT_TRUE_MSG(0 < arg_scale, "Error: 0 < scale!");
    imlib_draw_font(arg_img, arg_x_off, arg_y_off,
                      arg_c, arg_scale, arg_h_font, arg_w_font, arg_font);
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_draw_font_obj, 2, py_image_draw_font);

STATIC mp_obj_t py_image_draw_string(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    const mp_obj_t *arg_vec;
    uint offset = py_helper_consume_array(n_args, args, 1, 3, &arg_vec);
    int arg_x_off = mp_obj_get_int(arg_vec[0]);
    int arg_y_off = mp_obj_get_int(arg_vec[1]);
    // const char *arg_str = mp_obj_str_get_str(arg_vec[2]);
    const mp_obj_t arg_str = arg_vec[2];
    int arg_c =
        py_helper_keyword_color(arg_img, n_args, args, offset + 0, kw_args, -1); // White.
    float arg_scale =
        py_helper_keyword_float(n_args, args, offset + 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_scale), 1.0);
    PY_ASSERT_TRUE_MSG(0 < arg_scale, "Error: 0 < scale!");
    int arg_x_spacing =
        py_helper_keyword_int(n_args, args, offset + 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_x_spacing), 0);
    int arg_y_spacing =
        py_helper_keyword_int(n_args, args, offset + 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_y_spacing), 2);
    bool arg_mono_space =
        py_helper_keyword_int(n_args, args, offset + 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_mono_space), 0);

    imlib_draw_string(arg_img, arg_x_off, arg_y_off, arg_str,
                      arg_c, arg_scale, arg_x_spacing, arg_y_spacing,
                      arg_mono_space);
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_draw_string_obj, 2, py_image_draw_string);

STATIC mp_obj_t py_image_draw_cross(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    const mp_obj_t *arg_vec;
    uint offset = py_helper_consume_array(n_args, args, 1, 2, &arg_vec);
    int arg_x = mp_obj_get_int(arg_vec[0]);
    int arg_y = mp_obj_get_int(arg_vec[1]);

    int arg_c =
        py_helper_keyword_color(arg_img, n_args, args, offset + 0, kw_args, -1); // White.
    int arg_s =
        py_helper_keyword_int(n_args, args, offset + 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_size), 5);
    int arg_thickness =
        py_helper_keyword_int(n_args, args, offset + 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_thickness), 1);

    imlib_draw_line(arg_img, arg_x - arg_s, arg_y        , arg_x + arg_s, arg_y        , arg_c, arg_thickness);
    imlib_draw_line(arg_img, arg_x        , arg_y - arg_s, arg_x        , arg_y + arg_s, arg_c, arg_thickness);
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_draw_cross_obj, 2, py_image_draw_cross);

STATIC mp_obj_t py_image_draw_arrow(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    const mp_obj_t *arg_vec;
    uint offset = py_helper_consume_array(n_args, args, 1, 4, &arg_vec);
    int arg_x0 = mp_obj_get_int(arg_vec[0]);
    int arg_y0 = mp_obj_get_int(arg_vec[1]);
    int arg_x1 = mp_obj_get_int(arg_vec[2]);
    int arg_y1 = mp_obj_get_int(arg_vec[3]);

    int arg_c =
        py_helper_keyword_color(arg_img, n_args, args, offset + 0, kw_args, -1); // White.
    int arg_s =
        py_helper_keyword_int(n_args, args, offset + 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_size), 10);
    int arg_thickness =
        py_helper_keyword_int(n_args, args, offset + 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_thickness), 1);

    int dx = (arg_x1 - arg_x0);
    int dy = (arg_y1 - arg_y0);
    float length = fast_sqrtf((dx * dx) + (dy * dy));

    float ux = IM_DIV(dx, length);
    float uy = IM_DIV(dy, length);
    float vx = -uy;
    float vy = ux;

    int a0x = fast_roundf(arg_x1 - (arg_s * ux) + (arg_s * vx * 0.5));
    int a0y = fast_roundf(arg_y1 - (arg_s * uy) + (arg_s * vy * 0.5));
    int a1x = fast_roundf(arg_x1 - (arg_s * ux) - (arg_s * vx * 0.5));
    int a1y = fast_roundf(arg_y1 - (arg_s * uy) - (arg_s * vy * 0.5));

    imlib_draw_line(arg_img, arg_x0, arg_y0, arg_x1, arg_y1, arg_c, arg_thickness);
    imlib_draw_line(arg_img, arg_x1, arg_y1, a0x, a0y, arg_c, arg_thickness);
    imlib_draw_line(arg_img, arg_x1, arg_y1, a1x, a1y, arg_c, arg_thickness);
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_draw_arrow_obj, 2, py_image_draw_arrow);

STATIC mp_obj_t py_image_draw_image(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    image_t *arg_other =
        py_helper_arg_to_image_mutable(args[1]);

    const mp_obj_t *arg_vec;
    uint offset = py_helper_consume_array(n_args, args, 2, 2, &arg_vec);
    int arg_cx = mp_obj_get_int(arg_vec[0]);
    int arg_cy = mp_obj_get_int(arg_vec[1]);

    float arg_x_scale =
        py_helper_keyword_float(n_args, args, offset + 0, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_x_scale), 1.0f);
    PY_ASSERT_TRUE_MSG((0.0f <= arg_x_scale), "Error: 0.0 <= x_scale!");
    float arg_y_scale =
        py_helper_keyword_float(n_args, args, offset + 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_y_scale), 1.0f);
    PY_ASSERT_TRUE_MSG((0.0f <= arg_y_scale), "Error: 0.0 <= y_scale!");
    float arg_alpha =
        py_helper_keyword_int(n_args, args, offset + 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_alpha), 256) / 256.0f;
    PY_ASSERT_TRUE_MSG((0 <= arg_alpha) && (arg_alpha <= 1), "Error: 0 <= alpha <= 256!");
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, offset + 3, kw_args);

    imlib_draw_image(arg_img, arg_other, arg_cx, arg_cy, arg_x_scale, arg_y_scale, arg_alpha, arg_msk);
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_draw_image_obj, 3, py_image_draw_image);

#ifndef OMV_MINIMUM
STATIC mp_obj_t py_image_draw_keypoints(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    int arg_c =
        py_helper_keyword_color(arg_img, n_args, args, 2, kw_args, -1); // White.
    int arg_s =
        py_helper_keyword_int(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_size), 10);
    int arg_thickness =
        py_helper_keyword_int(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_thickness), 1);
    bool arg_fill =
        py_helper_keyword_int(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_fill), false);

    if (MP_OBJ_IS_TYPE(args[1], &mp_type_tuple) || MP_OBJ_IS_TYPE(args[1], &mp_type_list)) {
        size_t len;
        mp_obj_t *items;
        mp_obj_get_array(args[1], &len, &items);
        for (size_t i = 0; i < len; i++) {
            mp_obj_t *tuple;
            mp_obj_get_array_fixed_n(items[i], 3, &tuple);
            int cx = mp_obj_get_int(tuple[0]);
            int cy = mp_obj_get_int(tuple[1]);
            int angle = mp_obj_get_int(tuple[2]) % 360;
            int si = sin_table[angle] * arg_s;
            int co = cos_table[angle] * arg_s;
            imlib_draw_line(arg_img, cx, cy, cx + co, cy + si, arg_c, arg_thickness);
            imlib_draw_circle(arg_img, cx, cy, (arg_s - 2) / 2, arg_c, arg_thickness, arg_fill);
        }
    } else {
        py_kp_obj_t *kpts_obj = py_kpts_obj(args[1]);
        for (int i = 0, ii = array_length(kpts_obj->kpts); i < ii; i++) {
            kp_t *kp = array_at(kpts_obj->kpts, i);
            int cx = kp->x;
            int cy = kp->y;
            int angle = kp->angle % 360;
            int si = sin_table[angle] * arg_s;
            int co = cos_table[angle] * arg_s;
            imlib_draw_line(arg_img, cx, cy, cx + co, cy + si, arg_c, arg_thickness);
            imlib_draw_circle(arg_img, cx, cy, (arg_s - 2) / 2, arg_c, arg_thickness, arg_fill);
        }
    }

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_draw_keypoints_obj, 2, py_image_draw_keypoints);
#endif //OMV_MINIMUM

#ifdef IMLIB_ENABLE_FLOOD_FILL
STATIC mp_obj_t py_image_flood_fill(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    const mp_obj_t *arg_vec;
    uint offset = py_helper_consume_array(n_args, args, 1, 2, &arg_vec);
    int arg_x_off = mp_obj_get_int(arg_vec[0]);
    int arg_y_off = mp_obj_get_int(arg_vec[1]);

    float arg_seed_threshold =
        py_helper_keyword_float(n_args, args, offset + 0, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_seed_threshold), 0.05);
    PY_ASSERT_TRUE_MSG((0.0f <= arg_seed_threshold) && (arg_seed_threshold <= 1.0f),
                       "Error: 0.0 <= seed_threshold <= 1.0!");
    float arg_floating_threshold =
        py_helper_keyword_float(n_args, args, offset + 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_floating_threshold), 0.05);
    PY_ASSERT_TRUE_MSG((0.0f <= arg_floating_threshold) && (arg_floating_threshold <= 1.0f),
                       "Error: 0.0 <= floating_threshold <= 1.0!");
    int arg_c =
        py_helper_keyword_color(arg_img, n_args, args, offset + 2, kw_args, -1); // White.
    bool arg_invert =
        py_helper_keyword_float(n_args, args, offset + 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_invert), false);
    bool clear_background =
        py_helper_keyword_float(n_args, args, offset + 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_clear_background), false);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, offset + 5, kw_args);

    fb_alloc_mark();
    imlib_flood_fill(arg_img, arg_x_off, arg_y_off,
                     arg_seed_threshold, arg_floating_threshold,
                     arg_c, arg_invert, clear_background, arg_msk);
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_flood_fill_obj, 2, py_image_flood_fill);
#endif // IMLIB_ENABLE_FLOOD_FILL

#ifdef IMLIB_ENABLE_BINARY_OPS
/////////////////
// Binary Methods
/////////////////

STATIC mp_obj_t py_image_binary(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    list_t arg_thresholds;
    list_init(&arg_thresholds, sizeof(color_thresholds_list_lnk_data_t));
    py_helper_arg_to_thresholds(args[1], &arg_thresholds);

    bool arg_invert =
        py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_invert), false);
    bool arg_zero =
        py_helper_keyword_int(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_zero), false);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 4, kw_args);
    bool arg_to_bitmap =
        py_helper_keyword_int(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_to_bitmap), false);
    bool arg_copy =
        py_helper_keyword_int(n_args, args, 6, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_copy), false);

    if (arg_to_bitmap && (!arg_copy)) {
        switch(arg_img->bpp) {
            case IMAGE_BPP_GRAYSCALE: {
                PY_ASSERT_TRUE_MSG((arg_img->w >= (sizeof(uint32_t)/sizeof(uint8_t))),
                                   "Can't convert to bitmap in place!");
                break;
            }
            case IMAGE_BPP_RGB565: {
                PY_ASSERT_TRUE_MSG((arg_img->w >= (sizeof(uint32_t)/sizeof(uint16_t))),
                                   "Can't convert to bitmap in place!");
                break;
            }
            default: {
                break;
            }
        }
    }

    image_t out;
    out.w = arg_img->w;
    out.h = arg_img->h;
    out.bpp = arg_to_bitmap ? IMAGE_BPP_BINARY : arg_img->bpp;
    out.data = arg_copy ? xalloc(image_size(&out)) : arg_img->data;
    out.pix_ai = arg_copy ? xalloc(out.w*out.h*3) : arg_img->pix_ai;

    fb_alloc_mark();
    imlib_binary(&out, arg_img, &arg_thresholds, arg_invert, arg_zero, arg_msk);
    fb_alloc_free_till_mark();

    list_free(&arg_thresholds);

    if (arg_to_bitmap && (!arg_copy) && is_img_data_in_main_fb(out.data)) {
        MAIN_FB()->bpp = out.bpp;
    }

    return py_image_from_struct(&out);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_binary_obj, 2, py_image_binary);

STATIC mp_obj_t py_image_invert(mp_obj_t img_obj)
{
    imlib_invert(py_helper_arg_to_image_mutable(img_obj));
    return img_obj;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_image_invert_obj, py_image_invert);

STATIC mp_obj_t py_image_b_and(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 2, kw_args);

    fb_alloc_mark();

    if (MP_OBJ_IS_STR(args[1])) {
        imlib_b_and(arg_img, mp_obj_str_get_str(args[1]), NULL, 0, arg_msk);
    } else if (MP_OBJ_IS_TYPE(args[1], &py_image_type)) {
        imlib_b_and(arg_img, NULL, py_helper_arg_to_image_mutable(args[1]), 0, arg_msk);
    } else {
        imlib_b_and(arg_img, NULL, NULL,
                    py_helper_keyword_color(arg_img, n_args, args, 1, NULL, 0),
                    arg_msk);
    }

    fb_alloc_free_till_mark();

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_b_and_obj, 2, py_image_b_and);

STATIC mp_obj_t py_image_b_nand(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 2, kw_args);

    fb_alloc_mark();

    if (MP_OBJ_IS_STR(args[1])) {
        imlib_b_nand(arg_img, mp_obj_str_get_str(args[1]), NULL, 0, arg_msk);
    } else if (MP_OBJ_IS_TYPE(args[1], &py_image_type)) {
        imlib_b_nand(arg_img, NULL, py_helper_arg_to_image_mutable(args[1]), 0, arg_msk);
    } else {
        imlib_b_nand(arg_img, NULL, NULL,
                     py_helper_keyword_color(arg_img, n_args, args, 1, NULL, 0),
                     arg_msk);
    }

    fb_alloc_free_till_mark();

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_b_nand_obj, 2, py_image_b_nand);

STATIC mp_obj_t py_image_b_or(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 2, kw_args);

    fb_alloc_mark();

    if (MP_OBJ_IS_STR(args[1])) {
        imlib_b_or(arg_img, mp_obj_str_get_str(args[1]), NULL, 0, arg_msk);
    } else if (MP_OBJ_IS_TYPE(args[1], &py_image_type)) {
        imlib_b_or(arg_img, NULL, py_helper_arg_to_image_mutable(args[1]), 0, arg_msk);
    } else {
        imlib_b_or(arg_img, NULL, NULL,
                   py_helper_keyword_color(arg_img, n_args, args, 1, NULL, 0),
                   arg_msk);
    }

    fb_alloc_free_till_mark();

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_b_or_obj, 2, py_image_b_or);

STATIC mp_obj_t py_image_b_nor(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 2, kw_args);

    fb_alloc_mark();

    if (MP_OBJ_IS_STR(args[1])) {
        imlib_b_nor(arg_img, mp_obj_str_get_str(args[1]), NULL, 0, arg_msk);
    } else if (MP_OBJ_IS_TYPE(args[1], &py_image_type)) {
        imlib_b_nor(arg_img, NULL, py_helper_arg_to_image_mutable(args[1]), 0, arg_msk);
    } else {
        imlib_b_nor(arg_img, NULL, NULL,
                    py_helper_keyword_color(arg_img, n_args, args, 1, NULL, 0),
                    arg_msk);
    }

    fb_alloc_free_till_mark();

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_b_nor_obj, 2, py_image_b_nor);

STATIC mp_obj_t py_image_b_xor(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 2, kw_args);

    fb_alloc_mark();

    if (MP_OBJ_IS_STR(args[1])) {
        imlib_b_xor(arg_img, mp_obj_str_get_str(args[1]), NULL, 0, arg_msk);
    } else if (MP_OBJ_IS_TYPE(args[1], &py_image_type)) {
        imlib_b_xor(arg_img, NULL, py_helper_arg_to_image_mutable(args[1]), 0, arg_msk);
    } else {
        imlib_b_xor(arg_img, NULL, NULL,
                    py_helper_keyword_color(arg_img, n_args, args, 1, NULL, 0),
                    arg_msk);
    }

    fb_alloc_free_till_mark();

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_b_xor_obj, 2, py_image_b_xor);

STATIC mp_obj_t py_image_b_xnor(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 2, kw_args);

    fb_alloc_mark();

    if (MP_OBJ_IS_STR(args[1])) {
        imlib_b_xnor(arg_img, mp_obj_str_get_str(args[1]), NULL, 0, arg_msk);
    } else if (MP_OBJ_IS_TYPE(args[1], &py_image_type)) {
        imlib_b_xnor(arg_img, NULL, py_helper_arg_to_image_mutable(args[1]), 0, arg_msk);
    } else {
        imlib_b_xnor(arg_img, NULL, NULL,
                     py_helper_keyword_color(arg_img, n_args, args, 1, NULL, 0),
                     arg_msk);
    }

    fb_alloc_free_till_mark();

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_b_xnor_obj, 2, py_image_b_xnor);

STATIC mp_obj_t py_image_erode(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    int arg_ksize =
        py_helper_arg_to_ksize(args[1]);
    int arg_threshold =
        py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold),
            py_helper_ksize_to_n(arg_ksize) - 1);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 3, kw_args);

    fb_alloc_mark();
    imlib_erode(py_helper_arg_to_image_mutable(args[0]), arg_ksize, arg_threshold, arg_msk);
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_erode_obj, 2, py_image_erode);

STATIC mp_obj_t py_image_dilate(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    int arg_ksize =
        py_helper_arg_to_ksize(args[1]);
    int arg_threshold =
        py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold),
            0);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 3, kw_args);

    fb_alloc_mark();
    imlib_dilate(py_helper_arg_to_image_mutable(args[0]), arg_ksize, arg_threshold, arg_msk);
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_dilate_obj, 2, py_image_dilate);

STATIC mp_obj_t py_image_open(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    int arg_ksize =
        py_helper_arg_to_ksize(args[1]);
    int arg_threshold =
        py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold), 0);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 3, kw_args);

    fb_alloc_mark();
    imlib_open(py_helper_arg_to_image_mutable(args[0]), arg_ksize, arg_threshold, arg_msk);
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_open_obj, 2, py_image_open);

STATIC mp_obj_t py_image_close(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    int arg_ksize =
        py_helper_arg_to_ksize(args[1]);
    int arg_threshold =
        py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold), 0);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 3, kw_args);

    fb_alloc_mark();
    imlib_close(py_helper_arg_to_image_mutable(args[0]), arg_ksize, arg_threshold, arg_msk);
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_close_obj, 2, py_image_close);
#endif // IMLIB_ENABLE_BINARY_OPS

#ifdef IMLIB_ENABLE_MATH_OPS
///////////////
// Math Methods
///////////////

STATIC mp_obj_t py_image_top_hat(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    int arg_ksize =
        py_helper_arg_to_ksize(args[1]);
    int arg_threshold =
        py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold), 0);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 3, kw_args);

    fb_alloc_mark();
    imlib_top_hat(py_helper_arg_to_image_mutable(args[0]), arg_ksize, arg_threshold, arg_msk);
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_top_hat_obj, 2, py_image_top_hat);

STATIC mp_obj_t py_image_black_hat(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    int arg_ksize =
        py_helper_arg_to_ksize(args[1]);
    int arg_threshold =
        py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold), 0);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 3, kw_args);

    fb_alloc_mark();
    imlib_black_hat(py_helper_arg_to_image_mutable(args[0]), arg_ksize, arg_threshold, arg_msk);
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_black_hat_obj, 2, py_image_black_hat);

STATIC mp_obj_t py_image_gamma_corr(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    float arg_gamma =
        py_helper_keyword_float(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_gamma), 1.0f);
    float arg_contrast =
        py_helper_keyword_float(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_contrast), 1.0f);
    float arg_brightness =
        py_helper_keyword_float(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_brightness), 0.0f);

    fb_alloc_mark();
    imlib_gamma_corr(arg_img, arg_gamma, arg_contrast, arg_brightness);
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_gamma_corr_obj, 1, py_image_gamma_corr);

STATIC mp_obj_t py_image_negate(mp_obj_t img_obj)
{
    imlib_negate(py_helper_arg_to_image_mutable(img_obj));
    return img_obj;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_image_negate_obj, py_image_negate);

STATIC mp_obj_t py_image_replace(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    bool arg_hmirror =
        py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_hmirror), false);
    bool arg_vflip =
        py_helper_keyword_int(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_vflip), false);
    bool arg_transpose =
        py_helper_keyword_int(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_transpose), false);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 5, kw_args);

    if (arg_transpose) {
        size_t size0 = image_size(arg_img);
        int w = arg_img->w;
        int h = arg_img->h;
        arg_img->w = h;
        arg_img->h = w;
        size_t size1 = image_size(arg_img);
        arg_img->w = w;
        arg_img->h = h;
        PY_ASSERT_TRUE_MSG(size1 <= size0,
                           "Unable to transpose the image because it would grow in size!");
    }

    fb_alloc_mark();

    mp_obj_t arg_1 = (n_args > 1) ? args[1] : args[0];

    if (MP_OBJ_IS_STR(arg_1)) {
        imlib_replace(arg_img, mp_obj_str_get_str(arg_1), NULL, 0,
                      arg_hmirror, arg_vflip, arg_transpose, arg_msk);
    } else if (MP_OBJ_IS_TYPE(arg_1, &py_image_type)) {
        imlib_replace(arg_img, NULL, py_helper_arg_to_image_mutable(arg_1), 0,
                      arg_hmirror, arg_vflip, arg_transpose, arg_msk);
    } else {
        imlib_replace(arg_img, NULL, NULL,
                      py_helper_keyword_color(arg_img, n_args, args, 1, NULL, 0),
                      arg_hmirror, arg_vflip, arg_transpose, arg_msk);
    }

    fb_alloc_free_till_mark();

    if (is_img_data_in_main_fb(arg_img->data)) {
        MAIN_FB()->w = arg_img->w;
        MAIN_FB()->h = arg_img->h;
    }

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_replace_obj, 1, py_image_replace);

STATIC mp_obj_t py_image_add(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 2, kw_args);

    fb_alloc_mark();

    if (MP_OBJ_IS_STR(args[1])) {
        imlib_add(arg_img, mp_obj_str_get_str(args[1]), NULL, 0, arg_msk);
    } else if (MP_OBJ_IS_TYPE(args[1], &py_image_type)) {
        imlib_add(arg_img, NULL, py_helper_arg_to_image_mutable(args[1]), 0, arg_msk);
    } else {
        imlib_add(arg_img, NULL, NULL,
                  py_helper_keyword_color(arg_img, n_args, args, 1, NULL, 0),
                  arg_msk);
    }

    fb_alloc_free_till_mark();

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_add_obj, 2, py_image_add);

STATIC mp_obj_t py_image_sub(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    bool arg_reverse =
        py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_reverse), false);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 3, kw_args);

    fb_alloc_mark();

    if (MP_OBJ_IS_STR(args[1])) {
        imlib_sub(arg_img, mp_obj_str_get_str(args[1]), NULL, 0, arg_reverse, arg_msk);
    } else if (MP_OBJ_IS_TYPE(args[1], &py_image_type)) {
        imlib_sub(arg_img, NULL, py_helper_arg_to_image_mutable(args[1]), 0, arg_reverse, arg_msk);
    } else {
        imlib_sub(arg_img, NULL, NULL,
                  py_helper_keyword_color(arg_img, n_args, args, 1, NULL, 0),
                  arg_reverse, arg_msk);
    }

    fb_alloc_free_till_mark();

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_sub_obj, 2, py_image_sub);

STATIC mp_obj_t py_image_mul(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    bool arg_invert =
        py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_invert), false);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 3, kw_args);

    fb_alloc_mark();

    if (MP_OBJ_IS_STR(args[1])) {
        imlib_mul(arg_img, mp_obj_str_get_str(args[1]), NULL, 0, arg_invert, arg_msk);
    } else if (MP_OBJ_IS_TYPE(args[1], &py_image_type)) {
        imlib_mul(arg_img, NULL, py_helper_arg_to_image_mutable(args[1]), 0, arg_invert, arg_msk);
    } else {
        imlib_mul(arg_img, NULL, NULL,
                  py_helper_keyword_color(arg_img, n_args, args, 1, NULL, 0),
                  arg_invert, arg_msk);
    }

    fb_alloc_free_till_mark();

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_mul_obj, 2, py_image_mul);

STATIC mp_obj_t py_image_div(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    bool arg_invert =
        py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_invert), false);
    bool arg_mod =
        py_helper_keyword_int(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_mod), false);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 4, kw_args);

    fb_alloc_mark();

    if (MP_OBJ_IS_STR(args[1])) {
        imlib_div(arg_img, mp_obj_str_get_str(args[1]), NULL, 0,
                  arg_invert, arg_mod, arg_msk);
    } else if (MP_OBJ_IS_TYPE(args[1], &py_image_type)) {
        imlib_div(arg_img, NULL, py_helper_arg_to_image_mutable(args[1]), 0,
                  arg_invert, arg_mod, arg_msk);
    } else {
        imlib_div(arg_img, NULL, NULL,
                  py_helper_keyword_color(arg_img, n_args, args, 1, NULL, 0),
                  arg_invert, arg_mod, arg_msk);
    }

    fb_alloc_free_till_mark();

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_div_obj, 2, py_image_div);

STATIC mp_obj_t py_image_min(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 2, kw_args);

    fb_alloc_mark();

    if (MP_OBJ_IS_STR(args[1])) {
        imlib_min(arg_img, mp_obj_str_get_str(args[1]), NULL, 0, arg_msk);
    } else if (MP_OBJ_IS_TYPE(args[1], &py_image_type)) {
        imlib_min(arg_img, NULL, py_helper_arg_to_image_mutable(args[1]), 0, arg_msk);
    } else {
        imlib_min(arg_img, NULL, NULL,
                  py_helper_keyword_color(arg_img, n_args, args, 1, NULL, 0),
                  arg_msk);
    }

    fb_alloc_free_till_mark();

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_min_obj, 2, py_image_min);

STATIC mp_obj_t py_image_max(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 2, kw_args);

    fb_alloc_mark();

    if (MP_OBJ_IS_STR(args[1])) {
        imlib_max(arg_img, mp_obj_str_get_str(args[1]), NULL, 0, arg_msk);
    } else if (MP_OBJ_IS_TYPE(args[1], &py_image_type)) {
        imlib_max(arg_img, NULL, py_helper_arg_to_image_mutable(args[1]), 0, arg_msk);
    } else {
        imlib_max(arg_img, NULL, NULL,
                  py_helper_keyword_color(arg_img, n_args, args, 1, NULL, 0),
                  arg_msk);
    }

    fb_alloc_free_till_mark();

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_max_obj, 2, py_image_max);

STATIC mp_obj_t py_image_difference(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 2, kw_args);

    fb_alloc_mark();

    if (MP_OBJ_IS_STR(args[1])) {
        imlib_difference(arg_img, mp_obj_str_get_str(args[1]), NULL, 0, arg_msk);
    } else if (MP_OBJ_IS_TYPE(args[1], &py_image_type)) {
        imlib_difference(arg_img, NULL, py_helper_arg_to_image_mutable(args[1]), 0, arg_msk);
    } else {
        imlib_difference(arg_img, NULL, NULL,
                         py_helper_keyword_color(arg_img, n_args, args, 1, NULL, 0),
                         arg_msk);
    }

    fb_alloc_free_till_mark();

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_difference_obj, 2, py_image_difference);

STATIC mp_obj_t py_image_blend(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    float arg_alpha =
        py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_alpha), 128) / 256.0f;
    PY_ASSERT_TRUE_MSG((0 <= arg_alpha) && (arg_alpha <= 1), "Error: 0 <= alpha <= 256!");
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 3, kw_args);

    fb_alloc_mark();

    if (MP_OBJ_IS_STR(args[1])) {
        imlib_blend(arg_img, mp_obj_str_get_str(args[1]), NULL, 0, arg_alpha, arg_msk);
    } else if (MP_OBJ_IS_TYPE(args[1], &py_image_type)) {
        imlib_blend(arg_img, NULL, py_helper_arg_to_image_mutable(args[1]), 0, arg_alpha, arg_msk);
    } else {
        imlib_blend(arg_img, NULL, NULL,
                    py_helper_keyword_color(arg_img, n_args, args, 1, NULL, 0),
                    arg_alpha, arg_msk);
    }

    fb_alloc_free_till_mark();

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_blend_obj, 2, py_image_blend);
#endif//IMLIB_ENABLE_MATH_OPS

////////////////////
// Filtering Methods
////////////////////
#ifndef OMV_MINIMUM
static mp_obj_t py_image_histeq(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    bool arg_adaptive =
        py_helper_keyword_int(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_adaptive), false);
    float arg_clip_limit =
        py_helper_keyword_float(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_clip_limit), -1);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 3, kw_args);

    fb_alloc_mark();
    if (arg_adaptive) imlib_clahe_histeq(arg_img, arg_clip_limit, arg_msk); else imlib_histeq(arg_img, arg_msk);
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_histeq_obj, 1, py_image_histeq);

#endif //OMV_MINIMUM

#ifdef IMLIB_ENABLE_MEAN
STATIC mp_obj_t py_image_mean(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    int arg_ksize =
        py_helper_arg_to_ksize(args[1]);
    bool arg_threshold =
        py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold), false);
    int arg_offset =
        py_helper_keyword_int(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_offset), 0);
    bool arg_invert =
        py_helper_keyword_int(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_invert), false);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 5, kw_args);

    fb_alloc_mark();
    imlib_mean_filter(arg_img, arg_ksize, arg_threshold, arg_offset, arg_invert, arg_msk);
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_mean_obj, 2, py_image_mean);
#endif // IMLIB_ENABLE_MEAN

#ifdef IMLIB_ENABLE_MEDIAN
STATIC mp_obj_t py_image_median(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    int arg_ksize =
        py_helper_arg_to_ksize(args[1]);
    float arg_percentile =
        py_helper_keyword_float(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_percentile), 0.5f);
    PY_ASSERT_TRUE_MSG((0 <= arg_percentile) && (arg_percentile <= 1), "Error: 0 <= percentile <= 1!");
    bool arg_threshold =
        py_helper_keyword_int(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold), false);
    int arg_offset =
        py_helper_keyword_int(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_offset), 0);
    bool arg_invert =
        py_helper_keyword_int(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_invert), false);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 6, kw_args);

    fb_alloc_mark();
    imlib_median_filter(arg_img, arg_ksize, arg_percentile, arg_threshold, arg_offset, arg_invert, arg_msk);
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_median_obj, 2, py_image_median);
#endif // IMLIB_ENABLE_MEDIAN

#ifdef IMLIB_ENABLE_MODE
STATIC mp_obj_t py_image_mode(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    int arg_ksize =
        py_helper_arg_to_ksize(args[1]);
    bool arg_threshold =
        py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold), false);
    int arg_offset =
        py_helper_keyword_int(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_offset), 0);
    bool arg_invert =
        py_helper_keyword_int(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_invert), false);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 5, kw_args);

    fb_alloc_mark();
    imlib_mode_filter(arg_img, arg_ksize, arg_threshold, arg_offset, arg_invert, arg_msk);
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_mode_obj, 2, py_image_mode);
#endif // IMLIB_ENABLE_MODE

#ifdef IMLIB_ENABLE_MIDPOINT
STATIC mp_obj_t py_image_midpoint(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    int arg_ksize =
        py_helper_arg_to_ksize(args[1]);
    float arg_bias =
        py_helper_keyword_float(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_bias), 0.5f);
    PY_ASSERT_TRUE_MSG((0 <= arg_bias) && (arg_bias <= 1), "Error: 0 <= bias <= 1!");
    bool arg_threshold =
        py_helper_keyword_int(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold), false);
    int arg_offset =
        py_helper_keyword_int(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_offset), 0);
    bool arg_invert =
        py_helper_keyword_int(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_invert), false);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 6, kw_args);

    fb_alloc_mark();
    imlib_midpoint_filter(arg_img, arg_ksize, arg_bias, arg_threshold, arg_offset, arg_invert, arg_msk);
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_midpoint_obj, 2, py_image_midpoint);
#endif // IMLIB_ENABLE_MIDPOINT

#ifdef IMLIB_ENABLE_MORPH
STATIC mp_obj_t py_image_morph(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    int arg_ksize =
        py_helper_arg_to_ksize(args[1]);

    int n = py_helper_ksize_to_n(arg_ksize);

    mp_obj_t *krn;
    mp_obj_get_array_fixed_n(args[2], n, &krn);

    fb_alloc_mark();

    int *arg_krn = fb_alloc(n * sizeof(int));
    int arg_m = 0;

    for (int i = 0; i < n; i++) {
        arg_krn[i] = mp_obj_get_int(krn[i]);
        arg_m += arg_krn[i];
    }

    if (arg_m == 0) {
        arg_m = 1;
    }

    float arg_mul =
        py_helper_keyword_float(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_mul), 1.0f / arg_m);
    float arg_add =
        py_helper_keyword_float(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_add), 0.0f);
    bool arg_threshold =
        py_helper_keyword_int(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold), false);
    int arg_offset =
        py_helper_keyword_int(n_args, args, 6, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_offset), 0);
    bool arg_invert =
        py_helper_keyword_int(n_args, args, 7, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_invert), false);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 8, kw_args);

    imlib_morph(arg_img, arg_ksize, arg_krn, arg_mul, arg_add, arg_threshold, arg_offset, arg_invert, arg_msk);
    fb_free();
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_morph_obj, 3, py_image_morph);
#endif //IMLIB_ENABLE_MORPH

#ifdef IMLIB_ENABLE_GAUSSIAN
STATIC mp_obj_t py_image_gaussian(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    int arg_ksize =
        py_helper_arg_to_ksize(args[1]);

    int k_2 = arg_ksize * 2;
    int n = k_2 + 1;

    fb_alloc_mark();

    int *pascal = fb_alloc(n * sizeof(int));
    pascal[0] = 1;

    for (int i = 0; i < k_2; i++) { // Compute a row of pascal's triangle.
        pascal[i + 1] = (pascal[i] * (k_2 - i)) / (i + 1);
    }

    int *arg_krn = fb_alloc(n * n * sizeof(int));
    int arg_m = 0;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int temp = pascal[i] * pascal[j];
            arg_krn[(i * n) + j] = temp;
            arg_m += temp;
        }
    }

    if (py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_unsharp), false)) {
        arg_krn[((n/2)*n)+(n/2)] -= arg_m * 2;
        arg_m = -arg_m;
    }

    float arg_mul =
        py_helper_keyword_float(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_mul), 1.0f / arg_m);
    float arg_add =
        py_helper_keyword_float(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_add), 0.0f);
    bool arg_threshold =
        py_helper_keyword_int(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold), false);
    int arg_offset =
        py_helper_keyword_int(n_args, args, 6, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_offset), 0);
    bool arg_invert =
        py_helper_keyword_int(n_args, args, 7, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_invert), false);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 8, kw_args);

    imlib_morph(arg_img, arg_ksize, arg_krn, arg_mul, arg_add, arg_threshold, arg_offset, arg_invert, arg_msk);
    fb_free();
    fb_free();
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_gaussian_obj, 2, py_image_gaussian);
#endif // IMLIB_ENABLE_GAUSSIAN

#ifdef IMLIB_ENABLE_LAPLACIAN
STATIC mp_obj_t py_image_laplacian(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    int arg_ksize =
        py_helper_arg_to_ksize(args[1]);

    int k_2 = arg_ksize * 2;
    int n = k_2 + 1;

    fb_alloc_mark();

    int *pascal = fb_alloc(n * sizeof(int));
    pascal[0] = 1;

    for (int i = 0; i < k_2; i++) { // Compute a row of pascal's triangle.
        pascal[i + 1] = (pascal[i] * (k_2 - i)) / (i + 1);
    }

    int *arg_krn = fb_alloc(n * n * sizeof(int));
    int arg_m = 0;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int temp = pascal[i] * pascal[j];
            arg_krn[(i * n) + j] = -temp;
            arg_m += temp;
        }
    }

    arg_krn[((n/2)*n)+(n/2)] += arg_m;
    arg_m = arg_krn[((n/2)*n)+(n/2)];

    if (py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_sharpen), false)) {
        arg_krn[((n/2)*n)+(n/2)] += arg_m;
    }

    float arg_mul =
        py_helper_keyword_float(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_mul), 1.0f / arg_m);
    float arg_add =
        py_helper_keyword_float(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_add), 0.0f);
    bool arg_threshold =
        py_helper_keyword_int(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold), false);
    int arg_offset =
        py_helper_keyword_int(n_args, args, 6, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_offset), 0);
    bool arg_invert =
        py_helper_keyword_int(n_args, args, 7, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_invert), false);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 8, kw_args);

    imlib_morph(arg_img, arg_ksize, arg_krn, arg_mul, arg_add, arg_threshold, arg_offset, arg_invert, arg_msk);
    fb_free();
    fb_free();
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_laplacian_obj, 2, py_image_laplacian);
#endif // IMLIB_ENABLE_LAPLACIAN

#ifdef IMLIB_ENABLE_BILATERAL
STATIC mp_obj_t py_image_bilateral(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    int arg_ksize =
        py_helper_arg_to_ksize(args[1]);
    float arg_color_sigma =
        py_helper_keyword_float(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_color_sigma), 0.1);
    float arg_space_sigma =
        py_helper_keyword_float(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_space_sigma), 1);
    bool arg_threshold =
        py_helper_keyword_int(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold), false);
    int arg_offset =
        py_helper_keyword_int(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_offset), 0);
    bool arg_invert =
        py_helper_keyword_int(n_args, args, 6, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_invert), false);
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 7, kw_args);

    fb_alloc_mark();
    imlib_bilateral_filter(arg_img, arg_ksize, arg_color_sigma, arg_space_sigma, arg_threshold, arg_offset, arg_invert, arg_msk);
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_bilateral_obj, 2, py_image_bilateral);
#endif // IMLIB_ENABLE_BILATERAL

#ifdef IMLIB_ENABLE_CARTOON
STATIC mp_obj_t py_image_cartoon(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    float arg_seed_threshold =
        py_helper_keyword_float(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_seed_threshold), 0.05);
    PY_ASSERT_TRUE_MSG((0.0f <= arg_seed_threshold) && (arg_seed_threshold <= 1.0f),
                       "Error: 0.0 <= seed_threshold <= 1.0!");
    float arg_floating_threshold =
        py_helper_keyword_float(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_floating_threshold), 0.05);
    PY_ASSERT_TRUE_MSG((0.0f <= arg_floating_threshold) && (arg_floating_threshold <= 1.0f),
                       "Error: 0.0 <= floating_threshold <= 1.0!");
    image_t *arg_msk =
        py_helper_keyword_to_image_mutable_mask(n_args, args, 3, kw_args);

    fb_alloc_mark();
    imlib_cartoon_filter(arg_img, arg_seed_threshold, arg_floating_threshold, arg_msk);
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_cartoon_obj, 1, py_image_cartoon);
#endif // IMLIB_ENABLE_CARTOON

/////////////////////////
// Shadow Removal Methods
/////////////////////////

#ifdef IMLIB_ENABLE_REMOVE_SHADOWS
STATIC mp_obj_t py_image_remove_shadows(size_t n_args, const mp_obj_t *args)
{
    image_t *arg_img =
        py_helper_arg_to_image_color(args[0]);

    fb_alloc_mark();

    if (n_args < 2) {
        imlib_remove_shadows(arg_img, NULL, NULL, 0, true);
    } else if (MP_OBJ_IS_STR(args[1])) {
        imlib_remove_shadows(arg_img, mp_obj_str_get_str(args[1]), NULL, 0, false);
    } else if (MP_OBJ_IS_TYPE(args[1], &py_image_type)) {
        imlib_remove_shadows(arg_img, NULL, py_helper_arg_to_image_color(args[1]), 0, false);
    } else {
        imlib_remove_shadows(arg_img, NULL, NULL,
                             py_helper_keyword_color(arg_img, n_args, args, 1, NULL, 0),
                             false);
    }

    fb_alloc_free_till_mark();

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(py_image_remove_shadows_obj, 1, 2, py_image_remove_shadows);
#endif // IMLIB_ENABLE_REMOVE_SHADOWS

#ifdef IMLIB_ENABLE_CHROMINVAR
STATIC mp_obj_t py_image_chrominvar(mp_obj_t img_obj)
{
    fb_alloc_mark();
    imlib_chrominvar(py_helper_arg_to_image_color(img_obj));
    fb_alloc_free_till_mark();
    return img_obj;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_image_chrominvar_obj, py_image_chrominvar);
#endif // IMLIB_ENABLE_CHROMINVAR

#ifdef IMLIB_ENABLE_ILLUMINVAR
STATIC mp_obj_t py_image_illuminvar(mp_obj_t img_obj)
{
    fb_alloc_mark();
    imlib_illuminvar(py_helper_arg_to_image_color(img_obj));
    fb_alloc_free_till_mark();
    return img_obj;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_image_illuminvar_obj, py_image_illuminvar);
#endif // IMLIB_ENABLE_ILLUMINVAR

////////////////////
// Geometric Methods
////////////////////

#ifdef IMLIB_ENABLE_LINPOLAR
static mp_obj_t py_image_linpolar(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    bool arg_reverse =
        py_helper_keyword_int(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_reverse), false);

    fb_alloc_mark();
    imlib_logpolar(arg_img, true, arg_reverse);
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_linpolar_obj, 1, py_image_linpolar);
#endif // IMLIB_ENABLE_LINPOLAR

#ifdef IMLIB_ENABLE_LOGPOLAR
static mp_obj_t py_image_logpolar(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    bool arg_reverse =
        py_helper_keyword_int(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_reverse), false);

    fb_alloc_mark();
    imlib_logpolar(arg_img, false, arg_reverse);
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_logpolar_obj, 1, py_image_logpolar);
#endif // IMLIB_ENABLE_LOGPOLAR

STATIC mp_obj_t py_image_lens_corr(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
     float arg_strength =
        py_helper_keyword_float(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_strength), 1.8f);
    PY_ASSERT_TRUE_MSG(arg_strength > 0.0f, "Strength must be > 0!");
    float arg_zoom =
        py_helper_keyword_float(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_zoom), 1.0f);
    PY_ASSERT_TRUE_MSG(arg_zoom > 0.0f, "Zoom must be > 0!");

    fb_alloc_mark();
    imlib_lens_corr(arg_img, arg_strength, arg_zoom);
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_lens_corr_obj, 1, py_image_lens_corr);

#ifdef IMLIB_ENABLE_ROTATION_CORR
STATIC mp_obj_t py_image_rotation_corr(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
image_t *arg_img =
        py_helper_arg_to_image_mutable(args[0]);
    float arg_x_rotation =
        IM_DEG2RAD(py_helper_keyword_float(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_x_rotation), 0.0f));
    float arg_y_rotation =
        IM_DEG2RAD(py_helper_keyword_float(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_y_rotation), 0.0f));
    float arg_z_rotation =
        IM_DEG2RAD(py_helper_keyword_float(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_z_rotation), 0.0f));
    float arg_x_translation =
        py_helper_keyword_float(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_x_translation), 0.0f);
    float arg_y_translation =
        py_helper_keyword_float(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_y_translation), 0.0f);
    float arg_zoom =
        py_helper_keyword_float(n_args, args, 6, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_zoom), 1.0f);
    PY_ASSERT_TRUE_MSG(arg_zoom > 0.0f, "Zoom must be > 0!");
    float arg_fov =
        IM_DEG2RAD(py_helper_keyword_float(n_args, args, 7, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_fov), 60.0f));
    PY_ASSERT_TRUE_MSG((0.0f < arg_fov) && (arg_fov < 180.0f), "FOV must be > 0 and < 180!");
    float *arg_corners = py_helper_keyword_corner_array(n_args, args, 8, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_corners));

    fb_alloc_mark();
    imlib_rotation_corr(arg_img,
                        arg_x_rotation, arg_y_rotation, arg_z_rotation,
                        arg_x_translation, arg_y_translation,
                        arg_zoom, arg_fov, arg_corners);
    fb_alloc_free_till_mark();
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_rotation_corr_obj, 1, py_image_rotation_corr);
#endif // IMLIB_ENABLE_ROTATION_CORR

//////////////
// Get Methods
//////////////

#ifdef IMLIB_ENABLE_GET_SIMILARITY
// Similarity Object //
#define py_similarity_obj_size 4
typedef struct py_similarity_obj {
    mp_obj_base_t base;
    mp_obj_t avg, std, min, max;
} py_similarity_obj_t;

static void py_similarity_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_similarity_obj_t *self = self_in;
    mp_printf(print,
              "{\"mean\":%f, \"stdev\":%f, \"min\":%f, \"max\":%f}",
              (double) mp_obj_get_float(self->avg),
              (double) mp_obj_get_float(self->std),
              (double) mp_obj_get_float(self->min),
              (double) mp_obj_get_float(self->max));
}

static mp_obj_t py_similarity_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    if (value == MP_OBJ_SENTINEL) { // load
        py_similarity_obj_t *self = self_in;
        if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
            mp_bound_slice_t slice;
            if (!mp_seq_get_fast_slice_indexes(py_similarity_obj_size, index, &slice)) {
                nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
            }
            mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
            mp_seq_copy(result->items, &(self->avg) + slice.start, result->len, mp_obj_t);
            return result;
        }
        switch (mp_get_index(self->base.type, py_similarity_obj_size, index, false)) {
            case 0: return self->avg;
            case 1: return self->std;
            case 2: return self->min;
            case 3: return self->max;
        }
    }
    return MP_OBJ_NULL; // op not supported
}

mp_obj_t py_similarity_mean(mp_obj_t self_in) { return ((py_similarity_obj_t *) self_in)->avg; }
mp_obj_t py_similarity_stdev(mp_obj_t self_in) { return ((py_similarity_obj_t *) self_in)->std; }
mp_obj_t py_similarity_min(mp_obj_t self_in) { return ((py_similarity_obj_t *) self_in)->min; }
mp_obj_t py_similarity_max(mp_obj_t self_in) { return ((py_similarity_obj_t *) self_in)->max; }

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_similarity_mean_obj, py_similarity_mean);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_similarity_stdev_obj, py_similarity_stdev);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_similarity_min_obj, py_similarity_min);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_similarity_max_obj, py_similarity_max);

STATIC const mp_rom_map_elem_t py_similarity_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_mean), MP_ROM_PTR(&py_similarity_mean_obj) },
    { MP_ROM_QSTR(MP_QSTR_stdev), MP_ROM_PTR(&py_similarity_stdev_obj) },
    { MP_ROM_QSTR(MP_QSTR_min), MP_ROM_PTR(&py_similarity_min_obj) },
    { MP_ROM_QSTR(MP_QSTR_max), MP_ROM_PTR(&py_similarity_max_obj) }
};

STATIC MP_DEFINE_CONST_DICT(py_similarity_locals_dict, py_similarity_locals_dict_table);

static const mp_obj_type_t py_similarity_type = {
    { &mp_type_type },
    .name  = MP_QSTR_similarity,
    .print = py_similarity_print,
    .subscr = py_similarity_subscr,
    .locals_dict = (mp_obj_t) &py_similarity_locals_dict
};

static mp_obj_t py_image_get_similarity(mp_obj_t img_obj, mp_obj_t other_obj)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(img_obj);
    float avg, std, min, max;

    fb_alloc_mark();

    if (MP_OBJ_IS_STR(other_obj)) {
        imlib_get_similarity(arg_img, mp_obj_str_get_str(other_obj), NULL, 0, &avg, &std, &min, &max);
    } else if (MP_OBJ_IS_TYPE(other_obj, &py_image_type)) {
        imlib_get_similarity(arg_img, NULL, py_helper_arg_to_image_mutable(other_obj), 0, &avg, &std, &min, &max);
    } else {
        imlib_get_similarity(arg_img, NULL, NULL,
                             py_helper_keyword_color(arg_img, 1, &other_obj, 0, NULL, 0),
                             &avg, &std, &min, &max);
    }

    fb_alloc_free_till_mark();

    py_similarity_obj_t *o = m_new_obj(py_similarity_obj_t);
    o->base.type = &py_similarity_type;
    o->avg = mp_obj_new_float(avg);
    o->std = mp_obj_new_float(std);
    o->min = mp_obj_new_float(min);
    o->max = mp_obj_new_float(max);
    return o;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(py_image_get_similarity_obj, py_image_get_similarity);
#endif // IMLIB_ENABLE_GET_SIMILARITY

#ifndef OMV_MINIMUM
// Statistics Object //
#define py_statistics_obj_size 24
typedef struct py_statistics_obj {
    mp_obj_base_t base;
    image_bpp_t bpp;
    mp_obj_t LMean, LMedian, LMode, LSTDev, LMin, LMax, LLQ, LUQ,
        AMean, AMedian, AMode, ASTDev, AMin, AMax, ALQ, AUQ,
        BMean, BMedian, BMode, BSTDev, BMin, BMax, BLQ, BUQ;
} py_statistics_obj_t;

static void py_statistics_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_statistics_obj_t *self = self_in;
    switch(self->bpp) {
        case IMAGE_BPP_BINARY: {
            mp_printf(print, "{\"mean\":%d, \"median\":%d, \"mode\":%d, \"stdev\":%d, \"min\":%d, \"max\":%d, \"lq\":%d, \"uq\":%d}",
                      mp_obj_get_int(self->LMean),
                      mp_obj_get_int(self->LMedian),
                      mp_obj_get_int(self->LMode),
                      mp_obj_get_int(self->LSTDev),
                      mp_obj_get_int(self->LMin),
                      mp_obj_get_int(self->LMax),
                      mp_obj_get_int(self->LLQ),
                      mp_obj_get_int(self->LUQ));
            break;
        }
        case IMAGE_BPP_GRAYSCALE: {
            mp_printf(print, "{\"mean\":%d, \"median\":%d, \"mode\":%d, \"stdev\":%d, \"min\":%d, \"max\":%d, \"lq\":%d, \"uq\":%d}",
                      mp_obj_get_int(self->LMean),
                      mp_obj_get_int(self->LMedian),
                      mp_obj_get_int(self->LMode),
                      mp_obj_get_int(self->LSTDev),
                      mp_obj_get_int(self->LMin),
                      mp_obj_get_int(self->LMax),
                      mp_obj_get_int(self->LLQ),
                      mp_obj_get_int(self->LUQ));
            break;
        }
        case IMAGE_BPP_RGB565: {
            mp_printf(print, "{\"l_mean\":%d, \"l_median\":%d, \"l_mode\":%d, \"l_stdev\":%d, \"l_min\":%d, \"l_max\":%d, \"l_lq\":%d, \"l_uq\":%d,"
                             " \"a_mean\":%d, \"a_median\":%d, \"a_mode\":%d, \"a_stdev\":%d, \"a_min\":%d, \"a_max\":%d, \"a_lq\":%d, \"a_uq\":%d,"
                             " \"b_mean\":%d, \"b_median\":%d, \"b_mode\":%d, \"b_stdev\":%d, \"b_min\":%d, \"b_max\":%d, \"b_lq\":%d, \"b_uq\":%d}",
                      mp_obj_get_int(self->LMean),
                      mp_obj_get_int(self->LMedian),
                      mp_obj_get_int(self->LMode),
                      mp_obj_get_int(self->LSTDev),
                      mp_obj_get_int(self->LMin),
                      mp_obj_get_int(self->LMax),
                      mp_obj_get_int(self->LLQ),
                      mp_obj_get_int(self->LUQ),
                      mp_obj_get_int(self->AMean),
                      mp_obj_get_int(self->AMedian),
                      mp_obj_get_int(self->AMode),
                      mp_obj_get_int(self->ASTDev),
                      mp_obj_get_int(self->AMin),
                      mp_obj_get_int(self->AMax),
                      mp_obj_get_int(self->ALQ),
                      mp_obj_get_int(self->AUQ),
                      mp_obj_get_int(self->BMean),
                      mp_obj_get_int(self->BMedian),
                      mp_obj_get_int(self->BMode),
                      mp_obj_get_int(self->BSTDev),
                      mp_obj_get_int(self->BMin),
                      mp_obj_get_int(self->BMax),
                      mp_obj_get_int(self->BLQ),
                      mp_obj_get_int(self->BUQ));
            break;
        }
        default: {
            mp_printf(print, "{}");
            break;
        }
    }
}

static mp_obj_t py_statistics_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    if (value == MP_OBJ_SENTINEL) { // load
        py_statistics_obj_t *self = self_in;
        if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
            mp_bound_slice_t slice;
            if (!mp_seq_get_fast_slice_indexes(py_statistics_obj_size, index, &slice)) {
                nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
            }
            mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
            mp_seq_copy(result->items, &(self->LMean) + slice.start, result->len, mp_obj_t);
            return result;
        }
        switch (mp_get_index(self->base.type, py_statistics_obj_size, index, false)) {
            case 0: return self->LMean;
            case 1: return self->LMedian;
            case 2: return self->LMode;
            case 3: return self->LSTDev;
            case 4: return self->LMin;
            case 5: return self->LMax;
            case 6: return self->LLQ;
            case 7: return self->LUQ;
            case 8: return self->AMean;
            case 9: return self->AMedian;
            case 10: return self->AMode;
            case 11: return self->ASTDev;
            case 12: return self->AMin;
            case 13: return self->AMax;
            case 14: return self->ALQ;
            case 15: return self->AUQ;
            case 16: return self->BMean;
            case 17: return self->BMedian;
            case 18: return self->BMode;
            case 19: return self->BSTDev;
            case 20: return self->BMin;
            case 21: return self->BMax;
            case 22: return self->BLQ;
            case 23: return self->BUQ;
        }
    }
    return MP_OBJ_NULL; // op not supported
}

mp_obj_t py_statistics_mean(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->LMean; }
mp_obj_t py_statistics_median(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->LMedian; }
mp_obj_t py_statistics_mode(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->LMode; }
mp_obj_t py_statistics_stdev(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->LSTDev; }
mp_obj_t py_statistics_min(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->LMin; }
mp_obj_t py_statistics_max(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->LMax; }
mp_obj_t py_statistics_lq(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->LLQ; }
mp_obj_t py_statistics_uq(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->LUQ; }
mp_obj_t py_statistics_l_mean(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->LMean; }
mp_obj_t py_statistics_l_median(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->LMedian; }
mp_obj_t py_statistics_l_mode(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->LMode; }
mp_obj_t py_statistics_l_stdev(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->LSTDev; }
mp_obj_t py_statistics_l_min(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->LMin; }
mp_obj_t py_statistics_l_max(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->LMax; }
mp_obj_t py_statistics_l_lq(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->LLQ; }
mp_obj_t py_statistics_l_uq(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->LUQ; }
mp_obj_t py_statistics_a_mean(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->AMean; }
mp_obj_t py_statistics_a_median(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->AMedian; }
mp_obj_t py_statistics_a_mode(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->AMode; }
mp_obj_t py_statistics_a_stdev(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->ASTDev; }
mp_obj_t py_statistics_a_min(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->AMin; }
mp_obj_t py_statistics_a_max(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->AMax; }
mp_obj_t py_statistics_a_lq(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->ALQ; }
mp_obj_t py_statistics_a_uq(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->AUQ; }
mp_obj_t py_statistics_b_mean(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->BMean; }
mp_obj_t py_statistics_b_median(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->BMedian; }
mp_obj_t py_statistics_b_mode(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->BMode; }
mp_obj_t py_statistics_b_stdev(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->BSTDev; }
mp_obj_t py_statistics_b_min(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->BMin; }
mp_obj_t py_statistics_b_max(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->BMax; }
mp_obj_t py_statistics_b_lq(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->BLQ; }
mp_obj_t py_statistics_b_uq(mp_obj_t self_in) { return ((py_statistics_obj_t *) self_in)->BUQ; }

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_mean_obj, py_statistics_mean);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_median_obj, py_statistics_median);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_mode_obj, py_statistics_mode);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_stdev_obj, py_statistics_stdev);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_min_obj, py_statistics_min);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_max_obj, py_statistics_max);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_lq_obj, py_statistics_lq);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_uq_obj, py_statistics_uq);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_l_mean_obj, py_statistics_l_mean);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_l_median_obj, py_statistics_l_median);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_l_mode_obj, py_statistics_l_mode);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_l_stdev_obj, py_statistics_l_stdev);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_l_min_obj, py_statistics_l_min);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_l_max_obj, py_statistics_l_max);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_l_lq_obj, py_statistics_l_lq);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_l_uq_obj, py_statistics_l_uq);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_a_mean_obj, py_statistics_a_mean);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_a_median_obj, py_statistics_a_median);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_a_mode_obj, py_statistics_a_mode);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_a_stdev_obj, py_statistics_a_stdev);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_a_min_obj, py_statistics_a_min);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_a_max_obj, py_statistics_a_max);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_a_lq_obj, py_statistics_a_lq);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_a_uq_obj, py_statistics_a_uq);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_b_mean_obj, py_statistics_b_mean);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_b_median_obj, py_statistics_b_median);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_b_mode_obj, py_statistics_b_mode);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_b_stdev_obj, py_statistics_b_stdev);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_b_min_obj, py_statistics_b_min);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_b_max_obj, py_statistics_b_max);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_b_lq_obj, py_statistics_b_lq);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_statistics_b_uq_obj, py_statistics_b_uq);

STATIC const mp_rom_map_elem_t py_statistics_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_mean), MP_ROM_PTR(&py_statistics_mean_obj) },
    { MP_ROM_QSTR(MP_QSTR_median), MP_ROM_PTR(&py_statistics_median_obj) },
    { MP_ROM_QSTR(MP_QSTR_mode), MP_ROM_PTR(&py_statistics_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_stdev), MP_ROM_PTR(&py_statistics_stdev_obj) },
    { MP_ROM_QSTR(MP_QSTR_min), MP_ROM_PTR(&py_statistics_min_obj) },
    { MP_ROM_QSTR(MP_QSTR_max), MP_ROM_PTR(&py_statistics_max_obj) },
    { MP_ROM_QSTR(MP_QSTR_lq), MP_ROM_PTR(&py_statistics_lq_obj) },
    { MP_ROM_QSTR(MP_QSTR_uq), MP_ROM_PTR(&py_statistics_uq_obj) },
    { MP_ROM_QSTR(MP_QSTR_l_mean), MP_ROM_PTR(&py_statistics_l_mean_obj) },
    { MP_ROM_QSTR(MP_QSTR_l_median), MP_ROM_PTR(&py_statistics_l_median_obj) },
    { MP_ROM_QSTR(MP_QSTR_l_mode), MP_ROM_PTR(&py_statistics_l_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_l_stdev), MP_ROM_PTR(&py_statistics_l_stdev_obj) },
    { MP_ROM_QSTR(MP_QSTR_l_min), MP_ROM_PTR(&py_statistics_l_min_obj) },
    { MP_ROM_QSTR(MP_QSTR_l_max), MP_ROM_PTR(&py_statistics_l_max_obj) },
    { MP_ROM_QSTR(MP_QSTR_l_lq), MP_ROM_PTR(&py_statistics_l_lq_obj) },
    { MP_ROM_QSTR(MP_QSTR_l_uq), MP_ROM_PTR(&py_statistics_l_uq_obj) },
    { MP_ROM_QSTR(MP_QSTR_a_mean), MP_ROM_PTR(&py_statistics_a_mean_obj) },
    { MP_ROM_QSTR(MP_QSTR_a_median), MP_ROM_PTR(&py_statistics_a_median_obj) },
    { MP_ROM_QSTR(MP_QSTR_a_mode), MP_ROM_PTR(&py_statistics_a_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_a_stdev), MP_ROM_PTR(&py_statistics_a_stdev_obj) },
    { MP_ROM_QSTR(MP_QSTR_a_min), MP_ROM_PTR(&py_statistics_a_min_obj) },
    { MP_ROM_QSTR(MP_QSTR_a_max), MP_ROM_PTR(&py_statistics_a_max_obj) },
    { MP_ROM_QSTR(MP_QSTR_a_lq), MP_ROM_PTR(&py_statistics_a_lq_obj) },
    { MP_ROM_QSTR(MP_QSTR_a_uq), MP_ROM_PTR(&py_statistics_a_uq_obj) },
    { MP_ROM_QSTR(MP_QSTR_b_mean), MP_ROM_PTR(&py_statistics_b_mean_obj) },
    { MP_ROM_QSTR(MP_QSTR_b_median), MP_ROM_PTR(&py_statistics_b_median_obj) },
    { MP_ROM_QSTR(MP_QSTR_b_mode), MP_ROM_PTR(&py_statistics_b_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_b_stdev), MP_ROM_PTR(&py_statistics_b_stdev_obj) },
    { MP_ROM_QSTR(MP_QSTR_b_min), MP_ROM_PTR(&py_statistics_b_min_obj) },
    { MP_ROM_QSTR(MP_QSTR_b_max), MP_ROM_PTR(&py_statistics_b_max_obj) },
    { MP_ROM_QSTR(MP_QSTR_b_lq), MP_ROM_PTR(&py_statistics_b_lq_obj) },
    { MP_ROM_QSTR(MP_QSTR_b_uq), MP_ROM_PTR(&py_statistics_b_uq_obj) }
};

STATIC MP_DEFINE_CONST_DICT(py_statistics_locals_dict, py_statistics_locals_dict_table);

static const mp_obj_type_t py_statistics_type = {
    { &mp_type_type },
    .name  = MP_QSTR_statistics,
    .print = py_statistics_print,
    .subscr = py_statistics_subscr,
    .locals_dict = (mp_obj_t) &py_statistics_locals_dict
};

// Percentile Object //
#define py_percentile_obj_size 3
typedef struct py_percentile_obj {
    mp_obj_base_t base;
    image_bpp_t bpp;
    mp_obj_t LValue, AValue, BValue;
} py_percentile_obj_t;

static void py_percentile_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_percentile_obj_t *self = self_in;
    switch(self->bpp) {
        case IMAGE_BPP_BINARY: {
            mp_printf(print, "{\"value\":%d}",
                      mp_obj_get_int(self->LValue));
            break;
        }
        case IMAGE_BPP_GRAYSCALE: {
            mp_printf(print, "{\"value\":%d}",
                      mp_obj_get_int(self->LValue));
            break;
        }
        case IMAGE_BPP_RGB565: {
            mp_printf(print, "{\"l_value:%d\", \"a_value\":%d, \"b_value\":%d}",
                      mp_obj_get_int(self->LValue),
                      mp_obj_get_int(self->AValue),
                      mp_obj_get_int(self->BValue));
            break;
        }
        default: {
            mp_printf(print, "{}");
            break;
        }
    }
}

static mp_obj_t py_percentile_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    if (value == MP_OBJ_SENTINEL) { // load
        py_percentile_obj_t *self = self_in;
        if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
            mp_bound_slice_t slice;
            if (!mp_seq_get_fast_slice_indexes(py_percentile_obj_size, index, &slice)) {
                nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
            }
            mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
            mp_seq_copy(result->items, &(self->LValue) + slice.start, result->len, mp_obj_t);
            return result;
        }
        switch (mp_get_index(self->base.type, py_percentile_obj_size, index, false)) {
            case 0: return self->LValue;
            case 1: return self->AValue;
            case 2: return self->BValue;
        }
    }
    return MP_OBJ_NULL; // op not supported
}

mp_obj_t py_percentile_value(mp_obj_t self_in) { return ((py_percentile_obj_t *) self_in)->LValue; }
mp_obj_t py_percentile_l_value(mp_obj_t self_in) { return ((py_percentile_obj_t *) self_in)->LValue; }
mp_obj_t py_percentile_a_value(mp_obj_t self_in) { return ((py_percentile_obj_t *) self_in)->AValue; }
mp_obj_t py_percentile_b_value(mp_obj_t self_in) { return ((py_percentile_obj_t *) self_in)->BValue; }

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_percentile_value_obj, py_percentile_value);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_percentile_l_value_obj, py_percentile_l_value);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_percentile_a_value_obj, py_percentile_a_value);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_percentile_b_value_obj, py_percentile_b_value);

STATIC const mp_rom_map_elem_t py_percentile_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&py_percentile_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_l_value), MP_ROM_PTR(&py_percentile_l_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_a_value), MP_ROM_PTR(&py_percentile_a_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_b_value), MP_ROM_PTR(&py_percentile_b_value_obj) }
};

STATIC MP_DEFINE_CONST_DICT(py_percentile_locals_dict, py_percentile_locals_dict_table);

static const mp_obj_type_t py_percentile_type = {
    { &mp_type_type },
    .name  = MP_QSTR_percentile,
    .print = py_percentile_print,
    .subscr = py_percentile_subscr,
    .locals_dict = (mp_obj_t) &py_percentile_locals_dict
};

// Threshold Object //
#define py_threshold_obj_size 3
typedef struct py_threshold_obj {
    mp_obj_base_t base;
    image_bpp_t bpp;
    mp_obj_t LValue, AValue, BValue;
} py_threshold_obj_t;

static void py_threshold_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_threshold_obj_t *self = self_in;
    switch(self->bpp) {
        case IMAGE_BPP_BINARY: {
            mp_printf(print, "{\"value\":%d}",
                      mp_obj_get_int(self->LValue));
            break;
        }
        case IMAGE_BPP_GRAYSCALE: {
            mp_printf(print, "{\"value\":%d}",
                      mp_obj_get_int(self->LValue));
            break;
        }
        case IMAGE_BPP_RGB565: {
            mp_printf(print, "{\"l_value\":%d, \"a_value\":%d, \"b_value\":%d}",
                      mp_obj_get_int(self->LValue),
                      mp_obj_get_int(self->AValue),
                      mp_obj_get_int(self->BValue));
            break;
        }
        default: {
            mp_printf(print, "{}");
            break;
        }
    }
}

static mp_obj_t py_threshold_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    if (value == MP_OBJ_SENTINEL) { // load
        py_threshold_obj_t *self = self_in;
        if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
            mp_bound_slice_t slice;
            if (!mp_seq_get_fast_slice_indexes(py_threshold_obj_size, index, &slice)) {
                nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
            }
            mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
            mp_seq_copy(result->items, &(self->LValue) + slice.start, result->len, mp_obj_t);
            return result;
        }
        switch (mp_get_index(self->base.type, py_threshold_obj_size, index, false)) {
            case 0: return self->LValue;
            case 1: return self->AValue;
            case 2: return self->BValue;
        }
    }
    return MP_OBJ_NULL; // op not supported
}

mp_obj_t py_threshold_value(mp_obj_t self_in) { return ((py_threshold_obj_t *) self_in)->LValue; }
mp_obj_t py_threshold_l_value(mp_obj_t self_in) { return ((py_threshold_obj_t *) self_in)->LValue; }
mp_obj_t py_threshold_a_value(mp_obj_t self_in) { return ((py_threshold_obj_t *) self_in)->AValue; }
mp_obj_t py_threshold_b_value(mp_obj_t self_in) { return ((py_threshold_obj_t *) self_in)->BValue; }

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_threshold_value_obj, py_threshold_value);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_threshold_l_value_obj, py_threshold_l_value);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_threshold_a_value_obj, py_threshold_a_value);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_threshold_b_value_obj, py_threshold_b_value);

STATIC const mp_rom_map_elem_t py_threshold_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&py_threshold_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_l_value), MP_ROM_PTR(&py_threshold_l_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_a_value), MP_ROM_PTR(&py_threshold_a_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_b_value), MP_ROM_PTR(&py_threshold_b_value_obj) }
};

STATIC MP_DEFINE_CONST_DICT(py_threshold_locals_dict, py_threshold_locals_dict_table);

static const mp_obj_type_t py_threshold_type = {
    { &mp_type_type },
    .name  = MP_QSTR_threshold,
    .print = py_threshold_print,
    .subscr = py_threshold_subscr,
    .locals_dict = (mp_obj_t) &py_threshold_locals_dict
};

// Histogram Object //
#define py_histogram_obj_size 3
typedef struct py_histogram_obj {
    mp_obj_base_t base;
    image_bpp_t bpp;
    mp_obj_t LBins, ABins, BBins;
} py_histogram_obj_t;

static void py_histogram_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_histogram_obj_t *self = self_in;
    switch(self->bpp) {
        case IMAGE_BPP_BINARY: {
            mp_printf(print, "{\"bins\":");
            mp_obj_print_helper(print, self->LBins, kind);
            mp_printf(print, "}");
            break;
        }
        case IMAGE_BPP_GRAYSCALE: {
            mp_printf(print, "{\"bins\":");
            mp_obj_print_helper(print, self->LBins, kind);
            mp_printf(print, "}");
            break;
        }
        case IMAGE_BPP_RGB565: {
            mp_printf(print, "{\"l_bins\":");
            mp_obj_print_helper(print, self->LBins, kind);
            mp_printf(print, ", \"a_bins\":");
            mp_obj_print_helper(print, self->ABins, kind);
            mp_printf(print, ", \"b_bins\":");
            mp_obj_print_helper(print, self->BBins, kind);
            mp_printf(print, "}");
            break;
        }
        default: {
            mp_printf(print, "{}");
            break;
        }
    }
}

static mp_obj_t py_histogram_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    if (value == MP_OBJ_SENTINEL) { // load
        py_histogram_obj_t *self = self_in;
        if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
            mp_bound_slice_t slice;
            if (!mp_seq_get_fast_slice_indexes(py_histogram_obj_size, index, &slice)) {
                nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
            }
            mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
            mp_seq_copy(result->items, &(self->LBins) + slice.start, result->len, mp_obj_t);
            return result;
        }
        switch (mp_get_index(self->base.type, py_histogram_obj_size, index, false)) {
            case 0: return self->LBins;
            case 1: return self->ABins;
            case 2: return self->BBins;
        }
    }
    return MP_OBJ_NULL; // op not supported
}

mp_obj_t py_histogram_bins(mp_obj_t self_in) { return ((py_histogram_obj_t *) self_in)->LBins; }
mp_obj_t py_histogram_l_bins(mp_obj_t self_in) { return ((py_histogram_obj_t *) self_in)->LBins; }
mp_obj_t py_histogram_a_bins(mp_obj_t self_in) { return ((py_histogram_obj_t *) self_in)->ABins; }
mp_obj_t py_histogram_b_bins(mp_obj_t self_in) { return ((py_histogram_obj_t *) self_in)->BBins; }

mp_obj_t py_histogram_get_percentile(mp_obj_t self_in, mp_obj_t percentile)
{
    histogram_t hist;
    hist.LBinCount = ((mp_obj_list_t *) ((py_histogram_obj_t *) self_in)->LBins)->len;
    hist.ABinCount = ((mp_obj_list_t *) ((py_histogram_obj_t *) self_in)->ABins)->len;
    hist.BBinCount = ((mp_obj_list_t *) ((py_histogram_obj_t *) self_in)->BBins)->len;
    fb_alloc_mark();
    hist.LBins = fb_alloc(hist.LBinCount * sizeof(float));
    hist.ABins = fb_alloc(hist.ABinCount * sizeof(float));
    hist.BBins = fb_alloc(hist.BBinCount * sizeof(float));

    for (int i = 0; i < hist.LBinCount; i++) {
        hist.LBins[i] = mp_obj_get_float(((mp_obj_list_t *) ((py_histogram_obj_t *) self_in)->LBins)->items[i]);
    }

    for (int i = 0; i < hist.ABinCount; i++) {
        hist.ABins[i] = mp_obj_get_float(((mp_obj_list_t *) ((py_histogram_obj_t *) self_in)->ABins)->items[i]);
    }

    for (int i = 0; i < hist.BBinCount; i++) {
        hist.BBins[i] = mp_obj_get_float(((mp_obj_list_t *) ((py_histogram_obj_t *) self_in)->BBins)->items[i]);
    }

    percentile_t p;
    imlib_get_percentile(&p, ((py_histogram_obj_t *) self_in)->bpp, &hist, mp_obj_get_float(percentile));
    if (hist.BBinCount) fb_free();
    if (hist.ABinCount) fb_free();
    if (hist.LBinCount) fb_free();
    fb_alloc_free_till_mark();

    py_percentile_obj_t *o = m_new_obj(py_percentile_obj_t);
    o->base.type = &py_percentile_type;
    o->bpp = ((py_histogram_obj_t *) self_in)->bpp;

    o->LValue = mp_obj_new_int(p.LValue);
    o->AValue = mp_obj_new_int(p.AValue);
    o->BValue = mp_obj_new_int(p.BValue);

    return o;
}

mp_obj_t py_histogram_get_threshold(mp_obj_t self_in)
{
    histogram_t hist;
    hist.LBinCount = ((mp_obj_list_t *) ((py_histogram_obj_t *) self_in)->LBins)->len;
    hist.ABinCount = ((mp_obj_list_t *) ((py_histogram_obj_t *) self_in)->ABins)->len;
    hist.BBinCount = ((mp_obj_list_t *) ((py_histogram_obj_t *) self_in)->BBins)->len;
    fb_alloc_mark();
    hist.LBins = fb_alloc(hist.LBinCount * sizeof(float));
    hist.ABins = fb_alloc(hist.ABinCount * sizeof(float));
    hist.BBins = fb_alloc(hist.BBinCount * sizeof(float));

    for (int i = 0; i < hist.LBinCount; i++) {
        hist.LBins[i] = mp_obj_get_float(((mp_obj_list_t *) ((py_histogram_obj_t *) self_in)->LBins)->items[i]);
    }

    for (int i = 0; i < hist.ABinCount; i++) {
        hist.ABins[i] = mp_obj_get_float(((mp_obj_list_t *) ((py_histogram_obj_t *) self_in)->ABins)->items[i]);
    }

    for (int i = 0; i < hist.BBinCount; i++) {
        hist.BBins[i] = mp_obj_get_float(((mp_obj_list_t *) ((py_histogram_obj_t *) self_in)->BBins)->items[i]);
    }

    threshold_t t;
    imlib_get_threshold(&t, ((py_histogram_obj_t *) self_in)->bpp, &hist);
    if (hist.BBinCount) fb_free();
    if (hist.ABinCount) fb_free();
    if (hist.LBinCount) fb_free();
    fb_alloc_free_till_mark();

    py_threshold_obj_t *o = m_new_obj(py_threshold_obj_t);
    o->base.type = &py_threshold_type;
    o->bpp = ((py_threshold_obj_t *) self_in)->bpp;

    o->LValue = mp_obj_new_int(t.LValue);
    o->AValue = mp_obj_new_int(t.AValue);
    o->BValue = mp_obj_new_int(t.BValue);

    return o;
}

mp_obj_t py_histogram_get_statistics(mp_obj_t self_in)
{
    histogram_t hist;
    hist.LBinCount = ((mp_obj_list_t *) ((py_histogram_obj_t *) self_in)->LBins)->len;
    hist.ABinCount = ((mp_obj_list_t *) ((py_histogram_obj_t *) self_in)->ABins)->len;
    hist.BBinCount = ((mp_obj_list_t *) ((py_histogram_obj_t *) self_in)->BBins)->len;
    fb_alloc_mark();
    hist.LBins = fb_alloc(hist.LBinCount * sizeof(float));
    hist.ABins = fb_alloc(hist.ABinCount * sizeof(float));
    hist.BBins = fb_alloc(hist.BBinCount * sizeof(float));

    for (int i = 0; i < hist.LBinCount; i++) {
        hist.LBins[i] = mp_obj_get_float(((mp_obj_list_t *) ((py_histogram_obj_t *) self_in)->LBins)->items[i]);
    }

    for (int i = 0; i < hist.ABinCount; i++) {
        hist.ABins[i] = mp_obj_get_float(((mp_obj_list_t *) ((py_histogram_obj_t *) self_in)->ABins)->items[i]);
    }

    for (int i = 0; i < hist.BBinCount; i++) {
        hist.BBins[i] = mp_obj_get_float(((mp_obj_list_t *) ((py_histogram_obj_t *) self_in)->BBins)->items[i]);
    }

    statistics_t stats;
    imlib_get_statistics(&stats, ((py_histogram_obj_t *) self_in)->bpp, &hist);
    if (hist.BBinCount) fb_free();
    if (hist.ABinCount) fb_free();
    if (hist.LBinCount) fb_free();
    fb_alloc_free_till_mark();

    py_statistics_obj_t *o = m_new_obj(py_statistics_obj_t);
    o->base.type = &py_statistics_type;
    o->bpp = ((py_histogram_obj_t *) self_in)->bpp;

    o->LMean = mp_obj_new_int(stats.LMean);
    o->LMedian = mp_obj_new_int(stats.LMedian);
    o->LMode= mp_obj_new_int(stats.LMode);
    o->LSTDev = mp_obj_new_int(stats.LSTDev);
    o->LMin = mp_obj_new_int(stats.LMin);
    o->LMax = mp_obj_new_int(stats.LMax);
    o->LLQ = mp_obj_new_int(stats.LLQ);
    o->LUQ = mp_obj_new_int(stats.LUQ);
    o->AMean = mp_obj_new_int(stats.AMean);
    o->AMedian = mp_obj_new_int(stats.AMedian);
    o->AMode= mp_obj_new_int(stats.AMode);
    o->ASTDev = mp_obj_new_int(stats.ASTDev);
    o->AMin = mp_obj_new_int(stats.AMin);
    o->AMax = mp_obj_new_int(stats.AMax);
    o->ALQ = mp_obj_new_int(stats.ALQ);
    o->AUQ = mp_obj_new_int(stats.AUQ);
    o->BMean = mp_obj_new_int(stats.BMean);
    o->BMedian = mp_obj_new_int(stats.BMedian);
    o->BMode= mp_obj_new_int(stats.BMode);
    o->BSTDev = mp_obj_new_int(stats.BSTDev);
    o->BMin = mp_obj_new_int(stats.BMin);
    o->BMax = mp_obj_new_int(stats.BMax);
    o->BLQ = mp_obj_new_int(stats.BLQ);
    o->BUQ = mp_obj_new_int(stats.BUQ);

    return o;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_histogram_bins_obj, py_histogram_bins);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_histogram_l_bins_obj, py_histogram_l_bins);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_histogram_a_bins_obj, py_histogram_a_bins);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_histogram_b_bins_obj, py_histogram_b_bins);
STATIC MP_DEFINE_CONST_FUN_OBJ_2(py_histogram_get_percentile_obj, py_histogram_get_percentile);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_histogram_get_threshold_obj, py_histogram_get_threshold);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_histogram_get_statistics_obj, py_histogram_get_statistics);

STATIC const mp_rom_map_elem_t py_histogram_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_bins), MP_ROM_PTR(&py_histogram_bins_obj) },
    { MP_ROM_QSTR(MP_QSTR_l_bins), MP_ROM_PTR(&py_histogram_l_bins_obj) },
    { MP_ROM_QSTR(MP_QSTR_a_bins), MP_ROM_PTR(&py_histogram_a_bins_obj) },
    { MP_ROM_QSTR(MP_QSTR_b_bins), MP_ROM_PTR(&py_histogram_b_bins_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_percentile), MP_ROM_PTR(&py_histogram_get_percentile_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_threshold), MP_ROM_PTR(&py_histogram_get_threshold_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_stats), MP_ROM_PTR(&py_histogram_get_statistics_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_statistics), MP_ROM_PTR(&py_histogram_get_statistics_obj) },
    { MP_ROM_QSTR(MP_QSTR_statistics), MP_ROM_PTR(&py_histogram_get_statistics_obj) }
};

STATIC MP_DEFINE_CONST_DICT(py_histogram_locals_dict, py_histogram_locals_dict_table);

static const mp_obj_type_t py_histogram_type = {
    { &mp_type_type },
    .name  = MP_QSTR_histogram,
    .print = py_histogram_print,
    .subscr = py_histogram_subscr,
    .locals_dict = (mp_obj_t) &py_histogram_locals_dict
};

static mp_obj_t py_image_get_histogram(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    list_t thresholds;
    list_init(&thresholds, sizeof(color_thresholds_list_lnk_data_t));
    py_helper_keyword_thresholds(n_args, args, 1, kw_args, &thresholds);
    bool invert = py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_invert), false);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 3, kw_args, &roi);

    histogram_t hist;
    switch(arg_img->bpp) {
        case IMAGE_BPP_BINARY: {
            int bins = py_helper_keyword_int(n_args, args, n_args, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_bins),
                                             (COLOR_BINARY_MAX-COLOR_BINARY_MIN+1));
            PY_ASSERT_TRUE_MSG(bins >= 2, "bins must be >= 2");
            hist.LBinCount = py_helper_keyword_int(n_args, args, n_args, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_l_bins), bins);
            PY_ASSERT_TRUE_MSG(hist.LBinCount >= 2, "l_bins must be >= 2");
            hist.ABinCount = 0;
            hist.BBinCount = 0;
            fb_alloc_mark();
            hist.LBins = fb_alloc(hist.LBinCount * sizeof(float));
            hist.ABins = NULL;
            hist.BBins = NULL;
            imlib_get_histogram(&hist, arg_img, &roi, &thresholds, invert);
            list_free(&thresholds);
            break;
        }
        case IMAGE_BPP_GRAYSCALE: {
            int bins = py_helper_keyword_int(n_args, args, n_args, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_bins),
                                             (COLOR_GRAYSCALE_MAX-COLOR_GRAYSCALE_MIN+1));
            PY_ASSERT_TRUE_MSG(bins >= 2, "bins must be >= 2");
            hist.LBinCount = py_helper_keyword_int(n_args, args, n_args, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_l_bins), bins);
            PY_ASSERT_TRUE_MSG(hist.LBinCount >= 2, "l_bins must be >= 2");
            hist.ABinCount = 0;
            hist.BBinCount = 0;
            fb_alloc_mark();
            hist.LBins = fb_alloc(hist.LBinCount * sizeof(float));
            hist.ABins = NULL;
            hist.BBins = NULL;
            imlib_get_histogram(&hist, arg_img, &roi, &thresholds, invert);
            list_free(&thresholds);
            break;
        }
        case IMAGE_BPP_RGB565: {
            int l_bins = py_helper_keyword_int(n_args, args, n_args, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_bins),
                                               (COLOR_L_MAX-COLOR_L_MIN+1));
            PY_ASSERT_TRUE_MSG(l_bins >= 2, "bins must be >= 2");
            hist.LBinCount = py_helper_keyword_int(n_args, args, n_args, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_l_bins), l_bins);
            PY_ASSERT_TRUE_MSG(hist.LBinCount >= 2, "l_bins must be >= 2");
            int a_bins = py_helper_keyword_int(n_args, args, n_args, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_bins),
                                               (COLOR_A_MAX-COLOR_A_MIN+1));
            PY_ASSERT_TRUE_MSG(a_bins >= 2, "bins must be >= 2");
            hist.ABinCount = py_helper_keyword_int(n_args, args, n_args, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_a_bins), a_bins);
            PY_ASSERT_TRUE_MSG(hist.ABinCount >= 2, "a_bins must be >= 2");
            int b_bins = py_helper_keyword_int(n_args, args, n_args, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_bins),
                                               (COLOR_B_MAX-COLOR_B_MIN+1));
            PY_ASSERT_TRUE_MSG(b_bins >= 2, "bins must be >= 2");
            hist.BBinCount = py_helper_keyword_int(n_args, args, n_args, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_b_bins), b_bins);
            PY_ASSERT_TRUE_MSG(hist.BBinCount >= 2, "b_bins must be >= 2");
            fb_alloc_mark();
            hist.LBins = fb_alloc(hist.LBinCount * sizeof(float));
            hist.ABins = fb_alloc(hist.ABinCount * sizeof(float));
            hist.BBins = fb_alloc(hist.BBinCount * sizeof(float));
            imlib_get_histogram(&hist, arg_img, &roi, &thresholds, invert);
            list_free(&thresholds);
            break;
        }
        default: {
            return MP_OBJ_NULL;
        }
    }

    py_histogram_obj_t *o = m_new_obj(py_histogram_obj_t);
    o->base.type = &py_histogram_type;
    o->bpp = arg_img->bpp;

    o->LBins = mp_obj_new_list(hist.LBinCount, NULL);
    o->ABins = mp_obj_new_list(hist.ABinCount, NULL);
    o->BBins = mp_obj_new_list(hist.BBinCount, NULL);

    for (int i = 0; i < hist.LBinCount; i++) {
        ((mp_obj_list_t *) o->LBins)->items[i] = mp_obj_new_float(hist.LBins[i]);
    }

    for (int i = 0; i < hist.ABinCount; i++) {
        ((mp_obj_list_t *) o->ABins)->items[i] = mp_obj_new_float(hist.ABins[i]);
    }

    for (int i = 0; i < hist.BBinCount; i++) {
        ((mp_obj_list_t *) o->BBins)->items[i] = mp_obj_new_float(hist.BBins[i]);
    }

    if (hist.BBinCount) fb_free();
    if (hist.ABinCount) fb_free();
    if (hist.LBinCount) fb_free();
    fb_alloc_free_till_mark();

    return o;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_get_histogram_obj, 1, py_image_get_histogram);

static mp_obj_t py_image_get_statistics(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    list_t thresholds;
    list_init(&thresholds, sizeof(color_thresholds_list_lnk_data_t));
    py_helper_keyword_thresholds(n_args, args, 1, kw_args, &thresholds);
    bool invert = py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_invert), false);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 3, kw_args, &roi);

    histogram_t hist;
    switch(arg_img->bpp) {
        case IMAGE_BPP_BINARY: {
            int bins = py_helper_keyword_int(n_args, args, n_args, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_bins),
                                             (COLOR_BINARY_MAX-COLOR_BINARY_MIN+1));
            PY_ASSERT_TRUE_MSG(bins >= 2, "bins must be >= 2");
            hist.LBinCount = py_helper_keyword_int(n_args, args, n_args, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_l_bins), bins);
            PY_ASSERT_TRUE_MSG(hist.LBinCount >= 2, "l_bins must be >= 2");
            hist.ABinCount = 0;
            hist.BBinCount = 0;
            fb_alloc_mark();
            hist.LBins = fb_alloc(hist.LBinCount * sizeof(float));
            hist.ABins = NULL;
            hist.BBins = NULL;
            imlib_get_histogram(&hist, arg_img, &roi, &thresholds, invert);
            list_free(&thresholds);
            break;
        }
        case IMAGE_BPP_GRAYSCALE: {
            int bins = py_helper_keyword_int(n_args, args, n_args, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_bins),
                                             (COLOR_GRAYSCALE_MAX-COLOR_GRAYSCALE_MIN+1));
            PY_ASSERT_TRUE_MSG(bins >= 2, "bins must be >= 2");
            hist.LBinCount = py_helper_keyword_int(n_args, args, n_args, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_l_bins), bins);
            PY_ASSERT_TRUE_MSG(hist.LBinCount >= 2, "l_bins must be >= 2");
            hist.ABinCount = 0;
            hist.BBinCount = 0;
            fb_alloc_mark();
            hist.LBins = fb_alloc(hist.LBinCount * sizeof(float));
            hist.ABins = NULL;
            hist.BBins = NULL;
            imlib_get_histogram(&hist, arg_img, &roi, &thresholds, invert);
            list_free(&thresholds);
            break;
        }
        case IMAGE_BPP_RGB565: {
            int l_bins = py_helper_keyword_int(n_args, args, n_args, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_bins),
                                               (COLOR_L_MAX-COLOR_L_MIN+1));
            PY_ASSERT_TRUE_MSG(l_bins >= 2, "bins must be >= 2");
            hist.LBinCount = py_helper_keyword_int(n_args, args, n_args, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_l_bins), l_bins);
            PY_ASSERT_TRUE_MSG(hist.LBinCount >= 2, "l_bins must be >= 2");
            int a_bins = py_helper_keyword_int(n_args, args, n_args, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_bins),
                                               (COLOR_A_MAX-COLOR_A_MIN+1));
            PY_ASSERT_TRUE_MSG(a_bins >= 2, "bins must be >= 2");
            hist.ABinCount = py_helper_keyword_int(n_args, args, n_args, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_a_bins), a_bins);
            PY_ASSERT_TRUE_MSG(hist.ABinCount >= 2, "a_bins must be >= 2");
            int b_bins = py_helper_keyword_int(n_args, args, n_args, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_bins),
                                               (COLOR_B_MAX-COLOR_B_MIN+1));
            PY_ASSERT_TRUE_MSG(b_bins >= 2, "bins must be >= 2");
            hist.BBinCount = py_helper_keyword_int(n_args, args, n_args, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_b_bins), b_bins);
            PY_ASSERT_TRUE_MSG(hist.BBinCount >= 2, "b_bins must be >= 2");
            fb_alloc_mark();
            hist.LBins = fb_alloc(hist.LBinCount * sizeof(float));
            hist.ABins = fb_alloc(hist.ABinCount * sizeof(float));
            hist.BBins = fb_alloc(hist.BBinCount * sizeof(float));
            imlib_get_histogram(&hist, arg_img, &roi, &thresholds, invert);
            list_free(&thresholds);
            break;
        }
        default: {
            return MP_OBJ_NULL;
        }
    }

    statistics_t stats;
    imlib_get_statistics(&stats, arg_img->bpp, &hist);
    if (hist.BBinCount) fb_free();
    if (hist.ABinCount) fb_free();
    if (hist.LBinCount) fb_free();
    fb_alloc_free_till_mark();

    py_statistics_obj_t *o = m_new_obj(py_statistics_obj_t);
    o->base.type = &py_statistics_type;
    o->bpp = arg_img->bpp;

    o->LMean = mp_obj_new_int(stats.LMean);
    o->LMedian = mp_obj_new_int(stats.LMedian);
    o->LMode= mp_obj_new_int(stats.LMode);
    o->LSTDev = mp_obj_new_int(stats.LSTDev);
    o->LMin = mp_obj_new_int(stats.LMin);
    o->LMax = mp_obj_new_int(stats.LMax);
    o->LLQ = mp_obj_new_int(stats.LLQ);
    o->LUQ = mp_obj_new_int(stats.LUQ);
    o->AMean = mp_obj_new_int(stats.AMean);
    o->AMedian = mp_obj_new_int(stats.AMedian);
    o->AMode= mp_obj_new_int(stats.AMode);
    o->ASTDev = mp_obj_new_int(stats.ASTDev);
    o->AMin = mp_obj_new_int(stats.AMin);
    o->AMax = mp_obj_new_int(stats.AMax);
    o->ALQ = mp_obj_new_int(stats.ALQ);
    o->AUQ = mp_obj_new_int(stats.AUQ);
    o->BMean = mp_obj_new_int(stats.BMean);
    o->BMedian = mp_obj_new_int(stats.BMedian);
    o->BMode= mp_obj_new_int(stats.BMode);
    o->BSTDev = mp_obj_new_int(stats.BSTDev);
    o->BMin = mp_obj_new_int(stats.BMin);
    o->BMax = mp_obj_new_int(stats.BMax);
    o->BLQ = mp_obj_new_int(stats.BLQ);
    o->BUQ = mp_obj_new_int(stats.BUQ);

    return o;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_get_statistics_obj, 1, py_image_get_statistics);

// Line Object //
#define py_line_obj_size 8
typedef struct py_line_obj {
    mp_obj_base_t base;
    mp_obj_t x1, y1, x2, y2, length, magnitude, theta, rho;
} py_line_obj_t;

static void py_line_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_line_obj_t *self = self_in;
    mp_printf(print,
              "{\"x1\":%d, \"y1\":%d, \"x2\":%d, \"y2\":%d, \"length\":%d, \"magnitude\":%d, \"theta\":%d, \"rho\":%d}",
              mp_obj_get_int(self->x1),
              mp_obj_get_int(self->y1),
              mp_obj_get_int(self->x2),
              mp_obj_get_int(self->y2),
              mp_obj_get_int(self->length),
              mp_obj_get_int(self->magnitude),
              mp_obj_get_int(self->theta),
              mp_obj_get_int(self->rho));
}

static mp_obj_t py_line_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    if (value == MP_OBJ_SENTINEL) { // load
        py_line_obj_t *self = self_in;
        if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
            mp_bound_slice_t slice;
            if (!mp_seq_get_fast_slice_indexes(py_line_obj_size, index, &slice)) {
                nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
            }
            mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
            mp_seq_copy(result->items, &(self->x1) + slice.start, result->len, mp_obj_t);
            return result;
        }
        switch (mp_get_index(self->base.type, py_line_obj_size, index, false)) {
            case 0: return self->x1;
            case 1: return self->y1;
            case 2: return self->x2;
            case 3: return self->y2;
            case 4: return self->length;
            case 5: return self->magnitude;
            case 6: return self->theta;
            case 7: return self->rho;
        }
    }
    return MP_OBJ_NULL; // op not supported
}

mp_obj_t py_line_line(mp_obj_t self_in)
{
    return mp_obj_new_tuple(4, (mp_obj_t []) {((py_line_obj_t *) self_in)->x1,
                                              ((py_line_obj_t *) self_in)->y1,
                                              ((py_line_obj_t *) self_in)->x2,
                                              ((py_line_obj_t *) self_in)->y2});
}

mp_obj_t py_line_x1(mp_obj_t self_in) { return ((py_line_obj_t *) self_in)->x1; }
mp_obj_t py_line_y1(mp_obj_t self_in) { return ((py_line_obj_t *) self_in)->y1; }
mp_obj_t py_line_x2(mp_obj_t self_in) { return ((py_line_obj_t *) self_in)->x2; }
mp_obj_t py_line_y2(mp_obj_t self_in) { return ((py_line_obj_t *) self_in)->y2; }
mp_obj_t py_line_length(mp_obj_t self_in) { return ((py_line_obj_t *) self_in)->length; }
mp_obj_t py_line_magnitude(mp_obj_t self_in) { return ((py_line_obj_t *) self_in)->magnitude; }
mp_obj_t py_line_theta(mp_obj_t self_in) { return ((py_line_obj_t *) self_in)->theta; }
mp_obj_t py_line_rho(mp_obj_t self_in) { return ((py_line_obj_t *) self_in)->rho; }

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_line_line_obj, py_line_line);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_line_x1_obj, py_line_x1);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_line_y1_obj, py_line_y1);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_line_x2_obj, py_line_x2);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_line_y2_obj, py_line_y2);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_line_length_obj, py_line_length);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_line_magnitude_obj, py_line_magnitude);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_line_theta_obj, py_line_theta);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_line_rho_obj, py_line_rho);

STATIC const mp_rom_map_elem_t py_line_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_line), MP_ROM_PTR(&py_line_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_x1), MP_ROM_PTR(&py_line_x1_obj) },
    { MP_ROM_QSTR(MP_QSTR_y1), MP_ROM_PTR(&py_line_y1_obj) },
    { MP_ROM_QSTR(MP_QSTR_x2), MP_ROM_PTR(&py_line_x2_obj) },
    { MP_ROM_QSTR(MP_QSTR_y2), MP_ROM_PTR(&py_line_y2_obj) },
    { MP_ROM_QSTR(MP_QSTR_length), MP_ROM_PTR(&py_line_length_obj) },
    { MP_ROM_QSTR(MP_QSTR_magnitude), MP_ROM_PTR(&py_line_magnitude_obj) },
    { MP_ROM_QSTR(MP_QSTR_theta), MP_ROM_PTR(&py_line_theta_obj) },
    { MP_ROM_QSTR(MP_QSTR_rho), MP_ROM_PTR(&py_line_rho_obj) }
};

STATIC MP_DEFINE_CONST_DICT(py_line_locals_dict, py_line_locals_dict_table);

static const mp_obj_type_t py_line_type = {
    { &mp_type_type },
    .name  = MP_QSTR_line,
    .print = py_line_print,
    .subscr = py_line_subscr,
    .locals_dict = (mp_obj_t) &py_line_locals_dict
};

static mp_obj_t py_image_get_regression(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    list_t thresholds;
    list_init(&thresholds, sizeof(color_thresholds_list_lnk_data_t));
    py_helper_arg_to_thresholds(args[1], &thresholds);
    if (!list_size(&thresholds)) return mp_const_none;
    bool invert = py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_invert), false);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 3, kw_args, &roi);

    unsigned int x_stride = py_helper_keyword_int(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_x_stride), 2);
    PY_ASSERT_TRUE_MSG(x_stride > 0, "x_stride must not be zero.");
    unsigned int y_stride = py_helper_keyword_int(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_y_stride), 1);
    PY_ASSERT_TRUE_MSG(y_stride > 0, "y_stride must not be zero.");
    unsigned int area_threshold = py_helper_keyword_int(n_args, args, 6, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_area_threshold), 10);
    unsigned int pixels_threshold = py_helper_keyword_int(n_args, args, 7, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_pixels_threshold), 10);
    bool robust = py_helper_keyword_int(n_args, args, 8, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_robust), false);

    find_lines_list_lnk_data_t out;
    fb_alloc_mark();
    bool result = imlib_get_regression(&out, arg_img, &roi, x_stride, y_stride, &thresholds, invert, area_threshold, pixels_threshold, robust);
    fb_alloc_free_till_mark();
    list_free(&thresholds);
    if (!result) {
        return mp_const_none;
    }

    py_line_obj_t *o = m_new_obj(py_line_obj_t);
    o->base.type = &py_line_type;
    o->x1 = mp_obj_new_int(out.line.x1);
    o->y1 = mp_obj_new_int(out.line.y1);
    o->x2 = mp_obj_new_int(out.line.x2);
    o->y2 = mp_obj_new_int(out.line.y2);
    int x_diff = out.line.x2 - out.line.x1;
    int y_diff = out.line.y2 - out.line.y1;
    o->length = mp_obj_new_int(fast_roundf(fast_sqrtf((x_diff * x_diff) + (y_diff * y_diff))));
    o->magnitude = mp_obj_new_int(out.magnitude);
    o->theta = mp_obj_new_int(out.theta);
    o->rho = mp_obj_new_int(out.rho);

    return o;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_get_regression_obj, 2, py_image_get_regression);

///////////////
// Find Methods
///////////////

// Blob Object //
#define py_blob_obj_size 10
typedef struct py_blob_obj {
    mp_obj_base_t base;
    mp_obj_t x, y, w, h, pixels, cx, cy, rotation, code, count;
} py_blob_obj_t;

static void py_blob_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_blob_obj_t *self = self_in;
    mp_printf(print,
              "{\"x\":%d, \"y\":%d, \"w\":%d, \"h\":%d, \"pixels\":%d, \"cx\":%d, \"cy\":%d, \"rotation\":%f, \"code\":%d, \"count\":%d}",
              mp_obj_get_int(self->x),
              mp_obj_get_int(self->y),
              mp_obj_get_int(self->w),
              mp_obj_get_int(self->h),
              mp_obj_get_int(self->pixels),
              mp_obj_get_int(self->cx),
              mp_obj_get_int(self->cy),
              (double) mp_obj_get_float(self->rotation),
              mp_obj_get_int(self->code),
              mp_obj_get_int(self->count));
}

static mp_obj_t py_blob_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    if (value == MP_OBJ_SENTINEL) { // load
        py_blob_obj_t *self = self_in;
        if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
            mp_bound_slice_t slice;
            if (!mp_seq_get_fast_slice_indexes(py_blob_obj_size, index, &slice)) {
                nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
            }
            mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
            mp_seq_copy(result->items, &(self->x) + slice.start, result->len, mp_obj_t);
            return result;
        }
        switch (mp_get_index(self->base.type, py_blob_obj_size, index, false)) {
            case 0: return self->x;
            case 1: return self->y;
            case 2: return self->w;
            case 3: return self->h;
            case 4: return self->pixels;
            case 5: return self->cx;
            case 6: return self->cy;
            case 7: return self->rotation;
            case 8: return self->code;
            case 9: return self->count;
        }
    }
    return MP_OBJ_NULL; // op not supported
}

mp_obj_t py_blob_rect(mp_obj_t self_in)
{
    return mp_obj_new_tuple(4, (mp_obj_t []) {((py_blob_obj_t *) self_in)->x,
                                              ((py_blob_obj_t *) self_in)->y,
                                              ((py_blob_obj_t *) self_in)->w,
                                              ((py_blob_obj_t *) self_in)->h});
}

mp_obj_t py_blob_x(mp_obj_t self_in) { return ((py_blob_obj_t *) self_in)->x; }
mp_obj_t py_blob_y(mp_obj_t self_in) { return ((py_blob_obj_t *) self_in)->y; }
mp_obj_t py_blob_w(mp_obj_t self_in) { return ((py_blob_obj_t *) self_in)->w; }
mp_obj_t py_blob_h(mp_obj_t self_in) { return ((py_blob_obj_t *) self_in)->h; }
mp_obj_t py_blob_pixels(mp_obj_t self_in) { return ((py_blob_obj_t *) self_in)->pixels; }
mp_obj_t py_blob_cx(mp_obj_t self_in) { return ((py_blob_obj_t *) self_in)->cx; }
mp_obj_t py_blob_cy(mp_obj_t self_in) { return ((py_blob_obj_t *) self_in)->cy; }
mp_obj_t py_blob_rotation(mp_obj_t self_in) { return ((py_blob_obj_t *) self_in)->rotation; }
mp_obj_t py_blob_code(mp_obj_t self_in) { return ((py_blob_obj_t *) self_in)->code; }
mp_obj_t py_blob_count(mp_obj_t self_in) { return ((py_blob_obj_t *) self_in)->count; }
mp_obj_t py_blob_area(mp_obj_t self_in) {
    return mp_obj_new_int(mp_obj_get_int(((py_blob_obj_t *) self_in)->w) * mp_obj_get_int(((py_blob_obj_t *) self_in)->h));
}
mp_obj_t py_blob_density(mp_obj_t self_in) {
    int area = mp_obj_get_int(((py_blob_obj_t *) self_in)->w) * mp_obj_get_int(((py_blob_obj_t *) self_in)->h);
    if (area) return mp_obj_new_float(mp_obj_get_int(((py_blob_obj_t *) self_in)->pixels) / ((float) area));
    return mp_obj_new_float(0.0f);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_blob_rect_obj, py_blob_rect);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_blob_x_obj, py_blob_x);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_blob_y_obj, py_blob_y);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_blob_w_obj, py_blob_w);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_blob_h_obj, py_blob_h);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_blob_pixels_obj, py_blob_pixels);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_blob_cx_obj, py_blob_cx);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_blob_cy_obj, py_blob_cy);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_blob_rotation_obj, py_blob_rotation);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_blob_code_obj, py_blob_code);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_blob_count_obj, py_blob_count);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_blob_area_obj, py_blob_area);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_blob_density_obj, py_blob_density);

STATIC const mp_rom_map_elem_t py_blob_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&py_blob_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_x), MP_ROM_PTR(&py_blob_x_obj) },
    { MP_ROM_QSTR(MP_QSTR_y), MP_ROM_PTR(&py_blob_y_obj) },
    { MP_ROM_QSTR(MP_QSTR_w), MP_ROM_PTR(&py_blob_w_obj) },
    { MP_ROM_QSTR(MP_QSTR_h), MP_ROM_PTR(&py_blob_h_obj) },
    { MP_ROM_QSTR(MP_QSTR_pixels), MP_ROM_PTR(&py_blob_pixels_obj) },
    { MP_ROM_QSTR(MP_QSTR_cx), MP_ROM_PTR(&py_blob_cx_obj) },
    { MP_ROM_QSTR(MP_QSTR_cy), MP_ROM_PTR(&py_blob_cy_obj) },
    { MP_ROM_QSTR(MP_QSTR_rotation), MP_ROM_PTR(&py_blob_rotation_obj) },
    { MP_ROM_QSTR(MP_QSTR_code), MP_ROM_PTR(&py_blob_code_obj) },
    { MP_ROM_QSTR(MP_QSTR_count), MP_ROM_PTR(&py_blob_count_obj) },
    { MP_ROM_QSTR(MP_QSTR_area), MP_ROM_PTR(&py_blob_area_obj) } ,
    { MP_ROM_QSTR(MP_QSTR_density), MP_ROM_PTR(&py_blob_density_obj) }
};

STATIC MP_DEFINE_CONST_DICT(py_blob_locals_dict, py_blob_locals_dict_table);

static const mp_obj_type_t py_blob_type = {
    { &mp_type_type },
    .name  = MP_QSTR_blob,
    .print = py_blob_print,
    .subscr = py_blob_subscr,
    .locals_dict = (mp_obj_t) &py_blob_locals_dict
};

static bool py_image_find_blobs_threshold_cb(void *fun_obj, find_blobs_list_lnk_data_t *blob)
{
    py_blob_obj_t *o = m_new_obj(py_blob_obj_t);
    o->base.type = &py_blob_type;
    o->x = mp_obj_new_int(blob->rect.x);
    o->y = mp_obj_new_int(blob->rect.y);
    o->w = mp_obj_new_int(blob->rect.w);
    o->h = mp_obj_new_int(blob->rect.h);
    o->pixels = mp_obj_new_int(blob->pixels);
    o->cx = mp_obj_new_int(blob->centroid.x);
    o->cy = mp_obj_new_int(blob->centroid.y);
    o->rotation = mp_obj_new_float(blob->rotation);
    o->code = mp_obj_new_int(blob->code);
    o->count = mp_obj_new_int(blob->count);

    return mp_obj_is_true(mp_call_function_1(fun_obj, o));
}

static bool py_image_find_blobs_merge_cb(void *fun_obj, find_blobs_list_lnk_data_t *blob0, find_blobs_list_lnk_data_t *blob1)
{
    py_blob_obj_t *o0 = m_new_obj(py_blob_obj_t);
    o0->base.type = &py_blob_type;
    o0->x = mp_obj_new_int(blob0->rect.x);
    o0->y = mp_obj_new_int(blob0->rect.y);
    o0->w = mp_obj_new_int(blob0->rect.w);
    o0->h = mp_obj_new_int(blob0->rect.h);
    o0->pixels = mp_obj_new_int(blob0->pixels);
    o0->cx = mp_obj_new_int(blob0->centroid.x);
    o0->cy = mp_obj_new_int(blob0->centroid.y);
    o0->rotation = mp_obj_new_float(blob0->rotation);
    o0->code = mp_obj_new_int(blob0->code);
    o0->count = mp_obj_new_int(blob0->count);

    py_blob_obj_t *o1 = m_new_obj(py_blob_obj_t);
    o1->base.type = &py_blob_type;
    o1->x = mp_obj_new_int(blob1->rect.x);
    o1->y = mp_obj_new_int(blob1->rect.y);
    o1->w = mp_obj_new_int(blob1->rect.w);
    o1->h = mp_obj_new_int(blob1->rect.h);
    o1->pixels = mp_obj_new_int(blob1->pixels);
    o1->cx = mp_obj_new_int(blob1->centroid.x);
    o1->cy = mp_obj_new_int(blob1->centroid.y);
    o1->rotation = mp_obj_new_float(blob1->rotation);
    o1->code = mp_obj_new_int(blob1->code);
    o1->count = mp_obj_new_int(blob1->count);

    return mp_obj_is_true(mp_call_function_2(fun_obj, o0, o1));
}

static mp_obj_t py_image_find_blobs(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    list_t thresholds;
    list_init(&thresholds, sizeof(color_thresholds_list_lnk_data_t));
    py_helper_arg_to_thresholds(args[1], &thresholds);
    if (!list_size(&thresholds)) return mp_obj_new_list(0, NULL);
    bool invert = py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_invert), false);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 3, kw_args, &roi);

    unsigned int x_stride = py_helper_keyword_int(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_x_stride), 2);
    PY_ASSERT_TRUE_MSG(x_stride > 0, "x_stride must not be zero.");
    unsigned int y_stride = py_helper_keyword_int(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_y_stride), 1);
    PY_ASSERT_TRUE_MSG(y_stride > 0, "y_stride must not be zero.");
    unsigned int area_threshold = py_helper_keyword_int(n_args, args, 6, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_area_threshold), 10);
    unsigned int pixels_threshold = py_helper_keyword_int(n_args, args, 7, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_pixels_threshold), 10);
    bool merge = py_helper_keyword_int(n_args, args, 8, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_merge), false);
    int margin = py_helper_keyword_int(n_args, args, 9, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_margin), 0);
    mp_obj_t threshold_cb = py_helper_keyword_object(n_args, args, 10, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold_cb));
    mp_obj_t merge_cb = py_helper_keyword_object(n_args, args, 11, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_merge_cb));

    list_t out;
    fb_alloc_mark();
    imlib_find_blobs(&out, arg_img, &roi, x_stride, y_stride, &thresholds, invert,
            area_threshold, pixels_threshold, merge, margin,
            py_image_find_blobs_threshold_cb, threshold_cb, py_image_find_blobs_merge_cb, merge_cb);
    fb_alloc_free_till_mark();
    list_free(&thresholds);

    mp_obj_list_t *objects_list = mp_obj_new_list(list_size(&out), NULL);
    for (size_t i = 0; list_size(&out); i++) {
        find_blobs_list_lnk_data_t lnk_data;
        list_pop_front(&out, &lnk_data);

        py_blob_obj_t *o = m_new_obj(py_blob_obj_t);
        o->base.type = &py_blob_type;
        o->x = mp_obj_new_int(lnk_data.rect.x);
        o->y = mp_obj_new_int(lnk_data.rect.y);
        o->w = mp_obj_new_int(lnk_data.rect.w);
        o->h = mp_obj_new_int(lnk_data.rect.h);
        o->pixels = mp_obj_new_int(lnk_data.pixels);
        o->cx = mp_obj_new_int(lnk_data.centroid.x);
        o->cy = mp_obj_new_int(lnk_data.centroid.y);
        o->rotation = mp_obj_new_float(lnk_data.rotation);
        o->code = mp_obj_new_int(lnk_data.code);
        o->count = mp_obj_new_int(lnk_data.count);

        objects_list->items[i] = o;
    }

    return objects_list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_find_blobs_obj, 2, py_image_find_blobs);

#endif //OMV_MINIMUM

#ifdef IMLIB_ENABLE_FIND_LINES
static mp_obj_t py_image_find_lines(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 1, kw_args, &roi);

    unsigned int x_stride = py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_x_stride), 2);
    PY_ASSERT_TRUE_MSG(x_stride > 0, "x_stride must not be zero.");
    unsigned int y_stride = py_helper_keyword_int(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_y_stride), 1);
    PY_ASSERT_TRUE_MSG(y_stride > 0, "y_stride must not be zero.");
    uint32_t threshold = py_helper_keyword_int(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold), 1000);
    unsigned int theta_margin = py_helper_keyword_int(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_theta_margin), 25);
    unsigned int rho_margin = py_helper_keyword_int(n_args, args, 6, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_rho_margin), 25);

    list_t out;
    fb_alloc_mark();
    imlib_find_lines(&out, arg_img, &roi, x_stride, y_stride, threshold, theta_margin, rho_margin);
    fb_alloc_free_till_mark();

    mp_obj_list_t *objects_list = mp_obj_new_list(list_size(&out), NULL);
    for (size_t i = 0; list_size(&out); i++) {
        find_lines_list_lnk_data_t lnk_data;
        list_pop_front(&out, &lnk_data);

        py_line_obj_t *o = m_new_obj(py_line_obj_t);
        o->base.type = &py_line_type;
        o->x1 = mp_obj_new_int(lnk_data.line.x1);
        o->y1 = mp_obj_new_int(lnk_data.line.y1);
        o->x2 = mp_obj_new_int(lnk_data.line.x2);
        o->y2 = mp_obj_new_int(lnk_data.line.y2);
        int x_diff = lnk_data.line.x2 - lnk_data.line.x1;
        int y_diff = lnk_data.line.y2 - lnk_data.line.y1;
        o->length = mp_obj_new_int(fast_roundf(fast_sqrtf((x_diff * x_diff) + (y_diff * y_diff))));
        o->magnitude = mp_obj_new_int(lnk_data.magnitude);
        o->theta = mp_obj_new_int(lnk_data.theta);
        o->rho = mp_obj_new_int(lnk_data.rho);

        objects_list->items[i] = o;
    }

    return objects_list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_find_lines_obj, 1, py_image_find_lines);
#endif // IMLIB_ENABLE_FIND_LINES

#ifdef IMLIB_ENABLE_FIND_LINE_SEGMENTS
static mp_obj_t py_image_find_line_segments(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 1, kw_args, &roi);

    unsigned int merge_distance = py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_merge_distance), 0);
    unsigned int max_theta_diff = py_helper_keyword_int(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_max_theta_diff), 15);

    list_t out;
    fb_alloc_mark();
    imlib_lsd_find_line_segments(&out, arg_img, &roi, merge_distance, max_theta_diff);
    fb_alloc_free_till_mark();

    mp_obj_list_t *objects_list = mp_obj_new_list(list_size(&out), NULL);
    for (size_t i = 0; list_size(&out); i++) {
        find_lines_list_lnk_data_t lnk_data;
        list_pop_front(&out, &lnk_data);

        py_line_obj_t *o = m_new_obj(py_line_obj_t);
        o->base.type = &py_line_type;
        o->x1 = mp_obj_new_int(lnk_data.line.x1);
        o->y1 = mp_obj_new_int(lnk_data.line.y1);
        o->x2 = mp_obj_new_int(lnk_data.line.x2);
        o->y2 = mp_obj_new_int(lnk_data.line.y2);
        int x_diff = lnk_data.line.x2 - lnk_data.line.x1;
        int y_diff = lnk_data.line.y2 - lnk_data.line.y1;
        o->length = mp_obj_new_int(fast_roundf(fast_sqrtf((x_diff * x_diff) + (y_diff * y_diff))));
        o->magnitude = mp_obj_new_int(lnk_data.magnitude);
        o->theta = mp_obj_new_int(lnk_data.theta);
        o->rho = mp_obj_new_int(lnk_data.rho);

        objects_list->items[i] = o;
    }

    return objects_list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_find_line_segments_obj, 1, py_image_find_line_segments);
#endif // IMLIB_ENABLE_FIND_LINE_SEGMENTS

#ifdef IMLIB_ENABLE_FIND_CIRCLES
// Circle Object //
#define py_circle_obj_size 4
typedef struct py_circle_obj {
    mp_obj_base_t base;
    mp_obj_t x, y, r, magnitude;
} py_circle_obj_t;

static void py_circle_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_circle_obj_t *self = self_in;
    mp_printf(print,
              "{\"x\":%d, \"y\":%d, \"r\":%d, \"magnitude\":%d}",
              mp_obj_get_int(self->x),
              mp_obj_get_int(self->y),
              mp_obj_get_int(self->r),
              mp_obj_get_int(self->magnitude));
}

static mp_obj_t py_circle_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    if (value == MP_OBJ_SENTINEL) { // load
        py_circle_obj_t *self = self_in;
        if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
            mp_bound_slice_t slice;
            if (!mp_seq_get_fast_slice_indexes(py_circle_obj_size, index, &slice)) {
                nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
            }
            mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
            mp_seq_copy(result->items, &(self->x) + slice.start, result->len, mp_obj_t);
            return result;
        }
        switch (mp_get_index(self->base.type, py_circle_obj_size, index, false)) {
            case 0: return self->x;
            case 1: return self->y;
            case 2: return self->r;
            case 3: return self->magnitude;
        }
    }
    return MP_OBJ_NULL; // op not supported
}

mp_obj_t py_circle_circle(mp_obj_t self_in)
{
    return mp_obj_new_tuple(3, (mp_obj_t []) {((py_circle_obj_t *) self_in)->x,
                                              ((py_circle_obj_t *) self_in)->y,
                                              ((py_circle_obj_t *) self_in)->r});
}

mp_obj_t py_circle_x(mp_obj_t self_in) { return ((py_circle_obj_t *) self_in)->x; }
mp_obj_t py_circle_y(mp_obj_t self_in) { return ((py_circle_obj_t *) self_in)->y; }
mp_obj_t py_circle_r(mp_obj_t self_in) { return ((py_circle_obj_t *) self_in)->r; }
mp_obj_t py_circle_magnitude(mp_obj_t self_in) { return ((py_circle_obj_t *) self_in)->magnitude; }

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_circle_circle_obj, py_circle_circle);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_circle_x_obj, py_circle_x);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_circle_y_obj, py_circle_y);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_circle_r_obj, py_circle_r);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_circle_magnitude_obj, py_circle_magnitude);

STATIC const mp_rom_map_elem_t py_circle_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_circle), MP_ROM_PTR(&py_circle_circle_obj) },
    { MP_ROM_QSTR(MP_QSTR_x), MP_ROM_PTR(&py_circle_x_obj) },
    { MP_ROM_QSTR(MP_QSTR_y), MP_ROM_PTR(&py_circle_y_obj) },
    { MP_ROM_QSTR(MP_QSTR_r), MP_ROM_PTR(&py_circle_r_obj) },
    { MP_ROM_QSTR(MP_QSTR_magnitude), MP_ROM_PTR(&py_circle_magnitude_obj) }
};

STATIC MP_DEFINE_CONST_DICT(py_circle_locals_dict, py_circle_locals_dict_table);

static const mp_obj_type_t py_circle_type = {
    { &mp_type_type },
    .name  = MP_QSTR_circle,
    .print = py_circle_print,
    .subscr = py_circle_subscr,
    .locals_dict = (mp_obj_t) &py_circle_locals_dict
};

static mp_obj_t py_image_find_circles(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 1, kw_args, &roi);

    unsigned int x_stride = py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_x_stride), 2);
    PY_ASSERT_TRUE_MSG(x_stride > 0, "x_stride must not be zero.");
    unsigned int y_stride = py_helper_keyword_int(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_y_stride), 1);
    PY_ASSERT_TRUE_MSG(y_stride > 0, "y_stride must not be zero.");
    uint32_t threshold = py_helper_keyword_int(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold), 2000);
    unsigned int x_margin = py_helper_keyword_int(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_x_margin), 10);
    unsigned int y_margin = py_helper_keyword_int(n_args, args, 6, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_y_margin), 10);
    unsigned int r_margin = py_helper_keyword_int(n_args, args, 7, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_r_margin), 10);
    unsigned int r_min = IM_MAX(py_helper_keyword_int(n_args, args, 8, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_r_min),
            2), 2);
    unsigned int r_max = IM_MIN(py_helper_keyword_int(n_args, args, 9, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_r_max),
            IM_MIN((roi.w / 2), (roi.h / 2))), IM_MIN((roi.w / 2), (roi.h / 2)));
    unsigned int r_step = py_helper_keyword_int(n_args, args, 10, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_r_step), 2);

    list_t out;
    fb_alloc_mark();
    imlib_find_circles(&out, arg_img, &roi, x_stride, y_stride, threshold, x_margin, y_margin, r_margin,
                       r_min, r_max, r_step);
    fb_alloc_free_till_mark();

    mp_obj_list_t *objects_list = mp_obj_new_list(list_size(&out), NULL);
    for (size_t i = 0; list_size(&out); i++) {
        find_circles_list_lnk_data_t lnk_data;
        list_pop_front(&out, &lnk_data);

        py_circle_obj_t *o = m_new_obj(py_circle_obj_t);
        o->base.type = &py_circle_type;
        o->x = mp_obj_new_int(lnk_data.p.x);
        o->y = mp_obj_new_int(lnk_data.p.y);
        o->r = mp_obj_new_int(lnk_data.r);
        o->magnitude = mp_obj_new_int(lnk_data.magnitude);

        objects_list->items[i] = o;
    }

    return objects_list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_find_circles_obj, 1, py_image_find_circles);
#endif // IMLIB_ENABLE_FIND_CIRCLES

#ifdef IMLIB_ENABLE_FIND_RECTS
// Rect Object //
#define py_rect_obj_size 5
typedef struct py_rect_obj {
    mp_obj_base_t base;
    mp_obj_t corners;
    mp_obj_t x, y, w, h, magnitude;
} py_rect_obj_t;

static void py_rect_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_rect_obj_t *self = self_in;
    mp_printf(print,
              "{\"x\":%d, \"y\":%d, \"w\":%d, \"h\":%d, \"magnitude\":%d}",
              mp_obj_get_int(self->x),
              mp_obj_get_int(self->y),
              mp_obj_get_int(self->w),
              mp_obj_get_int(self->h),
              mp_obj_get_int(self->magnitude));
}

static mp_obj_t py_rect_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    if (value == MP_OBJ_SENTINEL) { // load
        py_rect_obj_t *self = self_in;
        if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
            mp_bound_slice_t slice;
            if (!mp_seq_get_fast_slice_indexes(py_rect_obj_size, index, &slice)) {
                nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
            }
            mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
            mp_seq_copy(result->items, &(self->x) + slice.start, result->len, mp_obj_t);
            return result;
        }
        switch (mp_get_index(self->base.type, py_rect_obj_size, index, false)) {
            case 0: return self->x;
            case 1: return self->y;
            case 2: return self->w;
            case 3: return self->h;
            case 4: return self->magnitude;
        }
    }
    return MP_OBJ_NULL; // op not supported
}

mp_obj_t py_rect_corners(mp_obj_t self_in) { return ((py_rect_obj_t *) self_in)->corners; }
mp_obj_t py_rect_rect(mp_obj_t self_in)
{
    return mp_obj_new_tuple(4, (mp_obj_t []) {((py_rect_obj_t *) self_in)->x,
                                              ((py_rect_obj_t *) self_in)->y,
                                              ((py_rect_obj_t *) self_in)->w,
                                              ((py_rect_obj_t *) self_in)->h});
}

mp_obj_t py_rect_x(mp_obj_t self_in) { return ((py_rect_obj_t *) self_in)->x; }
mp_obj_t py_rect_y(mp_obj_t self_in) { return ((py_rect_obj_t *) self_in)->y; }
mp_obj_t py_rect_w(mp_obj_t self_in) { return ((py_rect_obj_t *) self_in)->w; }
mp_obj_t py_rect_h(mp_obj_t self_in) { return ((py_rect_obj_t *) self_in)->h; }
mp_obj_t py_rect_magnitude(mp_obj_t self_in) { return ((py_rect_obj_t *) self_in)->magnitude; }

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_rect_corners_obj, py_rect_corners);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_rect_rect_obj, py_rect_rect);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_rect_x_obj, py_rect_x);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_rect_y_obj, py_rect_y);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_rect_w_obj, py_rect_w);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_rect_h_obj, py_rect_h);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_rect_magnitude_obj, py_rect_magnitude);

STATIC const mp_rom_map_elem_t py_rect_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_corners), MP_ROM_PTR(&py_rect_corners_obj) },
    { MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&py_rect_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_x), MP_ROM_PTR(&py_rect_x_obj) },
    { MP_ROM_QSTR(MP_QSTR_y), MP_ROM_PTR(&py_rect_y_obj) },
    { MP_ROM_QSTR(MP_QSTR_w), MP_ROM_PTR(&py_rect_w_obj) },
    { MP_ROM_QSTR(MP_QSTR_h), MP_ROM_PTR(&py_rect_h_obj) },
    { MP_ROM_QSTR(MP_QSTR_magnitude), MP_ROM_PTR(&py_rect_magnitude_obj) }
};

STATIC MP_DEFINE_CONST_DICT(py_rect_locals_dict, py_rect_locals_dict_table);

static const mp_obj_type_t py_rect_type = {
    { &mp_type_type },
    .name  = MP_QSTR_rect,
    .print = py_rect_print,
    .subscr = py_rect_subscr,
    .locals_dict = (mp_obj_t) &py_rect_locals_dict
};

static mp_obj_t py_image_find_rects(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 1, kw_args, &roi);

    uint32_t threshold = py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold), 1000);

    list_t out;
    fb_alloc_mark();
    imlib_find_rects(&out, arg_img, &roi, threshold);
    fb_alloc_free_till_mark();

    mp_obj_list_t *objects_list = mp_obj_new_list(list_size(&out), NULL);
    for (size_t i = 0; list_size(&out); i++) {
        find_rects_list_lnk_data_t lnk_data;
        list_pop_front(&out, &lnk_data);

        py_rect_obj_t *o = m_new_obj(py_rect_obj_t);
        o->base.type = &py_rect_type;
        o->corners = mp_obj_new_tuple(4, (mp_obj_t [])
            {mp_obj_new_tuple(2, (mp_obj_t []) {mp_obj_new_int(lnk_data.corners[0].x), mp_obj_new_int(lnk_data.corners[0].y)}),
             mp_obj_new_tuple(2, (mp_obj_t []) {mp_obj_new_int(lnk_data.corners[1].x), mp_obj_new_int(lnk_data.corners[1].y)}),
             mp_obj_new_tuple(2, (mp_obj_t []) {mp_obj_new_int(lnk_data.corners[2].x), mp_obj_new_int(lnk_data.corners[2].y)}),
             mp_obj_new_tuple(2, (mp_obj_t []) {mp_obj_new_int(lnk_data.corners[3].x), mp_obj_new_int(lnk_data.corners[3].y)})});
        o->x = mp_obj_new_int(lnk_data.rect.x);
        o->y = mp_obj_new_int(lnk_data.rect.y);
        o->w = mp_obj_new_int(lnk_data.rect.w);
        o->h = mp_obj_new_int(lnk_data.rect.h);
        o->magnitude = mp_obj_new_int(lnk_data.magnitude);

        objects_list->items[i] = o;
    }

    return objects_list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_find_rects_obj, 1, py_image_find_rects);
#endif // IMLIB_ENABLE_FIND_RECTS

#ifdef IMLIB_ENABLE_QRCODES
// QRCode Object //
#define py_qrcode_obj_size 10
typedef struct py_qrcode_obj {
    mp_obj_base_t base;
    mp_obj_t corners;
    mp_obj_t x, y, w, h, payload, version, ecc_level, mask, data_type, eci;
} py_qrcode_obj_t;

static void py_qrcode_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_qrcode_obj_t *self = self_in;
    mp_printf(print,
              "{\"x\":%d, \"y\":%d, \"w\":%d, \"h\":%d, \"payload\":\"%s\","
              " \"version\":%d, \"ecc_level\":%d, \"mask\":%d, \"data_type\":%d, \"eci\":%d}",
              mp_obj_get_int(self->x),
              mp_obj_get_int(self->y),
              mp_obj_get_int(self->w),
              mp_obj_get_int(self->h),
              mp_obj_str_get_str(self->payload),
              mp_obj_get_int(self->version),
              mp_obj_get_int(self->ecc_level),
              mp_obj_get_int(self->mask),
              mp_obj_get_int(self->data_type),
              mp_obj_get_int(self->eci));
}

static mp_obj_t py_qrcode_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    if (value == MP_OBJ_SENTINEL) { // load
        py_qrcode_obj_t *self = self_in;
        if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
            mp_bound_slice_t slice;
            if (!mp_seq_get_fast_slice_indexes(py_qrcode_obj_size, index, &slice)) {
                nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
            }
            mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
            mp_seq_copy(result->items, &(self->x) + slice.start, result->len, mp_obj_t);
            return result;
        }
        switch (mp_get_index(self->base.type, py_qrcode_obj_size, index, false)) {
            case 0: return self->x;
            case 1: return self->y;
            case 2: return self->w;
            case 3: return self->h;
            case 4: return self->payload;
            case 5: return self->version;
            case 6: return self->ecc_level;
            case 7: return self->mask;
            case 8: return self->data_type;
            case 9: return self->eci;
        }
    }
    return MP_OBJ_NULL; // op not supported
}

mp_obj_t py_qrcode_corners(mp_obj_t self_in) { return ((py_qrcode_obj_t *) self_in)->corners; }
mp_obj_t py_qrcode_rect(mp_obj_t self_in)
{
    return mp_obj_new_tuple(4, (mp_obj_t []) {((py_qrcode_obj_t *) self_in)->x,
                                              ((py_qrcode_obj_t *) self_in)->y,
                                              ((py_qrcode_obj_t *) self_in)->w,
                                              ((py_qrcode_obj_t *) self_in)->h});
}

mp_obj_t py_qrcode_x(mp_obj_t self_in) { return ((py_qrcode_obj_t *) self_in)->x; }
mp_obj_t py_qrcode_y(mp_obj_t self_in) { return ((py_qrcode_obj_t *) self_in)->y; }
mp_obj_t py_qrcode_w(mp_obj_t self_in) { return ((py_qrcode_obj_t *) self_in)->w; }
mp_obj_t py_qrcode_h(mp_obj_t self_in) { return ((py_qrcode_obj_t *) self_in)->h; }
mp_obj_t py_qrcode_payload(mp_obj_t self_in) { return ((py_qrcode_obj_t *) self_in)->payload; }
mp_obj_t py_qrcode_version(mp_obj_t self_in) { return ((py_qrcode_obj_t *) self_in)->version; }
mp_obj_t py_qrcode_ecc_level(mp_obj_t self_in) { return ((py_qrcode_obj_t *) self_in)->ecc_level; }
mp_obj_t py_qrcode_mask(mp_obj_t self_in) { return ((py_qrcode_obj_t *) self_in)->mask; }
mp_obj_t py_qrcode_data_type(mp_obj_t self_in) { return ((py_qrcode_obj_t *) self_in)->data_type; }
mp_obj_t py_qrcode_eci(mp_obj_t self_in) { return ((py_qrcode_obj_t *) self_in)->eci; }
mp_obj_t py_qrcode_is_numeric(mp_obj_t self_in) { return mp_obj_new_bool(mp_obj_get_int(((py_qrcode_obj_t *) self_in)->data_type) == 1); }
mp_obj_t py_qrcode_is_alphanumeric(mp_obj_t self_in) { return mp_obj_new_bool(mp_obj_get_int(((py_qrcode_obj_t *) self_in)->data_type) == 2); }
mp_obj_t py_qrcode_is_binary(mp_obj_t self_in) { return mp_obj_new_bool(mp_obj_get_int(((py_qrcode_obj_t *) self_in)->data_type) == 4); }
mp_obj_t py_qrcode_is_kanji(mp_obj_t self_in) { return mp_obj_new_bool(mp_obj_get_int(((py_qrcode_obj_t *) self_in)->data_type) == 8); }

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_qrcode_corners_obj, py_qrcode_corners);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_qrcode_rect_obj, py_qrcode_rect);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_qrcode_x_obj, py_qrcode_x);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_qrcode_y_obj, py_qrcode_y);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_qrcode_w_obj, py_qrcode_w);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_qrcode_h_obj, py_qrcode_h);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_qrcode_payload_obj, py_qrcode_payload);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_qrcode_version_obj, py_qrcode_version);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_qrcode_ecc_level_obj, py_qrcode_ecc_level);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_qrcode_mask_obj, py_qrcode_mask);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_qrcode_data_type_obj, py_qrcode_data_type);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_qrcode_eci_obj, py_qrcode_eci);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_qrcode_is_numeric_obj, py_qrcode_is_numeric);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_qrcode_is_alphanumeric_obj, py_qrcode_is_alphanumeric);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_qrcode_is_binary_obj, py_qrcode_is_binary);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_qrcode_is_kanji_obj, py_qrcode_is_kanji);

STATIC const mp_rom_map_elem_t py_qrcode_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_corners), MP_ROM_PTR(&py_qrcode_corners_obj) },
    { MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&py_qrcode_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_x), MP_ROM_PTR(&py_qrcode_x_obj) },
    { MP_ROM_QSTR(MP_QSTR_y), MP_ROM_PTR(&py_qrcode_y_obj) },
    { MP_ROM_QSTR(MP_QSTR_w), MP_ROM_PTR(&py_qrcode_w_obj) },
    { MP_ROM_QSTR(MP_QSTR_h), MP_ROM_PTR(&py_qrcode_h_obj) },
    { MP_ROM_QSTR(MP_QSTR_payload), MP_ROM_PTR(&py_qrcode_payload_obj) },
    { MP_ROM_QSTR(MP_QSTR_version), MP_ROM_PTR(&py_qrcode_version_obj) },
    { MP_ROM_QSTR(MP_QSTR_ecc_level), MP_ROM_PTR(&py_qrcode_ecc_level_obj) },
    { MP_ROM_QSTR(MP_QSTR_mask), MP_ROM_PTR(&py_qrcode_mask_obj) },
    { MP_ROM_QSTR(MP_QSTR_data_type), MP_ROM_PTR(&py_qrcode_data_type_obj) },
    { MP_ROM_QSTR(MP_QSTR_eci), MP_ROM_PTR(&py_qrcode_eci_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_numeric), MP_ROM_PTR(&py_qrcode_is_numeric_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_alphanumeric), MP_ROM_PTR(&py_qrcode_is_alphanumeric_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_binary), MP_ROM_PTR(&py_qrcode_is_binary_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_kanji), MP_ROM_PTR(&py_qrcode_is_kanji_obj) }
};

STATIC MP_DEFINE_CONST_DICT(py_qrcode_locals_dict, py_qrcode_locals_dict_table);

static const mp_obj_type_t py_qrcode_type = {
    { &mp_type_type },
    .name  = MP_QSTR_qrcode,
    .print = py_qrcode_print,
    .subscr = py_qrcode_subscr,
    .locals_dict = (mp_obj_t) &py_qrcode_locals_dict
};

static mp_obj_t py_image_find_qrcodes(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 1, kw_args, &roi);

    list_t out;
    fb_alloc_mark();
    imlib_find_qrcodes(&out, arg_img, &roi);
    fb_alloc_free_till_mark();

    mp_obj_list_t *objects_list = mp_obj_new_list(list_size(&out), NULL);
    for (size_t i = 0; list_size(&out); i++) {
        find_qrcodes_list_lnk_data_t lnk_data;
        list_pop_front(&out, &lnk_data);

        py_qrcode_obj_t *o = m_new_obj(py_qrcode_obj_t);
        o->base.type = &py_qrcode_type;
        o->corners = mp_obj_new_tuple(4, (mp_obj_t [])
            {mp_obj_new_tuple(2, (mp_obj_t []) {mp_obj_new_int(lnk_data.corners[0].x), mp_obj_new_int(lnk_data.corners[0].y)}),
             mp_obj_new_tuple(2, (mp_obj_t []) {mp_obj_new_int(lnk_data.corners[1].x), mp_obj_new_int(lnk_data.corners[1].y)}),
             mp_obj_new_tuple(2, (mp_obj_t []) {mp_obj_new_int(lnk_data.corners[2].x), mp_obj_new_int(lnk_data.corners[2].y)}),
             mp_obj_new_tuple(2, (mp_obj_t []) {mp_obj_new_int(lnk_data.corners[3].x), mp_obj_new_int(lnk_data.corners[3].y)})});
        o->x = mp_obj_new_int(lnk_data.rect.x);
        o->y = mp_obj_new_int(lnk_data.rect.y);
        o->w = mp_obj_new_int(lnk_data.rect.w);
        o->h = mp_obj_new_int(lnk_data.rect.h);
        o->payload = mp_obj_new_str(lnk_data.payload, lnk_data.payload_len);
        o->version = mp_obj_new_int(lnk_data.version);
        o->ecc_level = mp_obj_new_int(lnk_data.ecc_level);
        o->mask = mp_obj_new_int(lnk_data.mask);
        o->data_type = mp_obj_new_int(lnk_data.data_type);
        o->eci = mp_obj_new_int(lnk_data.eci);

        objects_list->items[i] = o;
        xfree(lnk_data.payload);
    }

    return objects_list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_find_qrcodes_obj, 1, py_image_find_qrcodes);
#endif // IMLIB_ENABLE_QRCODES

#ifdef IMLIB_ENABLE_APRILTAGS
// AprilTag Object //
#define py_apriltag_obj_size 18
typedef struct py_apriltag_obj {
    mp_obj_base_t base;
    mp_obj_t corners;
    mp_obj_t x, y, w, h, id, family, cx, cy, rotation, decision_margin, hamming, goodness;
    mp_obj_t x_translation, y_translation, z_translation;
    mp_obj_t x_rotation, y_rotation, z_rotation;
} py_apriltag_obj_t;

static void py_apriltag_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_apriltag_obj_t *self = self_in;
    mp_printf(print,
              "{\"x\":%d, \"y\":%d, \"w\":%d, \"h\":%d, \"id\":%d,"
              " \"family\":%d, \"cx\":%d, \"cy\":%d, \"rotation\":%f, \"decision_margin\":%f, \"hamming\":%d, \"goodness\":%f,"
              " \"x_translation\":%f, \"y_translation\":%f, \"z_translation\":%f,"
              " \"x_rotation\":%f, \"y_rotation\":%f, \"z_rotation\":%f}",
              mp_obj_get_int(self->x),
              mp_obj_get_int(self->y),
              mp_obj_get_int(self->w),
              mp_obj_get_int(self->h),
              mp_obj_get_int(self->id),
              mp_obj_get_int(self->family),
              mp_obj_get_int(self->cx),
              mp_obj_get_int(self->cy),
              (double) mp_obj_get_float(self->rotation),
              (double) mp_obj_get_float(self->decision_margin),
              mp_obj_get_int(self->hamming),
              (double) mp_obj_get_float(self->goodness),
              (double) mp_obj_get_float(self->x_translation),
              (double) mp_obj_get_float(self->y_translation),
              (double) mp_obj_get_float(self->z_translation),
              (double) mp_obj_get_float(self->x_rotation),
              (double) mp_obj_get_float(self->y_rotation),
              (double) mp_obj_get_float(self->z_rotation));
}

static mp_obj_t py_apriltag_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    if (value == MP_OBJ_SENTINEL) { // load
        py_apriltag_obj_t *self = self_in;
        if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
            mp_bound_slice_t slice;
            if (!mp_seq_get_fast_slice_indexes(py_apriltag_obj_size, index, &slice)) {
                nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
            }
            mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
            mp_seq_copy(result->items, &(self->x) + slice.start, result->len, mp_obj_t);
            return result;
        }
        switch (mp_get_index(self->base.type, py_apriltag_obj_size, index, false)) {
            case 0: return self->x;
            case 1: return self->y;
            case 2: return self->w;
            case 3: return self->h;
            case 4: return self->id;
            case 5: return self->family;
            case 6: return self->cx;
            case 7: return self->cy;
            case 8: return self->rotation;
            case 9: return self->decision_margin;
            case 10: return self->hamming;
            case 11: return self->goodness;
            case 12: return self->x_translation;
            case 13: return self->y_translation;
            case 14: return self->z_translation;
            case 15: return self->x_rotation;
            case 16: return self->y_rotation;
            case 17: return self->z_rotation;
        }
    }
    return MP_OBJ_NULL; // op not supported
}

mp_obj_t py_apriltag_corners(mp_obj_t self_in) { return ((py_apriltag_obj_t *) self_in)->corners; }
mp_obj_t py_apriltag_rect(mp_obj_t self_in)
{
    return mp_obj_new_tuple(4, (mp_obj_t []) {((py_apriltag_obj_t *) self_in)->x,
                                              ((py_apriltag_obj_t *) self_in)->y,
                                              ((py_apriltag_obj_t *) self_in)->w,
                                              ((py_apriltag_obj_t *) self_in)->h});
}

mp_obj_t py_apriltag_x(mp_obj_t self_in) { return ((py_apriltag_obj_t *) self_in)->x; }
mp_obj_t py_apriltag_y(mp_obj_t self_in) { return ((py_apriltag_obj_t *) self_in)->y; }
mp_obj_t py_apriltag_w(mp_obj_t self_in) { return ((py_apriltag_obj_t *) self_in)->w; }
mp_obj_t py_apriltag_h(mp_obj_t self_in) { return ((py_apriltag_obj_t *) self_in)->h; }
mp_obj_t py_apriltag_id(mp_obj_t self_in) { return ((py_apriltag_obj_t *) self_in)->id; }
mp_obj_t py_apriltag_family(mp_obj_t self_in) { return ((py_apriltag_obj_t *) self_in)->family; }
mp_obj_t py_apriltag_cx(mp_obj_t self_in) { return ((py_apriltag_obj_t *) self_in)->cx; }
mp_obj_t py_apriltag_cy(mp_obj_t self_in) { return ((py_apriltag_obj_t *) self_in)->cy; }
mp_obj_t py_apriltag_rotation(mp_obj_t self_in) { return ((py_apriltag_obj_t *) self_in)->rotation; }
mp_obj_t py_apriltag_decision_margin(mp_obj_t self_in) { return ((py_apriltag_obj_t *) self_in)->decision_margin; }
mp_obj_t py_apriltag_hamming(mp_obj_t self_in) { return ((py_apriltag_obj_t *) self_in)->hamming; }
mp_obj_t py_apriltag_goodness(mp_obj_t self_in) { return ((py_apriltag_obj_t *) self_in)->goodness; }
mp_obj_t py_apriltag_x_translation(mp_obj_t self_in) { return ((py_apriltag_obj_t *) self_in)->x_translation; }
mp_obj_t py_apriltag_y_translation(mp_obj_t self_in) { return ((py_apriltag_obj_t *) self_in)->y_translation; }
mp_obj_t py_apriltag_z_translation(mp_obj_t self_in) { return ((py_apriltag_obj_t *) self_in)->z_translation; }
mp_obj_t py_apriltag_x_rotation(mp_obj_t self_in) { return ((py_apriltag_obj_t *) self_in)->x_rotation; }
mp_obj_t py_apriltag_y_rotation(mp_obj_t self_in) { return ((py_apriltag_obj_t *) self_in)->y_rotation; }
mp_obj_t py_apriltag_z_rotation(mp_obj_t self_in) { return ((py_apriltag_obj_t *) self_in)->z_rotation; }

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_apriltag_corners_obj, py_apriltag_corners);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_apriltag_rect_obj, py_apriltag_rect);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_apriltag_x_obj, py_apriltag_x);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_apriltag_y_obj, py_apriltag_y);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_apriltag_w_obj, py_apriltag_w);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_apriltag_h_obj, py_apriltag_h);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_apriltag_id_obj, py_apriltag_id);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_apriltag_family_obj, py_apriltag_family);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_apriltag_cx_obj, py_apriltag_cx);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_apriltag_cy_obj, py_apriltag_cy);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_apriltag_rotation_obj, py_apriltag_rotation);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_apriltag_decision_margin_obj, py_apriltag_decision_margin);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_apriltag_hamming_obj, py_apriltag_hamming);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_apriltag_goodness_obj, py_apriltag_goodness);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_apriltag_x_translation_obj, py_apriltag_x_translation);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_apriltag_y_translation_obj, py_apriltag_y_translation);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_apriltag_z_translation_obj, py_apriltag_z_translation);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_apriltag_x_rotation_obj, py_apriltag_x_rotation);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_apriltag_y_rotation_obj, py_apriltag_y_rotation);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_apriltag_z_rotation_obj, py_apriltag_z_rotation);

STATIC const mp_rom_map_elem_t py_apriltag_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_corners), MP_ROM_PTR(&py_apriltag_corners_obj) },
    { MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&py_apriltag_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_x), MP_ROM_PTR(&py_apriltag_x_obj) },
    { MP_ROM_QSTR(MP_QSTR_y), MP_ROM_PTR(&py_apriltag_y_obj) },
    { MP_ROM_QSTR(MP_QSTR_w), MP_ROM_PTR(&py_apriltag_w_obj) },
    { MP_ROM_QSTR(MP_QSTR_h), MP_ROM_PTR(&py_apriltag_h_obj) },
    { MP_ROM_QSTR(MP_QSTR_id), MP_ROM_PTR(&py_apriltag_id_obj) },
    { MP_ROM_QSTR(MP_QSTR_family), MP_ROM_PTR(&py_apriltag_family_obj) },
    { MP_ROM_QSTR(MP_QSTR_cx), MP_ROM_PTR(&py_apriltag_cx_obj) },
    { MP_ROM_QSTR(MP_QSTR_cy), MP_ROM_PTR(&py_apriltag_cy_obj) },
    { MP_ROM_QSTR(MP_QSTR_rotation), MP_ROM_PTR(&py_apriltag_rotation_obj) },
    { MP_ROM_QSTR(MP_QSTR_decision_margin), MP_ROM_PTR(&py_apriltag_decision_margin_obj) },
    { MP_ROM_QSTR(MP_QSTR_hamming), MP_ROM_PTR(&py_apriltag_hamming_obj) },
    { MP_ROM_QSTR(MP_QSTR_goodness), MP_ROM_PTR(&py_apriltag_goodness_obj) },
    { MP_ROM_QSTR(MP_QSTR_x_translation), MP_ROM_PTR(&py_apriltag_x_translation_obj) },
    { MP_ROM_QSTR(MP_QSTR_y_translation), MP_ROM_PTR(&py_apriltag_y_translation_obj) },
    { MP_ROM_QSTR(MP_QSTR_z_translation), MP_ROM_PTR(&py_apriltag_z_translation_obj) },
    { MP_ROM_QSTR(MP_QSTR_x_rotation), MP_ROM_PTR(&py_apriltag_x_rotation_obj) },
    { MP_ROM_QSTR(MP_QSTR_y_rotation), MP_ROM_PTR(&py_apriltag_y_rotation_obj) },
    { MP_ROM_QSTR(MP_QSTR_z_rotation), MP_ROM_PTR(&py_apriltag_z_rotation_obj) }
};

STATIC MP_DEFINE_CONST_DICT(py_apriltag_locals_dict, py_apriltag_locals_dict_table);

static const mp_obj_type_t py_apriltag_type = {
    { &mp_type_type },
    .name  = MP_QSTR_apriltag,
    .print = py_apriltag_print,
    .subscr = py_apriltag_subscr,
    .locals_dict = (mp_obj_t) &py_apriltag_locals_dict
};

static mp_obj_t py_image_find_apriltags(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 1, kw_args, &roi);
    PY_ASSERT_TRUE_MSG((roi.w * roi.h) < 65536, "The maximum supported resolution for find_apriltags() is < 64K pixels.");
    if ((roi.w < 4) || (roi.h < 4)) {
        return mp_obj_new_list(0, NULL);
    }

    apriltag_families_t families = py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_families), TAG36H11);
    // 2.8mm Focal Length w/ OV7725 sensor for reference.
    float fx = py_helper_keyword_float(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_fx), (2.8 / 3.984) * arg_img->w);
    // 2.8mm Focal Length w/ OV7725 sensor for reference.
    float fy = py_helper_keyword_float(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_fy), (2.8 / 2.952) * arg_img->h);
    // Use the image versus the roi here since the image should be projected from the camera center.
    float cx = py_helper_keyword_float(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_cx), arg_img->w * 0.5);
    // Use the image versus the roi here since the image should be projected from the camera center.
    float cy = py_helper_keyword_float(n_args, args, 6, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_cy), arg_img->h * 0.5);

    list_t out;
    fb_alloc_mark();
    imlib_find_apriltags(&out, arg_img, &roi, families, fx, fy, cx, cy);
    fb_alloc_free_till_mark();

    mp_obj_list_t *objects_list = mp_obj_new_list(list_size(&out), NULL);
    for (size_t i = 0; list_size(&out); i++) {
        find_apriltags_list_lnk_data_t lnk_data;
        list_pop_front(&out, &lnk_data);

        py_apriltag_obj_t *o = m_new_obj(py_apriltag_obj_t);
        o->base.type = &py_apriltag_type;
        o->corners = mp_obj_new_tuple(4, (mp_obj_t [])
            {mp_obj_new_tuple(2, (mp_obj_t []) {mp_obj_new_int(lnk_data.corners[0].x), mp_obj_new_int(lnk_data.corners[0].y)}),
             mp_obj_new_tuple(2, (mp_obj_t []) {mp_obj_new_int(lnk_data.corners[1].x), mp_obj_new_int(lnk_data.corners[1].y)}),
             mp_obj_new_tuple(2, (mp_obj_t []) {mp_obj_new_int(lnk_data.corners[2].x), mp_obj_new_int(lnk_data.corners[2].y)}),
             mp_obj_new_tuple(2, (mp_obj_t []) {mp_obj_new_int(lnk_data.corners[3].x), mp_obj_new_int(lnk_data.corners[3].y)})});
        o->x = mp_obj_new_int(lnk_data.rect.x);
        o->y = mp_obj_new_int(lnk_data.rect.y);
        o->w = mp_obj_new_int(lnk_data.rect.w);
        o->h = mp_obj_new_int(lnk_data.rect.h);
        o->id = mp_obj_new_int(lnk_data.id);
        o->family = mp_obj_new_int(lnk_data.family);
        o->cx = mp_obj_new_int(lnk_data.centroid.x);
        o->cy = mp_obj_new_int(lnk_data.centroid.y);
        o->rotation = mp_obj_new_float(lnk_data.z_rotation);
        o->decision_margin = mp_obj_new_float(lnk_data.decision_margin);
        o->hamming = mp_obj_new_int(lnk_data.hamming);
        o->goodness = mp_obj_new_float(lnk_data.goodness);
        o->x_translation = mp_obj_new_float(lnk_data.x_translation);
        o->y_translation = mp_obj_new_float(lnk_data.y_translation);
        o->z_translation = mp_obj_new_float(lnk_data.z_translation);
        o->x_rotation = mp_obj_new_float(lnk_data.x_rotation);
        o->y_rotation = mp_obj_new_float(lnk_data.y_rotation);
        o->z_rotation = mp_obj_new_float(lnk_data.z_rotation);

        objects_list->items[i] = o;
    }

    return objects_list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_find_apriltags_obj, 1, py_image_find_apriltags);
#endif // IMLIB_ENABLE_APRILTAGS

#ifdef IMLIB_ENABLE_DATAMATRICES
// DataMatrix Object //
#define py_datamatrix_obj_size 10
typedef struct py_datamatrix_obj {
    mp_obj_base_t base;
    mp_obj_t corners;
    mp_obj_t x, y, w, h, payload, rotation, rows, columns, capacity, padding;
} py_datamatrix_obj_t;

static void py_datamatrix_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_datamatrix_obj_t *self = self_in;
    mp_printf(print,
              "{\"x\":%d, \"y\":%d, \"w\":%d, \"h\":%d, \"payload\":\"%s\","
              " \"rotation\":%f, \"rows\":%d, \"columns\":%d, \"capacity\":%d, \"padding\":%d}",
              mp_obj_get_int(self->x),
              mp_obj_get_int(self->y),
              mp_obj_get_int(self->w),
              mp_obj_get_int(self->h),
              mp_obj_str_get_str(self->payload),
              (double) mp_obj_get_float(self->rotation),
              mp_obj_get_int(self->rows),
              mp_obj_get_int(self->columns),
              mp_obj_get_int(self->capacity),
              mp_obj_get_int(self->padding));
}

static mp_obj_t py_datamatrix_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    if (value == MP_OBJ_SENTINEL) { // load
        py_datamatrix_obj_t *self = self_in;
        if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
            mp_bound_slice_t slice;
            if (!mp_seq_get_fast_slice_indexes(py_datamatrix_obj_size, index, &slice)) {
                nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
            }
            mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
            mp_seq_copy(result->items, &(self->x) + slice.start, result->len, mp_obj_t);
            return result;
        }
        switch (mp_get_index(self->base.type, py_datamatrix_obj_size, index, false)) {
            case 0: return self->x;
            case 1: return self->y;
            case 2: return self->w;
            case 3: return self->h;
            case 4: return self->payload;
            case 5: return self->rotation;
            case 6: return self->rows;
            case 7: return self->columns;
            case 8: return self->capacity;
            case 9: return self->padding;
        }
    }
    return MP_OBJ_NULL; // op not supported
}

mp_obj_t py_datamatrix_corners(mp_obj_t self_in) { return ((py_datamatrix_obj_t *) self_in)->corners; }
mp_obj_t py_datamatrix_rect(mp_obj_t self_in)
{
    return mp_obj_new_tuple(4, (mp_obj_t []) {((py_datamatrix_obj_t *) self_in)->x,
                                              ((py_datamatrix_obj_t *) self_in)->y,
                                              ((py_datamatrix_obj_t *) self_in)->w,
                                              ((py_datamatrix_obj_t *) self_in)->h});
}

mp_obj_t py_datamatrix_x(mp_obj_t self_in) { return ((py_datamatrix_obj_t *) self_in)->x; }
mp_obj_t py_datamatrix_y(mp_obj_t self_in) { return ((py_datamatrix_obj_t *) self_in)->y; }
mp_obj_t py_datamatrix_w(mp_obj_t self_in) { return ((py_datamatrix_obj_t *) self_in)->w; }
mp_obj_t py_datamatrix_h(mp_obj_t self_in) { return ((py_datamatrix_obj_t *) self_in)->h; }
mp_obj_t py_datamatrix_payload(mp_obj_t self_in) { return ((py_datamatrix_obj_t *) self_in)->payload; }
mp_obj_t py_datamatrix_rotation(mp_obj_t self_in) { return ((py_datamatrix_obj_t *) self_in)->rotation; }
mp_obj_t py_datamatrix_rows(mp_obj_t self_in) { return ((py_datamatrix_obj_t *) self_in)->rows; }
mp_obj_t py_datamatrix_columns(mp_obj_t self_in) { return ((py_datamatrix_obj_t *) self_in)->columns; }
mp_obj_t py_datamatrix_capacity(mp_obj_t self_in) { return ((py_datamatrix_obj_t *) self_in)->capacity; }
mp_obj_t py_datamatrix_padding(mp_obj_t self_in) { return ((py_datamatrix_obj_t *) self_in)->padding; }

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_datamatrix_corners_obj, py_datamatrix_corners);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_datamatrix_rect_obj, py_datamatrix_rect);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_datamatrix_x_obj, py_datamatrix_x);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_datamatrix_y_obj, py_datamatrix_y);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_datamatrix_w_obj, py_datamatrix_w);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_datamatrix_h_obj, py_datamatrix_h);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_datamatrix_payload_obj, py_datamatrix_payload);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_datamatrix_rotation_obj, py_datamatrix_rotation);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_datamatrix_rows_obj, py_datamatrix_rows);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_datamatrix_columns_obj, py_datamatrix_columns);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_datamatrix_capacity_obj, py_datamatrix_capacity);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_datamatrix_padding_obj, py_datamatrix_padding);

STATIC const mp_rom_map_elem_t py_datamatrix_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_corners), MP_ROM_PTR(&py_datamatrix_corners_obj) },
    { MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&py_datamatrix_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_x), MP_ROM_PTR(&py_datamatrix_x_obj) },
    { MP_ROM_QSTR(MP_QSTR_y), MP_ROM_PTR(&py_datamatrix_y_obj) },
    { MP_ROM_QSTR(MP_QSTR_w), MP_ROM_PTR(&py_datamatrix_w_obj) },
    { MP_ROM_QSTR(MP_QSTR_h), MP_ROM_PTR(&py_datamatrix_h_obj) },
    { MP_ROM_QSTR(MP_QSTR_payload), MP_ROM_PTR(&py_datamatrix_payload_obj) },
    { MP_ROM_QSTR(MP_QSTR_rotation), MP_ROM_PTR(&py_datamatrix_rotation_obj) },
    { MP_ROM_QSTR(MP_QSTR_rows), MP_ROM_PTR(&py_datamatrix_rows_obj) },
    { MP_ROM_QSTR(MP_QSTR_columns), MP_ROM_PTR(&py_datamatrix_columns_obj) },
    { MP_ROM_QSTR(MP_QSTR_capacity), MP_ROM_PTR(&py_datamatrix_capacity_obj) },
    { MP_ROM_QSTR(MP_QSTR_padding), MP_ROM_PTR(&py_datamatrix_padding_obj) }
};

STATIC MP_DEFINE_CONST_DICT(py_datamatrix_locals_dict, py_datamatrix_locals_dict_table);

static const mp_obj_type_t py_datamatrix_type = {
    { &mp_type_type },
    .name  = MP_QSTR_datamatrix,
    .print = py_datamatrix_print,
    .subscr = py_datamatrix_subscr,
    .locals_dict = (mp_obj_t) &py_datamatrix_locals_dict
};

static mp_obj_t py_image_find_datamatrices(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 1, kw_args, &roi);

    int effort = py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_effort), 200);

    list_t out;
    fb_alloc_mark();
    imlib_find_datamatrices(&out, arg_img, &roi, effort);
    fb_alloc_free_till_mark();

    mp_obj_list_t *objects_list = mp_obj_new_list(list_size(&out), NULL);
    for (size_t i = 0; list_size(&out); i++) {
        find_datamatrices_list_lnk_data_t lnk_data;
        list_pop_front(&out, &lnk_data);

        py_datamatrix_obj_t *o = m_new_obj(py_datamatrix_obj_t);
        o->base.type = &py_datamatrix_type;
        o->corners = mp_obj_new_tuple(4, (mp_obj_t [])
            {mp_obj_new_tuple(2, (mp_obj_t []) {mp_obj_new_int(lnk_data.corners[0].x), mp_obj_new_int(lnk_data.corners[0].y)}),
             mp_obj_new_tuple(2, (mp_obj_t []) {mp_obj_new_int(lnk_data.corners[1].x), mp_obj_new_int(lnk_data.corners[1].y)}),
             mp_obj_new_tuple(2, (mp_obj_t []) {mp_obj_new_int(lnk_data.corners[2].x), mp_obj_new_int(lnk_data.corners[2].y)}),
             mp_obj_new_tuple(2, (mp_obj_t []) {mp_obj_new_int(lnk_data.corners[3].x), mp_obj_new_int(lnk_data.corners[3].y)})});
        o->x = mp_obj_new_int(lnk_data.rect.x);
        o->y = mp_obj_new_int(lnk_data.rect.y);
        o->w = mp_obj_new_int(lnk_data.rect.w);
        o->h = mp_obj_new_int(lnk_data.rect.h);
        o->payload = mp_obj_new_str(lnk_data.payload, lnk_data.payload_len);
        o->rotation = mp_obj_new_float(IM_DEG2RAD(lnk_data.rotation));
        o->rows = mp_obj_new_int(lnk_data.rows);
        o->columns = mp_obj_new_int(lnk_data.columns);
        o->capacity = mp_obj_new_int(lnk_data.capacity);
        o->padding = mp_obj_new_int(lnk_data.padding);

        objects_list->items[i] = o;
        xfree(lnk_data.payload);
    }

    return objects_list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_find_datamatrices_obj, 1, py_image_find_datamatrices);
#endif // IMLIB_ENABLE_DATAMATRICES

#ifdef IMLIB_ENABLE_BARCODES
// BarCode Object //
#define py_barcode_obj_size 8
typedef struct py_barcode_obj {
    mp_obj_base_t base;
    mp_obj_t corners;
    mp_obj_t x, y, w, h, payload, type, rotation, quality;
} py_barcode_obj_t;

static void py_barcode_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_barcode_obj_t *self = self_in;
    mp_printf(print,
              "{\"x\":%d, \"y\":%d, \"w\":%d, \"h\":%d, \"payload\":\"%s\","
              " \"type\":%d, \"rotation\":%f, \"quality\":%d}",
              mp_obj_get_int(self->x),
              mp_obj_get_int(self->y),
              mp_obj_get_int(self->w),
              mp_obj_get_int(self->h),
              mp_obj_str_get_str(self->payload),
              mp_obj_get_int(self->type),
              (double) mp_obj_get_float(self->rotation),
              mp_obj_get_int(self->quality));
}

static mp_obj_t py_barcode_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    if (value == MP_OBJ_SENTINEL) { // load
        py_barcode_obj_t *self = self_in;
        if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
            mp_bound_slice_t slice;
            if (!mp_seq_get_fast_slice_indexes(py_barcode_obj_size, index, &slice)) {
                nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
            }
            mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
            mp_seq_copy(result->items, &(self->x) + slice.start, result->len, mp_obj_t);
            return result;
        }
        switch (mp_get_index(self->base.type, py_barcode_obj_size, index, false)) {
            case 0: return self->x;
            case 1: return self->y;
            case 2: return self->w;
            case 3: return self->h;
            case 4: return self->payload;
            case 5: return self->type;
            case 6: return self->rotation;
            case 7: return self->quality;
        }
    }
    return MP_OBJ_NULL; // op not supported
}

mp_obj_t py_barcode_corners(mp_obj_t self_in) { return ((py_barcode_obj_t *) self_in)->corners; }
mp_obj_t py_barcode_rect(mp_obj_t self_in)
{
    return mp_obj_new_tuple(4, (mp_obj_t []) {((py_barcode_obj_t *) self_in)->x,
                                              ((py_barcode_obj_t *) self_in)->y,
                                              ((py_barcode_obj_t *) self_in)->w,
                                              ((py_barcode_obj_t *) self_in)->h});
}

mp_obj_t py_barcode_x(mp_obj_t self_in) { return ((py_barcode_obj_t *) self_in)->x; }
mp_obj_t py_barcode_y(mp_obj_t self_in) { return ((py_barcode_obj_t *) self_in)->y; }
mp_obj_t py_barcode_w(mp_obj_t self_in) { return ((py_barcode_obj_t *) self_in)->w; }
mp_obj_t py_barcode_h(mp_obj_t self_in) { return ((py_barcode_obj_t *) self_in)->h; }
mp_obj_t py_barcode_payload_fun(mp_obj_t self_in) { return ((py_barcode_obj_t *) self_in)->payload; }
mp_obj_t py_barcode_type_fun(mp_obj_t self_in) { return ((py_barcode_obj_t *) self_in)->type; }
mp_obj_t py_barcode_rotation_fun(mp_obj_t self_in) { return ((py_barcode_obj_t *) self_in)->rotation; }
mp_obj_t py_barcode_quality_fun(mp_obj_t self_in) { return ((py_barcode_obj_t *) self_in)->quality; }

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_barcode_corners_obj, py_barcode_corners);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_barcode_rect_obj, py_barcode_rect);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_barcode_x_obj, py_barcode_x);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_barcode_y_obj, py_barcode_y);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_barcode_w_obj, py_barcode_w);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_barcode_h_obj, py_barcode_h);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_barcode_payload_fun_obj, py_barcode_payload_fun);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_barcode_type_fun_obj, py_barcode_type_fun);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_barcode_rotation_fun_obj, py_barcode_rotation_fun);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_barcode_quality_fun_obj, py_barcode_quality_fun);

STATIC const mp_rom_map_elem_t py_barcode_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_corners), MP_ROM_PTR(&py_barcode_corners_obj) },
    { MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&py_barcode_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_x), MP_ROM_PTR(&py_barcode_x_obj) },
    { MP_ROM_QSTR(MP_QSTR_y), MP_ROM_PTR(&py_barcode_y_obj) },
    { MP_ROM_QSTR(MP_QSTR_w), MP_ROM_PTR(&py_barcode_w_obj) },
    { MP_ROM_QSTR(MP_QSTR_h), MP_ROM_PTR(&py_barcode_h_obj) },
    { MP_ROM_QSTR(MP_QSTR_payload), MP_ROM_PTR(&py_barcode_payload_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_type), MP_ROM_PTR(&py_barcode_type_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_rotation), MP_ROM_PTR(&py_barcode_rotation_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_quality), MP_ROM_PTR(&py_barcode_quality_fun_obj) }
};

STATIC MP_DEFINE_CONST_DICT(py_barcode_locals_dict, py_barcode_locals_dict_table);

static const mp_obj_type_t py_barcode_type = {
    { &mp_type_type },
    .name  = MP_QSTR_barcode,
    .print = py_barcode_print,
    .subscr = py_barcode_subscr,
    .locals_dict = (mp_obj_t) &py_barcode_locals_dict
};

static mp_obj_t py_image_find_barcodes(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 1, kw_args, &roi);

    list_t out;
    fb_alloc_mark();
    imlib_find_barcodes(&out, arg_img, &roi);
    fb_alloc_free_till_mark();

    mp_obj_list_t *objects_list = mp_obj_new_list(list_size(&out), NULL);
    for (size_t i = 0; list_size(&out); i++) {
        find_barcodes_list_lnk_data_t lnk_data;
        list_pop_front(&out, &lnk_data);

        py_barcode_obj_t *o = m_new_obj(py_barcode_obj_t);
        o->base.type = &py_barcode_type;
        o->corners = mp_obj_new_tuple(4, (mp_obj_t [])
            {mp_obj_new_tuple(2, (mp_obj_t []) {mp_obj_new_int(lnk_data.corners[0].x), mp_obj_new_int(lnk_data.corners[0].y)}),
             mp_obj_new_tuple(2, (mp_obj_t []) {mp_obj_new_int(lnk_data.corners[1].x), mp_obj_new_int(lnk_data.corners[1].y)}),
             mp_obj_new_tuple(2, (mp_obj_t []) {mp_obj_new_int(lnk_data.corners[2].x), mp_obj_new_int(lnk_data.corners[2].y)}),
             mp_obj_new_tuple(2, (mp_obj_t []) {mp_obj_new_int(lnk_data.corners[3].x), mp_obj_new_int(lnk_data.corners[3].y)})});
        o->x = mp_obj_new_int(lnk_data.rect.x);
        o->y = mp_obj_new_int(lnk_data.rect.y);
        o->w = mp_obj_new_int(lnk_data.rect.w);
        o->h = mp_obj_new_int(lnk_data.rect.h);
        o->payload = mp_obj_new_str(lnk_data.payload, lnk_data.payload_len);
        o->type = mp_obj_new_int(lnk_data.type);
        o->rotation = mp_obj_new_float(IM_DEG2RAD(lnk_data.rotation));
        o->quality = mp_obj_new_int(lnk_data.quality);

        objects_list->items[i] = o;
        xfree(lnk_data.payload);
    }

    return objects_list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_find_barcodes_obj, 1, py_image_find_barcodes);
#endif // IMLIB_ENABLE_BARCODES

#ifdef IMLIB_ENABLE_FIND_DISPLACEMENT
// Displacement Object //
#define py_displacement_obj_size 5
typedef struct py_displacement_obj {
    mp_obj_base_t base;
    mp_obj_t x_translation, y_translation, rotation, scale, response;
} py_displacement_obj_t;

static void py_displacement_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_displacement_obj_t *self = self_in;
    mp_printf(print,
              "{\"x_translation\":%f, \"y_translation\":%f, \"rotation\":%f, \"scale\":%f, \"response\":%f}",
              (double) mp_obj_get_float(self->x_translation),
              (double) mp_obj_get_float(self->y_translation),
              (double) mp_obj_get_float(self->rotation),
              (double) mp_obj_get_float(self->scale),
              (double) mp_obj_get_float(self->response));
}

static mp_obj_t py_displacement_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    if (value == MP_OBJ_SENTINEL) { // load
        py_displacement_obj_t *self = self_in;
        if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
            mp_bound_slice_t slice;
            if (!mp_seq_get_fast_slice_indexes(py_displacement_obj_size, index, &slice)) {
                nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
            }
            mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
            mp_seq_copy(result->items, &(self->x_translation) + slice.start, result->len, mp_obj_t);
            return result;
        }
        switch (mp_get_index(self->base.type, py_displacement_obj_size, index, false)) {
            case 0: return self->x_translation;
            case 1: return self->y_translation;
            case 2: return self->rotation;
            case 3: return self->scale;
            case 4: return self->response;
        }
    }
    return MP_OBJ_NULL; // op not supported
}

mp_obj_t py_displacement_x_translation(mp_obj_t self_in) { return ((py_displacement_obj_t *) self_in)->x_translation; }
mp_obj_t py_displacement_y_translation(mp_obj_t self_in) { return ((py_displacement_obj_t *) self_in)->y_translation; }
mp_obj_t py_displacement_rotation(mp_obj_t self_in) { return ((py_displacement_obj_t *) self_in)->rotation; }
mp_obj_t py_displacement_scale(mp_obj_t self_in) { return ((py_displacement_obj_t *) self_in)->scale; }
mp_obj_t py_displacement_response(mp_obj_t self_in) { return ((py_displacement_obj_t *) self_in)->response; }

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_displacement_x_translation_obj, py_displacement_x_translation);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_displacement_y_translation_obj, py_displacement_y_translation);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_displacement_rotation_obj, py_displacement_rotation);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_displacement_scale_obj, py_displacement_scale);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_displacement_response_obj, py_displacement_response);

STATIC const mp_rom_map_elem_t py_displacement_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_x_translation), MP_ROM_PTR(&py_displacement_x_translation_obj) },
    { MP_ROM_QSTR(MP_QSTR_y_translation), MP_ROM_PTR(&py_displacement_y_translation_obj) },
    { MP_ROM_QSTR(MP_QSTR_rotation), MP_ROM_PTR(&py_displacement_rotation_obj) },
    { MP_ROM_QSTR(MP_QSTR_scale), MP_ROM_PTR(&py_displacement_scale_obj) },
    { MP_ROM_QSTR(MP_QSTR_response), MP_ROM_PTR(&py_displacement_response_obj) }
};

STATIC MP_DEFINE_CONST_DICT(py_displacement_locals_dict, py_displacement_locals_dict_table);

static const mp_obj_type_t py_displacement_type = {
    { &mp_type_type },
    .name  = MP_QSTR_displacement,
    .print = py_displacement_print,
    .subscr = py_displacement_subscr,
    .locals_dict = (mp_obj_t) &py_displacement_locals_dict
};

static mp_obj_t py_image_find_displacement(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);
    image_t *arg_template_img = py_helper_arg_to_image_mutable(args[1]);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 2, kw_args, &roi);

    rectangle_t template_roi;
    py_helper_keyword_rectangle(arg_template_img, n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_template_roi), &template_roi);

    PY_ASSERT_FALSE_MSG((roi.w != template_roi.w) || (roi.h != template_roi.h), "ROI(w,h) != TEMPLATE_ROI(w,h)");

    bool logpolar = py_helper_keyword_int(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_logpolar), false);
    bool fix_rotation_scale = py_helper_keyword_int(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_fix_rotation_scale), false);

    float x, y, r, s, response;
    fb_alloc_mark();
    imlib_phasecorrelate(arg_img, arg_template_img, &roi, &template_roi, logpolar, fix_rotation_scale, &x, &y, &r, &s, &response);
    fb_alloc_free_till_mark();

    py_displacement_obj_t *o = m_new_obj(py_displacement_obj_t);
    o->base.type = &py_displacement_type;
    o->x_translation = mp_obj_new_float(x);
    o->y_translation = mp_obj_new_float(y);
    o->rotation = mp_obj_new_float(r);
    o->scale = mp_obj_new_float(s);
    o->response = mp_obj_new_float(response);

    return o;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_find_displacement_obj, 2, py_image_find_displacement);
#endif // IMLIB_ENABLE_FIND_DISPLACEMENT

#ifndef OMV_MINIMUM
static mp_obj_t py_image_find_template(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_grayscale(args[0]);
    image_t *arg_template = py_helper_arg_to_image_grayscale(args[1]);
    float arg_thresh = mp_obj_get_float(args[2]);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 3, kw_args, &roi);

    // Make sure ROI is bigger than or equal to template size
    PY_ASSERT_TRUE_MSG((roi.w >= arg_template->w && roi.h >= arg_template->h),
            "Region of interest is smaller than template!");

    // Make sure ROI is smaller than or equal to image size
    PY_ASSERT_TRUE_MSG(((roi.x + roi.w) <= arg_img->w && (roi.y + roi.h) <= arg_img->h),
            "Region of interest is bigger than image!");

    int step = py_helper_keyword_int(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_step), 2);
    int search = py_helper_keyword_int(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_search), SEARCH_EX);

    // Find template
    rectangle_t r;
    float corr;
    if (search == SEARCH_DS) {
        corr = imlib_template_match_ds(arg_img, arg_template, &r);
    } else {
        corr = imlib_template_match_ex(arg_img, arg_template, &roi, step, &r);
    }

    if (corr > arg_thresh) {
        mp_obj_t rec_obj[4] = {
            mp_obj_new_int(r.x),
            mp_obj_new_int(r.y),
            mp_obj_new_int(r.w),
            mp_obj_new_int(r.h)
        };
        return mp_obj_new_tuple(4, rec_obj);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_find_template_obj, 3, py_image_find_template);

static mp_obj_t py_image_find_features(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);
    cascade_t *cascade = py_cascade_cobj(args[1]);
    cascade->threshold = py_helper_keyword_float(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold), 0.5f);
    cascade->scale_factor = py_helper_keyword_float(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_scale_factor), 1.5f);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 4, kw_args, &roi);

    // Make sure ROI is bigger than feature size
    PY_ASSERT_TRUE_MSG((roi.w > cascade->window.w && roi.h > cascade->window.h),
            "Region of interest is smaller than detector window!");

    // Detect objects
    array_t *objects_array = imlib_detect_objects(arg_img, cascade, &roi);

    // Add detected objects to a new Python list...
    mp_obj_t objects_list = mp_obj_new_list(0, NULL);
    for (int i=0; i<array_length(objects_array); i++) {
        rectangle_t *r = array_at(objects_array, i);
        mp_obj_t rec_obj[4] = {
            mp_obj_new_int(r->x),
            mp_obj_new_int(r->y),
            mp_obj_new_int(r->w),
            mp_obj_new_int(r->h),
        };
        mp_obj_list_append(objects_list, mp_obj_new_tuple(4, rec_obj));
    }
    array_free(objects_array);
    return objects_list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_find_features_obj, 2, py_image_find_features);

static mp_obj_t py_image_find_eye(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_grayscale(args[0]);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 1, kw_args, &roi);

    point_t iris;
    imlib_find_iris(arg_img, &iris, &roi);

    mp_obj_t eye_obj[2] = {
        mp_obj_new_int(iris.x),
        mp_obj_new_int(iris.y),
    };

    return mp_obj_new_tuple(2, eye_obj);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_find_eye_obj, 2, py_image_find_eye);

static mp_obj_t py_image_find_lbp(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_grayscale(args[0]);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 1, kw_args, &roi);

    py_lbp_obj_t *lbp_obj = m_new_obj(py_lbp_obj_t);
    lbp_obj->base.type = &py_lbp_type;
    lbp_obj->hist = imlib_lbp_desc(arg_img, &roi);
    return lbp_obj;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_find_lbp_obj, 2, py_image_find_lbp);

static mp_obj_t py_image_find_keypoints(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 1, kw_args, &roi);

    int threshold =
        py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold), 20);
    bool normalized =
        py_helper_keyword_int(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_normalized), false);
    float scale_factor =
        py_helper_keyword_float(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_scale_factor), 1.5f);
    int max_keypoints =
        py_helper_keyword_int(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_max_keypoints), 100);
    corner_detector_t corner_detector =
        py_helper_keyword_int(n_args, args, 6, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_corner_detector), CORNER_AGAST);

    #ifndef IMLIB_ENABLE_FAST
    // Force AGAST when FAST is disabled.
    corner_detector = CORNER_AGAST;
    #endif

    // Find keypoints
    array_t *kpts = orb_find_keypoints(arg_img, normalized, threshold, scale_factor, max_keypoints, corner_detector, &roi);

    if (array_length(kpts)) {
        py_kp_obj_t *kp_obj = m_new_obj(py_kp_obj_t);
        kp_obj->base.type = &py_kp_type;
        kp_obj->kpts = kpts;
        kp_obj->threshold = threshold;
        kp_obj->normalized = normalized;
        return kp_obj;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_find_keypoints_obj, 1, py_image_find_keypoints);

#endif //OMV_MINIMUM

#ifdef IMLIB_ENABLE_BINARY_OPS
static mp_obj_t py_image_find_edges(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_grayscale(args[0]);
    edge_detector_t edge_type = mp_obj_get_int(args[1]);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 2, kw_args, &roi);

    int thresh[2] = {100, 200};
    mp_obj_t thresh_obj = py_helper_keyword_object(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold));

    if (thresh_obj) {
        mp_obj_t *thresh_array;
        mp_obj_get_array_fixed_n(thresh_obj, 2, &thresh_array);
        thresh[0] = mp_obj_get_int(thresh_array[0]);
        thresh[1] = mp_obj_get_int(thresh_array[1]);
    }

    switch (edge_type) {
        case EDGE_SIMPLE: {
            imlib_edge_simple(arg_img, &roi, thresh[0], thresh[1]);
            break;
        }
        case EDGE_CANNY: {
            imlib_edge_canny(arg_img, &roi, thresh[0], thresh[1]);
            break;
        }

    }

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_find_edges_obj, 2, py_image_find_edges);
#endif

#ifdef IMLIB_ENABLE_HOG
static mp_obj_t py_image_find_hog(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_grayscale(args[0]);

    rectangle_t roi;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 1, kw_args, &roi);

    int size = py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_size), 8);

    imlib_find_hog(arg_img, &roi, size);

    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_find_hog_obj, 1, py_image_find_hog);
#endif // IMLIB_ENABLE_HOG

#ifdef IMLIB_ENABLE_SELECTIVE_SEARCH
static mp_obj_t py_image_selective_search(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *img = py_helper_arg_to_image_mutable(args[0]);
    int t = py_helper_keyword_int(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold), 500);
    int s = py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_size), 20);
    float a1 = py_helper_keyword_float(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_a1), 1.0f);
    float a2 = py_helper_keyword_float(n_args, args, 4, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_a1), 1.0f);
    float a3 = py_helper_keyword_float(n_args, args, 5, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_a1), 1.0f);
    array_t *proposals_array = imlib_selective_search(img, t, s, a1, a2, a3);

    // Add proposals to a new Python list...
    mp_obj_t proposals_list = mp_obj_new_list(0, NULL);
    for (int i=0; i<array_length(proposals_array); i++) {
        rectangle_t *r = array_at(proposals_array, i);
        mp_obj_t rec_obj[4] = {
            mp_obj_new_int(r->x),
            mp_obj_new_int(r->y),
            mp_obj_new_int(r->w),
            mp_obj_new_int(r->h),
        };
        mp_obj_list_append(proposals_list, mp_obj_new_tuple(4, rec_obj));
    }

    array_free(proposals_array);
    return proposals_list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_selective_search_obj, 1, py_image_selective_search);
#endif // IMLIB_ENABLE_SELECTIVE_SEARCH

static mp_obj_t py_image_dump_roi(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);
    const mp_obj_t *arg_vec;
    /* uint offset = */ py_helper_consume_array(n_args, args, 1, 4, &arg_vec);
    int x0 = mp_obj_get_int(arg_vec[0]);
    int y0 = mp_obj_get_int(arg_vec[1]);
    int w = mp_obj_get_int(arg_vec[2]);
	int h = mp_obj_get_int(arg_vec[3]);
	int x,y;
	uint8_t* ptr=arg_img->pixels;
	uint16_t width = arg_img->w;
	for(y=y0;y<y0+h;y++)
	{
		for(x=x0;x<x0+w;x++) //TODO:pixel format
		{
			mp_printf(&mp_plat_print, "0x%02x 0x%02x; ",ptr[2*width*y+2*x], ptr[2*width*y+2*x+1]);
			if((x-x0)%8==7) mp_printf(&mp_plat_print, "\n");
		}
		mp_printf(&mp_plat_print, "\n");
	}
	mp_printf(&mp_plat_print, "\n");
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_dump_roi_obj, 2, py_image_dump_roi);

static mp_obj_t py_image_conv3(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
	image_t *arg_img = py_helper_arg_to_image_mutable(args[0]);
    mp_map_elem_t *kw_arg = mp_map_lookup(kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_kern), MP_MAP_LOOKUP);
	float krn[9];
	int i;
	
    if (kw_arg) {
        mp_obj_t *arg_conv3;
        mp_obj_get_array_fixed_n(kw_arg->value, 9, &arg_conv3);
		for(i=0; i<9; i++)
		{
			krn[i] = mp_obj_get_float(arg_conv3[i]);
		}
    } else if (n_args > 1) {
        mp_obj_t *arg_conv3;
        mp_obj_get_array_fixed_n(args[1], 9, &arg_conv3);
		for(i=0; i<9; i++)
		{
			krn[i] = mp_obj_get_float(arg_conv3[i]);
		}
	} else 
	{
		mp_printf(&mp_plat_print, "please input 9 kern parm!\n");
		return mp_const_none;
	}
	imlib_conv3(arg_img, krn);
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_conv3_obj, 2, py_image_conv3);

static mp_obj_t py_image_resize(mp_obj_t img_obj, mp_obj_t w_obj, mp_obj_t h_obj)
{
	image_t* img = (image_t *) py_image_cobj(img_obj);
	int w = mp_obj_get_int(w_obj);
	int h = mp_obj_get_int(h_obj);
	int w0 = img->w;
	int h0 = img->h;
	switch (img->bpp)
	{
	case IMAGE_BPP_GRAYSCALE:
	{
		uint8_t* out = xalloc(w*h);
		uint8_t* in = img->pixels;
		float sx=(float)(img->w)/w;
		float sy=(float)(img->h)/h;
		int x,y, x0,y0,x1,y1,val_x1,val_y1;
		float xf,yf;
		mp_obj_t image = py_image(w, h, img->bpp, out);
		if(w >= w0 || h >= h0)
		{
			for(y=0;y<h;y++)
			{
				yf = (y+0.5)*sy-0.5;
				y0 = (int)yf;
				y1 = y0 + 1;
				val_y1 = y0<h0-1 ? y1 : y0;
				for(x=0;x<w;x++)
				{
					xf = (x+0.5)*sx-0.5;
					x0 = (int)xf;
					x1 = x0 + 1;
					val_x1 = x0<w0-1 ? x1 : x0;
					out[y*w+x] = (uint8_t)(in[y0*w0+x0]*(x1-xf)*(y1-yf)+\
								in[y0*w0+val_x1]*(xf-x0)*(y1-yf)+\
								in[val_y1*w0+x0]*(x1-xf)*(yf-y0)+\
								in[val_y1*w0+val_x1]*(xf-x0)*(yf-y0));
				}
			}
		}
		else
		{
			for(y=0;y<h;y++)
			{
				y0 = y*sy;
				y1 = (y+1)*sy;
				for(x=0;x<w;x++)
				{
					x0 = x*sy;
					x1 = (x+1)*sy;
					int sum,xx,yy;
					sum=0;
					for(yy=y0; yy<=y1; yy++)
					{
						for(xx=x0; xx<=x1; xx++)
						{
							sum+=in[yy*w0+xx];
						}
					}
					out[y*w+x]=sum/((y1-y0+1)*(x1-x0+1)); 	//avg to get better picture
				}
			}
		}
		return image;
		break;
	}
	case IMAGE_BPP_RGB565: 
	{
		uint16_t* out = xalloc(w*h*2);
		uint16_t* in = (uint16_t*)img->pixels;
		float sx=(float)(w0)/w;
		float sy=(float)(h0)/h;
		int x,y, x0,y0;
		uint16_t x1, x2, y1, y2;

		mp_obj_t image = py_image(w, h, img->bpp, out); //TODO: zepan says here maybe have bug ... ?
        if(w0 == w && h0 == h)
        {
            for(y=0, y0=0; y<h; y++,y0++)
            {
                for(x=0, x0=0; x<w; x++,x0++)
                {
                    out[y*w+x] = in[y0*w0+x0];
                }
            }
        }
        else if(w0 >= w || h0 > h)
        {
            for(y=0;y<h;y++)
            {
                y0 = y*sy;
                for(x=0;x<w;x++)
                {
                    x0 = x*sx;
                    out[y*w+x] = in[y0*w0+x0];
                }
            }
        }
        else
        {
            float x_src, y_src;
            float temp1, temp2;
            uint8_t temp_r, temp_g, temp_b;
            for(y=0;y<h;y++)
            {
                for(x=0;x<w;x++)
                {
                    x_src = (x + 0.5f) * sx - 0.5f;
                    y_src = (y + 0.5f) * sy - 0.5f;
                    x1 = (uint16_t)x_src;
                    x2 = x1 + 1;
                    y1 = (uint16_t)y_src;
                    y2 = y1 + 1;
                    if (x2 >= w0 || y2 >= h0)
                    {
                        out[ x + y * w] = in[x1 + y1 * w0];
                        continue;
                    }
                    // if( (x2 - x_src) > (x_src - x1) )
                    // {
                    //     x_src = x1;
                    // }
                    // else
                    // {
                    //     x_src = x2;
                    // }
                    // if( (y2 - y_src) > (y_src - y2) )
                    // {
                    //     y_src = y2;
                    // }
                    // else
                    // {
                    //     y_src = y1;
                    // }
                    // out[x + y * w] = in[(uint16_t)x_src + (uint16_t)y_src*w0];

                    temp1 = (x2 - x_src) * COLOR_RGB565_TO_R5(in[ x1 + y1 * w0]) + (x_src - x1) * COLOR_RGB565_TO_R5(in[x2 + y1 * w0]);
                    temp2 = (x2 - x_src) * COLOR_RGB565_TO_R5(in[ x1 + y2 * w0]) + (x_src - x1) * COLOR_RGB565_TO_R5(in[x2 + y2 * w0]);
                    temp_r = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);
                    temp1 = (x2 - x_src) * COLOR_RGB565_TO_G6(in[ x1 + y1 * w0]) + (x_src - x1) * COLOR_RGB565_TO_G6(in[x2 + y1 * w0]);
                    temp2 = (x2 - x_src) * COLOR_RGB565_TO_G6(in[ x1 + y2 * w0]) + (x_src - x1) * COLOR_RGB565_TO_G6(in[x2 + y2 * w0]);
                    temp_g = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);
                    temp1 = (x2 - x_src) * COLOR_RGB565_TO_B5(in[ x1 + y1 * w0]) + (x_src - x1) * COLOR_RGB565_TO_B5(in[x2 + y1 * w0]);
                    temp2 = (x2 - x_src) * COLOR_RGB565_TO_B5(in[ x1 + y2 * w0]) + (x_src - x1) * COLOR_RGB565_TO_B5(in[x2 + y2 * w0]);
                    temp_b = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);
                    out[x + y * w] = COLOR_R5_G6_B5_TO_RGB565(temp_r, temp_g, temp_b);
                }
            }
        }
		return image;
		break;
	}
	default:
		mp_printf(&mp_plat_print, "only support grayscale, 565 zoom out now\r\n");
		return mp_const_none;
		break;
	}
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(py_image_resize_obj, py_image_resize);

static mp_obj_t py_image_pix_to_ai(mp_obj_t img_obj)
{
	image_t* img = (image_t *) py_image_cobj(img_obj);
	uint16_t w = img->w;
	uint16_t h = img->h;
	switch (img->bpp)
	{
	case IMAGE_BPP_GRAYSCALE:
	{
		uint8_t* out = img->pix_ai;
		if(out == NULL) {
			out = xalloc(w*h);	//TODO: check 128bit align
			img->pix_ai = out;	
		}
		uint8_t* in = img->pixels;
		memcpy(out, in, w*h);
		return mp_const_none;		
		break;
	}
	case IMAGE_BPP_RGB565: 
	{	
		uint8_t* out = img->pix_ai;
		if(out == NULL) {
			out = xalloc( ( (w+63)&(~0x3F) ) * h*3); // 64B align for conv function	//TODO: check 128bit align
			img->pix_ai = out;	
		}
		uint8_t* r = out;
		uint8_t* g = out+w*h;
		uint8_t* b = out+w*h*2;
		uint16_t* in = (uint16_t*)img->pixels;
		uint32_t index;

		for(index=0; index < w*h; index++)
		{
			r[index] = COLOR_RGB565_TO_R8(in[index]);
			g[index] = COLOR_RGB565_TO_G8(in[index]);
			b[index] = COLOR_RGB565_TO_B8(in[index]);
		}
		return mp_const_none;
	}
	default:
		mp_printf(&mp_plat_print, "only support grayscale now\r\n");
		return mp_const_none;
		break;
	}
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_image_pix_to_ai_obj, py_image_pix_to_ai);


static mp_obj_t py_image_ai_to_pix(mp_obj_t img_obj)
{
	image_t* img = (image_t *) py_image_cobj(img_obj);
	uint16_t w = img->w;
	uint16_t h = img->h;
	
	if(img->pix_ai == NULL) return mp_const_none;
	switch (img->bpp)
	{
	case IMAGE_BPP_GRAYSCALE:
	{
		memcpy(img->pixels, img->pix_ai, w*h);
		return mp_const_none;
		break;
	}
	case IMAGE_BPP_RGB565: 
	{
		uint8_t* out = img->pix_ai; //TODO: check 128bit align
		uint8_t* r = out;
		uint8_t* g = out+w*h;
		uint8_t* b = out+w*h*2;
		uint16_t* in = img->pixels;
		uint32_t index;
		for(index=0; index < w*h; index++)
		{
			in[index] = COLOR_R8_G8_B8_TO_RGB565(r[index],g[index],b[index]);
		}
		return mp_const_none;
	}
	default:
		mp_printf(&mp_plat_print, "only support grayscale now\r\n");
		return mp_const_none;
	}
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_image_ai_to_pix_obj, py_image_ai_to_pix);



static mp_obj_t py_image_strech_char(mp_obj_t img_obj, mp_obj_t de_dark_obj)
{
	image_t* img = (image_t *) py_image_cobj(img_obj);
	int de_dark = mp_obj_get_int(de_dark_obj);
	uint16_t w = img->w;
	uint16_t h = img->h;
	
	switch (img->bpp)
	{
	case IMAGE_BPP_GRAYSCALE:
	{
		uint32_t index;
		uint16_t x,y,graymax;
		int sx2,sx,/* dx,*/ex,/* stdx,*/ r2;
		int gate,dat;
		uint8_t* in = img->pixels;
		sx=0; sx2=0; graymax=0;
		for(index=0;index<w*h;index++)
		{
			if(in[index]>graymax)graymax=in[index];
			sx+=in[index];
			sx2+=((int)in[index]*(int)in[index]);
		}
		//strech
		ex=sx/w/h;
		// dx=sx2/w/h-(sx*sx/w/w/h/h);
		// stdx=(int)sqrt(dx);
		gate=ex;//+stdx/8;
		//mp_printf(&mp_plat_print, "ex=%d,dx=%d,stdx=%d,gate=%d,graymax=%d\r\n",ex,dx,stdx,gate,graymax);
		for(index=0; index < w*h; index++)
		{
			x=index%w;
			y=index/w;
			dat=in[index];
			dat=(dat-gate)*255/(graymax-gate);
			dat=dat<0?0:(dat>255?255:dat);
			r2=(x-w/2)*(x-w/2)+(y-h/2)*(y-h/2);
			if(de_dark) dat=(int)(dat/(1.0+32.0*r2*r2/w/w/h/h)); 
			in[index]=dat;
		}
		return mp_const_none;
		break;
	}
	default:
		mp_printf(&mp_plat_print, "only support grayscale now\r\n");
		return mp_const_none;
		break;
	}
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(py_image_strech_char_obj, py_image_strech_char);


static float min(uint8_t* data, uint16_t x, uint16_t y)
{
	int i,j;
	float m=data[x+1];
	//mp_printf(&mp_plat_print, "#m=%d ",m);
	for(j=1;j<y-1;j++)
	{
		for(i=1;i<x-1;i++)
		{
			if(data[j*x+i]<m){m = data[j*x+i];}
		}
	}
	//mp_printf(&mp_plat_print, "\r\nmin m=%d ",m);
	return m;
}

static float max(uint8_t* data, uint16_t x, uint16_t y)
{
	int i,j;
	float m=data[x+1];
	//mp_printf(&mp_plat_print, "#m=%d ",m);
	for(j=1;j<y-1;j++)
	{
		for(i=1;i<x-1;i++)
		{
			if(data[j*x+i]>m){m = data[j*x+i];}
		}
	}
	//mp_printf(&mp_plat_print, "\r\nmax m=%d ",m);
	return m;
}

#define MAX_SCALE 5
static mp_obj_t py_image_stretch(mp_obj_t img_obj, mp_obj_t min_obj, mp_obj_t max_obj)
{
	image_t* img = (image_t *) py_image_cobj(img_obj);
	int to_min = mp_obj_get_int(min_obj);
	int to_max = mp_obj_get_int(max_obj);
	int w = img->w;
	int h = img->h;
	switch(img->bpp)
	{
	case IMAGE_BPP_GRAYSCALE:
	{
		uint8_t min0 = min(img->pixels, w,h);
		uint8_t max0 = max(img->pixels, w,h);
		int x,y;
		float scale;
		uint8_t* data = img->pixels;
		if(max0==min0) return mp_const_none;	//can't stretch
		scale = (to_max-to_min)/(max0-min0);
		//mp_printf(&mp_plat_print, "min0:%d,max0:%d,s:%f\r\n",min0,max0,scale);
		if(scale > MAX_SCALE) scale = MAX_SCALE;
		//TODO: optimize linear
		for(y=0;y<h;y++)
		{
			for(x=0;x<w;x++)
			{
				data[y*w+x] = (data[y*w+x]-min0)*scale+to_min;
			}
		}
		return mp_const_none;
		break;
	}
	default:
		mp_printf(&mp_plat_print, "only support grayscale now\r\n");
		return mp_const_none;
		break;
	}
	
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(py_image_stretch_obj, py_image_stretch);


static mp_obj_t py_image_cut(size_t n_args, const mp_obj_t *args)
{
	image_t* img = (image_t *) py_image_cobj(args[0]);

	int x0  =  mp_obj_get_int(args[1]);
	int y0  =  mp_obj_get_int(args[2]);	
	int w_cut  =  mp_obj_get_int(args[3]);
	int h_cut  =  mp_obj_get_int(args[4]);
	int w=img->w;
	int h=img->h;
	int y1 = y0+h_cut > h ? h : y0+h_cut;
	int x1 = x0+w_cut > w ? w : x0+w_cut;
	int x,y;
//printf("%d,%d,%d,%d\r\n",x0,y0,w_cut,h_cut);	
	
	switch(img->bpp)
	{
	case IMAGE_BPP_GRAYSCALE:
	{
		uint8_t* out = xalloc(w_cut*h_cut);
		uint8_t* in = img->pixels;
		mp_obj_t image = py_image(w_cut, h_cut, img->bpp, out); //TODO: here have bug
		for(y=y0;y<y1;y++)
		{
			for(x=x0;x<x1;x++)
			{
				out[(y-y0)*w_cut+x-x0] = in[y*w+x];;
			}
		}
		return image;	
		break;
	}
	case IMAGE_BPP_RGB565:
	{
		uint16_t* out = xalloc(w_cut*h_cut*2);
		uint16_t* in = (uint16_t*)img->pixels;
		mp_obj_t image = py_image(w_cut, h_cut, img->bpp, out); //TODO: here have bug
		for(y=y0;y<y1;y++)
		{
			for(x=x0;x<x1;x++)
			{
				out[(y-y0)*w_cut+x-x0] = in[y*w+x];
			}
		}
		return image;	
		break;
	}
	default:
		printf("pixel format not support!\r\n");
		return mp_const_none;
	}
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(py_image_cut_obj, 4, 5, py_image_cut);

#ifndef OMV_MINIMUM
//输入灰度图像，找出面积最大的连通域
#define MAX_DOMAIN_CNT 254

static void _get_hv_pixel(image_t* img, uint16_t* h0, uint16_t* h1, \
						uint16_t* h2, uint16_t* v0, uint16_t* v1, uint16_t* v2){
	uint8_t* data = img->pixels;
	uint16_t w = img->w; 
	uint16_t h = img->h;
	
	uint16_t minx=w;
	uint16_t miny=h;
	uint16_t maxx=0;
	uint16_t maxy=0;
	uint16_t _h1=0; uint16_t _h2=0; uint16_t _v1=0; uint16_t _v2=0;
	
	for (int y = 0, yy = img->h; y < yy; y++) {
		//uint32_t *row_ptr = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(img, y);
		uint32_t *img_row_ptr = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(img, y);
		for (int x = 0, xx = img->w; x < xx; x++) {
			if (IMAGE_GET_BINARY_PIXEL_FAST(img_row_ptr, x) ) {
				if(y<miny) miny=y; else if(y>maxy) maxy=y; 
				if(x<minx) minx=x; else if(x>maxx) maxx=x;
				if(y==0) _h1+=1;   else if(y==h-1) _h2+=1;
				if(x==0) _v1+=1;   else if(x==w-1) _v2+=1;
			} 
		}
	}
	
	*h0 = maxx-minx+1;
	*v0 = maxy-miny+1;
	*h1 = _h1; *h2 = _h2;
	*v1 = _v1; *v2 = _v2;
	
	return;
}

#define EDGE_GATE 0.7
static mp_obj_t py_image_find_domain(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *img = py_helper_arg_to_image_mutable(args[0]);
	if(img->bpp != IMAGE_BPP_GRAYSCALE) 
		mp_raise_msg(&mp_type_ValueError,"only support grayscale pic"); 
	
	float edge_gate = mp_obj_get_float(args[1]);
	// printf("domain edge gate=%.3f\r\n", edge_gate);

	uint16_t w = img->w;
	uint16_t h = img->h;
	uint8_t* pic = img->pixels;
	
	uint16_t domian_cnt[MAX_DOMAIN_CNT];
	uint8_t p_x[MAX_DOMAIN_CNT];
	uint8_t p_y[MAX_DOMAIN_CNT];
	for(int i=0; i < MAX_DOMAIN_CNT; i++) {
		domian_cnt[i] = 0;
	}

	uint8_t color_idx = 0;

	for(int y=0; y<h; y++) { 
		if(color_idx>=MAX_DOMAIN_CNT)
			break;
		for(int x=0; x<w; x++){
			int oft = x+y*w;
			if(pic[oft] > color_idx){
				domian_cnt[color_idx] = imlib_flood_fill(img, x, y,0, 0,color_idx+1, 0, 0, NULL);
				p_x[color_idx] = x;
				p_y[color_idx] = y;
				//printf("(%d, %d) -> %d, count=%d\r\n",x,y,color_idx+1,domian_cnt[color_idx]);
				color_idx += 1;
				
			}
		}
	}
	
	int maxcnt = 0;
	int maxidx = -1;
	int max2cnt = 0;
	int max2idx = -1;
	for(int i=0; i< MAX_DOMAIN_CNT; i++){
		if(domian_cnt[i]>maxcnt) {
			max2cnt= maxcnt;
			max2idx= maxidx;
			maxcnt = domian_cnt[i];
			maxidx = i;
		} else if(domian_cnt[i]>max2cnt) {
			max2cnt = domian_cnt[i];
			max2idx = i;
		}
	}
	//printf("max domain idx=%d, cnt=%d\r\n",maxidx, maxcnt);
	if(maxcnt == 0) { //没有连通域
		return mp_const_none;
	} else if (max2cnt<w+h) { //只有一块连通域, 或者第二块连通域面积过小
		image_t out;
		out.w = img->w;
		out.h = img->h;
		out.bpp = IMAGE_BPP_BINARY;
		uint8_t* data = xalloc(image_size(&out));
		mp_obj_t image = py_image(w, h, IMAGE_BPP_BINARY, data);
		imlib_flood_fill_int(&(((py_image_obj_t *)image)->_cobj), img, p_x[maxidx], p_y[maxidx], 0, 0, NULL, NULL);
		return image;
	} else { //有超过两块较大的连通域
		image_t out0, out1;
		out0.w = out1.w = img->w; 
		out0.h = out1.h = img->h;
		out0.bpp = out1.bpp = IMAGE_BPP_BINARY;
		out0.data = fb_alloc0(image_size(&out0));
		out1.data = fb_alloc0(image_size(&out1));
		imlib_flood_fill_int(&out0, img, p_x[maxidx], p_y[maxidx], 0, 0, NULL, NULL);
		imlib_flood_fill_int(&out1, img, p_x[max2idx], p_y[max2idx], 0, 0, NULL, NULL);
		uint16_t r0_h0,r0_h1,r0_h2; //水平方向像素范围，上边界像素数，下边界像素数
		uint16_t r0_v0,r0_v1,r0_v2; //
		uint16_t r1_h0,r1_h1,r1_h2; //水平方向像素范围，上边界像素数，下边界像素数
		uint16_t r1_v0,r1_v1,r1_v2; //
		_get_hv_pixel(&out0, &r0_h0,&r0_h1,&r0_h2, &r0_v0,&r0_v1,&r0_v2);
		_get_hv_pixel(&out1, &r1_h0,&r1_h1,&r1_h2, &r1_v0,&r1_v1,&r1_v2);
		uint16_t r0_h, r0_v, r1_h, r1_v;
		r0_h = r0_h1>r0_h2 ? r0_h1 : r0_h2;
		r1_h = r1_h1>r1_h2 ? r1_h1 : r1_h2;
		r0_v = r0_v1>r0_v2 ? r0_v1 : r0_v2;
		r1_v = r1_v1>r1_v2 ? r1_v1 : r1_v2;
		float r0_hrate, r0_vrate, r1_hrate, r1_vrate;
		r0_hrate = 1.0*r0_h/r0_h0;	r0_vrate = 1.0*r0_v/r0_v0;
		r1_hrate = 1.0*r1_h/r1_h0;	r1_vrate = 1.0*r1_v/r1_v0;
		// printf("r0cnt=%04d; h0=%03d, h1=%03d, h2=%03d; v0=%03d, v1=%03d, v2=%03d; hrate=%.3f, vrate=%.3f\r\n",
			//  maxcnt, r0_h0, r0_h1, r0_h2, r0_v0, r0_v1, r0_v2, r0_hrate, r0_vrate );
		// printf("r1cnt=%04d; h0=%03d, h1=%03d, h2=%03d; v0=%03d, v1=%03d, v2=%03d; hrate=%.3f, vrate=%.3f\r\n",
			//  max2cnt, r1_h0, r1_h1, r1_h2, r1_v0, r1_v1, r1_v2, r1_hrate, r1_vrate );
		int r_flag = -1;
		if(r0_hrate < edge_gate || r0_vrate < edge_gate) { //r0是线
			r_flag = 0; //使用r0
		} else if(r1_hrate < edge_gate || r1_vrate < edge_gate) { //r0不是线,r1是线
			r_flag = 1;
		} else { //两个都不是线，选面积大的
			r_flag = 0;
		}
		// printf("rflag=%d\r\n\r\n", r_flag);
		uint8_t* data = xalloc(image_size(&out0));
		mp_obj_t image = py_image(w, h, IMAGE_BPP_BINARY, data);
		memcpy(((py_image_obj_t *)image)->_cobj.data, r_flag?out1.data:out0.data, image_size(&out0));
		fb_free();
		fb_free();
		return image;
	}	
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_find_domain_obj, 2, py_image_find_domain);

#endif // OMV_MINIMUM

static const mp_rom_map_elem_t locals_dict_table[] = {
    /* Basic Methods */
    {MP_ROM_QSTR(MP_QSTR___del__),             MP_ROM_PTR(&py_image_del_obj)},
    {MP_ROM_QSTR(MP_QSTR_width),               MP_ROM_PTR(&py_image_width_obj)},
    {MP_ROM_QSTR(MP_QSTR_height),              MP_ROM_PTR(&py_image_height_obj)},
    {MP_ROM_QSTR(MP_QSTR_format),              MP_ROM_PTR(&py_image_format_obj)},
    {MP_ROM_QSTR(MP_QSTR_size),                MP_ROM_PTR(&py_image_size_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_pixel),           MP_ROM_PTR(&py_image_get_pixel_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_pixel),           MP_ROM_PTR(&py_image_set_pixel_obj)},
    {MP_ROM_QSTR(MP_QSTR_mean_pool),           MP_ROM_PTR(&py_image_mean_pool_obj)},
    {MP_ROM_QSTR(MP_QSTR_mean_pooled),         MP_ROM_PTR(&py_image_mean_pooled_obj)},
    {MP_ROM_QSTR(MP_QSTR_midpoint_pool),       MP_ROM_PTR(&py_image_midpoint_pool_obj)},
    {MP_ROM_QSTR(MP_QSTR_midpoint_pooled),     MP_ROM_PTR(&py_image_midpoint_pooled_obj)},
    {MP_ROM_QSTR(MP_QSTR_to_bitmap),           MP_ROM_PTR(&py_image_to_bitmap_obj)},
    {MP_ROM_QSTR(MP_QSTR_to_grayscale),        MP_ROM_PTR(&py_image_to_grayscale_obj)},
    {MP_ROM_QSTR(MP_QSTR_to_rgb565),           MP_ROM_PTR(&py_image_to_rgb565_obj)},
    {MP_ROM_QSTR(MP_QSTR_to_rainbow),          MP_ROM_PTR(&py_image_to_rainbow_obj)},
    {MP_ROM_QSTR(MP_QSTR_compress),            MP_ROM_PTR(&py_image_compress_obj)},
    {MP_ROM_QSTR(MP_QSTR_compress_for_ide),    MP_ROM_PTR(&py_image_compress_for_ide_obj)},
    {MP_ROM_QSTR(MP_QSTR_compressed),          MP_ROM_PTR(&py_image_compressed_obj)},
    {MP_ROM_QSTR(MP_QSTR_compressed_for_ide),  MP_ROM_PTR(&py_image_compressed_for_ide_obj)},
    {MP_ROM_QSTR(MP_QSTR_copy),                MP_ROM_PTR(&py_image_copy_obj)},
    {MP_ROM_QSTR(MP_QSTR_save),                MP_ROM_PTR(&py_image_save_obj)},
    {MP_ROM_QSTR(MP_QSTR_to_bytes),            MP_ROM_PTR(&py_image_to_bytes_obj)},
    /* Drawing Methods */
    {MP_ROM_QSTR(MP_QSTR_clear),               MP_ROM_PTR(&py_image_clear_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_line),           MP_ROM_PTR(&py_image_draw_line_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_rectangle),      MP_ROM_PTR(&py_image_draw_rectangle_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_circle),         MP_ROM_PTR(&py_image_draw_circle_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_ellipse),        MP_ROM_PTR(&py_image_draw_ellipse_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_font),         MP_ROM_PTR(&py_image_draw_font_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_string),         MP_ROM_PTR(&py_image_draw_string_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_cross),          MP_ROM_PTR(&py_image_draw_cross_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_arrow),          MP_ROM_PTR(&py_image_draw_arrow_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_image),          MP_ROM_PTR(&py_image_draw_image_obj)},
#ifdef IMLIB_ENABLE_FLOOD_FILL
    {MP_ROM_QSTR(MP_QSTR_flood_fill),          MP_ROM_PTR(&py_image_flood_fill_obj)},
#endif
#ifndef OMV_MINIMUM
    {MP_ROM_QSTR(MP_QSTR_draw_keypoints),      MP_ROM_PTR(&py_image_draw_keypoints_obj)},
#endif
    /* Binary Methods */
#ifdef IMLIB_ENABLE_BINARY_OPS
    {MP_ROM_QSTR(MP_QSTR_binary),              MP_ROM_PTR(&py_image_binary_obj)},
    {MP_ROM_QSTR(MP_QSTR_invert),              MP_ROM_PTR(&py_image_invert_obj)},
    {MP_ROM_QSTR(MP_QSTR_and),                 MP_ROM_PTR(&py_image_b_and_obj)},
    {MP_ROM_QSTR(MP_QSTR_b_and),               MP_ROM_PTR(&py_image_b_and_obj)},
    {MP_ROM_QSTR(MP_QSTR_nand),                MP_ROM_PTR(&py_image_b_nand_obj)},
    {MP_ROM_QSTR(MP_QSTR_b_nand),              MP_ROM_PTR(&py_image_b_nand_obj)},
    {MP_ROM_QSTR(MP_QSTR_or),                  MP_ROM_PTR(&py_image_b_or_obj)},
    {MP_ROM_QSTR(MP_QSTR_b_or),                MP_ROM_PTR(&py_image_b_or_obj)},
    {MP_ROM_QSTR(MP_QSTR_nor),                 MP_ROM_PTR(&py_image_b_nor_obj)},
    {MP_ROM_QSTR(MP_QSTR_b_nor),               MP_ROM_PTR(&py_image_b_nor_obj)},
    {MP_ROM_QSTR(MP_QSTR_xor),                 MP_ROM_PTR(&py_image_b_xor_obj)},
    {MP_ROM_QSTR(MP_QSTR_b_xor),               MP_ROM_PTR(&py_image_b_xor_obj)},
    {MP_ROM_QSTR(MP_QSTR_xnor),                MP_ROM_PTR(&py_image_b_xnor_obj)},
    {MP_ROM_QSTR(MP_QSTR_b_xnor),              MP_ROM_PTR(&py_image_b_xnor_obj)},
    {MP_ROM_QSTR(MP_QSTR_erode),               MP_ROM_PTR(&py_image_erode_obj)},
    {MP_ROM_QSTR(MP_QSTR_dilate),              MP_ROM_PTR(&py_image_dilate_obj)},
    {MP_ROM_QSTR(MP_QSTR_open),                MP_ROM_PTR(&py_image_open_obj)},
    {MP_ROM_QSTR(MP_QSTR_close),               MP_ROM_PTR(&py_image_close_obj)},
#endif
#ifdef IMLIB_ENABLE_MATH_OPS
    {MP_ROM_QSTR(MP_QSTR_top_hat),             MP_ROM_PTR(&py_image_top_hat_obj)},
    {MP_ROM_QSTR(MP_QSTR_black_hat),           MP_ROM_PTR(&py_image_black_hat_obj)},
    /* Math Methods */
    {MP_ROM_QSTR(MP_QSTR_gamma_corr),          MP_ROM_PTR(&py_image_gamma_corr_obj)},
    {MP_ROM_QSTR(MP_QSTR_negate),              MP_ROM_PTR(&py_image_negate_obj)},
    {MP_ROM_QSTR(MP_QSTR_assign),              MP_ROM_PTR(&py_image_replace_obj)},
    {MP_ROM_QSTR(MP_QSTR_replace),             MP_ROM_PTR(&py_image_replace_obj)},
    {MP_ROM_QSTR(MP_QSTR_set),                 MP_ROM_PTR(&py_image_replace_obj)},
    {MP_ROM_QSTR(MP_QSTR_add),                 MP_ROM_PTR(&py_image_add_obj)},
    {MP_ROM_QSTR(MP_QSTR_sub),                 MP_ROM_PTR(&py_image_sub_obj)},
    {MP_ROM_QSTR(MP_QSTR_mul),                 MP_ROM_PTR(&py_image_mul_obj)},
    {MP_ROM_QSTR(MP_QSTR_div),                 MP_ROM_PTR(&py_image_div_obj)},
    {MP_ROM_QSTR(MP_QSTR_min),                 MP_ROM_PTR(&py_image_min_obj)},
    {MP_ROM_QSTR(MP_QSTR_max),                 MP_ROM_PTR(&py_image_max_obj)},
    {MP_ROM_QSTR(MP_QSTR_difference),          MP_ROM_PTR(&py_image_difference_obj)},
    {MP_ROM_QSTR(MP_QSTR_blend),               MP_ROM_PTR(&py_image_blend_obj)},
#endif
    /* Filtering Methods */
#ifndef OMV_MINIMUM
    {MP_ROM_QSTR(MP_QSTR_histeq),              MP_ROM_PTR(&py_image_histeq_obj)},
#endif
#ifdef IMLIB_ENABLE_MEAN
    {MP_ROM_QSTR(MP_QSTR_mean),                MP_ROM_PTR(&py_image_mean_obj)},
#endif
#ifdef IMLIB_ENABLE_MEDIAN
    {MP_ROM_QSTR(MP_QSTR_median),              MP_ROM_PTR(&py_image_median_obj)},
#endif
#ifdef IMLIB_ENABLE_MODE
    {MP_ROM_QSTR(MP_QSTR_mode),                MP_ROM_PTR(&py_image_mode_obj)},
#endif
#ifdef IMLIB_ENABLE_MIDPOINT
    {MP_ROM_QSTR(MP_QSTR_midpoint),            MP_ROM_PTR(&py_image_midpoint_obj)},
#endif
#ifdef IMLIB_ENABLE_MORPH
    {MP_ROM_QSTR(MP_QSTR_morph),               MP_ROM_PTR(&py_image_morph_obj)},
#endif
#ifdef IMLIB_ENABLE_GAUSSIAN
    {MP_ROM_QSTR(MP_QSTR_blur),                MP_ROM_PTR(&py_image_gaussian_obj)},
    {MP_ROM_QSTR(MP_QSTR_gaussian),            MP_ROM_PTR(&py_image_gaussian_obj)},
    {MP_ROM_QSTR(MP_QSTR_gaussian_blur),       MP_ROM_PTR(&py_image_gaussian_obj)},
#endif
#ifdef IMLIB_ENABLE_LAPLACIAN
    {MP_ROM_QSTR(MP_QSTR_laplacian),           MP_ROM_PTR(&py_image_laplacian_obj)},
#endif
#ifdef IMLIB_ENABLE_BILATERAL
    {MP_ROM_QSTR(MP_QSTR_bilateral),           MP_ROM_PTR(&py_image_bilateral_obj)},
#endif
#ifdef IMLIB_ENABLE_CARTOON
    {MP_ROM_QSTR(MP_QSTR_cartoon),             MP_ROM_PTR(&py_image_cartoon_obj)},
#endif
    /* Shadow Removal Methods */
#ifdef IMLIB_ENABLE_REMOVE_SHADOWS
    {MP_ROM_QSTR(MP_QSTR_remove_shadows),      MP_ROM_PTR(&py_image_remove_shadows_obj)},
#endif
#ifdef IMLIB_ENABLE_CHROMINVAR
    {MP_ROM_QSTR(MP_QSTR_chrominvar),          MP_ROM_PTR(&py_image_chrominvar_obj)},
#endif
#ifdef IMLIB_ENABLE_ILLUMINVAR
    {MP_ROM_QSTR(MP_QSTR_illuminvar),          MP_ROM_PTR(&py_image_illuminvar_obj)},
#endif
    /* Geometric Methods */
#ifdef IMLIB_ENABLE_LINPOLAR
    {MP_ROM_QSTR(MP_QSTR_linpolar),            MP_ROM_PTR(&py_image_linpolar_obj)},
#endif
#ifdef IMLIB_ENABLE_LOGPOLAR
    {MP_ROM_QSTR(MP_QSTR_logpolar),            MP_ROM_PTR(&py_image_logpolar_obj)},
#endif
    {MP_ROM_QSTR(MP_QSTR_lens_corr),           MP_ROM_PTR(&py_image_lens_corr_obj)},
#ifdef IMLIB_ENABLE_ROTATION_CORR
    {MP_ROM_QSTR(MP_QSTR_rotation_corr),       MP_ROM_PTR(&py_image_rotation_corr_obj)},
#endif
    /* Get Methods */
#ifdef IMLIB_ENABLE_GET_SIMILARITY
    {MP_ROM_QSTR(MP_QSTR_get_similarity),      MP_ROM_PTR(&py_image_get_similarity_obj)},
#endif
#ifndef OMV_MINIMUM
    {MP_ROM_QSTR(MP_QSTR_get_hist),            MP_ROM_PTR(&py_image_get_histogram_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_histogram),       MP_ROM_PTR(&py_image_get_histogram_obj)},
    {MP_ROM_QSTR(MP_QSTR_histogram),           MP_ROM_PTR(&py_image_get_histogram_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_stats),           MP_ROM_PTR(&py_image_get_statistics_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_statistics),      MP_ROM_PTR(&py_image_get_statistics_obj)},
    {MP_ROM_QSTR(MP_QSTR_statistics),          MP_ROM_PTR(&py_image_get_statistics_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_regression),      MP_ROM_PTR(&py_image_get_regression_obj)},
    /* Find Methods */
    {MP_ROM_QSTR(MP_QSTR_find_blobs),          MP_ROM_PTR(&py_image_find_blobs_obj)},
#endif //OMV_MINIMUM
#ifdef IMLIB_ENABLE_FIND_LINES
    {MP_ROM_QSTR(MP_QSTR_find_lines),          MP_ROM_PTR(&py_image_find_lines_obj)},
#endif
#ifdef IMLIB_ENABLE_FIND_LINE_SEGMENTS
    {MP_ROM_QSTR(MP_QSTR_find_line_segments),  MP_ROM_PTR(&py_image_find_line_segments_obj)},
#endif
#ifdef IMLIB_ENABLE_FIND_CIRCLES
    {MP_ROM_QSTR(MP_QSTR_find_circles),        MP_ROM_PTR(&py_image_find_circles_obj)},
#endif
#ifdef IMLIB_ENABLE_FIND_RECTS
    {MP_ROM_QSTR(MP_QSTR_find_rects),          MP_ROM_PTR(&py_image_find_rects_obj)},
#endif
#ifdef IMLIB_ENABLE_QRCODES
    {MP_ROM_QSTR(MP_QSTR_find_qrcodes),        MP_ROM_PTR(&py_image_find_qrcodes_obj)},
#endif
#ifdef IMLIB_ENABLE_APRILTAGS
    {MP_ROM_QSTR(MP_QSTR_find_apriltags),      MP_ROM_PTR(&py_image_find_apriltags_obj)},
#endif
#ifdef IMLIB_ENABLE_DATAMATRICES
    {MP_ROM_QSTR(MP_QSTR_find_datamatrices),   MP_ROM_PTR(&py_image_find_datamatrices_obj)},
#endif
#ifdef IMLIB_ENABLE_BARCODES
    {MP_ROM_QSTR(MP_QSTR_find_barcodes),       MP_ROM_PTR(&py_image_find_barcodes_obj)},
#endif
#ifdef IMLIB_ENABLE_FIND_DISPLACEMENT
    {MP_ROM_QSTR(MP_QSTR_find_displacement),   MP_ROM_PTR(&py_image_find_displacement_obj)},
#endif
#ifndef OMV_MINIMUM
    {MP_ROM_QSTR(MP_QSTR_find_template),       MP_ROM_PTR(&py_image_find_template_obj)},
    {MP_ROM_QSTR(MP_QSTR_find_features),       MP_ROM_PTR(&py_image_find_features_obj)},
    {MP_ROM_QSTR(MP_QSTR_find_eye),            MP_ROM_PTR(&py_image_find_eye_obj)},
    {MP_ROM_QSTR(MP_QSTR_find_lbp),            MP_ROM_PTR(&py_image_find_lbp_obj)},
    {MP_ROM_QSTR(MP_QSTR_find_keypoints),      MP_ROM_PTR(&py_image_find_keypoints_obj)},
    {MP_ROM_QSTR(MP_QSTR_find_domain),    MP_ROM_PTR(&py_image_find_domain_obj)},
#endif //OMV_MINIMUM
#ifdef IMLIB_ENABLE_BINARY_OPS
    {MP_ROM_QSTR(MP_QSTR_find_edges),          MP_ROM_PTR(&py_image_find_edges_obj)},
#endif
#ifdef IMLIB_ENABLE_HOG
    {MP_ROM_QSTR(MP_QSTR_find_hog),            MP_ROM_PTR(&py_image_find_hog_obj)},
#endif
#ifdef IMLIB_ENABLE_SELECTIVE_SEARCH
    {MP_ROM_QSTR(MP_QSTR_selective_search),    MP_ROM_PTR(&py_image_selective_search_obj)},
#endif
	{MP_ROM_QSTR(MP_QSTR_dump_roi),    MP_ROM_PTR(&py_image_dump_roi_obj)},
	{MP_ROM_QSTR(MP_QSTR_conv3),    MP_ROM_PTR(&py_image_conv3_obj)},
	{MP_ROM_QSTR(MP_QSTR_resize),    MP_ROM_PTR(&py_image_resize_obj)},
	{MP_ROM_QSTR(MP_QSTR_ai_to_pix),    MP_ROM_PTR(&py_image_ai_to_pix_obj)},
	{MP_ROM_QSTR(MP_QSTR_pix_to_ai),    MP_ROM_PTR(&py_image_pix_to_ai_obj)},
	{MP_ROM_QSTR(MP_QSTR_strech_char),    MP_ROM_PTR(&py_image_strech_char_obj)},
	{MP_ROM_QSTR(MP_QSTR_stretch),    MP_ROM_PTR(&py_image_stretch_obj)},
	{MP_ROM_QSTR(MP_QSTR_cut),    MP_ROM_PTR(&py_image_cut_obj)},
};

STATIC MP_DEFINE_CONST_DICT(locals_dict, locals_dict_table);

static const mp_obj_type_t py_image_type = {
    { &mp_type_type },
    .name  = MP_QSTR_Image,
    .print = py_image_print,
    .buffer_p = { .get_buffer = py_image_get_buffer },
    .subscr = py_image_subscr,
    .locals_dict = (mp_obj_t) &locals_dict
};

// ImageWriter Object //
typedef struct py_imagewriter_obj {
    mp_obj_base_t base;
    mp_obj_t fp;
    uint32_t ms;
} py_imagewriter_obj_t;

static void py_imagewriter_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_imagewriter_obj_t *self = self_in;
    mp_printf(print, "{\"size\":%d}", vfs_internal_size(&self->fp));
}

mp_obj_t py_imagewriter_size(mp_obj_t self_in)
{
    return mp_obj_new_int(vfs_internal_size(&((py_imagewriter_obj_t *) self_in)->fp));
}

mp_obj_t py_imagewriter_add_frame(mp_obj_t self_in, mp_obj_t img_obj)
{
    // Don't use the file buffer here...

    mp_obj_t fp = &((py_imagewriter_obj_t *) self_in)->fp;
    PY_ASSERT_TYPE(img_obj, &py_image_type);
    image_t *arg_img = &((py_image_obj_t *) img_obj)->_cobj;

    uint32_t ms = systick_current_millis(); // Write out elapsed ms.
    write_long_raise(fp, ms - ((py_imagewriter_obj_t *) self_in)->ms);
    ((py_imagewriter_obj_t *) self_in)->ms = ms;

    write_long_raise(fp, arg_img->w);
    write_long_raise(fp, arg_img->h);
    write_long_raise(fp, arg_img->bpp);

    uint32_t size = image_size(arg_img);

    write_data_raise(fp, arg_img->data, size);
    if (size % 16) write_data(fp, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16 - (size % 16)); // Pad to multiple of 16 bytes.
    return self_in;
}

mp_obj_t py_imagewriter_close(mp_obj_t self_in)
{
    file_close(&((py_imagewriter_obj_t *) self_in)->fp);
    return self_in;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_imagewriter_size_obj, py_imagewriter_size);
STATIC MP_DEFINE_CONST_FUN_OBJ_2(py_imagewriter_add_frame_obj, py_imagewriter_add_frame);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_imagewriter_close_obj, py_imagewriter_close);

STATIC const mp_rom_map_elem_t py_imagewriter_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_size), MP_ROM_PTR(&py_imagewriter_size_obj) },
    { MP_ROM_QSTR(MP_QSTR_add_frame), MP_ROM_PTR(&py_imagewriter_add_frame_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&py_imagewriter_close_obj) }
};

STATIC MP_DEFINE_CONST_DICT(py_imagewriter_locals_dict, py_imagewriter_locals_dict_table);

static const mp_obj_type_t py_imagewriter_type = {
    { &mp_type_type },
    .name  = MP_QSTR_imagewriter,
    .print = py_imagewriter_print,
    .locals_dict = (mp_obj_t) &py_imagewriter_locals_dict
};

mp_obj_t py_image_imagewriter(mp_obj_t path)
{
    py_imagewriter_obj_t *obj = m_new_obj(py_imagewriter_obj_t);
    obj->base.type = &py_imagewriter_type;
    file_write_open_raise(&obj->fp, mp_obj_str_get_str(path));

    write_long(obj->fp, *((uint32_t *) "OMV ")); // OpenMV
    write_long(obj->fp, *((uint32_t *) "IMG ")); // Image
    write_long(obj->fp, *((uint32_t *) "STR ")); // Stream
    write_long(obj->fp, *((uint32_t *) "V1.0")); // v1.0

    obj->ms = systick_current_millis();
    return obj;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_image_imagewriter_obj, py_image_imagewriter);

// ImageReader Object //
typedef struct py_imagereader_obj {
    mp_obj_base_t base;
    mp_obj_t fp;
    uint32_t ms;
} py_imagereader_obj_t;

static void py_imagereader_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_imagereader_obj_t *self = self_in;
    mp_printf(print, "{\"size\":%d}", vfs_internal_size(&self->fp));
}

mp_obj_t py_imagereader_size(mp_obj_t self_in)
{
    return mp_obj_new_int(vfs_internal_size(&((py_imagereader_obj_t *) self_in)->fp));
}

mp_obj_t py_imagereader_next_frame(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    // Don't use the file buffer here...

    bool copy_to_fb = py_helper_keyword_int(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_copy_to_fb), true);
    if (copy_to_fb) fb_update_jpeg_buffer();

    mp_obj_t fp = &((py_imagereader_obj_t *) args[0])->fp;
    image_t image = {0};

    if (file_eof(fp)) {
        if (!py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_loop), true)) {
            return mp_const_none;
        }

        file_seek(fp, 16, 0); // skip past the header

        if (file_eof(fp)) { // empty file
            return mp_const_none;
        }
    }

    uint32_t ms_tmp;
    read_long(fp, &ms_tmp);

    uint32_t ms; // Wait for elapsed ms.
    for (ms = systick_current_millis();
         ((ms - ((py_imagewriter_obj_t *) args[0])->ms) < ms_tmp);
         ms = systick_current_millis()) {
        //__WFI();
    }

    ((py_imagewriter_obj_t *) args[0])->ms = ms;

    read_long(fp, (uint32_t *) &image.w);
    read_long(fp, (uint32_t *) &image.h);
    read_long(fp, (uint32_t *) &image.bpp);

    uint32_t size = image_size(&image);

    if (copy_to_fb) {
        PY_ASSERT_TRUE_MSG((size <= OMV_RAW_BUF_SIZE), "FB Overflow!");
#if CONFIG_MAIXPY_OMV_DOUBLE_BUFF
        image.data = MAIN_FB()->pixels[g_sensor_buff_index_out];
#else
        image.data = MAIN_FB()->pixels;
#endif
        MAIN_FB()->w = image.w;
        MAIN_FB()->h = image.h;
        MAIN_FB()->bpp = image.bpp;
    } else {
        image.data = xalloc(size);
    }

    char ignore[15];
    read_data(fp, image.data, size);
    if (size % 16) read_data(fp, ignore, 16 - (size % 16)); // Read in to multiple of 16 bytes.

    return py_image_from_struct(&image);
}

mp_obj_t py_imagereader_close(mp_obj_t self_in)
{
    file_close(&((py_imagereader_obj_t *) self_in)->fp);
    return self_in;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_imagereader_size_obj, py_imagereader_size);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_imagereader_next_frame_obj, 1, py_imagereader_next_frame);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_imagereader_close_obj, py_imagereader_close);

STATIC const mp_rom_map_elem_t py_imagereader_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_size), MP_ROM_PTR(&py_imagereader_size_obj) },
    { MP_ROM_QSTR(MP_QSTR_next_frame), MP_ROM_PTR(&py_imagereader_next_frame_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&py_imagereader_close_obj) }
};

STATIC MP_DEFINE_CONST_DICT(py_imagereader_locals_dict, py_imagereader_locals_dict_table);

static const mp_obj_type_t py_imagereader_type = {
    { &mp_type_type },
    .name  = MP_QSTR_imagereader,
    .print = py_imagereader_print,
    .locals_dict = (mp_obj_t) &py_imagereader_locals_dict
};

mp_obj_t py_image_imagereader(mp_obj_t path)
{
    py_imagereader_obj_t *obj = m_new_obj(py_imagereader_obj_t);
    obj->base.type = &py_imagereader_type;
    file_read_open_raise(&obj->fp, mp_obj_str_get_str(path));

    read_long_expect(&obj->fp, *((uint32_t *) "OMV ")); // OpenMV
    read_long_expect(&obj->fp, *((uint32_t *) "IMG ")); // Image
    read_long_expect(&obj->fp, *((uint32_t *) "STR ")); // Stream
    read_long_expect(&obj->fp, *((uint32_t *) "V1.0")); // v1.0

    obj->ms = systick_current_millis();
    return obj;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_image_imagereader_obj, py_image_imagereader);

mp_obj_t py_image(int w, int h, int bpp, void *pixels)
{
    py_image_obj_t *o = m_new_obj(py_image_obj_t);
    o->base.type = &py_image_type;
    o->_cobj.w = w;
    o->_cobj.h = h;
    o->_cobj.bpp = bpp;
    o->_cobj.pixels = pixels;
    o->_cobj.pix_ai = NULL;
    return o;
}

mp_obj_t py_image_from_struct(image_t *img)
{
    py_image_obj_t *o = m_new_obj(py_image_obj_t);
    o->base.type = &py_image_type;
    o->_cobj = *img;
    return o;
}

mp_obj_t py_image_rgb_to_lab(mp_obj_t tuple)
{
    mp_obj_t *rgb;
    mp_obj_get_array_fixed_n(tuple, 3, &rgb);

    simple_color_t rgb_color, lab_color;
    rgb_color.red = mp_obj_get_int(rgb[0]);
    rgb_color.green = mp_obj_get_int(rgb[1]);
    rgb_color.blue = mp_obj_get_int(rgb[2]);
    imlib_rgb_to_lab(&rgb_color, &lab_color);

    return mp_obj_new_tuple(3, (mp_obj_t[3])
            {mp_obj_new_int(lab_color.L),
             mp_obj_new_int(lab_color.A),
             mp_obj_new_int(lab_color.B)});
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_image_rgb_to_lab_obj, py_image_rgb_to_lab);

mp_obj_t py_image_lab_to_rgb(mp_obj_t tuple)
{
    mp_obj_t *lab;
    mp_obj_get_array_fixed_n(tuple, 3, &lab);

    simple_color_t lab_color, rgb_color;
    lab_color.L = mp_obj_get_int(lab[0]);
    lab_color.A = mp_obj_get_int(lab[1]);
    lab_color.B = mp_obj_get_int(lab[2]);
    imlib_lab_to_rgb(&lab_color, &rgb_color);

    return mp_obj_new_tuple(3, (mp_obj_t[3])
            {mp_obj_new_int(rgb_color.red),
             mp_obj_new_int(rgb_color.green),
             mp_obj_new_int(rgb_color.blue)});
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_image_lab_to_rgb_obj, py_image_lab_to_rgb);

mp_obj_t py_image_rgb_to_grayscale(mp_obj_t tuple)
{
    mp_obj_t *rgb;
    mp_obj_get_array_fixed_n(tuple, 3, &rgb);

    simple_color_t rgb_color, grayscale_color;
    rgb_color.red = mp_obj_get_int(rgb[0]);
    rgb_color.green = mp_obj_get_int(rgb[1]);
    rgb_color.blue = mp_obj_get_int(rgb[2]);
    imlib_rgb_to_grayscale(&rgb_color, &grayscale_color);

    return mp_obj_new_int(grayscale_color.G);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_image_rgb_to_grayscale_obj, py_image_rgb_to_grayscale);

mp_obj_t py_image_grayscale_to_rgb(mp_obj_t not_tuple)
{
    simple_color_t grayscale_color, rgb_color;
    grayscale_color.G = mp_obj_get_int(not_tuple);
    imlib_grayscale_to_rgb(&grayscale_color, &rgb_color);

    return mp_obj_new_tuple(3, (mp_obj_t[3])
            {mp_obj_new_int(rgb_color.red),
             mp_obj_new_int(rgb_color.green),
             mp_obj_new_int(rgb_color.blue)});
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_image_grayscale_to_rgb_obj, py_image_grayscale_to_rgb);

// extern const uint8_t img[];
mp_obj_t py_image_load_image(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    point_t xy = {0, 0};
    bool copy_to_fb = py_helper_keyword_int(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_copy_to_fb), 0) > 0 ? true : false;
    bool from_bytes = py_helper_keyword_int(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_from_bytes), 0) > 0 ? true : false;
	py_helper_keyword_xy(NULL, n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_size), &xy);
    if (copy_to_fb) fb_update_jpeg_buffer();

    image_t image = {0};
    memset(&image, 0, sizeof(image_t));
    
    if((xy.x >0 && n_args >= 2) || ((xy.x <= 0 && n_args >= 1)))
    {
        if(copy_to_fb)
        {
#if CONFIG_MAIXPY_OMV_DOUBLE_BUFF
            image.data = MAIN_FB()->pixels[g_sensor_buff_index_out];
#else
            image.data = MAIN_FB()->pixels;
#endif
        }
        if( mp_obj_is_str_or_bytes(args[0]) )
        {
            if(from_bytes)
            {
                GET_STR_LEN(args[0], bytes_len);
                imlib_load_image(&image, NULL, NULL, (uint8_t*)mp_obj_str_get_str(args[0]), (uint32_t)bytes_len); //TODO: param buf should be const
            }
            else
            {
                imlib_load_image(&image, mp_obj_str_get_str(args[0]), NULL, NULL, 0);
            }
        }
        else
        {
            imlib_load_image(&image, NULL, args[0], NULL, 0);
        }
    }
    else
    {
        image.w = CONFIG_LCD_DEFAULT_WIDTH;
        image.h = CONFIG_LCD_DEFAULT_HEIGHT;
		if(xy.x >0 && xy.y >0)
		{
			image.w = xy.x;
			image.h = xy.y;
		}
        image.bpp = IMAGE_BPP_RGB565;
        if(copy_to_fb)
        {
            MAIN_FB()->w = image.w;
            MAIN_FB()->h = image.h;
            MAIN_FB()->bpp = image.bpp;
#if CONFIG_MAIXPY_OMV_DOUBLE_BUFF
            image.data = MAIN_FB()->pixels[g_sensor_buff_index_out];
#else
            image.data = MAIN_FB()->pixels;
#endif
        }
        else
        {
            image.data = xalloc(image.w*image.h*2);
        }
    }
    return py_image_from_struct(&image);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_load_image_obj, 0, py_image_load_image);

mp_obj_t py_image_font_load(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    mp_int_t index = mp_obj_get_int(args[0]);
    mp_int_t width = mp_obj_get_int(args[1]);
    mp_int_t high = mp_obj_get_int(args[2]);
    
    if (index == UTF8)
    {
        if(mp_obj_get_type(args[3]) == &mp_type_int)
        {
            mp_int_t font_addr = mp_obj_get_int(args[3]);
            if(font_addr <= 0)
            {
                mp_raise_ValueError("[MAIXPY]image: font_addr must > 0 ");
                return mp_const_false;
            }
            else
            {
                font_load(index, width, high, ArrayIn, (void *)font_addr);
            }
        }
        else if(mp_obj_get_type(args[3]) == &mp_type_str)
        {
            const char *path = mp_obj_str_get_str(args[3]);
            if( NULL != strstr(path,".Dzk") )
            {
                mp_obj_t fp;
                FRESULT res;
                
                if ((res = file_read_open_raise(&fp, path)) == FR_OK) {
                    font_load(index, width, high, FileIn, fp);
                }
                // File open or write error
                if (res != FR_OK) {
                    nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, ffs_strerror(res)));
                }
            }
            else
            {
                mp_raise_ValueError("[MAIXPY]image: font format don't match, only supply .Dzk ");
                return mp_const_false;
            }
        }
    }
    else
    {
        // Use default font
        font_load(index, width, high, BuildIn, NULL);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_font_load_obj, 4, py_image_font_load);

mp_obj_t py_image_font_free(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    font_free();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_font_free_obj, 0, py_image_font_free);

#ifndef OMV_MINIMUM
mp_obj_t py_image_load_cascade(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    cascade_t cascade;
    const char *path = mp_obj_str_get_str(args[0]);

    // Load cascade from file or flash
    int res = imlib_load_cascade(&cascade, path);
    if (res != FR_OK) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, ffs_strerror(res)));
    }

    // Read the number of stages
    int stages = py_helper_keyword_int(n_args, args, 1, kw_args, MP_OBJ_NEW_QSTR(qstr_from_str("stages")), cascade.n_stages);
    // Check the number of stages
    if (stages > 0 && stages < cascade.n_stages) {
        cascade.n_stages = stages;
    }

    // Return micropython cascade object
    py_cascade_obj_t *o = m_new_obj(py_cascade_obj_t);
    o->base.type = &py_cascade_type;
    o->_cobj = cascade;
    return o;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_load_cascade_obj, 1, py_image_load_cascade);

mp_obj_t py_image_load_descriptor(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    mp_obj_t fp;
    mp_uint_t bytes;
    FRESULT res;

    uint32_t desc_type;
    mp_obj_t desc = mp_const_none;
    const char *path = mp_obj_str_get_str(args[0]);

    if ((res = file_read_open(&fp, path)) == FR_OK) {
        // Read descriptor type
        res = file_read(&fp, &desc_type, sizeof(desc_type), &bytes);
        if (res != FR_OK || bytes  != sizeof(desc_type)) {
            goto error;
        }

        // Load descriptor
        switch (desc_type) {
            case DESC_LBP: {
                py_lbp_obj_t *lbp = m_new_obj(py_lbp_obj_t);
                lbp->base.type = &py_lbp_type;

                res = imlib_lbp_desc_load(&fp, &lbp->hist);
                if (res == FR_OK) {
                    desc = lbp;
                }
                break;
            }

            case DESC_ORB: {
                array_t *kpts = NULL;
                array_alloc(&kpts, xfree);

                res = orb_load_descriptor(&fp, kpts);
                if (res == FR_OK) {
                    // Return keypoints MP object
                    py_kp_obj_t *kp_obj = m_new_obj(py_kp_obj_t);
                    kp_obj->base.type = &py_kp_type;
                    kp_obj->kpts = kpts;
                    kp_obj->threshold = 10;
                    kp_obj->normalized = false;
                    desc = kp_obj;
                }
                break;
            }
        }

        file_close(&fp);
    }

error:
    // File open or write error
    if (res != FR_OK) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, ffs_strerror(res)));
    }

    // If no file error and descriptor is still none, then it's not supported.
    if (desc == mp_const_none) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Descriptor type is not supported"));
    }
    return desc;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_load_descriptor_obj, 1, py_image_load_descriptor);

mp_obj_t py_image_save_descriptor(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    mp_obj_t fp;
    mp_uint_t bytes;
    FRESULT res;

    uint32_t desc_type;
    const char *path = mp_obj_str_get_str(args[1]);

    if ((res = file_write_open(&fp, path)) == FR_OK) {
        // Find descriptor type
        mp_obj_type_t *desc_obj_type = mp_obj_get_type(args[0]);
        if (desc_obj_type ==  &py_lbp_type) {
            desc_type = DESC_LBP;
        } else if (desc_obj_type ==  &py_kp_type) {
            desc_type = DESC_ORB;
        }

        // Write descriptor type
        res = file_write(&fp, &desc_type, sizeof(desc_type), &bytes);
        if (res != FR_OK || bytes  !=  sizeof(desc_type)) {
            goto error;
        }

        // Write descriptor
        switch (desc_type) {
            case DESC_LBP: {
                py_lbp_obj_t *lbp = ((py_lbp_obj_t*)args[0]);
                res = imlib_lbp_desc_save(&fp, lbp->hist);
                break;
            }

            case DESC_ORB: {
                py_kp_obj_t *kpts = ((py_kp_obj_t*)args[0]);
                res = orb_save_descriptor(&fp, kpts->kpts);
                break;
            }
        }
        // ignore unsupported descriptors when saving
        file_close(&fp);
    }

error:
    // File open or read error
    if (res != FR_OK) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, ffs_strerror(res)));
    }
    return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_save_descriptor_obj, 2, py_image_save_descriptor);

static mp_obj_t py_image_match_descriptor(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    mp_obj_t match_obj = mp_const_none;
    mp_obj_type_t *desc1_type = mp_obj_get_type(args[0]);
    mp_obj_type_t *desc2_type = mp_obj_get_type(args[1]);
    PY_ASSERT_TRUE_MSG((desc1_type == desc2_type), "Descriptors have different types!");

    if (desc1_type ==  &py_lbp_type) {
        py_lbp_obj_t *lbp1 = ((py_lbp_obj_t*)args[0]);
        py_lbp_obj_t *lbp2 = ((py_lbp_obj_t*)args[1]);

        // Sanity checks
        PY_ASSERT_TYPE(lbp1, &py_lbp_type);
        PY_ASSERT_TYPE(lbp2, &py_lbp_type);

        // Match descriptors
        match_obj = mp_obj_new_int(imlib_lbp_desc_distance(lbp1->hist, lbp2->hist));
    } else if (desc1_type == &py_kp_type) {
        py_kp_obj_t *kpts1 = ((py_kp_obj_t*)args[0]);
        py_kp_obj_t *kpts2 = ((py_kp_obj_t*)args[1]);
        int threshold = py_helper_keyword_int(n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold), 85);
        int filter_outliers = py_helper_keyword_int(n_args, args, 3, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_filter_outliers), false);

        // Sanity checks
        PY_ASSERT_TYPE(kpts1, &py_kp_type);
        PY_ASSERT_TYPE(kpts2, &py_kp_type);
        PY_ASSERT_TRUE_MSG((threshold >=0 && threshold <= 100), "Expected threshold between 0 and 100");

        int theta = 0;          // Estimated angle of rotation
        int count = 0;          // Number of matches
        point_t c = {0};        // Centroid
        rectangle_t r = {0};    // Bounding rectangle
        // List of matching keypoints indices
        mp_obj_t match_list = mp_obj_new_list(0, NULL);

        if (array_length(kpts1->kpts) && array_length(kpts1->kpts)) {
            int *match = fb_alloc(array_length(kpts1->kpts) * sizeof(int) * 2);

            // Match the two keypoint sets
            count = orb_match_keypoints(kpts1->kpts, kpts2->kpts, match, threshold, &r, &c, &theta);

            // Add matching keypoints to Python list.
            for (int i=0; i<count*2; i+=2) {
                mp_obj_t index_obj[2] = {
                    mp_obj_new_int(match[i+0]),
                    mp_obj_new_int(match[i+1]),
                };
                mp_obj_list_append(match_list, mp_obj_new_tuple(2, index_obj));
            }

            // Free match list
            fb_free();

            if (filter_outliers == true) {
                count = orb_filter_keypoints(kpts2->kpts, &r, &c);
            }
        }

        py_kptmatch_obj_t *o = m_new_obj(py_kptmatch_obj_t);
        o->base.type = &py_kptmatch_type;
        o->cx    = mp_obj_new_int(c.x);
        o->cy    = mp_obj_new_int(c.y);
        o->x     = mp_obj_new_int(r.x);
        o->y     = mp_obj_new_int(r.y);
        o->w     = mp_obj_new_int(r.w);
        o->h     = mp_obj_new_int(r.h);
        o->count = mp_obj_new_int(count);
        o->theta = mp_obj_new_int(theta);
        o->match = match_list;
        match_obj = o;
    } else {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Descriptor type is not supported"));
    }

    return match_obj;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_image_match_descriptor_obj, 2, py_image_match_descriptor);

int py_image_descriptor_from_roi(image_t *img, const char *path, rectangle_t *roi)
{
    mp_obj_t fp;
    FRESULT res = FR_OK;

    mp_printf(&mp_plat_print, "Save Descriptor: ROI(%d %d %d %d)\n", roi->x, roi->y, roi->w, roi->h);
    array_t *kpts = orb_find_keypoints(img, false, 20, 1.5f, 100, CORNER_AGAST, roi);
    mp_printf(&mp_plat_print, "Save Descriptor: KPTS(%d)\n", array_length(kpts));

    if (array_length(kpts)) {
        if ((res = file_write_open(&fp, path)) == FR_OK) {
            res = orb_save_descriptor(&fp, kpts);
            file_close(&fp);
        }
        // File open/write error
        if (res != FR_OK) {
            nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, ffs_strerror(res)));
        }
    }
    return 0;
}

#endif //OMV_MINIMUM

//input: SRC,DST point list
//output: transform matrix
mp_obj_t py_image_GetAffineTransform(mp_obj_t src_list, mp_obj_t dst_list)
{
	uint16_t src_pos[10][2];	//max 10 points
	uint16_t dst_pos[10][2];	//max 10 points
	float T[3][3];	//only support 2D affine
	if(MP_OBJ_IS_TYPE(src_list, &mp_type_list) && \
	MP_OBJ_IS_TYPE(dst_list, &mp_type_list) ){
		mp_uint_t src_l_len, dst_l_len;
		mp_obj_t *src_l;
		mp_obj_t *dst_l;
		mp_obj_get_array(src_list, &src_l_len, &src_l);
		mp_obj_get_array(dst_list, &dst_l_len, &dst_l);	
		PY_ASSERT_TRUE_MSG(src_l_len == dst_l_len, "src len must equal with dst len");
		PY_ASSERT_TRUE_MSG(src_l_len<=10, "must<=10 points");
		// get src and dst points
		for(int i=0; i<src_l_len; i++) {
			mp_obj_t *tuple;
            mp_obj_get_array_fixed_n(src_l[i], 2, &tuple);
            src_pos[i][0] = mp_obj_get_int(tuple[0]);
            src_pos[i][1] = mp_obj_get_int(tuple[1]);
            mp_obj_get_array_fixed_n(dst_l[i], 2, &tuple);
            dst_pos[i][0] = mp_obj_get_int(tuple[0]);
            dst_pos[i][1] = mp_obj_get_int(tuple[1]);
			//printf("point %d: (%d,%d)->(%d,%d)\r\n", i, src_pos[i][0],src_pos[i][1],dst_pos[i][0],dst_pos[i][1]);
		}
		imlib_affine_getTansform(src_pos, dst_pos, src_l_len, T);
		/*printf("%.3f,%.3f,%.3f\r\n",T[0][0],T[0][1],T[0][2]);
		printf("%.3f,%.3f,%.3f\r\n",T[1][0],T[1][1],T[1][2]);
		printf("%.3f,%.3f,%.3f\r\n",T[2][0],T[2][1],T[2][2]);*/
        // return: [[1,2,3], [1,2,3], [1,2,3]]
		return mp_obj_new_tuple(9, (mp_obj_t [9]) 
			{mp_obj_new_float(T[0][0]),
			mp_obj_new_float(T[0][1]),
			mp_obj_new_float(T[0][2]),
			mp_obj_new_float(T[1][0]),
			mp_obj_new_float(T[1][1]),
			mp_obj_new_float(T[1][2]),
			mp_obj_new_float(T[2][0]),
			mp_obj_new_float(T[2][1]),
			mp_obj_new_float(T[2][2])});
	}
	else{
		nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Need input [[x0,y0],[x1,y1],...]\r\n"));
	}
	return MP_OBJ_NULL;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(py_image_GetAffineTransform_obj, py_image_GetAffineTransform);


//img_src + T -> dat_img
mp_obj_t py_image_warpAffine_ai(mp_obj_t src_img_obj , mp_obj_t dst_img_obj , mp_obj_t transform_obj)
{
	float T[3*3];
	mp_obj_t *tuple;
	mp_obj_get_array_fixed_n(transform_obj, 9, &tuple);
	for(int i=0; i<9; i++) {
		T[i] = mp_obj_get_float(tuple[i]);
	}
	
	image_t* src_img = (image_t *) py_image_cobj(src_img_obj);
	image_t* dst_img = (image_t *) py_image_cobj(dst_img_obj);
	int ret = imlib_affine_ai(src_img, dst_img, T);
    if(ret != 0)
    {
        if(ret == -1)
        {
            mp_raise_msg(&mp_type_ValueError, "src_img->bpp != dst_img->bpp");
        }
        else if(ret == -2)
        {
            mp_raise_msg(&mp_type_ValueError, "only support ai image affine");
        }
        else if(ret == -3)
        {
            mp_raise_msg(&mp_type_ValueError,"only support grayscale or RGB565 pic");
            
        }
        mp_raise_OSError(ret);
    }
	
	return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_3(py_image_warpAffine_ai_obj, py_image_warpAffine_ai);



static const mp_rom_map_elem_t globals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__),            MP_OBJ_NEW_QSTR(MP_QSTR_image)},
#ifndef OMV_MINIMUM
    {MP_ROM_QSTR(MP_QSTR_SEARCH_EX),           MP_ROM_INT(SEARCH_EX)},
    {MP_ROM_QSTR(MP_QSTR_SEARCH_DS),           MP_ROM_INT(SEARCH_DS)},
    {MP_ROM_QSTR(MP_QSTR_EDGE_CANNY),          MP_ROM_INT(EDGE_CANNY)},
    {MP_ROM_QSTR(MP_QSTR_EDGE_SIMPLE),         MP_ROM_INT(EDGE_SIMPLE)},
    {MP_ROM_QSTR(MP_QSTR_CORNER_FAST),         MP_ROM_INT(CORNER_FAST)},
    {MP_ROM_QSTR(MP_QSTR_CORNER_AGAST),        MP_ROM_INT(CORNER_AGAST)},
#endif
#ifdef IMLIB_ENABLE_APRILTAGS
    {MP_ROM_QSTR(MP_QSTR_TAG16H5),             MP_ROM_INT(TAG16H5)},
    {MP_ROM_QSTR(MP_QSTR_TAG25H7),             MP_ROM_INT(TAG25H7)},
    {MP_ROM_QSTR(MP_QSTR_TAG25H9),             MP_ROM_INT(TAG25H9)},
    {MP_ROM_QSTR(MP_QSTR_TAG36H10),            MP_ROM_INT(TAG36H10)},
    {MP_ROM_QSTR(MP_QSTR_TAG36H11),            MP_ROM_INT(TAG36H11)},
    {MP_ROM_QSTR(MP_QSTR_ARTOOLKIT),           MP_ROM_INT(ARTOOLKIT)},
#endif
#ifdef IMLIB_ENABLE_BARCODES
    {MP_ROM_QSTR(MP_QSTR_EAN2),                MP_ROM_INT(BARCODE_EAN2)},
    {MP_ROM_QSTR(MP_QSTR_EAN5),                MP_ROM_INT(BARCODE_EAN5)},
    {MP_ROM_QSTR(MP_QSTR_EAN8),                MP_ROM_INT(BARCODE_EAN8)},
    {MP_ROM_QSTR(MP_QSTR_UPCE),                MP_ROM_INT(BARCODE_UPCE)},
    {MP_ROM_QSTR(MP_QSTR_ISBN10),              MP_ROM_INT(BARCODE_ISBN10)},
    {MP_ROM_QSTR(MP_QSTR_UPCA),                MP_ROM_INT(BARCODE_UPCA)},
    {MP_ROM_QSTR(MP_QSTR_EAN13),               MP_ROM_INT(BARCODE_EAN13)},
    {MP_ROM_QSTR(MP_QSTR_ISBN13),              MP_ROM_INT(BARCODE_ISBN13)},
    {MP_ROM_QSTR(MP_QSTR_I25),                 MP_ROM_INT(BARCODE_I25)},
    {MP_ROM_QSTR(MP_QSTR_DATABAR),             MP_ROM_INT(BARCODE_DATABAR)},
    {MP_ROM_QSTR(MP_QSTR_DATABAR_EXP),         MP_ROM_INT(BARCODE_DATABAR_EXP)},
    {MP_ROM_QSTR(MP_QSTR_CODABAR),             MP_ROM_INT(BARCODE_CODABAR)},
    {MP_ROM_QSTR(MP_QSTR_CODE39),              MP_ROM_INT(BARCODE_CODE39)},
    {MP_ROM_QSTR(MP_QSTR_PDF417),              MP_ROM_INT(BARCODE_PDF417)},
    {MP_ROM_QSTR(MP_QSTR_CODE93),              MP_ROM_INT(BARCODE_CODE93)},
    {MP_ROM_QSTR(MP_QSTR_CODE128),             MP_ROM_INT(BARCODE_CODE128)},
#endif
    {MP_ROM_QSTR(MP_QSTR_ASCII),               MP_ROM_INT(ASCII)},
    {MP_ROM_QSTR(MP_QSTR_UTF8),                MP_ROM_INT(UTF8)},
    {MP_ROM_QSTR(MP_QSTR_font_load),           MP_ROM_PTR(&py_image_font_load_obj)},
    {MP_ROM_QSTR(MP_QSTR_font_free),           MP_ROM_PTR(&py_image_font_free_obj)},
    {MP_ROM_QSTR(MP_QSTR_ImageWriter),         MP_ROM_PTR(&py_image_imagewriter_obj)},
    {MP_ROM_QSTR(MP_QSTR_Image),               MP_ROM_PTR(&py_image_load_image_obj)},
    {MP_ROM_QSTR(MP_QSTR_ImageWriter),         MP_ROM_PTR(&py_image_imagewriter_obj)},
    {MP_ROM_QSTR(MP_QSTR_ImageReader),         MP_ROM_PTR(&py_image_imagereader_obj)},
    {MP_ROM_QSTR(MP_QSTR_rgb_to_lab),          MP_ROM_PTR(&py_image_rgb_to_lab_obj)},
    {MP_ROM_QSTR(MP_QSTR_lab_to_rgb),          MP_ROM_PTR(&py_image_lab_to_rgb_obj)},
    {MP_ROM_QSTR(MP_QSTR_rgb_to_grayscale),    MP_ROM_PTR(&py_image_rgb_to_grayscale_obj)},
    {MP_ROM_QSTR(MP_QSTR_grayscale_to_rgb),    MP_ROM_PTR(&py_image_grayscale_to_rgb_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_affine_transform), MP_ROM_PTR(&py_image_GetAffineTransform_obj)},
	{MP_ROM_QSTR(MP_QSTR_warp_affine_ai),              MP_ROM_PTR(&py_image_warpAffine_ai_obj)},
#ifndef OMV_MINIMUM
    {MP_ROM_QSTR(MP_QSTR_HaarCascade),         MP_ROM_PTR(&py_image_load_cascade_obj)},
    {MP_ROM_QSTR(MP_QSTR_load_descriptor),     MP_ROM_PTR(&py_image_load_descriptor_obj)},
    {MP_ROM_QSTR(MP_QSTR_save_descriptor),     MP_ROM_PTR(&py_image_save_descriptor_obj)},
    {MP_ROM_QSTR(MP_QSTR_match_descriptor),    MP_ROM_PTR(&py_image_match_descriptor_obj)},
#endif
};

STATIC MP_DEFINE_CONST_DICT(globals_dict, globals_dict_table);

const mp_obj_module_t image_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_t) &globals_dict
};

