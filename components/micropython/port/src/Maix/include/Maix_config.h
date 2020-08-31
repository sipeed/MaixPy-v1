#include "obj.h"

mp_map_elem_t *dict_iter_next(mp_obj_dict_t *dict, size_t *cur);
mp_obj_t maix_config_init();
mp_obj_t maix_config_get_value(mp_obj_t key, mp_obj_t def_value);
