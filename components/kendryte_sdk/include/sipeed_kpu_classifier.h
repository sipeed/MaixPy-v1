#ifndef __SIPEED_KPU_CLASSIFIER_H
#define __SIPEED_KPU_CLASSIFIER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
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

int maix_kpu_classifier_init(void** obj, kpu_model_info_t* model, int class_num, int sample_num, bool flag, int flag2);
int maix_kpu_classifier_add_class_img(void* obj, image_t* img, int idx);
int maix_kpu_classifier_add_sample_img(void* obj, image_t* img);
int maix_kpu_classifier_del(void** obj);
int maix_kpu_classifier_train(void* obj);
int maix_kpu_classifier_predict(void* obj, image_t* img, float* min_distance, int* p_x, int* p_y, int* p_w, int* p_h);
int maix_kpu_classifier_rm_class_img(void* obj);
int maix_kpu_classifier_rm_sample_img(void* obj);
int maix_kpu_classifier_save(void* obj, const char* path);
int maix_kpu_classifier_load(void** obj, const char* path, kpu_model_info_t* kmodel, int* class_num, int* sample_num);

int maix_kpu_detector_init(void** obj, kpu_model_info_t* model, int class_num, int sample_num, int crop_size);
int maix_kpu_detector_add_class_img(void* obj, image_t* img);
int maix_kpu_detector_add_sample_img(void* obj, image_t* img);
int maix_kpu_detector_del(void** obj);
int maix_kpu_detector_train(void* obj);
int maix_kpu_detector_predict(void* obj, image_t* img, float* min_distance, int* p_x, int* p_y, int* p_w, int* p_h);
int maix_kpu_detector_rm_class_img(void* obj);
int maix_kpu_detector_rm_sample_img(void* obj);
int maix_kpu_detector_save(void* obj, const char* path);
int maix_kpu_detector_load(void** obj, const char* path, kpu_model_info_t* kmodel, int* class_num, int* sample_num);

#ifdef __cplusplus
}
#endif

#endif

