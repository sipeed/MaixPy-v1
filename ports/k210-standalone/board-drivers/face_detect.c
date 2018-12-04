#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "face_detect.h"
#include "platform.h"
#include "sysctl.h"
#include "w25qxx.h"
#include "lcd.h"
#include "dmac.h"
#include "plic.h"
#include "region_layer.h"
// #include "coprocessor.h"

#define AI_DMA_CHANNEL 1
#define __AI_DMA_INTERRUPT(x)	IRQN_DMA##x##_INTERRUPT
#define _AI_DMA_INTERRUPT(x)	__AI_DMA_INTERRUPT(x)
#define AI_DMA_INTERRUPT	_AI_DMA_INTERRUPT(AI_DMA_CHANNEL)

#define AI_CFG_ADDRESS (512 * 1024)
#define AI_CFG_SIZE (4 * 1024)

struct ai_reg_t {
	volatile uint64_t cfg_fifo;
	volatile uint64_t intr_status;
	volatile uint64_t intr_raw;
	volatile uint64_t intr_mask;
	volatile uint64_t intr_clear;
	volatile uint64_t fifo_threshold;
	volatile uint64_t output_fifo;
	volatile uint64_t reserve;
	volatile uint64_t bit_mode;
} __attribute__((packed, aligned(8)));
volatile struct ai_reg_t *const ai_reg = (volatile struct ai_reg_t *)AI_BASE_ADDR;

static OBJECT_INFO object_info;

typedef struct {
	uint32_t magic;
	uint32_t bit_mode;
	uint32_t layer;
	uint32_t layer_addr;
	uint32_t class;
	uint32_t label_addr;
	float threshold;
	float nms;
	uint32_t active_addr;
	uint32_t active_len;
	uint32_t softmax_addr;
	uint32_t softmax_len;
	uint32_t biases_addr;
	uint32_t biases_len;
	uint32_t reserve[2];
} MODEL_CFG;

typedef struct {
	char name[24];
	uint16_t len;
	uint16_t height;
	uint16_t width;
	uint16_t color;
} LABEL_CFG;

typedef struct {
	char name[24];
	uint16_t len;
	uint16_t height;
	uint16_t width;
	uint16_t color;
	uint32_t *ptr;
} LABEL_PARAM;

typedef struct {
	uint32_t reg_addr;
	uint32_t reg_len;
	uint32_t act_addr;
	uint32_t act_len;
	uint32_t arg_addr;
	uint32_t arg_len;
	uint32_t wht_addr;
	uint32_t wht_len;
} LAYER_CFG;

typedef struct {
	uint64_t *reg;
	uint64_t *act;
	uint64_t *arg;
	uint64_t *wht;
} LAYER_PARAM;

typedef struct {
	uint32_t bit_mode;
	uint32_t layer;
	uint32_t class;
	LAYER_PARAM *layer_param;
	LABEL_PARAM *label_param;
	uint32_t input_addr;
	uint32_t input_len;
	uint32_t output_addr;
	uint32_t output_len;
} AI_PARAM;//__attribute__(4);

static AI_PARAM ai_param;

static int ai_param_init(uint8_t mode)
{
	void *ptr;
	MODEL_CFG model_cfg;
	REGION_CFG region_cfg;
	LAYER_CFG layer_cfg;
	LABEL_CFG label_cfg;
	w25qxx_read_data(AI_CFG_ADDRESS + AI_CFG_SIZE * mode, (uint8_t *)&model_cfg, sizeof(MODEL_CFG), W25QXX_QUAD_FAST);
	printf("addr = %x,model_cfg.magic = %x\n",AI_CFG_ADDRESS + AI_CFG_SIZE * mode,model_cfg.magic);
	if (model_cfg.magic != 0x12345678)
	{
		printf("ai param magic is error\n");
		return 1;
	}
	region_cfg.class = model_cfg.class;
	region_cfg.threshold = model_cfg.threshold;
	region_cfg.nms = model_cfg.nms;
	ptr = malloc(model_cfg.active_len);
	if (ptr == NULL)
		return 1;
	region_cfg.active = (float *)ptr;
	ptr = malloc(model_cfg.softmax_len);
	if (ptr == NULL)
		return 1;
	region_cfg.softmax = (float *)ptr;
	ptr = malloc(model_cfg.biases_len);
	if (ptr == NULL)
		return 1;
	region_cfg.biases = (float *)ptr;
	w25qxx_read_data(model_cfg.active_addr, (uint8_t *)region_cfg.active, model_cfg.active_len, W25QXX_QUAD_FAST);
	w25qxx_read_data(model_cfg.softmax_addr, (uint8_t *)region_cfg.softmax, model_cfg.softmax_len, W25QXX_QUAD_FAST);
	w25qxx_read_data(model_cfg.biases_addr, (uint8_t *)region_cfg.biases, model_cfg.biases_len, W25QXX_QUAD_FAST);
	if (region_layer_init(&region_cfg))
		return 1;
	ai_param.output_addr = (uint32_t)region_cfg.input;
	ai_param.output_len = (region_cfg.input_len + 7) / 8;
	// layer init
	ai_param.layer = model_cfg.layer;
	ptr = malloc(sizeof(LAYER_PARAM) * ai_param.layer);
	if (ptr == NULL)
		return 1;
	ai_param.layer_param = (LAYER_PARAM *)ptr;
	for (uint32_t layer = 0; layer < ai_param.layer; layer++) {
		w25qxx_read_data(model_cfg.layer_addr + sizeof(LAYER_CFG) * layer, (uint8_t *)&layer_cfg, sizeof(LAYER_CFG), W25QXX_QUAD_FAST);
		// reg
		ptr = malloc(layer_cfg.reg_len);
		if (ptr == NULL)
			return 1;
		ai_param.layer_param[layer].reg = (uint64_t *)ptr;
		w25qxx_read_data(layer_cfg.reg_addr, (uint8_t *)ptr, layer_cfg.reg_len, W25QXX_QUAD_FAST);
		// act
		ptr = malloc(layer_cfg.act_len);
		if (ptr == NULL)
			return 1;
		ai_param.layer_param[layer].act = (uint64_t *)ptr;
		w25qxx_read_data(layer_cfg.act_addr, (uint8_t *)ptr, layer_cfg.act_len, W25QXX_QUAD_FAST);
		// arg
		ptr = malloc(layer_cfg.arg_len);
		if (ptr == NULL)
			return 1;
		ai_param.layer_param[layer].arg = (uint64_t *)ptr;
		w25qxx_read_data(layer_cfg.arg_addr, (uint8_t *)ptr, layer_cfg.arg_len, W25QXX_QUAD_FAST);
		// wht
		ptr = malloc(layer_cfg.wht_len);
		if (ptr == NULL)
			return 1;
		ai_param.layer_param[layer].wht = (uint64_t *)ptr;
		w25qxx_read_data(layer_cfg.wht_addr, (uint8_t *)ptr, layer_cfg.wht_len, W25QXX_QUAD_FAST);
	}
	// label init
	ai_param.class = model_cfg.class;
	if (ai_param.class > 1) {
		ptr = malloc(sizeof(LABEL_PARAM) * ai_param.class);
		if (ptr == NULL)
			return 1;
		ai_param.label_param = (LABEL_PARAM *)ptr;
		for (uint32_t class = 0; class < ai_param.class; class++) {
			w25qxx_read_data(model_cfg.label_addr + sizeof(LABEL_CFG) * class, (uint8_t *)&label_cfg, sizeof(LABEL_CFG), W25QXX_QUAD_FAST);

			ai_param.label_param[class].len = label_cfg.len;
			ai_param.label_param[class].height = label_cfg.height;
			ai_param.label_param[class].width = label_cfg.width;
			ai_param.label_param[class].color = label_cfg.color;
			memcpy(ai_param.label_param[class].name, label_cfg.name, 24);
		}
	}
	ai_param.bit_mode = model_cfg.bit_mode;
	return 0;
}

static int lable_init(void)
{
	if (ai_param.class <= 1)
		return 0;
	for (uint32_t class = 1; class < ai_param.class; class++) {
		ai_param.label_param[class].width *= ai_param.label_param[class].len;
		ai_param.label_param[class].ptr = (uint32_t *)malloc(ai_param.label_param[class].width * ai_param.label_param[class].height * 2);
		if (ai_param.label_param[class].ptr == NULL)
			return 1;
		// lcd_draw_string(ai_param.label_param[class].name, ai_param.label_param[class].ptr, BLACK, ai_param.label_param[class].color);
	}
	return 0;
}

static int ai_irq(void *ctx)
{
	ai_reg->intr_mask = 0x7;
	ai_reg->intr_clear = 0x7;
	for (uint32_t layer = 12; layer < ai_param.layer; layer++) {
		for (uint32_t index = 0; index < 12; index++)
			ai_reg->cfg_fifo = ai_param.layer_param[layer].reg[index];
	}
	return 0;
}

int ai_init(uint8_t mode)
{
	object_info.object_num = 0;
	if (ai_param_init(mode))
		return 1;
	if (lable_init())
		return 1;
	for (uint32_t layer = 0; layer < ai_param.layer; layer++) {
		ai_param.layer_param[layer].reg[4] &= 0x00000000FFFFFFFF;
		ai_param.layer_param[layer].reg[5] &= 0x00000000FFFFFFFF;
		ai_param.layer_param[layer].reg[7] &= 0x00000000FFFFFFFF;
		ai_param.layer_param[layer].reg[4] |= (((uint64_t)ai_param.layer_param[layer].arg) << 32);
		ai_param.layer_param[layer].reg[5] |= (((uint64_t)ai_param.layer_param[layer].wht) << 32);
		ai_param.layer_param[layer].reg[7] |= (((uint64_t)ai_param.layer_param[layer].act) << 32);
	}
	ai_param.input_addr = AI_IO_BASE_ADDR + (ai_param.layer_param[0].reg[1] & 0x7FFF) * 64;
	ai_param.input_len = 320 * 240 * 3 / 8;
	sysctl_clock_enable(SYSCTL_CLOCK_AI);
	ai_reg->bit_mode = ai_param.bit_mode;
	ai_reg->fifo_threshold = 0x10;
	ai_reg->intr_mask = 0x07;
	ai_reg->intr_clear = 0x07;
	dmac->channel[AI_DMA_CHANNEL].intstatus_en = 0x02;
	dmac->channel[AI_DMA_CHANNEL].intclear = 0xFFFFFFFF;
	if (ai_param.layer > 13) {
		plic_irq_enable(IRQN_AI_INTERRUPT);
		plic_set_priority(IRQN_AI_INTERRUPT, 1);
		plic_irq_register(IRQN_AI_INTERRUPT, ai_irq, NULL);
	}
	plic_irq_enable(AI_DMA_INTERRUPT);
	plic_set_priority(AI_DMA_INTERRUPT, 1);
	plic_irq_register(AI_DMA_INTERRUPT, ai_dma_irq, NULL);
	return 0;
}

void ai_cal_start(void)
{
	if (ai_param.layer > 13) {
		for (uint32_t layer = 0; layer < 12; layer++) {
			for (uint32_t index = 0; index < 12; index++)
				ai_reg->cfg_fifo = ai_param.layer_param[layer].reg[index];
		}
		ai_reg->intr_mask = 0x05;
	} else {
		for (uint32_t layer = 0; layer < ai_param.layer; layer++) {
			for (uint32_t index = 0; index < 12; index++)
				ai_reg->cfg_fifo = ai_param.layer_param[layer].reg[index];
		}
	}
}

void ai_data_input(uint32_t addr)
{

	dmac->channel[AI_DMA_CHANNEL].ctl = (((uint64_t)1 << 47) | ((uint64_t)15 << 48) |
					((uint64_t)1 << 38) | ((uint64_t)15 << 39) |
					((uint64_t)3 << 18) | ((uint64_t)3 << 14) |
					((uint64_t)3 << 11) | ((uint64_t)3 << 8));
	dmac->channel[AI_DMA_CHANNEL].cfg = ((uint64_t)1 << 49);
	dmac->channel[AI_DMA_CHANNEL].sar = (uint64_t)addr;
	dmac->channel[AI_DMA_CHANNEL].dar = (uint64_t)ai_param.input_addr;
	dmac->channel[AI_DMA_CHANNEL].block_ts = ai_param.input_len - 1;
	dmac->chen = 0x0101 << AI_DMA_CHANNEL;
}

void ai_data_output(void)
{
	sysctl_dma_select(AI_DMA_CHANNEL, SYSCTL_DMA_SELECT_AI_RX_REQ);
	dmac->channel[AI_DMA_CHANNEL].ctl = (((uint64_t)1 << 47) | ((uint64_t)7 << 48) |
					((uint64_t)1 << 38) | ((uint64_t)7 << 39) |
					((uint64_t)2 << 18) | ((uint64_t)2 << 14) |
					((uint64_t)3 << 11) | ((uint64_t)3 << 8) |
					((uint64_t)1 << 4));
	dmac->channel[AI_DMA_CHANNEL].cfg = (((uint64_t)1 << 49) | ((uint64_t)AI_DMA_CHANNEL << 44) |
				((uint64_t)AI_DMA_CHANNEL << 39) | ((uint64_t)2 << 32));
	dmac->channel[AI_DMA_CHANNEL].sar = (uint64_t)(&ai_reg->output_fifo);
	dmac->channel[AI_DMA_CHANNEL].dar = (uint64_t)ai_param.output_addr;
	dmac->channel[AI_DMA_CHANNEL].block_ts = ai_param.output_len - 1;
	dmac->chen = 0x0101 << AI_DMA_CHANNEL;
}
void ai_test(char* str)
{
	printf("[%s]:ai_param.layer_param[0].reg[0] = %ld\n",str,ai_param.layer_param[0].reg[0]);
}

void ai_cal_first(void)
{
	region_layer_cal_first();
}

void ai_cal_second(void)
{
	region_layer_cal_second();
	region_layer_detect_object(&object_info);
}

// void ai_result_send(void)
// {
// 	VIDEO_INFO_SEND video_info_send;

// 	video_info_send.object_num = object_info.object_num;
// 	for (uint8_t i = 0; i < object_info.object_num; i++) {
// 		video_info_send.object[i].x1 = object_info.object[i].x1;
// 		video_info_send.object[i].x2 = object_info.object[i].x2;
// 		video_info_send.object[i].y1 = object_info.object[i].y1;
// 		video_info_send.object[i].y2 = object_info.object[i].y2;
// 	}
// 	// coprocessor_send_video_data(&video_info_send);
// }

struct face_Data ai_draw_label(uint32_t *ptr)
{
	struct face_Data ret;
	for (uint8_t i = 0; i < object_info.object_num; i++) {
		if (ai_param.class > 1) {
			uint8_t class = object_info.object[i].class;

			lcd_draw_rectangle_cpu(ptr,object_info.object[i].x1,
									   object_info.object[i].y1,
									   object_info.object[i].x2, 
									   object_info.object[i].y2, 
									   ai_param.label_param[class].color);
			
			
			if ((object_info.object[i].x1 + 1 + ai_param.label_param[class].width < 320) &&
				(object_info.object[i].y1 + 1 + ai_param.label_param[class].height < 240))
				lcd_draw_picture(object_info.object[i].x1 + 1, 
								 object_info.object[i].y1 + 1,
								 ai_param.label_param[class].width, 
								 ai_param.label_param[class].height, 
								 ai_param.label_param[class].ptr);
			
		} else{
				// lcd_draw_rectangle(object_info.object[i].x1, object_info.object[i].y1,
				// 	object_info.object[i].x2, object_info.object[i].y2, 2, RED);
				uint8_t class = object_info.object[i].class;
				/*
				printf("(%d,%d)--(%d,%d) >> %d - %d%%\n", object_info.object[i].x1, 
														  object_info.object[i].y1,
														  object_info.object[i].x2, 
														  object_info.object[i].y2, 
														  class, 
														  (uint8_t)(object_info.object[i].prob * 100));
				*/
				lcd_draw_rectangle_cpu(ptr, object_info.object[i].x1,
											object_info.object[i].y1,
											object_info.object[i].x2, 
											object_info.object[i].y2, RED);				
				ret.x1 = object_info.object[i].x1;
				ret.y1 = object_info.object[i].y1;
				ret.x2 = object_info.object[i].x2;
				ret.y2 = object_info.object[i].y2;
			}
	}

	return ret;
}
