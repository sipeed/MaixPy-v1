/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * CPU frequency scaling module.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <mp.h>
#include <math.h>
#include "sysctl.h"
#include "py_cpufreq.h"
#include "py_helper.h"
#include "mpconfigboard.h"
#include "vfs_spiffs.h"
#include "sipeed_sys.h"


#define ARRAY_LENGTH(x) (sizeof(x)/sizeof(x[0]))
// static const uint32_t kpufreq_freqs[] = {100, 200, 400};
//static const uint32_t cpufreq_pllq[] = {5, 6, 7, 8, 9};

// static const uint32_t cpufreq_latency[] = { // Flash latency (see table 11)
//     FLASH_LATENCY_3, FLASH_LATENCY_4, FLASH_LATENCY_5, FLASH_LATENCY_7, FLASH_LATENCY_7
// };

uint32_t cpufreq_get_cpuclk()
{
    uint32_t cpuclk = sysctl_clock_get_freq(SYSCTL_CLOCK_CPU);

    return cpuclk;
}

mp_obj_t py_cpufreq_get_current_frequencies()
{
    mp_obj_t tuple[2] = {
        mp_obj_new_int(cpufreq_get_cpuclk() / (1000000)),
        mp_obj_new_int(sysctl_clock_get_freq(SYSCTL_CLOCK_AI) / (1000000)),
    };
    return mp_obj_new_tuple(2, tuple);
}

// mp_obj_t py_kpufreq_get_supported_frequencies()
// {

//     mp_obj_t freq_list = mp_obj_new_list(0, NULL);
//     for (int i=0; i<ARRAY_LENGTH(kpufreq_freqs); i++) {
//         mp_obj_list_append(freq_list, mp_obj_new_int(kpufreq_freqs[i]));
//     }
//     return freq_list;   
// }

mp_obj_t py_kpufreq_get_cpu()
{
    return mp_obj_new_int(cpufreq_get_cpuclk() / (1000000));
}

mp_obj_t py_kpufreq_get_kpu()
{
    return mp_obj_new_int(sysctl_clock_get_freq(SYSCTL_CLOCK_AI) / (1000000));
}

mp_obj_t py_cpufreq_set_frequency(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    config_data_t config;
    enum { 
        ARG_cpu,
        ARG_pll1,
        ARG_kpu_div,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_cpu, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_pll1, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_kpu_div, MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    load_config_from_spiffs(&config);

    if(args[ARG_cpu].u_int != 0)
        config.freq_cpu = args[ARG_cpu].u_int*1000000;
    if(args[ARG_pll1].u_int != 0)
        config.freq_pll1 = args[ARG_pll1].u_int*1000000;
    if(args[ARG_kpu_div].u_int != 0)
        config.kpu_div = args[ARG_kpu_div].u_int;
    uint32_t freq_kpu = config.freq_pll1 / config.kpu_div;
    // Frequency is Not supported.
    if ( freq_kpu > FREQ_KPU_MAX ||
         freq_kpu < FREQ_KPU_MIN) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Unsupported KPU frequency!"));
    }

    // Frequency is Not supported.
    if ( config.freq_cpu > FREQ_CPU_MAX  || 
        config.freq_cpu < FREQ_CPU_MIN ) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Unsupported CPU frequency!"));
    }

    // Return if frequency hasn't changed.
    if (( 20 > abs(config.freq_cpu - cpufreq_get_cpuclk())) && ( 20 > abs(freq_kpu - sysctl_clock_get_freq(SYSCTL_CLOCK_AI) ))) {
        mp_printf(&mp_plat_print, "No change\r\n");
        return mp_const_none;
    }
    if(!save_config_to_spiffs(&config))
        mp_printf(&mp_plat_print, "save config fail");
    mp_printf(&mp_plat_print, "\r\nreboot now\r\n");
    mp_hal_delay_ms(50);
    sipeed_sys_reset();

    return mp_const_true;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_cpufreq_set_freq_obj,0, py_cpufreq_set_frequency);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_cpufreq_get_current_freq_obj, py_cpufreq_get_current_frequencies);
// STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_kpufreq_get_supported_frequencies_obj, py_kpufreq_get_supported_frequencies);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_kpufreq_get_kpu_obj, py_kpufreq_get_kpu);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_kpufreq_get_cpu_obj, py_kpufreq_get_cpu);

static const mp_map_elem_t locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__),        MP_OBJ_NEW_QSTR(MP_QSTR_freq) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set),             (mp_obj_t)&py_cpufreq_set_freq_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get),             (mp_obj_t)&py_cpufreq_get_current_freq_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_kpu),         (mp_obj_t)&py_kpufreq_get_kpu_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_cpu),         (mp_obj_t)&py_kpufreq_get_cpu_obj },
};
STATIC MP_DEFINE_CONST_DICT(locals_dict, locals_dict_table);

const mp_obj_type_t cpufreq_type = {
    .base = { &mp_type_type },
    .name = MP_QSTR_freq,
    .locals_dict = (mp_obj_t)&locals_dict,
};
