#include "sipeed_yolo2.h"
#include "sipeed_kpu.h"
#include <stdlib.h>
#include <math.h>

// #include "lcd.h"

/* start of region_layer.c*/

typedef struct
{
    float x;
    float y;
    float w;
    float h;
} __attribute__((aligned(8))) box_t;

typedef struct
{
    int index;
    int class;
    float **probs;
} __attribute__((aligned(8))) sortable_box_t;


int region_layer_init(region_layer_t *rl, void* ctx)
{
    int flag = 0;
	uint16_t wi,hi,chi;
	uint16_t wo,ho,cho;
	size_t size;
	
	if(sipeed_kpu_model_get_input_shape(ctx, &wi, &hi, &chi) != SIPEED_KPU_ERR_NONE)
	{
		// mp_printf(&mp_plat_print, "[MAIXPY]rl: first layer not conv layer!\r\n");
		return -1;
	}
	if(sipeed_kpu_model_get_output_shape(ctx, &wo, &ho, &cho) != SIPEED_KPU_ERR_NONE)
	{
		// mp_printf(&mp_plat_print, "[MAIXPY]rl: can't fetch last layer!\r\n");
		return -1;
	}

    rl->coords = 4;
    rl->image_width = wi;
    rl->image_height = hi;

    rl->classes = cho / 5 - 5;
    rl->net_width = wi;
    rl->net_height = hi;
    rl->layer_width = wo;
    rl->layer_height = ho;
    rl->boxes_number = (rl->layer_width * rl->layer_height * rl->anchor_number);
    rl->output_number = (rl->boxes_number * (rl->classes + rl->coords + 1));
    
	sipeed_kpu_get_output(ctx, 0, &(rl->output), &size);	//module output -> rl output
	//mp_printf(&mp_plat_print, "size=%ld\r\n",size);
    //rl->scale = output_scale;
    //rl->bias = output_bias;

    /*rl->output = malloc(rl->output_number * sizeof(float));
    if (rl->output == NULL)
    {
        flag = -1;
        goto malloc_error;
    }*/
    rl->boxes = malloc(rl->boxes_number * sizeof(box_t));
    if (rl->boxes == NULL)
    {
        flag = -2;
        goto malloc_error;
    }
    rl->probs_buf = malloc(rl->boxes_number * (rl->classes + 1) * sizeof(float));
    if (rl->probs_buf == NULL)
    {
        flag = -3;
        goto malloc_error;
    }
    rl->probs = malloc(rl->boxes_number * sizeof(float *));
    if (rl->probs == NULL)
    {
        flag = -4;
        goto malloc_error;
    }
    /*rl->activate = malloc(256 * sizeof(float));
    if (rl->activate == NULL)
    {
        flag = -5;
        goto malloc_error;
    }
    rl->softmax = malloc(256 * sizeof(float));
    if (rl->softmax == NULL)
    {
        flag = -5;
        goto malloc_error;
    }
    for (int i = 0; i < 256; i++)
    {
        rl->activate[i] = 1.0 / (1.0 + expf(-(i * rl->scale + rl->bias)));
        rl->softmax[i] = expf(rl->scale * (i - 255));
    }*/
    for (uint32_t i = 0; i < rl->boxes_number; i++)
        rl->probs[i] = &(rl->probs_buf[i * (rl->classes + 1)]);
    return 0;
malloc_error:
    //free(rl->output);
    free(rl->boxes);
    free(rl->probs_buf);
    free(rl->probs);
    //free(rl->activate);
    //free(rl->softmax);
    return flag;
}

void region_layer_deinit(region_layer_t *rl)
{
    //free(rl->output);
    free(rl->boxes);
    free(rl->probs_buf);
    free(rl->probs);
    //free(rl->activate);
    //free(rl->softmax);
}

static void activate_array(region_layer_t *rl, int index, int n)
{
    float *output = &rl->output[index];
    //uint8_t *input = &rl->input[index];

    for (int i = 0; i < n; ++i)
        output[i] = 1.0 / (1.0 + expf(-output[i]));//rl->activate[input[i]];
}

static int entry_index(region_layer_t *rl, int location, int entry)
{
    int wh = rl->layer_width * rl->layer_height;
    int n = location / wh;
    int loc = location % wh;

    return n * wh * (rl->coords + rl->classes + 1) + entry * wh + loc;
}

static void softmax(float *data, int n, int stride)
{
    int i;
    // int diff;
    // float e;
    float sum = 0;
    float largest_i = data[0];

    for (i = 0; i < n; ++i)
    {
        if (data[i * stride] > largest_i)
            largest_i = data[i * stride];
    }
    for (i = 0; i < n; ++i)
    {
        float value = expf(data[i * stride] - largest_i);
        sum += value;
        data[i * stride] = value;
    }
    for (i = 0; i < n; ++i)
	{
        data[i * stride] /= sum;
	}
}

static void softmax_cpu(float *data, int n, int batch, int batch_offset, int groups, int stride)
{
    int g, b;

    for (b = 0; b < batch; ++b)
    {
        for (g = 0; g < groups; ++g)
            softmax(data + b * batch_offset + g, n, stride);
    }
}

static void forward_region_layer(region_layer_t *rl)
{
    int index;

    //for (index = 0; index < rl->output_number; index++)
    //    rl->output[index] = rl->input[index] * rl->scale + rl->bias;

    for (int n = 0; n < rl->anchor_number; ++n)
    {
        index = entry_index(rl, n * rl->layer_width * rl->layer_height, 0);
        activate_array(rl, index, 2 * rl->layer_width * rl->layer_height);
        index = entry_index(rl, n * rl->layer_width * rl->layer_height, 4);
        activate_array(rl, index, rl->layer_width * rl->layer_height);
    }

    index = entry_index(rl, 0, rl->coords + 1);
	softmax_cpu(rl->output + index, rl->classes, rl->anchor_number,\
		rl->output_number / rl->anchor_number, rl->layer_width * rl->layer_height,\
		rl->layer_width * rl->layer_height);
	
}

static void correct_region_boxes(region_layer_t *rl, box_t *boxes)
{
    uint32_t net_width = rl->net_width;
    uint32_t net_height = rl->net_height;
    uint32_t image_width = rl->image_width;
    uint32_t image_height = rl->image_height;
    uint32_t boxes_number = rl->boxes_number;
    int new_w = 0;
    int new_h = 0;

    if (((float)net_width / image_width) <
        ((float)net_height / image_height))
    {
        new_w = net_width;
        new_h = (image_height * net_width) / image_width;
    }
    else
    {
        new_h = net_height;
        new_w = (image_width * net_height) / image_height;
    }
    for (int i = 0; i < boxes_number; ++i)
    {
        box_t b = boxes[i];

        b.x = (b.x - (net_width - new_w) / 2. / net_width) /
              ((float)new_w / net_width);
        b.y = (b.y - (net_height - new_h) / 2. / net_height) /
              ((float)new_h / net_height);
        b.w *= (float)net_width / new_w;
        b.h *= (float)net_height / new_h;
        boxes[i] = b;
    }
}

static box_t get_region_box(float *x, float *biases, int n, int index, int i, int j, int w, int h, int stride)
{
    volatile box_t b;

    b.x = (i + x[index + 0 * stride]) / w;
    b.y = (j + x[index + 1 * stride]) / h;
    b.w = expf(x[index + 2 * stride]) * biases[2 * n] / w;
    b.h = expf(x[index + 3 * stride]) * biases[2 * n + 1] / h;
    return b;
}

static void get_region_boxes(region_layer_t *rl, float *predictions, float **probs, box_t *boxes)
{
    uint32_t layer_width = rl->layer_width;
    uint32_t layer_height = rl->layer_height;
    uint32_t anchor_number = rl->anchor_number;
    uint32_t classes = rl->classes;
    uint32_t coords = rl->coords;
    float threshold = rl->threshold;

    for (int i = 0; i < layer_width * layer_height; ++i)
    {
        int row = i / layer_width;
        int col = i % layer_width;

        for (int n = 0; n < anchor_number; ++n)
        {
            int index = n * layer_width * layer_height + i;

            for (int j = 0; j < classes; ++j)
                probs[index][j] = 0;
            int obj_index = entry_index(rl, n * layer_width * layer_height + i, coords);
            int box_index = entry_index(rl, n * layer_width * layer_height + i, 0);
            float scale = predictions[obj_index];

            boxes[index] = get_region_box(predictions, rl->anchor, n, box_index, col, row,
                                          layer_width, layer_height, layer_width * layer_height);

            float max = 0;

            for (int j = 0; j < classes; ++j)
            {
                int class_index = entry_index(rl, n * layer_width * layer_height + i, coords + 1 + j);
                float prob = scale * predictions[class_index];

                probs[index][j] = (prob > threshold) ? prob : 0;
                if (prob > max)
                    max = prob;
            }
            probs[index][classes] = max;
        }
    }
    correct_region_boxes(rl, boxes);
}

static int nms_comparator(const void *pa, const void *pb)
{
    sortable_box_t a = *(sortable_box_t *)pa;
    sortable_box_t b = *(sortable_box_t *)pb;
    float diff = a.probs[a.index][b.class] - b.probs[b.index][b.class];

    if (diff < 0)
        return 1;
    else if (diff > 0)
        return -1;
    return 0;
}

static float overlap(float x1, float w1, float x2, float w2)
{
    float l1 = x1 - w1 / 2;
    float l2 = x2 - w2 / 2;
    float left = l1 > l2 ? l1 : l2;
    float r1 = x1 + w1 / 2;
    float r2 = x2 + w2 / 2;
    float right = r1 < r2 ? r1 : r2;

    return right - left;
}

static float box_intersection(box_t a, box_t b)
{
    float w = overlap(a.x, a.w, b.x, b.w);
    float h = overlap(a.y, a.h, b.y, b.h);

    if (w < 0 || h < 0)
        return 0;
    return w * h;
}

static float box_union(box_t a, box_t b)
{
    float i = box_intersection(a, b);
    float u = a.w * a.h + b.w * b.h - i;

    return u;
}

static float box_iou(box_t a, box_t b)
{
    return box_intersection(a, b) / box_union(a, b);
}

static void do_nms_sort(region_layer_t *rl, box_t *boxes, float **probs)
{
    uint32_t boxes_number = rl->boxes_number;
    uint32_t classes = rl->classes;
    float nms_value = rl->nms_value;
    int i, j, k;
    sortable_box_t s[boxes_number];

    for (i = 0; i < boxes_number; ++i)
    {
        s[i].index = i;
        s[i].class = 0;
        s[i].probs = probs;
    }

    for (k = 0; k < classes; ++k)
    {
        for (i = 0; i < boxes_number; ++i)
            s[i].class = k;
        qsort(s, boxes_number, sizeof(sortable_box_t), nms_comparator);
        for (i = 0; i < boxes_number; ++i)
        {
            if (probs[s[i].index][k] == 0)
                continue;
            box_t a = boxes[s[i].index];

            for (j = i + 1; j < boxes_number; ++j)
            {
                box_t b = boxes[s[j].index];

                if (box_iou(a, b) > nms_value)
                    probs[s[j].index][k] = 0;
            }
        }
    }
}

static int max_index(float *a, int n)
{
    int i, max_i = 0;
    float max = a[0];

    for (i = 1; i < n; ++i)
    {
        if (a[i] > max)
        {
            max = a[i];
            max_i = i;
        }
    }
    return max_i;
}

static void region_layer_output(region_layer_t *rl, obj_info_t *obj_info)
{
    uint32_t obj_number = 0;
    uint32_t image_width = rl->image_width;
    uint32_t image_height = rl->image_height;
    float threshold = rl->threshold;
    box_t *boxes = (box_t *)rl->boxes;

    for (int i = 0; i < rl->boxes_number; ++i)
    {
        int class = max_index(rl->probs[i], rl->classes);
        float prob = rl->probs[i][class];
		//mp_printf(&mp_plat_print, "%.3f, ", prob);
        if (prob > threshold)
        {
            box_t *b = boxes + i;
            obj_info->obj[obj_number].x1 = b->x * image_width - (b->w * image_width / 2);
            obj_info->obj[obj_number].y1 = b->y * image_height - (b->h * image_height / 2);
            obj_info->obj[obj_number].x2 = b->x * image_width + (b->w * image_width / 2);
            obj_info->obj[obj_number].y2 = b->y * image_height + (b->h * image_height / 2);
            obj_info->obj[obj_number].classid = class;
            obj_info->obj[obj_number].prob = prob;
            obj_number++;
        }
    }
    obj_info->obj_number = obj_number;
}

void region_layer_run(region_layer_t *rl, obj_info_t *obj_info)
{
	forward_region_layer(rl);
    get_region_boxes(rl, rl->output, rl->probs, rl->boxes);
    do_nms_sort(rl, rl->boxes, rl->probs);
    region_layer_output(rl, obj_info);
}

void region_layer_draw_boxes(region_layer_t *rl, callback_draw_box callback)
{
    uint32_t image_width = rl->image_width;
    uint32_t image_height = rl->image_height;
    float threshold = rl->threshold;
    box_t *boxes = (box_t *)rl->boxes;

    for (int i = 0; i < rl->boxes_number; ++i)
    {
        int class = max_index(rl->probs[i], rl->classes);
        float prob = rl->probs[i][class];

        if (prob > threshold)
        {
            box_t *b = boxes + i;
            uint32_t x1 = b->x * image_width - (b->w * image_width / 2);
            uint32_t y1 = b->y * image_height - (b->h * image_height / 2);
            uint32_t x2 = b->x * image_width + (b->w * image_width / 2);
            uint32_t y2 = b->y * image_height + (b->h * image_height / 2);
            callback(x1, y1, x2, y2, class, prob);
        }
    }
}

// static void drawboxes(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t class, float prob)
// {
//     if (x1 >= 320)
//         x1 = 319;
//     if (x2 >= 320)
//         x2 = 319;
//     if (y1 >= 240)
//         y1 = 239;
//     if (y2 >= 240)
//         y2 = 239;

//     lcd_draw_rectangle(x1, y1, x2, y2, 2, RED);
// }

/* end of region_layer.c*/
