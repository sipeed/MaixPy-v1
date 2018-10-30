#include <stdint.h>
#include <stdlib.h>
#include "fastexp.h"
#include "region_layer.h"

typedef struct{
	int index;
	int class;
	float **probs;
} sortable_bbox;

typedef struct {
	volatile float x, y, w, h;
} box;

typedef struct {
	uint32_t class;
	uint32_t output_cnt;
	float threshold;
	float nms;
	float *active;
	float *softmax;
	float *biases;
	uint8_t *input_data;
	float *output_data;
	box *boxes;
	float **probs;
} REGION_PARAM;

#define region_layer_img_w 320
#define region_layer_img_h 240
#define region_layer_net_w 320
#define region_layer_net_h 240

#define region_layer_l_h 7
#define region_layer_l_w 10

#define region_layer_l_coords 4
#define region_layer_l_n 5
#define region_layer_boxes (region_layer_l_h * region_layer_l_w * region_layer_l_n) // l.w * l.h * l.n

static volatile REGION_PARAM *region_param;

static inline void activate_array(float *x, const int n, const uint8_t *input)
{
	for (int i = 0; i < n; ++i)
		x[i] = region_param->active[input[i]];
}

static inline int entry_index(int location, int entry)
{
	int n   = location / (region_layer_l_w * region_layer_l_h);
	int loc = location % (region_layer_l_w * region_layer_l_h);

	return n * region_layer_l_w * region_layer_l_h *
		(region_layer_l_coords + region_param->class + 1) +
		entry * region_layer_l_w * region_layer_l_h + loc;
}

static inline void softmax(const uint8_t *input, int n, int stride, float *output)
{
	int i;
	float e;
	float sum = 0;
	uint8_t largest_i = input[0];
	int diff;

	for (i = 0; i < n; ++i) {
		if (input[i * stride] > largest_i)
			largest_i = input[i * stride];
	}

	for (i = 0; i < n; ++i) {
		diff = input[i * stride] - largest_i;
		e = region_param->softmax[diff + 255];
		sum += e;
		output[i * stride] = e;
	}
	for (i = 0; i < n; ++i)
		output[i * stride] /= sum;
}

static inline void softmax_cpu(const uint8_t *input, int n, int batch, int batch_offset, int groups, int stride, float *output)
{
	int g, b;

	for (b = 0; b < batch; ++b) {
		for (g = 0; g < groups; ++g)
			softmax(input + b * batch_offset + g, n, stride, output + b * batch_offset + g);
	}
}

static inline void forward_region_layer(const uint8_t *input, float *output)
{
	int n, index;

	for (n = 0; n < region_layer_l_n; ++n) {
		index = entry_index(n * region_layer_l_w * region_layer_l_h, 0);
		activate_array(output + index, 2 * region_layer_l_w * region_layer_l_h, input + index);
		index = entry_index(n * region_layer_l_w * region_layer_l_h, 4);
		activate_array(output + index, region_layer_l_w * region_layer_l_h, input + index);
	}

	index = entry_index(0, 5);
	softmax_cpu(input + index, region_param->class, region_layer_l_n,
		region_param->output_cnt / region_layer_l_n,
		region_layer_l_w * region_layer_l_h,
		region_layer_l_w * region_layer_l_h, output + index);
}

static inline void correct_region_boxes(box *boxes)
{
	int new_w = 0;
	int new_h = 0;

	if (((float)region_layer_net_w / region_layer_img_w) <
	    ((float)region_layer_net_h / region_layer_img_h)) {
		new_w = region_layer_net_w;
		new_h = (region_layer_img_h * region_layer_net_w) / region_layer_img_w;
	} else {
		new_h = region_layer_net_h;
		new_w = (region_layer_img_w * region_layer_net_h) / region_layer_img_h;
	}
	for (int i = 0; i < region_layer_boxes; ++i) {
		box b = boxes[i];

		b.x = (b.x - (region_layer_net_w - new_w) / 2. / region_layer_net_w) /
		      ((float)new_w / region_layer_net_w);
		b.y = (b.y - (region_layer_net_h - new_h) / 2. / region_layer_net_h) /
		      ((float)new_h / region_layer_net_h);
		b.w *= (float)region_layer_net_w / new_w;
		b.h *= (float)region_layer_net_h / new_h;
		boxes[i] = b;
	}
}

static inline box get_region_box(float *x, const float *biases, int n, int index, int i, int j, int w, int h, int stride)
{
	box b;

	b.x = (i + x[index + 0 * stride]) / w;
	b.y = (j + x[index + 1 * stride]) / h;
	b.w = expf(x[index + 2 * stride]) * biases[2 * n] / w;
	b.h = expf(x[index + 3 * stride]) * biases[2 * n + 1] / h;
	return b;
}

static inline void get_region_boxes(float *predictions, float **probs, box *boxes)
{
	for (int i = 0; i < region_layer_l_w * region_layer_l_h; ++i) {
		int row = i / region_layer_l_w;
		int col = i % region_layer_l_w;

		for (int n = 0; n < region_layer_l_n; ++n) {
			int index = n * region_layer_l_w * region_layer_l_h + i;

			for (int j = 0; j < region_param->class; ++j)
				probs[index][j] = 0;
			int obj_index = entry_index(n * region_layer_l_w * region_layer_l_h + i, 4);
			int box_index = entry_index(n * region_layer_l_w * region_layer_l_h + i, 0);
			float scale  = predictions[obj_index];

			boxes[index] = get_region_box(
				predictions, region_param->biases, n, box_index, col, row,
				region_layer_l_w, region_layer_l_h,
				region_layer_l_w * region_layer_l_h);

			float max = 0;

			for (int j = 0; j < region_param->class; ++j) {
				int class_index = entry_index(n * region_layer_l_w * region_layer_l_h + i, 5 + j);
				float prob = scale * predictions[class_index];

				probs[index][j] = (prob > region_param->threshold) ? prob : 0;
				if (prob > max)
					max = prob;
			}
			probs[index][region_param->class] = max;
		}
	}
	correct_region_boxes(boxes);
}

static inline int nms_comparator(const void *pa, const void *pb)
{
	sortable_bbox a = *(sortable_bbox *)pa;
	sortable_bbox b = *(sortable_bbox *)pb;
	float diff = a.probs[a.index][b.class] - b.probs[b.index][b.class];

	if (diff < 0)
		return 1;
	else if (diff > 0)
		return -1;
	return 0;
}

static inline float overlap(float x1, float w1, float x2, float w2)
{
	float l1 = x1 - w1/2;
	float l2 = x2 - w2/2;
	float left = l1 > l2 ? l1 : l2;
	float r1 = x1 + w1/2;
	float r2 = x2 + w2/2;
	float right = r1 < r2 ? r1 : r2;

	return right - left;
}

static inline float box_intersection(box a, box b)
{
	float w = overlap(a.x, a.w, b.x, b.w);
	float h = overlap(a.y, a.h, b.y, b.h);

	if (w < 0 || h < 0)
		return 0;
	return w * h;
}

static inline float box_union(box a, box b)
{
	float i = box_intersection(a, b);
	float u = a.w * a.h + b.w * b.h - i;

	return u;
}

static inline float box_iou(box a, box b)
{
	return box_intersection(a, b)/box_union(a, b);
}

static inline void do_nms_sort(box *boxes, float **probs)
{
	int i, j, k;
	sortable_bbox s[region_layer_boxes];

	for (i = 0; i < region_layer_boxes; ++i) {
		s[i].index = i;
		s[i].class = 0;
		s[i].probs = probs;
	}

	for (k = 0; k < region_param->class; ++k) {
		for (i = 0; i < region_layer_boxes; ++i)
			s[i].class = k;
		qsort(s, region_layer_boxes, sizeof(sortable_bbox), nms_comparator);
		for (i = 0; i < region_layer_boxes; ++i) {
			if (probs[s[i].index][k] == 0)
				continue;
			box a = boxes[s[i].index];

			for (j = i+1; j < region_layer_boxes; ++j) {
				box b = boxes[s[j].index];

				if (box_iou(a, b) > region_param->nms)
					probs[s[j].index][k] = 0;
			}
		}
	}
}

static inline int max_index(float *a, int n)
{
	int i, max_i = 0;
	float max = a[0];

	for (i = 1; i < n; ++i) {
		if (a[i] > max) {
			max   = a[i];
			max_i = i;
		}
	}
	return max_i;
}

int region_layer_init(REGION_CFG *cfg)
{
	void *ptr;

	ptr = malloc(sizeof(REGION_PARAM));
	if (ptr == NULL)
		return 1;
	region_param = (REGION_PARAM *)ptr;
	region_param->class = cfg->class;
	region_param->threshold = cfg->threshold;
	region_param->nms = cfg->nms;
	region_param->active = cfg->active;
	region_param->softmax = cfg->softmax;
	region_param->biases = cfg->biases;
	region_param->output_cnt = (region_layer_boxes * (region_param->class + region_layer_l_coords + 1));
	cfg->input_len = region_param->output_cnt;
	ptr = malloc(sizeof(uint8_t) * region_param->output_cnt + 63);
	if (ptr == NULL)
		return 1;
	region_param->input_data = (uint8_t *)(((uint32_t)ptr + 63) & 0xFFFFFFC0);
	cfg->input = region_param->input_data;
	//TODO fix
	// ptr = malloc(sizeof(float) * region_param->output_cnt);
	// if (ptr == NULL)
	// 	return 1;
	// region_param->output_data = (float *)ptr;
	ptr = malloc(sizeof(float *) * region_layer_boxes);
	if (ptr == NULL)
		return 1;
	region_param->probs = (float **)ptr;
	for (uint32_t i = 0; i < region_layer_boxes; i++) {
		ptr = malloc(sizeof(float) * (region_param->class + 1));
		if (ptr == NULL)
			return 1;
		region_param->probs[i] = (float *)ptr;
	}
	ptr = malloc(sizeof(box) * region_layer_boxes);
	if (ptr == NULL)
		return 1;
	region_param->boxes = (box *)ptr;
	return 0;
}
//TODO fix
static float output[2100];
void region_layer_cal_first(void)
{
	forward_region_layer(region_param->input_data, output);
}

void region_layer_cal_second(void)
{
	get_region_boxes(output, region_param->probs, region_param->boxes);
	do_nms_sort(region_param->boxes, region_param->probs);
}

void region_layer_detect_object(OBJECT_INFO *info)
{
	uint16_t index = 0;

	for (int i = 0; i < region_layer_boxes; ++i) {
		int class  = max_index(region_param->probs[i], region_param->class);
		float prob = region_param->probs[i][class];

		if (prob > region_param->threshold) {
			box *b      = region_param->boxes + i;
			uint32_t x1 = b->x * region_layer_img_w -
				      (b->w * region_layer_img_w / 2);
			uint32_t y1 = b->y * region_layer_img_h -
				      (b->h * region_layer_img_h / 2);
			uint32_t x2 = b->x * region_layer_img_w +
				      (b->w * region_layer_img_w / 2);
			uint32_t y2 = b->y * region_layer_img_h +
				      (b->h * region_layer_img_h / 2);
			info->object[index].class = class;
			info->object[index].prob = prob * 100;
			info->object[index].x1 = x1 < region_layer_img_w ? x1 : region_layer_img_w -1;
			info->object[index].x2 = x2 < region_layer_img_w ? x2 : region_layer_img_w -1;
			info->object[index].y1 = y1 < region_layer_img_h ? y1 : region_layer_img_h -1;
			info->object[index].y2 = y2 < region_layer_img_h ? y2 : region_layer_img_h -1;
			index++;
			if (index == 20)
				break;
		}
	}
	info->object_num = index;
}
