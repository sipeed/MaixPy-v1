#ifndef __SIPEED_KPU_H
#define __SIPEED_KPU_H
#include "kpu.h"

int kpu_model_flash_get_size(uint32_t model_addr);
kpu_model_layer_type_t kpu_model_get_layer_type(kpu_model_context_t *ctx, uint32_t index);
char* kpu_model_get_layer_type_string(kpu_model_context_t *ctx, uint32_t index);
int kpu_model_get_layer_size(kpu_model_context_t *ctx, uint32_t index);
char* kpu_model_getname_from_type(kpu_model_layer_type_t type);
char* kpu_model_getdtype_from_type(kpu_model_layer_type_t type);
int kpu_model_print_layer_info(kpu_model_context_t *ctx);
uint8_t* kpu_model_get_layer_body(kpu_model_context_t *ctx, uint32_t index);
kpu_layer_argument_t* kpu_model_get_conv_layer(kpu_model_context_t *ctx, uint32_t index);
kpu_layer_argument_t* kpu_model_get_last_conv_layer(kpu_model_context_t *ctx);
int kpu_model_set_output(kpu_model_context_t *ctx, uint32_t index, uint32_t layers_length);
int kpu_model_get_input_shape(kpu_model_context_t *ctx, uint16_t* w, uint16_t* h, uint16_t* ch);
int kpu_model_get_output_shape(kpu_model_context_t *ctx, uint16_t* w, uint16_t* h, uint16_t* ch);


#endif


