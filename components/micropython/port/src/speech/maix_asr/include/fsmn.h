#include "stdlib.h"

int fsmn_init(int ftr_dim, int l_mem, int r_mem, int stride,\
	float* memw, float* ln_scale, float* ln_bias, int valid, int tlen);
void fsmn_deinit(void);
int fsmn_memblock_cal_valid(float* data_in, float* data_out, int tlen);
int fsmn_memblock_cal_pad(float* data_in, float* data_out, int tlen);
void fsmn_layernorm(float* data, int tlen);
void fsmn_cal(float* data, int tlen, float** result, int* rlen);