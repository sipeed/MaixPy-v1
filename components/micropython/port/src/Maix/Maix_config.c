/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>

#include "py/objlist.h"
#include "py/objstringio.h"
#include "py/parsenum.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/objmodule.h"
#include "py/objstringio.h"
#include "mphalport.h"
#include "vfs_internal.h"

#include "Maix_config.h"

// static void unit_test_json_config();

mp_map_elem_t *dict_iter_next(mp_obj_dict_t *dict, size_t *cur)
{
    size_t max = dict->map.alloc;
    mp_map_t *map = &dict->map;

    for (size_t i = *cur; i < max; i++)
    {
        if (mp_map_slot_is_filled(map, i))
        {
            *cur = i + 1;
            return &(map->table[i]);
        }
    }

    return NULL;
}

#define MAIX_CONFIG_PATH "/flash/config.json"

typedef struct
{
    mp_obj_base_t base;
    mp_obj_t cache;
    mp_obj_t args[3];
} maix_config_t;

static maix_config_t *config_obj = NULL;

static mp_obj_t maix_config_cache()
{
    // printf("%s\r\n", __func__);
    typedef struct
    {
        mp_obj_base_t base;
    } fs_info_t;

    int err = 0;
    fs_info_t *cfg = vfs_internal_open(MAIX_CONFIG_PATH, "rb", &err);
    if (err != 0)
    {
        // printf("no config time:%ld\r\n", systick_current_millis());
    }
    else
    {
        // printf("exist config time:%ld\r\n", systick_current_millis());
        config_obj->args[2] = MP_OBJ_FROM_PTR(&(cfg->base));
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0)
        {
            config_obj->cache = mp_call_method_n_kw(1, 0, config_obj->args);
            nlr_pop();
            vfs_internal_close(cfg, &err);
            return mp_const_true;
        }
        else
        {
            mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
        }
        vfs_internal_close(cfg, &err);
    }
    return mp_const_false;
}

mp_obj_t maix_config_get_value(mp_obj_t key, mp_obj_t def_value)
{
    // printf("%s\r\n", __func__);
    if (config_obj != NULL)
    {
        if (false == mp_obj_is_type(config_obj->cache, &mp_type_dict))
        {
            // maybe gc.collect()
            if (mp_const_false == maix_config_cache())
            {
                return def_value;
            }
        }
        // mp_printf(&mp_plat_print, "print(config_obj->cache)\r\n");
        // mp_obj_print_helper(&mp_plat_print, config_obj->cache, PRINT_STR);
        // mp_printf(&mp_plat_print, "\r\n");
        // mp_check_self(mp_obj_is_dict_type(config_obj->cache));
        mp_obj_dict_t *self = MP_OBJ_TO_PTR(config_obj->cache);
        mp_map_elem_t *elem = mp_map_lookup(&self->map, key, MP_MAP_LOOKUP);
        if (elem == NULL || elem->value == MP_OBJ_NULL)
        {
            return def_value; // not exist
        }
        else
        {
            return elem->value;
        }
    }
    return def_value;
}
MP_DEFINE_CONST_FUN_OBJ_2(maix_config_get_value_obj, maix_config_get_value);

mp_obj_t maix_config_init()
{
    // printf("%s\r\n", __func__);
    // unit_test_json_config();
    static maix_config_t tmp;
    mp_obj_t module_obj = mp_module_get(MP_QSTR_ujson);
    if (module_obj != MP_OBJ_NULL)
    {
        // mp_printf(&mp_plat_print, "import josn\r\n");
        mp_load_method_maybe(module_obj, MP_QSTR_load, tmp.args);
        if (tmp.args[0] != MP_OBJ_NULL)
        {
            config_obj = &tmp;
            return maix_config_cache();
            // return mp_const_true;
        }
    }
    mp_printf(&mp_plat_print, "[%s]|(%s)\r\n", __func__, "fail");
    return mp_const_false;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(maix_config_init_obj, maix_config_init);

static const mp_map_elem_t locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_config)},
    {MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&maix_config_init_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_value), MP_ROM_PTR(&maix_config_get_value_obj)},
};

STATIC MP_DEFINE_CONST_DICT(locals_dict, locals_dict_table);

const mp_obj_type_t Maix_config_type = {
    .base = {&mp_type_type},
    .name = MP_QSTR_config,
    .locals_dict = (mp_obj_dict_t *)&locals_dict};

#ifdef UNIT_TEST

/*
{
  "config_name": "config.json",
  "lcd":{
    "RST_IO":16,
    "DCX_IO":32
  },
  "freq_cpu": 416000000,
  "freq_pll1": 400000000,
  "kpu_div": 1
}
*/

static void unit_test_json_config()
{
    // unit_test get string
    {
        const char key[] = "config_name";
        mp_obj_t tmp = maix_config_get_value(mp_obj_new_str(key, sizeof(key) - 1), mp_obj_new_str("None Cfg", 8));
        if (mp_obj_is_str(tmp))
        {
            const char *value = mp_obj_str_get_str(tmp);
            mp_printf(&mp_plat_print, "%s %s\r\n", key, value);
        }
    }

    // get lcd dict key-value
    {
        const char key[] = "lcd";
        mp_obj_t tmp = maix_config_get_value(mp_obj_new_str(key, sizeof(key) - 1), mp_obj_new_dict(0));
        if (mp_obj_is_type(tmp, &mp_type_dict))
        {
            mp_obj_dict_t *self = MP_OBJ_TO_PTR(tmp);
            size_t cur = 0;
            mp_map_elem_t *next = NULL;
            bool first = true;
            while ((next = dict_iter_next(self, &cur)) != NULL)
            {
                if (!first)
                {
                    mp_print_str(&mp_plat_print, ", ");
                }
                first = false;
                mp_obj_print_helper(&mp_plat_print, next->key, PRINT_STR);
                mp_print_str(&mp_plat_print, ": ");
                mp_obj_print_helper(&mp_plat_print, next->value, PRINT_STR);
            }
        }
    }
}

static void unit_test_json_config()
{
    mp_obj_t module_obj = mp_module_get(MP_QSTR_ujson);
    if (module_obj != MP_OBJ_NULL)
    {
        mp_printf(&mp_plat_print, "import josn\r\n");
        mp_obj_t dest[3];
        mp_load_method_maybe(module_obj, MP_QSTR_loads, dest);
        if (dest[0] != MP_OBJ_NULL)
        {
            const char json[] = "{\"a\":1,\"b\":2,\"c\":3,\"d\":4,\"e\":\"helloworld\"}";
            mp_printf(&mp_plat_print, "nresult = josn.loads(%s)\r\n", json);
            dest[2] = mp_obj_new_str(json, sizeof(json) - 1);
            nlr_buf_t nlr;
            if (nlr_push(&nlr) == 0)
            {
                mp_obj_t result = mp_call_method_n_kw(1, 0, dest);
                mp_printf(&mp_plat_print, "print(result)\r\n");
                mp_obj_print_helper(&mp_plat_print, result, PRINT_STR);
                mp_printf(&mp_plat_print, "\r\n");
                const char goal[] = "e";
                //mp_check_self(mp_obj_is_dict_type(result));
                mp_obj_dict_t *self = MP_OBJ_TO_PTR(result);
                mp_map_elem_t *elem = mp_map_lookup(&self->map, mp_obj_new_str(goal, sizeof(goal) - 1), MP_MAP_LOOKUP);
                mp_obj_t value;
                if (elem == NULL || elem->value == MP_OBJ_NULL)
                {
                    // not exist
                }
                else
                {
                    value = elem->value;
                    //mp_check_self(mp_obj_is_str_type(value));
                    mp_printf(&mp_plat_print, "print(result.get('%s'))\r\n", goal);
                    mp_obj_print_helper(&mp_plat_print, value, PRINT_STR);
                    mp_printf(&mp_plat_print, "\r\n");
                }
                nlr_pop();
            }
            else
            {
                mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
            }
        }
        mp_load_method_maybe(module_obj, MP_QSTR_load, dest);
        if (dest[0] != MP_OBJ_NULL)
        {
            const char json[] = "{\"a\":1,\"b\":2,\"c\":3,\"d\":4,\"e\":\"helloworld\"}";
            mp_printf(&mp_plat_print, "nresult = josn.load(%s)\r\n", json);
            mp_obj_t obj = mp_obj_new_str(json, sizeof(json) - 1);
            size_t len;
            const char *buf = mp_obj_str_get_data(obj, &len);
            vstr_t vstr = {len, len, (char *)buf, true};
            mp_obj_stringio_t sio = {{&mp_type_stringio}, &vstr, 0, MP_OBJ_NULL};
            dest[2] = MP_OBJ_FROM_PTR(&sio);
            nlr_buf_t nlr;
            if (nlr_push(&nlr) == 0)
            {
                mp_obj_t result = mp_call_method_n_kw(1, 0, dest);
                mp_printf(&mp_plat_print, "print(result)\r\n");
                mp_obj_print_helper(&mp_plat_print, result, PRINT_STR);
                mp_printf(&mp_plat_print, "\r\n");
                const char goal[] = "a";
                //mp_check_self(mp_obj_is_dict_type(result));
                mp_obj_dict_t *self = MP_OBJ_TO_PTR(result);
                mp_map_elem_t *elem = mp_map_lookup(&self->map, mp_obj_new_str(goal, sizeof(goal) - 1), MP_MAP_LOOKUP);
                mp_obj_t value;
                if (elem == NULL || elem->value == MP_OBJ_NULL)
                {
                    // not exist
                }
                else
                {
                    value = elem->value;
                    //mp_check_self(mp_obj_is_str_type(value));
                    mp_printf(&mp_plat_print, "print(result.get('%s'))\r\n", goal);
                    mp_obj_print_helper(&mp_plat_print, value, PRINT_STR);
                    mp_printf(&mp_plat_print, "\r\n");
                }
                nlr_pop();
            }
            else
            {
                mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
            }
        }
        typedef struct
        {
            mp_obj_base_t base;
        } fs_info_t;
        {

            int err = 0;
            fs_info_t *cfg = vfs_internal_open("/flash/config.json", "rb", &err);
            if (err != 0)
            {
                printf("no config time:%ld\r\n", systick_current_millis());
            }
            else
            {
                // mp_stream_p_t* stream = (mp_stream_p_t*)cfg->base.type->protocol;
                printf("exist config time:%ld\r\n", systick_current_millis());
                mp_load_method_maybe(module_obj, MP_QSTR_load, dest);
                if (dest[0] != MP_OBJ_NULL)
                {
                    dest[2] = MP_OBJ_FROM_PTR(&(cfg->base));
                    nlr_buf_t nlr;
                    if (nlr_push(&nlr) == 0)
                    {
                        mp_obj_t result = mp_call_method_n_kw(1, 0, dest);
                        mp_printf(&mp_plat_print, "print(result)\r\n");
                        mp_obj_print_helper(&mp_plat_print, result, PRINT_STR);
                        mp_printf(&mp_plat_print, "\r\n");
                        const char goal[] = "a";
                        //mp_check_self(mp_obj_is_dict_type(result));
                        mp_obj_dict_t *self = MP_OBJ_TO_PTR(result);
                        mp_map_elem_t *elem = mp_map_lookup(&self->map, mp_obj_new_str(goal, sizeof(goal) - 1), MP_MAP_LOOKUP);
                        mp_obj_t value;
                        if (elem == NULL || elem->value == MP_OBJ_NULL)
                        {
                            // not exist
                        }
                        else
                        {
                            value = elem->value;
                            //mp_check_self(mp_obj_is_str_type(value));
                            mp_printf(&mp_plat_print, "print(result.get('%s'))\r\n", goal);
                            mp_obj_print_helper(&mp_plat_print, value, PRINT_STR);
                            mp_printf(&mp_plat_print, "\r\n");
                        }
                        nlr_pop();
                    }
                    else
                    {
                        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
                    }

                    vfs_internal_close(cfg, &err);
                }
            }
        }

        {

            int err = 0;
            fs_info_t *cfg = vfs_internal_open("/flash/config.json", "rb", &err);
            if (err != 0)
            {
                printf("no config time:%ld\r\n", systick_current_millis());
            }
            else
            {
                // mp_stream_p_t* stream = (mp_stream_p_t*)cfg->base.type->protocol;
                printf("exist config time:%ld\r\n", systick_current_millis());
                mp_load_method_maybe(module_obj, MP_QSTR_load, dest);
                if (dest[0] != MP_OBJ_NULL)
                {
                    dest[2] = MP_OBJ_FROM_PTR(&(cfg->base));
                    nlr_buf_t nlr;
                    if (nlr_push(&nlr) == 0)
                    {
                        mp_obj_t result = mp_call_method_n_kw(1, 0, dest);
                        mp_printf(&mp_plat_print, "print(result)\r\n");
                        mp_obj_print_helper(&mp_plat_print, result, PRINT_STR);
                        mp_printf(&mp_plat_print, "\r\n");
                        const char goal[] = "e";
                        //mp_check_self(mp_obj_is_dict_type(result));
                        mp_obj_dict_t *self = MP_OBJ_TO_PTR(result);
                        mp_map_elem_t *elem = mp_map_lookup(&self->map, mp_obj_new_str(goal, sizeof(goal) - 1), MP_MAP_LOOKUP);
                        mp_obj_t value;
                        if (elem == NULL || elem->value == MP_OBJ_NULL)
                        {
                            // not exist
                        }
                        else
                        {
                            value = elem->value;
                            //mp_check_self(mp_obj_is_str_type(value));
                            mp_printf(&mp_plat_print, "print(result.get('%s'))\r\n", goal);
                            mp_obj_print_helper(&mp_plat_print, value, PRINT_STR);
                            mp_printf(&mp_plat_print, "\r\n");
                        }
                        nlr_pop();
                    }
                    else
                    {
                        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
                    }

                    vfs_internal_close(cfg, &err);
                }
            }
        }
    }
}

#endif
