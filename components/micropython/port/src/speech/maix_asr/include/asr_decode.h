#ifndef _MF_ASR_DECODE_H
#define _MF_ASR_DECODE_H

#include <stdint.h>

#include "asr.h"

/*****************************************************************************/
// Macro definitions
/*****************************************************************************/
#define KW_MAX_PNY 		(6) //关键词最大6个字
#define ASR_KW_MAX_CNT 	(64) //最多64个关键词
#define ASR_MAX_PRED	(5)	//每次返回最多的预测结果数量
/*****************************************************************************/
// Enums
/*****************************************************************************/


/*****************************************************************************/
// Types
/*****************************************************************************/
typedef struct {
	uint16_t pny[KW_MAX_PNY];	//每个唤醒词最多 KW_MAX_PNY 个字
	uint16_t pny_cnt;			//该唤醒词的字数
	float	 gate;				//该唤醒词的识别门限
	// char* name;
}asr_kw_t;

typedef struct {
	uint16_t kw_idx;
	float p;
	// char* name;
}asr_res_t;

typedef void (*kwcb_t)(asr_res_t* asr_res, int asr_res_cnt);


int asr_reg_kw(asr_kw_t* kws, int cnt);
//void decode_cal_softmax(float * input, float * output, int frame_cnt, int dict_cnt);
//void decode_sort_result(float *input, uint16_t t_cnt, uint16_t pny_cnt, pnyp_t *output);
// void decode_pny2digit(pnyp_t* pnyp_list, uint8_t* digits);
// void decode_ctc_beam(pnyp_t* pnyp_list, uint16_t t_cnt, uint16_t pny_cnt);
void decode_ctc_pny_similar(pnyp_t* pnyp_list, asr_res_t** res, int* cnt);
void decode_digit(pnyp_t* pnyp_list, int t_cnt, char** res, uint8_t** orignal_res);
#endif

