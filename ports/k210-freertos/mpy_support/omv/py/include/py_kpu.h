#ifndef __PY_KPU_H
#define __PY_KPU_H

#include <stdint.h>
#include "kpu.h"
#include "py_helper.h"

typedef struct
{
    uint32_t obj_number;
    struct
    {
        uint32_t x1;
        uint32_t y1;
        uint32_t x2;
        uint32_t y2;
        uint32_t classid;
        float prob;
    } obj[10];
} __attribute__((aligned(8))) obj_info_t;

typedef struct
{
    float threshold;
    float nms_value;
    uint32_t coords;
    uint32_t anchor_number;
    float *anchor;
    uint32_t image_width;
    uint32_t image_height;
    uint32_t classes;
    uint32_t net_width;
    uint32_t net_height;
    uint32_t layer_width;
    uint32_t layer_height;
    uint32_t boxes_number;
    uint32_t output_number;
    float scale;
    float bias;
    void *boxes;
    uint8_t *input;
    float *output;
    float *probs_buf;
    float **probs;
    float *activate;
    float *softmax;
} __attribute__((aligned(8))) region_layer_t;

typedef struct py_kpu_class_rect
{
    int x1;
    int y1;
    int x2;
    int y2;
}__attribute__((aligned(8))) py_kpu_class_rect_t;

typedef struct py_kpu_class_list_link_data {
    py_kpu_class_rect_t rect;
    int classid;
    float value;
    //
    int index;
    int objnum;
} __attribute__((aligned(8))) py_kpu_class_list_link_data_t;

typedef struct py_kpu_class_obj {
    mp_obj_base_t base;
    mp_obj_t x1, y1, x2, y2, classid, index, value, objnum;
} __attribute__((aligned(8))) py_kpu_class_obj_t;

#endif
