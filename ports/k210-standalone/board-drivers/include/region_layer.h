#ifndef _REGION_LAYER_
#define _REGION_LAYER_

#include <stdint.h>

typedef struct {
	uint32_t class;
	float threshold;
	float nms;
	uint32_t input_len;
	uint8_t *input;
	float *active;
	float *softmax;
	float *biases;
} REGION_CFG;

typedef struct {
	uint16_t object_num;
	struct {
		uint16_t class;
		uint16_t prob;
		uint16_t x1;
		uint16_t x2;
		uint16_t y1;
		uint16_t y2;
	} object[20];
} OBJECT_INFO;

int region_layer_init(REGION_CFG *cfg);
void region_layer_cal_first(void);
void region_layer_cal_second(void);
void region_layer_detect_object(OBJECT_INFO *info);

#endif // _REGION_LAYER
