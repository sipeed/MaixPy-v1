#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "py_kpu.h"
#include "w25qxx.h"
#include "lcd.h"

#include <mp.h>
#include "mpconfigboard.h"

#include "imlib.h"
#include "py_assert.h"
#include "py_helper.h"

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

int region_layer_init(region_layer_t *rl, kpu_task_t *task)
{
    int flag = 0;
    kpu_layer_argument_t *last_layer = &task->layers[task->layers_length - 1];
    kpu_layer_argument_t *first_layer = &task->layers[0];

    rl->coords = 4;
    rl->image_width = 320;
    rl->image_height = 240;

    rl->classes = (last_layer->image_channel_num.data.o_ch_num + 1) / 5 - 5;
    rl->net_width = first_layer->image_size.data.i_row_wid + 1;
    rl->net_height = first_layer->image_size.data.i_col_high + 1;
    rl->layer_width = last_layer->image_size.data.o_row_wid + 1;
    rl->layer_height = last_layer->image_size.data.o_col_high + 1;
    rl->boxes_number = (rl->layer_width * rl->layer_height * rl->anchor_number);
    rl->output_number = (rl->boxes_number * (rl->classes + rl->coords + 1));
    rl->input = task->dst;
    rl->scale = task->output_scale;
    rl->bias = task->output_bias;

    rl->output = malloc(rl->output_number * sizeof(float));
    if (rl->output == NULL)
    {
        flag = -1;
        goto malloc_error;
    }
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
    rl->activate = malloc(256 * sizeof(float));
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
    }
    for (uint32_t i = 0; i < rl->boxes_number; i++)
        rl->probs[i] = &(rl->probs_buf[i * (rl->classes + 1)]);
    return 0;
malloc_error:
    free(rl->output);
    free(rl->boxes);
    free(rl->probs_buf);
    free(rl->probs);
    free(rl->activate);
    free(rl->softmax);
    return flag;
}

void region_layer_deinit(region_layer_t *rl)
{
    free(rl->output);
    free(rl->boxes);
    free(rl->probs_buf);
    free(rl->probs);
    free(rl->activate);
    free(rl->softmax);
}

static void activate_array(region_layer_t *rl, int index, int n)
{
    float *output = &rl->output[index];
    uint8_t *input = &rl->input[index];

    for (int i = 0; i < n; ++i)
        output[i] = rl->activate[input[i]];
}

static int entry_index(region_layer_t *rl, int location, int entry)
{
    int wh = rl->layer_width * rl->layer_height;
    int n = location / wh;
    int loc = location % wh;

    return n * wh * (rl->coords + rl->classes + 1) + entry * wh + loc;
}

static void softmax(region_layer_t *rl, uint8_t *input, int n, int stride, float *output)
{
    int i;
    int diff;
    float e;
    float sum = 0;
    uint8_t largest_i = input[0];

    for (i = 0; i < n; ++i)
    {
        if (input[i * stride] > largest_i)
            largest_i = input[i * stride];
    }

    for (i = 0; i < n; ++i)
    {
        diff = input[i * stride] - largest_i;
        e = rl->softmax[diff + 255];
        sum += e;
        output[i * stride] = e;
    }
    for (i = 0; i < n; ++i)
        output[i * stride] /= sum;
}

static void softmax_cpu(region_layer_t *rl, uint8_t *input, int n, int batch, int batch_offset, int groups, int stride, float *output)
{
    int g, b;

    for (b = 0; b < batch; ++b)
    {
        for (g = 0; g < groups; ++g)
            softmax(rl, input + b * batch_offset + g, n, stride, output + b * batch_offset + g);
    }
}

static void forward_region_layer(region_layer_t *rl)
{
    int index;

    for (index = 0; index < rl->output_number; index++)
        rl->output[index] = rl->input[index] * rl->scale + rl->bias;

    for (int n = 0; n < rl->anchor_number; ++n)
    {
        index = entry_index(rl, n * rl->layer_width * rl->layer_height, 0);
        activate_array(rl, index, 2 * rl->layer_width * rl->layer_height);
        index = entry_index(rl, n * rl->layer_width * rl->layer_height, 4);
        activate_array(rl, index, rl->layer_width * rl->layer_height);
    }

    index = entry_index(rl, 0, rl->coords + 1);
    softmax_cpu(rl, rl->input + index, rl->classes, rl->anchor_number,
                rl->output_number / rl->anchor_number, rl->layer_width * rl->layer_height,
                rl->layer_width * rl->layer_height, rl->output + index);
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

static int nms_comparator(void *pa, void *pb)
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
    uint32_t boxes_number = rl->boxes_number;
    float threshold = rl->threshold;
    box_t *boxes = (box_t *)rl->boxes;

    for (int i = 0; i < rl->boxes_number; ++i)
    {
        int class = max_index(rl->probs[i], rl->classes);
        float prob = rl->probs[i][class];

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

typedef void (*callback_draw_box)(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t class, float prob);

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

static void drawboxes(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t class, float prob)
{
    if (x1 >= 320)
        x1 = 319;
    if (x2 >= 320)
        x2 = 319;
    if (y1 >= 240)
        y1 = 239;
    if (y2 >= 240)
        y2 = 239;

    lcd_draw_rectangle(x1, y1, x2, y2, 2, RED);
}

/* end of region_layer.c*/

#define ANCHOR_NUM 5

static float g_anchor[ANCHOR_NUM * 2] = {1.889, 2.5245, 2.9465, 3.94056, 3.99987, 5.3658, 5.155437, 6.92275, 6.718375, 9.01025};

#define KMODEL_SIZE (380 * 1024)

#define AAA 1

#if AAA
static uint8_t *model_data = NULL;
#else
static uint8_t model_data[KMODEL_SIZE];
#endif

static kpu_task_t mpy_kpu_task;
static region_layer_t mpy_kpu_detect_rl;
static py_kpu_class_region_layer_arg_t rl_arg;

volatile static uint8_t g_ai_done_flag = 0;
volatile static uint8_t model_load_flag = 0, kpu_init_flag = 0;

static void ai_done(void *ctx)
{
    g_ai_done_flag = 1;
}

// 需要支持指定模型的存储地址
static int mpy_kpu_load(uint32_t model_addr,uint32_t model_size)
{
    if (model_data == NULL)
        model_data = (uint8_t *)malloc(model_size * sizeof(uint8_t));

    w25qxx_status_t status = w25qxx_read_data_dma(model_addr, model_data, model_size, W25QXX_QUAD_FAST);

    if(status != W25QXX_OK)
            return -1;
    
    int ret = kpu_model_load_from_buffer(&mpy_kpu_task, model_data, NULL);

    model_load_flag = 1;

    return ret;
}

static int mpy_kpu_task_init(uint8_t *rgb888_buf)
{
    if (model_load_flag)
    {
        mpy_kpu_task.src = rgb888_buf;
        mpy_kpu_task.dma_ch = 5;
        mpy_kpu_task.callback = ai_done;
        kpu_single_task_init(&mpy_kpu_task);

        mpy_kpu_detect_rl.anchor_number = rl_arg.anchor_number;
        mpy_kpu_detect_rl.anchor = rl_arg.anchor;
        mpy_kpu_detect_rl.threshold = rl_arg.threshold;
        mpy_kpu_detect_rl.nms_value = rl_arg.nms_value;
        region_layer_init(&mpy_kpu_detect_rl, &mpy_kpu_task);

        // printf("anchor_number:%d\nanchor:%x\nthreshold:%f\nnms_value:%f\n",
                                                                    //    rl_arg.anchor_number,
                                                                    //    rl_arg.anchor,
                                                                    //    rl_arg.threshold,
                                                                    //    rl_arg.nms_value);
        return 1;
    }
    else
    {
        printf("please load model !!!\r\n");
        return 0;
    }
}

static int mpy_kpu_run(obj_info_t *mpy_kpu_detect_info)
{
    /* starat to calculate */
    kpu_start(&mpy_kpu_task);
    while (!g_ai_done_flag)
        ;
    g_ai_done_flag = 0;
    /* start region layer */
    region_layer_run(&mpy_kpu_detect_rl, mpy_kpu_detect_info);
    return 1;
}

static int mpy_kpu_deint(int free_all)
{
    kpu_single_task_deinit(&mpy_kpu_task);
    region_layer_deinit(&mpy_kpu_detect_rl);

    if (free_all && model_load_flag && kpu_init_flag)
    {
        free(model_data);
        model_data = NULL;
        model_load_flag = 0;

        free(rl_arg.anchor);
        rl_arg.anchor = NULL;
        kpu_init_flag = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void py_kpu_class_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_kpu_class_obj_t *self = self_in;
    mp_printf(print,
              "{\"x\":%d, \"y\":%d, \"w\":%d, \"h\":%d, \"classid\":%d, \"index\":%d, \"value\":%f, \"objnum\":%d}",
              mp_obj_get_int(self->x),
              mp_obj_get_int(self->y),
              mp_obj_get_int(self->w),
              mp_obj_get_int(self->h),
              mp_obj_get_int(self->classid),
              mp_obj_get_int(self->index),
              (double)mp_obj_get_float(self->value),
              mp_obj_get_int(self->objnum));
}

static mp_obj_t py_kpu_class_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    // if (value == MP_OBJ_SENTINEL) { // load
    //     py_kpu_class_obj_t *self = self_in;
    //     if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
    //         mp_bound_slice_t slice;
    //         if (!mp_seq_get_fast_slice_indexes(py_kpu_class_obj_size, index, &slice)) {
    //             nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "only slices with step=1 (aka None) are supported"));
    //         }
    //         mp_obj_tuple_t *result = mp_obj_new_tuple(slice.stop - slice.start, NULL);
    //         mp_seq_copy(result->items, &(self->x) + slice.start, result->len, mp_obj_t);
    //         return result;
    //     }
    //     switch (mp_get_index(self->base.type, py_kpu_class_obj_size, index, false)) {
    //         case 0: return self->x1;
    //         case 1: return self->y1;
    //         case 2: return self->x2;
    //         case 3: return self->y2;
    //         case 4: return self->index;
    //         case 5: return self->value;
    //     }
    // }
    return MP_OBJ_NULL; // op not supported
}

mp_obj_t py_kpu_class_rect(mp_obj_t self_in)
{
    return mp_obj_new_tuple(4, (mp_obj_t[]){((py_kpu_class_obj_t *)self_in)->x,
                                            ((py_kpu_class_obj_t *)self_in)->y,
                                            ((py_kpu_class_obj_t *)self_in)->w,
                                            ((py_kpu_class_obj_t *)self_in)->h});
}

mp_obj_t py_kpu_class_x(mp_obj_t self_in) { return ((py_kpu_class_obj_t *)self_in)->x; }
mp_obj_t py_kpu_class_y(mp_obj_t self_in) { return ((py_kpu_class_obj_t *)self_in)->y; }
mp_obj_t py_kpu_class_w(mp_obj_t self_in) { return ((py_kpu_class_obj_t *)self_in)->w; }
mp_obj_t py_kpu_class_h(mp_obj_t self_in) { return ((py_kpu_class_obj_t *)self_in)->h; }
mp_obj_t py_kpu_class_classid(mp_obj_t self_in) { return ((py_kpu_class_obj_t *)self_in)->classid; }
mp_obj_t py_kpu_class_index(mp_obj_t self_in) { return ((py_kpu_class_obj_t *)self_in)->index; }
mp_obj_t py_kpu_class_value(mp_obj_t self_in) { return ((py_kpu_class_obj_t *)self_in)->value; }
mp_obj_t py_kpu_class_objnum(mp_obj_t self_in) { return ((py_kpu_class_obj_t *)self_in)->objnum; }

static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_class_rect_obj, py_kpu_class_rect);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_class_x_obj, py_kpu_class_x);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_class_y_obj, py_kpu_class_y);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_class_w_obj, py_kpu_class_w);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_class_h_obj, py_kpu_class_h);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_class_classid_obj, py_kpu_class_classid);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_class_index_obj, py_kpu_class_index);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_class_value_obj, py_kpu_class_value);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_class_objnum_obj, py_kpu_class_objnum);

static const mp_rom_map_elem_t py_kpu_class_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_rect),        MP_ROM_PTR(&py_kpu_class_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_x),          MP_ROM_PTR(&py_kpu_class_x_obj) },
    { MP_ROM_QSTR(MP_QSTR_y),          MP_ROM_PTR(&py_kpu_class_y_obj) },
    { MP_ROM_QSTR(MP_QSTR_w),          MP_ROM_PTR(&py_kpu_class_w_obj) },
    { MP_ROM_QSTR(MP_QSTR_h),          MP_ROM_PTR(&py_kpu_class_h_obj) },
    { MP_ROM_QSTR(MP_QSTR_classid),     MP_ROM_PTR(&py_kpu_class_classid_obj) },
    { MP_ROM_QSTR(MP_QSTR_index),       MP_ROM_PTR(&py_kpu_class_index_obj) },
    { MP_ROM_QSTR(MP_QSTR_value),       MP_ROM_PTR(&py_kpu_class_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_objnum),      MP_ROM_PTR(&py_kpu_class_objnum_obj) }
};

static MP_DEFINE_CONST_DICT(py_kpu_class_locals_dict, py_kpu_class_locals_dict_table);

static const mp_obj_type_t py_kpu_class_type = {
    { &mp_type_type },
    .name  = MP_QSTR_kpu_class,
    .print = py_kpu_class_print,
    // .subscr = py_kpu_class_subscr,
    .locals_dict = (mp_obj_t) &py_kpu_class_locals_dict
};

static mp_obj_t py_kpu_class_run(uint n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_image_cobj(args[0]);
    PY_ASSERT_TRUE_MSG(IM_IS_MUTABLE(arg_img), "Image format is not supported.");

    if (arg_img->pix_ai == NULL)
    {
        printf("pix_ai or pixels is NULL!\n");
        return mp_const_false;
    }
    if (mpy_kpu_task_init(arg_img->pix_ai) == 0)
        return mp_const_false;

    static obj_info_t mpy_kpu_detect_info;
    mpy_kpu_run(&mpy_kpu_detect_info);

    uint8_t obj_num = 0;
    obj_num = mpy_kpu_detect_info.obj_number;

    if (obj_num > 0)
    {
        list_t out;
        list_init(&out, sizeof(py_kpu_class_list_link_data_t));

        // fb_alloc_mark();

        for (uint8_t index = 0; index < obj_num; index++)
        {
            py_kpu_class_list_link_data_t lnk_data;
            lnk_data.rect.x = mpy_kpu_detect_info.obj[index].x1;
            lnk_data.rect.y = mpy_kpu_detect_info.obj[index].y1;
            lnk_data.rect.w = mpy_kpu_detect_info.obj[index].x2 - mpy_kpu_detect_info.obj[index].x1;
            lnk_data.rect.h = mpy_kpu_detect_info.obj[index].y2 - mpy_kpu_detect_info.obj[index].y1;
            lnk_data.classid = mpy_kpu_detect_info.obj[index].classid;
            lnk_data.value = mpy_kpu_detect_info.obj[index].prob;

            lnk_data.index = index;
            lnk_data.objnum = obj_num;
            list_push_back(&out, &lnk_data);
            
            // printf("index:%d\r\nx1:%d y1:%d x2:%d y2:%d class_id:%d prob:%f\r\n",
            //        index,
            //        mpy_kpu_detect_info.obj[index].x1,
            //        mpy_kpu_detect_info.obj[index].y1,
            //        mpy_kpu_detect_info.obj[index].x2,
            //        mpy_kpu_detect_info.obj[index].y2,
            //        mpy_kpu_detect_info.obj[index].classid,
            //        mpy_kpu_detect_info.obj[index].prob);
        }

        mp_obj_list_t *objects_list = mp_obj_new_list(list_size(&out), NULL);

        for (size_t i = 0; list_size(&out); i++)
        {
            py_kpu_class_list_link_data_t lnk_data;
            list_pop_front(&out, &lnk_data);

            py_kpu_class_obj_t *o = m_new_obj(py_kpu_class_obj_t);

            o->base.type = &py_kpu_class_type;

            o->x = mp_obj_new_int(lnk_data.rect.x);
            o->y = mp_obj_new_int(lnk_data.rect.y);
            o->w = mp_obj_new_int(lnk_data.rect.w);
            o->h = mp_obj_new_int(lnk_data.rect.h);
            o->classid = mp_obj_new_int(lnk_data.classid);
            o->index = mp_obj_new_int(lnk_data.index);
            o->value = mp_obj_new_float(lnk_data.value);
            o->objnum = mp_obj_new_int(lnk_data.objnum);

            objects_list->items[i] = o;
        }
        //    fb_alloc_free_till_mark();
        return objects_list;
    }
    else
    {
        return mp_const_none;
    }
    return mp_const_true;
}

///////////////////////////////////////////////////////////////////////////////

static mp_obj_t py_kpu_class_load(uint n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    const char *path = NULL;

    enum { ARG_addr, ARG_size, ARG_path};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_addr, MP_ARG_INT, {.u_int = 0x0} },
        { MP_QSTR_size, MP_ARG_INT, {.u_int = 0x0} },
        { MP_QSTR_path, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} }
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    printf("addr: %d\r\n",args[ARG_addr].u_int);
    printf("size: %d\r\n",args[ARG_size].u_int);
    if(args[ARG_path].u_obj != mp_const_none )
        printf("path: %s\r\n",mp_obj_str_get_str(args[ARG_path].u_obj));

    //现在只从flash读取模型，之后要支持从文件系统加载
    int ret = mpy_kpu_load(args[ARG_addr].u_int,args[ARG_size].u_int);
    if(ret == 0)
        return mp_const_true;
    else
    {
        char msg[50];
        sprintf(msg,"[MAIXPY]kpu: error at load model with %d",ret);
        mp_raise_ValueError(msg);
        return mp_const_false;
    }
}

static mp_obj_t py_kpu_init(uint n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_threshold, ARG_nms_value, ARG_anchor_number, ARG_anchor};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_threshold,            MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_nms_value,            MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_anchor_number,        MP_ARG_INT, {.u_int = 0x0}           },
        { MP_QSTR_anchor,               MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    float threshold, nms_value, *anchor = NULL;
    int anchor_number;

    threshold = mp_obj_get_float(args[ARG_threshold].u_obj);
    if(!(threshold >= 0.0 && threshold <= 1.0))
    {
        mp_raise_ValueError("[MAIXPY]kpu: threshold only support 0 to 1");
        return mp_const_false;
    }

    nms_value = mp_obj_get_float(args[ARG_nms_value].u_obj);
    if(!(nms_value >= 0.0 && nms_value <= 1.0))
    {
        mp_raise_ValueError("[MAIXPY]kpu: nms_value only support 0 to 1");
        return mp_const_false;
    }

    anchor_number = args[ARG_anchor_number].u_int;

    printf("threshold: %f\n", threshold);
    printf("nms_value: %f\n", nms_value);
    printf("anchor_number: %d\n", anchor_number);
    // printf("ARG_anchor: %s\n",mp_obj_get_type_str(args[ARG_anchor].u_obj));

    if(anchor_number > 0)
    {
        anchor = (float*)malloc(anchor_number * 2 * sizeof(float));

        mp_obj_t *items;
        mp_obj_get_array_fixed_n(args[ARG_anchor].u_obj, args[ARG_anchor_number].u_int*2, &items);
    
        for(uint8_t index; index < args[ARG_anchor_number].u_int*2; index++)
        {
            float tmp = mp_obj_get_float(items[index]);
            anchor[index] = tmp;
            printf("item[%d]: %f\n",index, tmp);
        }
    }
    else
    {
        mp_raise_ValueError("[MAIXPY]kpu: anchor_number should > 0");
        return mp_const_false;
    }
    
    rl_arg.threshold = threshold;
    rl_arg.nms_value = nms_value;
    rl_arg.anchor_number = anchor_number;    
    rl_arg.anchor = anchor;

    kpu_init_flag = 1;

    return mp_const_true;
}

static mp_obj_t py_kpu_class_deinit(uint n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    if(!MP_OBJ_IS_INT(pos_args[0]))
        mp_raise_ValueError("[MAIXPY]kpu: deinit only support 0 or 1");

    int free_all = mp_obj_get_int(pos_args[0]);

    mpy_kpu_deint(free_all);

    return mp_const_true;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_kpu_load_obj, 2, py_kpu_class_load);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_kpu_init_obj, 4, py_kpu_init);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_kpu_run_obj, 1, py_kpu_class_run);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_kpu_deinit_obj, 0, py_kpu_class_deinit);

static const mp_map_elem_t globals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__),                    MP_OBJ_NEW_QSTR(MP_QSTR_kpu) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_load),                        (mp_obj_t)&py_kpu_load_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_init),                        (mp_obj_t)&py_kpu_init_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_run),                         (mp_obj_t)&py_kpu_run_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_deinit),                      (mp_obj_t)&py_kpu_deinit_obj },
    { NULL, NULL },
};
STATIC MP_DEFINE_CONST_DICT(globals_dict, globals_dict_table);

const mp_obj_module_t kpu_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_t)&globals_dict,
};
