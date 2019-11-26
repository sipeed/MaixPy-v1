#ifndef __SIPEED_KPU_H
#define __SIPEED_KPU_H

#include "kpu.h"

typedef enum{
    SIPEED_KPU_ERR_NONE    =  0,
    SIPEED_KPU_ERR_PARAM,
    SIPEED_KPU_ERR_KMODEL_VERSION,
    SIPEED_KPU_ERR_KMODEL_FORMAT,
    SIPEED_KPU_ERR_DECRYPT,
    SIPEED_KPU_ERR_READ_FILE,
    SIPEED_KPU_ERR_NO_MEM,
    SIPEED_KPU_ERR_GET_CONV_LAYER,
    SIPEED_KPU_ERR_RUN_MODEL,
    SIPEED_KPU_ERR_MODELS_FULL,
    SIPEED_KPU_ERR_PERMITION,
    SIPEED_KPU_ERR_UNKNOWN,
} sipeed_kpu_err_t;



sipeed_kpu_err_t sipeed_kpu_model_load(void** ctx, uint32_t flash_addr, const char* filename, uint32_t* size);
sipeed_kpu_err_t sipeed_kpu_model_run(void* ctx, const uint8_t *src, dmac_channel_number_t dma_ch, kpu_done_callback_t done_callback, void *userdata);
sipeed_kpu_err_t sipeed_kpu_model_get_input_shape(void* ctx, uint16_t* w, uint16_t* h, uint16_t* ch);
sipeed_kpu_err_t sipeed_kpu_model_get_output_shape(void* ctx, uint16_t* w, uint16_t* h, uint16_t* ch);
sipeed_kpu_err_t sipeed_kpu_get_output(void* ctx, uint32_t index, uint8_t **data, size_t *size);
sipeed_kpu_err_t sipeed_kpu_model_destroy(void** ctx);
sipeed_kpu_err_t sipeed_kpu_model_print_layer_info(void* ctx);
sipeed_kpu_err_t sipeed_kpu_model_get_layer_type(void* ctx, uint32_t index, kpu_model_layer_type_t* type);
sipeed_kpu_err_t sipeed_kpu_model_set_output(void* ctx, uint32_t index, uint32_t layers_length);
kpu_layer_argument_t* sipeed_kpu_model_get_conv_layer(void* ctx, uint32_t index);
int sipeed_kpu_model_get_layer_num(void* ctx);
int sipeed_kpu_model_get_layer_size(void* ctx, uint32_t index);
char sipeed_kpu_model_getdtype_from_type(kpu_model_layer_type_t type);
char* sipeed_kpu_model_getname_from_type(kpu_model_layer_type_t type);

void sipeed_kpu_face_encode(float* feature, int8_t* compress_feature, uint32_t len);
float sipeed_kpu_face_compare(int8_t* feature0, int8_t* feature1, uint32_t len);


/**
 * @brief       Kpu run for AI( V1 API)
 *
 * @param[in]   task                Kpu handler
 * @param[in]   dma_ch              DMA for kpu
 * @param[in]   src                 The picture data
 * @param[in]   dest                The result of kpu
 * @param[in]   callback            The callback of kpu
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail.Kpu is busy.
 */
int kpu_run(kpu_task_t* task, dmac_channel_number_t dma_ch, const void *src, void* dest, plic_irq_callback_t callback);


#endif


