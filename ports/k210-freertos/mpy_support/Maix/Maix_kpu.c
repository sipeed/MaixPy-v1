#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "sipeed_yolo2.h"

#include "w25qxx.h"
#include "lcd.h"

#include <mp.h>
#include "mpconfigboard.h"

#include "imlib.h"
#include "py_assert.h"
#include "py_helper.h"

///////////////////////////////////////////////////////////////////////////////

#define _D  do{printf("%s --> %d\r\n",__func__,__LINE__);}while(0)

typedef struct py_kpu_net_obj
{
    mp_obj_base_t base;
    
    mp_obj_t        model_data;
    mp_obj_t        model_addr;
    mp_obj_t        model_size;
    mp_obj_t        model_path;
    mp_obj_t        kpu_task;
    mp_obj_t        net_args;
    mp_obj_t        net_deinit;
} __attribute__((aligned(8))) py_kpu_net_obj_t;

static void py_kpu_net_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_kpu_net_obj_t *self = self_in;

    //XXXX did not print all 
    mp_printf(print,
              "{\"model_data\":%x, \"model_addr\":%x, \"model_size\":%x, \"model_path\": \"\"}",
                mp_obj_new_int(MP_OBJ_TO_PTR(((py_kpu_net_obj_t *)self_in)->model_data)),
                mp_obj_get_int(self->model_addr),
                mp_obj_get_int(self->model_size)
                );
}

mp_obj_t py_kpu_net_model_data(mp_obj_t self_in) { return mp_obj_new_int(MP_OBJ_TO_PTR(((py_kpu_net_obj_t *)self_in)->model_data)); }
mp_obj_t py_kpu_net_model_addr(mp_obj_t self_in) { return ((py_kpu_net_obj_t *)self_in)->model_addr; }
mp_obj_t py_kpu_net_model_size(mp_obj_t self_in) { return ((py_kpu_net_obj_t *)self_in)->model_size; }
mp_obj_t py_kpu_net_model_path(mp_obj_t self_in) { return ((py_kpu_net_obj_t *)self_in)->model_path; }
mp_obj_t py_kpu_net_arg(mp_obj_t self_in)        { return ((py_kpu_net_obj_t *)self_in)->net_args; }

static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_net_model_data_obj,     py_kpu_net_model_data);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_net_model_addr_obj,     py_kpu_net_model_addr);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_net_model_size_obj,     py_kpu_net_model_size);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_net_model_path_obj,     py_kpu_net_model_path);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_net_arg_obj,            py_kpu_net_arg);

static const mp_rom_map_elem_t py_kpu_net_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_model_data),      MP_ROM_PTR(&py_kpu_net_model_data_obj) },
    { MP_ROM_QSTR(MP_QSTR_model_addr),      MP_ROM_PTR(&py_kpu_net_model_addr_obj) },
    { MP_ROM_QSTR(MP_QSTR_model_size),      MP_ROM_PTR(&py_kpu_net_model_size_obj) },
    { MP_ROM_QSTR(MP_QSTR_model_path),      MP_ROM_PTR(&py_kpu_net_model_path_obj) },
    { MP_ROM_QSTR(MP_QSTR_net_arg),         MP_ROM_PTR(&py_kpu_net_arg_obj) }
};

static MP_DEFINE_CONST_DICT(py_kpu_net_dict, py_kpu_net_dict_table);

static const mp_obj_type_t py_kpu_net_obj_type = {
    { &mp_type_type },
    .name  = MP_QSTR_kpu_net,
    .print = py_kpu_net_obj_print,
    .locals_dict = (mp_obj_t) &py_kpu_net_dict
};

STATIC mp_obj_t py_kpu_class_load(uint n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    mp_int_t addr,size;

    const char *path = NULL;

    enum { ARG_addr, ARG_size, ARG_path};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_addr, MP_ARG_INT, {.u_int = 0x0} },
        { MP_QSTR_size, MP_ARG_INT, {.u_int = 0x0} },
        { MP_QSTR_path, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} }
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if(args[ARG_path].u_obj != mp_const_none)
    {
        //load from file system
        mp_raise_ValueError("[MAIXPY]kpu: not support load model from file just now");
        return mp_const_false;
    }
    else
    {
        addr = args[ARG_addr].u_int;
        size = args[ARG_size].u_int;

        if(addr < 0 || size <= 0)
        {
            mp_raise_ValueError("[MAIXPY]kpu: addr and size must > 0 ");
            return mp_const_false;
        }

        py_kpu_net_obj_t  *o = m_new_obj(py_kpu_net_obj_t);

        o->base.type = &py_kpu_net_obj_type;

        o->model_data = MP_OBJ_FROM_PTR(malloc(size * sizeof(uint8_t)));
        if(MP_OBJ_TO_PTR(o->model_data) == NULL)
            goto malloc_err;

        o->model_addr = mp_obj_new_int(addr);
        o->model_size = mp_obj_new_int(size);
        o->model_path = mp_const_none;
        o->kpu_task = MP_OBJ_FROM_PTR(malloc(sizeof(kpu_task_t)));
        if(MP_OBJ_TO_PTR(o->kpu_task) == NULL)
            goto malloc_err;

        o->net_args = mp_const_none;
        o->net_deinit = mp_const_none;

        w25qxx_status_t status = w25qxx_read_data_dma(addr, MP_OBJ_TO_PTR(o->model_data), size, W25QXX_QUAD_FAST);
        if(status != W25QXX_OK)
            goto read_err;

        int ret = kpu_model_load_from_buffer(MP_OBJ_TO_PTR(o->kpu_task), MP_OBJ_TO_PTR(o->model_data), NULL);
        if(ret != 0)
            goto load_err;

        return MP_OBJ_FROM_PTR(o);

    uint64_t *tmp = NULL;

malloc_err:
    tmp = MP_OBJ_TO_PTR(o->model_data);
    if(tmp)
        free(tmp);
    tmp = MP_OBJ_TO_PTR(o->kpu_task);
    if(tmp)
        free(tmp);

    mp_raise_ValueError("[MAIXPY]kpu: malloc error");
    return mp_const_false;

read_err:
    tmp = MP_OBJ_TO_PTR(o->model_data);
    if(tmp)
        free(tmp);
    tmp = MP_OBJ_TO_PTR(o->kpu_task);
    if(tmp)
        free(tmp);

    mp_raise_ValueError("[MAIXPY]kpu: read model error");
    return mp_const_false;

load_err:
    tmp = MP_OBJ_TO_PTR(o->model_data);
    if(tmp)
        free(tmp);
    tmp = MP_OBJ_TO_PTR(o->kpu_task);
    if(tmp)
        free(tmp);

    mp_raise_ValueError("[MAIXPY]kpu: load model error");
    return mp_const_false;
    }
    return mp_const_false;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_kpu_class_load_obj, 2, py_kpu_class_load);

///////////////////////////////////////////////////////////////////////////////

typedef struct py_kpu_class_yolo_args_obj {
    mp_obj_base_t base;

    mp_obj_t threshold, nms_value, anchor_number, anchor, rl_args;
} __attribute__((aligned(8))) py_kpu_class_yolo_args_obj_t;

typedef struct py_kpu_class_region_layer_arg
{
    float threshold;
    float nms_value;
    int anchor_number;
    float *anchor;
}__attribute__((aligned(8))) py_kpu_class_yolo_region_layer_arg_t;

static void py_kpu_class_yolo2_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_kpu_class_yolo_args_obj_t *yolo_args = self_in;

    py_kpu_class_yolo_region_layer_arg_t *rl_arg = yolo_args->rl_args;

    char msg[300];

    uint8_t num = mp_obj_get_int(rl_arg->anchor_number);

    if(num>0)
    {
        sprintf(msg,"%f",rl_arg->anchor[0]);
        for(uint16_t i = 1; i < num * 2; i++)
            sprintf(msg,"%s, %f",msg, rl_arg->anchor[i]);
    }

    mp_printf(print,
              "{\"threshold\":%f, \"nms_value\":%f, \"anchor_number\":%d, \"anchor\":(%s)}",
                rl_arg->threshold,
                rl_arg->nms_value,
                rl_arg->anchor_number,
                msg);
}

mp_obj_t py_kpu_calss_yolo2_anchor(mp_obj_t self_in);

mp_obj_t py_kpu_calss_yolo2_threshold(mp_obj_t self_in) { return ((py_kpu_class_yolo_args_obj_t *)self_in)->threshold; }
mp_obj_t py_kpu_calss_yolo2_nms_value(mp_obj_t self_in) { return ((py_kpu_class_yolo_args_obj_t *)self_in)->nms_value; }
mp_obj_t py_kpu_calss_yolo2_anchor_number(mp_obj_t self_in) { return ((py_kpu_class_yolo_args_obj_t *)self_in)->anchor_number; }

static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_calss_yolo2_anchor_obj,          py_kpu_calss_yolo2_anchor);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_calss_yolo2_threshold_obj,       py_kpu_calss_yolo2_threshold);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_calss_yolo2_nms_value_obj,       py_kpu_calss_yolo2_nms_value);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_calss_yolo2_anchor_number_obj,   py_kpu_calss_yolo2_anchor_number);

static const mp_rom_map_elem_t py_kpu_class_yolo2_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_anchor),              MP_ROM_PTR(&py_kpu_calss_yolo2_anchor_obj) },
    { MP_ROM_QSTR(MP_QSTR_threshold),           MP_ROM_PTR(&py_kpu_calss_yolo2_threshold_obj) },
    { MP_ROM_QSTR(MP_QSTR_nms_value),           MP_ROM_PTR(&py_kpu_calss_yolo2_nms_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_anchor_number),       MP_ROM_PTR(&py_kpu_calss_yolo2_anchor_number_obj) }
};

static MP_DEFINE_CONST_DICT(py_kpu_class_yolo2_dict, py_kpu_class_yolo2_dict_table);

static const mp_obj_type_t py_kpu_class_yolo_args_obj_type = {
    { &mp_type_type },
    .name  = MP_QSTR_kpu_yolo2,
    .print = py_kpu_class_yolo2_print,
    // .subscr = py_kpu_calss_yolo2_subscr,
    .locals_dict = (mp_obj_t) &py_kpu_class_yolo2_dict
};

mp_obj_t py_kpu_calss_yolo2_anchor(mp_obj_t self_in)
{
    if(mp_obj_get_type(self_in) == &py_kpu_class_yolo_args_obj_type)
    {
        py_kpu_class_yolo_args_obj_t *yolo_args = MP_OBJ_TO_PTR(self_in);

        py_kpu_class_yolo_region_layer_arg_t *rl_arg = yolo_args->rl_args;

        mp_obj_t *tuple, *tmp;

        tmp = (mp_obj_t *)malloc(rl_arg->anchor_number * 2 * sizeof(mp_obj_t));

        for (uint8_t index = 0; index < rl_arg->anchor_number * 2; index++)
            tmp[index] = mp_obj_new_float(rl_arg->anchor[index]);

        tuple = mp_obj_new_tuple(rl_arg->anchor_number * 2, tmp);

        free(tmp);
        return tuple;
    }
    else
    {
        mp_raise_TypeError("[MAIXPY]kpu: object type error");
        return mp_const_false;
    }
}

mp_obj_t py_kpu_calss_yolo2_deinit(mp_obj_t self_in)
{
    if(mp_obj_get_type(self_in) == &py_kpu_class_yolo_args_obj_type)
    {
        py_kpu_class_yolo_args_obj_t *yolo_args = MP_OBJ_TO_PTR(self_in);

        py_kpu_class_yolo_region_layer_arg_t *rl_arg = yolo_args->rl_args;

        if(rl_arg->anchor)
            free(rl_arg->anchor);

        if(rl_arg)
            free(rl_arg);
        return mp_const_true;
    }
    else
    {
        mp_raise_TypeError("[MAIXPY]kpu: object type error");
        return mp_const_false;
    }
}

STATIC mp_obj_t py_kpu_class_init_yolo2(uint n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_kpu_net, ARG_threshold, ARG_nms_value, ARG_anchor_number, ARG_anchor};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_kpu_net,              MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_threshold,            MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_nms_value,            MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_anchor_number,        MP_ARG_INT, {.u_int = 0x0}           },
        { MP_QSTR_anchor,               MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if(mp_obj_get_type(args[ARG_kpu_net].u_obj) == &py_kpu_net_obj_type)
    {
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

        if(anchor_number > 0)
        {
            //need free
            anchor = (float*)malloc(anchor_number * 2 * sizeof(float));

            mp_obj_t *items;
            mp_obj_get_array_fixed_n(args[ARG_anchor].u_obj, args[ARG_anchor_number].u_int*2, &items);
        
            for(uint8_t index; index < args[ARG_anchor_number].u_int * 2; index++)
                anchor[index] = mp_obj_get_float(items[index]);
        }
        else
        {
            mp_raise_ValueError("[MAIXPY]kpu: anchor_number should > 0");
            return mp_const_false;
        }

        py_kpu_class_yolo_args_obj_t *yolo_args = m_new_obj(py_kpu_class_yolo_args_obj_t);

        yolo_args->base.type = &py_kpu_class_yolo_args_obj_type;

        yolo_args->threshold = mp_obj_new_float(threshold);
        yolo_args->nms_value = mp_obj_new_float(nms_value);
        yolo_args->anchor_number = mp_obj_new_int(anchor_number);

        mp_obj_t *tuple, *tmp;

        tmp = (mp_obj_t *)malloc(anchor_number * 2 * sizeof(mp_obj_t));

        for (uint8_t index = 0; index < anchor_number * 2; index++)
            tmp[index] = mp_obj_new_float(anchor[index]);

        tuple = mp_obj_new_tuple(anchor_number * 2, tmp);

        free(tmp);

        yolo_args->anchor = tuple;

        //need free
        py_kpu_class_yolo_region_layer_arg_t *rl_arg = malloc(sizeof(py_kpu_class_yolo_region_layer_arg_t));

        rl_arg->threshold = threshold;
        rl_arg->nms_value = nms_value;
        rl_arg->anchor_number = anchor_number;
        rl_arg->anchor = anchor;

        yolo_args->rl_args = MP_OBJ_FROM_PTR(rl_arg);

        py_kpu_net_obj_t *kpu_net = MP_OBJ_TO_PTR(args[ARG_kpu_net].u_obj);

        kpu_net->net_args = MP_OBJ_FROM_PTR(yolo_args);

        kpu_net->net_deinit = MP_OBJ_FROM_PTR(py_kpu_calss_yolo2_deinit);

        return mp_const_true;
    }
    else
    {
        mp_raise_ValueError("[MAIXPY]kpu: kpu_net type error");
        return mp_const_false;
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_kpu_class_init_yolo2_obj, 5, py_kpu_class_init_yolo2);

///////////////////////////////////////////////////////////////////////////////

typedef struct py_kpu_class_yolo2_find_obj {
    mp_obj_base_t base;

    mp_obj_t x, y, w, h, classid, index, value, objnum;
} __attribute__((aligned(8))) py_kpu_class_yolo2_find_obj_t;

typedef struct py_kpu_class_list_link_data {
    rectangle_t rect;
    int classid;
    float value;
    int index;
    int objnum;
} __attribute__((aligned(8))) py_kpu_class_yolo2__list_link_data_t;

static void py_kpu_class_yolo2_find_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_kpu_class_yolo2_find_obj_t *self = self_in;
    mp_printf(print,
              "{\"x\":%d, \"y\":%d, \"w\":%d, \"h\":%d, \"value\":%f, \"classid\":%d, \"index\":%d, \"objnum\":%d}",
              mp_obj_get_int(self->x),
              mp_obj_get_int(self->y),
              mp_obj_get_int(self->w),
              mp_obj_get_int(self->h),
              (double)mp_obj_get_float(self->value),
              mp_obj_get_int(self->classid),
              mp_obj_get_int(self->index),
              mp_obj_get_int(self->objnum));
}

mp_obj_t py_kpu_class_yolo2_find_rect(mp_obj_t self_in)
{
    return mp_obj_new_tuple(4, (mp_obj_t[]){((py_kpu_class_yolo2_find_obj_t *)self_in)->x,
                                            ((py_kpu_class_yolo2_find_obj_t *)self_in)->y,
                                            ((py_kpu_class_yolo2_find_obj_t *)self_in)->w,
                                            ((py_kpu_class_yolo2_find_obj_t *)self_in)->h});
}

mp_obj_t py_kpu_class_yolo2_find_x(mp_obj_t self_in) { return ((py_kpu_class_yolo2_find_obj_t *)self_in)->x; }
mp_obj_t py_kpu_class_yolo2_find_y(mp_obj_t self_in) { return ((py_kpu_class_yolo2_find_obj_t *)self_in)->y; }
mp_obj_t py_kpu_class_yolo2_find_w(mp_obj_t self_in) { return ((py_kpu_class_yolo2_find_obj_t *)self_in)->w; }
mp_obj_t py_kpu_class_yolo2_find_h(mp_obj_t self_in) { return ((py_kpu_class_yolo2_find_obj_t *)self_in)->h; }
mp_obj_t py_kpu_class_yolo2_find_classid(mp_obj_t self_in) { return ((py_kpu_class_yolo2_find_obj_t *)self_in)->classid; }
mp_obj_t py_kpu_class_yolo2_find_index(mp_obj_t self_in) { return ((py_kpu_class_yolo2_find_obj_t *)self_in)->index; }
mp_obj_t py_kpu_class_yolo2_find_value(mp_obj_t self_in) { return ((py_kpu_class_yolo2_find_obj_t *)self_in)->value; }
mp_obj_t py_kpu_class_yolo2_find_objnum(mp_obj_t self_in) { return ((py_kpu_class_yolo2_find_obj_t *)self_in)->objnum; }

static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_class_yolo2_find_rect_obj, py_kpu_class_yolo2_find_rect);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_class_yolo2_find_x_obj, py_kpu_class_yolo2_find_x);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_class_yolo2_find_y_obj, py_kpu_class_yolo2_find_y);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_class_yolo2_find_w_obj, py_kpu_class_yolo2_find_w);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_class_yolo2_find_h_obj, py_kpu_class_yolo2_find_h);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_class_yolo2_find_classid_obj, py_kpu_class_yolo2_find_classid);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_class_yolo2_find_index_obj, py_kpu_class_yolo2_find_index);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_class_yolo2_find_value_obj, py_kpu_class_yolo2_find_value);
static MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_class_yolo2_find_objnum_obj, py_kpu_class_yolo2_find_objnum);

static const mp_rom_map_elem_t py_kpu_class_yolo2_find_type_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_rect),        MP_ROM_PTR(&py_kpu_class_yolo2_find_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_x),          MP_ROM_PTR(&py_kpu_class_yolo2_find_x_obj) },
    { MP_ROM_QSTR(MP_QSTR_y),          MP_ROM_PTR(&py_kpu_class_yolo2_find_y_obj) },
    { MP_ROM_QSTR(MP_QSTR_w),          MP_ROM_PTR(&py_kpu_class_yolo2_find_w_obj) },
    { MP_ROM_QSTR(MP_QSTR_h),          MP_ROM_PTR(&py_kpu_class_yolo2_find_h_obj) },
    { MP_ROM_QSTR(MP_QSTR_classid),     MP_ROM_PTR(&py_kpu_class_yolo2_find_classid_obj) },
    { MP_ROM_QSTR(MP_QSTR_index),       MP_ROM_PTR(&py_kpu_class_yolo2_find_index_obj) },
    { MP_ROM_QSTR(MP_QSTR_value),       MP_ROM_PTR(&py_kpu_class_yolo2_find_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_objnum),      MP_ROM_PTR(&py_kpu_class_yolo2_find_objnum_obj) }
};

static MP_DEFINE_CONST_DICT(py_kpu_class_yolo2_find_type_locals_dict, py_kpu_class_yolo2_find_type_locals_dict_table);

static const mp_obj_type_t py_kpu_class_yolo2_find_type = {
    { &mp_type_type },
    .name  = MP_QSTR_kpu_yolo2_find,
    .print = py_kpu_class_yolo2_find_print,
    // .subscr = py_kpu_class_subscr,
    .locals_dict = (mp_obj_t) &py_kpu_class_yolo2_find_type_locals_dict
};

volatile static uint8_t g_ai_done_flag = 0;

static void ai_done(void *ctx)
{
    g_ai_done_flag = 1;
}

STATIC mp_obj_t py_kpu_class_run_yolo2(uint n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    if(mp_obj_get_type(pos_args[0]) == &py_kpu_net_obj_type)
    {
        image_t *arg_img = py_image_cobj(pos_args[1]);
        PY_ASSERT_TRUE_MSG(IM_IS_MUTABLE(arg_img), "Image format is not supported.");

        if (arg_img->pix_ai == NULL)
        {
            printf("pix_ai or pixels is NULL!\n");
            return mp_const_false;
        }

        py_kpu_net_obj_t *kpu_net = MP_OBJ_TO_PTR(pos_args[0]);

        kpu_task_t *kpu_task = MP_OBJ_TO_PTR(kpu_net->kpu_task);
        
        g_ai_done_flag = 0;

        kpu_task->src = arg_img->pix_ai;
        kpu_task->dma_ch = 5;
        kpu_task->callback = ai_done;
        kpu_single_task_init(kpu_task);

        py_kpu_class_yolo_args_obj_t *net_args = MP_OBJ_TO_PTR(kpu_net->net_args);
        py_kpu_class_yolo_region_layer_arg_t *rl_arg = net_args->rl_args;

        region_layer_t kpu_detect_rl;

        kpu_detect_rl.anchor_number = rl_arg->anchor_number;
        kpu_detect_rl.anchor = rl_arg->anchor;
        kpu_detect_rl.threshold = rl_arg->threshold;
        kpu_detect_rl.nms_value = rl_arg->nms_value;
        region_layer_init(&kpu_detect_rl, kpu_task);

        static obj_info_t mpy_kpu_detect_info;

        /* starat to calculate */
        kpu_start(kpu_task);
        while (!g_ai_done_flag)
            ;
        g_ai_done_flag = 0;
        /* start region layer */
        region_layer_run(&kpu_detect_rl, &mpy_kpu_detect_info);

        uint8_t obj_num = 0;
        obj_num = mpy_kpu_detect_info.obj_number;

        if (obj_num > 0)
        {
            list_t out;
            list_init(&out, sizeof(py_kpu_class_yolo2__list_link_data_t));

            for (uint8_t index = 0; index < obj_num; index++)
            {
                py_kpu_class_yolo2__list_link_data_t lnk_data;
                lnk_data.rect.x = mpy_kpu_detect_info.obj[index].x1;
                lnk_data.rect.y = mpy_kpu_detect_info.obj[index].y1;
                lnk_data.rect.w = mpy_kpu_detect_info.obj[index].x2 - mpy_kpu_detect_info.obj[index].x1;
                lnk_data.rect.h = mpy_kpu_detect_info.obj[index].y2 - mpy_kpu_detect_info.obj[index].y1;
                lnk_data.classid = mpy_kpu_detect_info.obj[index].classid;
                lnk_data.value = mpy_kpu_detect_info.obj[index].prob;

                lnk_data.index = index;
                lnk_data.objnum = obj_num;
                list_push_back(&out, &lnk_data);
            }

            mp_obj_list_t *objects_list = mp_obj_new_list(list_size(&out), NULL);

            for (size_t i = 0; list_size(&out); i++)
            {
                py_kpu_class_yolo2__list_link_data_t lnk_data;
                list_pop_front(&out, &lnk_data);

                py_kpu_class_yolo2_find_obj_t *o = m_new_obj(py_kpu_class_yolo2_find_obj_t);

                o->base.type = &py_kpu_class_yolo2_find_type;

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
            kpu_single_task_deinit(kpu_task);
            region_layer_deinit(&kpu_detect_rl);

            return objects_list;
        }
        else
        {
            kpu_single_task_deinit(kpu_task);
            region_layer_deinit(&kpu_detect_rl);

            return mp_const_none;
        }
    }
    else
    {
        mp_raise_ValueError("[MAIXPY]kpu: kpu_net type error");
        return mp_const_false;
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_kpu_class_run_yolo2_obj, 2, py_kpu_class_run_yolo2);

///////////////////////////////////////////////////////////////////////////////

typedef void (*call_net_arg_deinit)(mp_obj_t o);

void call_deinit(call_net_arg_deinit call_back, mp_obj_t o)
{
    call_back(o);
}

STATIC mp_obj_t py_kpu_deinit(uint n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    if(mp_obj_get_type(pos_args[0]) == &py_kpu_net_obj_type)
    {
        py_kpu_net_obj_t *kpu_net = MP_OBJ_TO_PTR(pos_args[0]);

        if(MP_OBJ_TO_PTR(kpu_net->model_data))
            free(MP_OBJ_TO_PTR(kpu_net->model_data));

        if(MP_OBJ_TO_PTR(kpu_net->kpu_task))
            free(MP_OBJ_TO_PTR(kpu_net->kpu_task));

        if(MP_OBJ_TO_PTR(kpu_net->net_deinit))
        {
            call_deinit(MP_OBJ_TO_PTR(kpu_net->net_deinit),kpu_net->net_args);
        }
        return mp_const_true;
    }
    else
    {
        mp_raise_ValueError("[MAIXPY]kpu: kpu_net type error");
        return mp_const_false;
    }

}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_kpu_deinit_obj, 1, py_kpu_deinit);
///////////////////////////////////////////////////////////////////////////////

STATIC mp_obj_t py_kpu_forward(uint n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    return mp_const_true;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_kpu_forward_obj, 0, py_kpu_forward);
///////////////////////////////////////////////////////////////////////////////

static const mp_map_elem_t globals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__),                    MP_OBJ_NEW_QSTR(MP_QSTR_kpu) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_load),                        (mp_obj_t)&py_kpu_class_load_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_init_yolo2),                  (mp_obj_t)&py_kpu_class_init_yolo2_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_run_yolo2),                    (mp_obj_t)&py_kpu_class_run_yolo2_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_deinit),                      (mp_obj_t)&py_kpu_deinit_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_forward),                      (mp_obj_t)&py_kpu_forward_obj },
    { NULL, NULL },
};

STATIC MP_DEFINE_CONST_DICT(globals_dict, globals_dict_table);

const mp_obj_module_t kpu_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_t)&globals_dict,
};
