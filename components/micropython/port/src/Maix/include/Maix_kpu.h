#ifndef __MAIX_KPU_H
#define __MAIX_KPU_H

#include "py/obj.h"

typedef struct py_kpu_net_obj
{
    mp_obj_base_t base;
    
    void*           kmodel_ctx;  //sipeed_model_ctx_t
    mp_obj_t        model_size;
    mp_obj_t        model_addr;
    mp_obj_t        model_path;
    mp_obj_t        max_layers;
    mp_obj_t        net_args;   // for yolo2
    mp_obj_t        net_deinit; // for yolo2
} __attribute__((aligned(8))) py_kpu_net_obj_t;

extern const mp_obj_type_t py_kpu_net_obj_type;

extern const mp_obj_type_t Maix_kpu_classifier_type;


#endif

