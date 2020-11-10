#ifndef ASR_CNN2_H_
#define ASR_CNN2_H_

#include "nncase.h"

int cnn2_init(int tlen);
void cnn2_deinit(void);
float* cnn2_run(kpu_model_context_t* task, float* data, int tlen);

#endif