/* Copyright 2019 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _NNCASE_H
#define _NNCASE_H

#include "kpu.h"

#ifdef __cplusplus
extern "C" {
#endif

int nncase_load_kmodel(kpu_model_context_t *ctx, const uint8_t *buffer);
int nncase_get_output(kpu_model_context_t *ctx, uint32_t index, uint8_t **data, size_t *size);
void nncase_model_free(kpu_model_context_t *ctx);
int nncase_run_kmodel(kpu_model_context_t *ctx, const uint8_t *src, dmac_channel_number_t dma_ch, kpu_done_callback_t done_callback, void *userdata);
//Add for MaixPy
void nncase_get_metadata(kpu_model_context_t *ctx, uint16_t* w, uint16_t* h, uint16_t* ch, uint16_t* output_cnt, uint16_t* layer_length);
int nncase_get_output_size(kpu_model_context_t *ctx, uint16_t idx);
#ifdef __cplusplus
}
#endif

#endif