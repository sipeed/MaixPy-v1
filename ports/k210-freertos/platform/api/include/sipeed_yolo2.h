#ifndef __SIPEED_YOLO2_H
#define __SIPEED_YOLO2_H

#include <stdint.h>

#include "kpu.h"

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
    //uint8_t *input;
    float *output;
    float *probs_buf;
    float **probs;
    float *activate;
    float *softmax;
} __attribute__((aligned(8))) region_layer_t;

typedef void (*callback_draw_box)(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t class, float prob);;

int region_layer_init(region_layer_t *rl, kpu_model_context_t *task);
void region_layer_deinit(region_layer_t *rl);
void region_layer_run(region_layer_t *rl, obj_info_t *obj_info);
void region_layer_draw_boxes(region_layer_t *rl, callback_draw_box callback);


#endif
