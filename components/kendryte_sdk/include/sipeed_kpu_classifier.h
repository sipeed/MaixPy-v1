#ifndef __SIPEED_KPU_CLASSIFIER_H
#define __SIPEED_KPU_CLASSIFIER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "imdefs.h"
#if 0
typedef struct image {
    int w;
    int h;
    int bpp;
    union {
        uint8_t *pixels;
        uint8_t *data;
    };
	uint8_t *pix_ai;	//for MAIX AI speed up
} __attribute__((aligned(8)))image_t;
#endif

typedef struct
{
    void*           kmodel_ctx;  //sipeed_model_ctx_t
    uint32_t        model_size;
    uint32_t        model_addr;
    const char*     model_path;
    uint32_t        max_layers;
} __attribute__((aligned(8))) kpu_model_info_t;

int maix_kpu_classifier_init(void** obj, kpu_model_info_t* model, int class_num, int sample_num);
int maix_kpu_classifier_add_class_img(void* obj, image_t* img);
int maix_kpu_classifier_add_sample_img(void* obj, image_t* img);
int maix_kpu_classifier_del(void** obj);
int maix_kpu_classifier_train(void* obj);
int maix_kpu_classifier_predict(void* obj, image_t* img, float* min_distance);

#ifdef __cplusplus
}
#endif

#endif

