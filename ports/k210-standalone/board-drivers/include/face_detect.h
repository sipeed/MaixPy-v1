#ifndef _AI_H_
#define _AI_H_

#include <stdint.h>

int ai_dma_irq(void *ctx);
int ai_init(uint8_t mode);
void ai_cal_start(void);
void ai_data_input(uint32_t addr);
void ai_data_output(void);
void ai_cal_first(void);
void ai_cal_second(void);
struct face_Data ai_draw_label(uint32_t *ptr);
void ai_result_send(void);
void ai_test(char* str);
struct face_Data
{
	uint16_t x1;
	uint16_t y1;
	uint16_t x2;
	uint16_t y2;
};

#endif
