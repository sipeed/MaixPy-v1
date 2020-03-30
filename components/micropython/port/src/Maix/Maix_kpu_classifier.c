#include "mpconfig.h"
#include "global_config.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py_image.h"
#include "sipeed_kpu_classifier.h"
#include "Maix_kpu.h"

const mp_obj_type_t Maix_kpu_classifier_type;

typedef struct {
    mp_obj_base_t         base;
    void* obj;
    py_kpu_net_obj_t* kpu_model;
    kpu_model_info_t* model;
} maix_kpu_classifier_t;


STATIC void init_obj(maix_kpu_classifier_t* self, py_kpu_net_obj_t* model, mp_int_t class_num, mp_int_t sample_num){
    self->model = m_new(kpu_model_info_t, 1);
    self->kpu_model = model;
    self->model->kmodel_ctx = model->kmodel_ctx;
    self->model->max_layers = model->max_layers;
    self->model->model_addr = model->model_addr;
    if(model->model_path == mp_const_none)
        self->model->model_path = NULL;
    else
        self->model->model_path = mp_obj_str_get_str(model->model_path);
    self->model->model_size = model->model_size;
    int ret = maix_kpu_classifier_init(&self->obj, self->model, (int)class_num, (int)sample_num);
    if(ret < 0)
        mp_raise_OSError(-ret);
}

STATIC int add_class_img(maix_kpu_classifier_t* self, image_t* img){
    int ret = maix_kpu_classifier_add_class_img(self->obj, img);
    if(ret < 0)
        mp_raise_OSError(-ret);
    return ret;
}

STATIC int add_sample_img(maix_kpu_classifier_t* self, image_t* img){
    int ret = maix_kpu_classifier_add_sample_img(self->obj, img);
    if(ret < 0)
        mp_raise_OSError(-ret);
    return ret;
}

STATIC void clear_obj(maix_kpu_classifier_t* self){
    int ret = maix_kpu_classifier_del(&self->obj);
    if(ret < 0)
        mp_raise_OSError(-ret);
}

STATIC void train(maix_kpu_classifier_t* self){
    int ret = maix_kpu_classifier_train(self->obj);
    if(ret < 0)
        mp_raise_OSError(-ret);
}

STATIC int predict(maix_kpu_classifier_t* self, image_t* img, float* min_distance){
    int ret = maix_kpu_classifier_predict(self->obj, img, min_distance);
    if(ret < 0)
        mp_raise_OSError(-ret);
    return ret;
}


mp_obj_t maix_kpu_classifier_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    maix_kpu_classifier_t* self = m_new_obj_with_finaliser(maix_kpu_classifier_t);
    self->base.type = &Maix_kpu_classifier_type;
    self->obj = NULL;
    if(n_args!=3 || n_kw!=0)
    {
        mp_raise_ValueError("model, class num, sample num");
    }
    if(mp_obj_get_type(args[0]) != &py_kpu_net_obj_type){
        mp_raise_ValueError("model");
    }
    init_obj(self, (py_kpu_net_obj_t*)args[0], mp_obj_get_int(args[1]), mp_obj_get_int(args[2]));
    return (mp_obj_t)self;
}

mp_obj_t classifier_add_class_img(mp_obj_t self_in, mp_obj_t img_in){
    if(mp_obj_get_type(self_in) != &Maix_kpu_classifier_type){
        mp_raise_ValueError("must be obj");
    }
    maix_kpu_classifier_t* self = (maix_kpu_classifier_t*)self_in;
    image_t* img = py_image_cobj(img_in);
    int ret_index = add_class_img(self, img);
    return mp_obj_new_int(ret_index);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(classifier_add_class_img_obj, classifier_add_class_img);


mp_obj_t classifier_add_sample_img(mp_obj_t self_in, mp_obj_t img_in){
    if(mp_obj_get_type(self_in) != &Maix_kpu_classifier_type){
        mp_raise_ValueError("must be obj");
    }
    maix_kpu_classifier_t* self = (maix_kpu_classifier_t*)self_in;
    image_t* img = py_image_cobj(img_in);
    int ret = add_sample_img(self, img);
    return mp_obj_new_int(ret);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(classifier_add_sample_img_obj, classifier_add_sample_img);

mp_obj_t classifier_del(mp_obj_t self_in){
    if(mp_obj_get_type(self_in) != &Maix_kpu_classifier_type){
        mp_raise_ValueError("must be obj");
    }
    mp_printf(&mp_plat_print, "classifier __del__\r\n");
    maix_kpu_classifier_t* self = (maix_kpu_classifier_t*)self_in;
    clear_obj(self);
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(classifier_del_obj, classifier_del);

mp_obj_t classifier_train(mp_obj_t self_in){
    if(mp_obj_get_type(self_in) != &Maix_kpu_classifier_type){
        mp_raise_ValueError("must be obj");
    }
    maix_kpu_classifier_t* self = (maix_kpu_classifier_t*)self_in;
    train(self);
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(classifier_train_obj, classifier_train);

mp_obj_t classifier_predict(mp_obj_t self_in, mp_obj_t img_in){
    if(mp_obj_get_type(self_in) != &Maix_kpu_classifier_type){
        mp_raise_ValueError("must be obj");
    }
    maix_kpu_classifier_t* self = (maix_kpu_classifier_t*)self_in;
    image_t* img = py_image_cobj(img_in);
    float min_distance;
    int ret_index = predict(self, img, &min_distance);
    mp_obj_t t[2];
    t[0] = mp_obj_new_int(ret_index);
    t[1] = mp_obj_new_float(min_distance);
    return mp_obj_new_tuple(2,t);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(classifier_predict_obj, classifier_predict);

STATIC const mp_map_elem_t locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),        MP_OBJ_NEW_QSTR(MP_QSTR_classifier) },
    { MP_ROM_QSTR(MP_QSTR_add_class_img),    (mp_obj_t)(&classifier_add_class_img_obj) },
    { MP_ROM_QSTR(MP_QSTR_add_sample_img),    (mp_obj_t)(&classifier_add_sample_img_obj) },
    { MP_ROM_QSTR(MP_QSTR_train),    (mp_obj_t)(&classifier_train_obj) },
    { MP_ROM_QSTR(MP_QSTR_predict),    (mp_obj_t)(&classifier_predict_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__),    (mp_obj_t)(&classifier_del_obj) },
};
STATIC MP_DEFINE_CONST_DICT(locals_dict, locals_dict_table);

const mp_obj_type_t Maix_kpu_classifier_type = {
    .base = { &mp_type_type },
    .name = MP_QSTR_classifier,
    .make_new = maix_kpu_classifier_make_new,
    .locals_dict = (mp_obj_dict_t*)&locals_dict
};

