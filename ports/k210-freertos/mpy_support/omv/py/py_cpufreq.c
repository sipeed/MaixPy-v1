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
#define M_1 1000000

#define ARRAY_LENGTH(x) (sizeof(x)/sizeof(x[0]))
static const uint32_t kpufreq_freqs[] = {100, 200, 400};
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

mp_obj_t py_kpufreq_get_supported_frequencies()
{

    mp_obj_t freq_list = mp_obj_new_list(0, NULL);
    for (int i=0; i<ARRAY_LENGTH(kpufreq_freqs); i++) {
        mp_obj_list_append(freq_list, mp_obj_new_int(kpufreq_freqs[i]));
    }
    return freq_list;   
}
#define CPU 0
#define KPU 1
#define FREQ_NUM 2
mp_obj_t py_cpufreq_set_frequency(uint n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    mp_map_elem_t *cpu_arg = mp_map_lookup(kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_cpu), MP_MAP_LOOKUP);
    mp_map_elem_t *kpu_arg = mp_map_lookup(kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_kpu), MP_MAP_LOOKUP);
    uint32_t freq_list[FREQ_NUM] = {0};
    uint32_t cpufreq = 0;
    uint32_t kpufreq = 0;
    if(NULL == cpu_arg && NULL == kpu_arg)
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Please enter frequency!"));

    if(cpu_arg != NULL)
        cpufreq = mp_obj_get_int(cpu_arg->value);
    else
        cpufreq = 0;
    freq_list[CPU] = cpufreq;

    if(kpu_arg != NULL)
        kpufreq = mp_obj_get_int(kpu_arg->value);
    else
        kpufreq = 0;
    freq_list[KPU] = kpufreq;

    // Check if frequency is supported
    int kpufreq_idx = -1;
    for (int i=0; i<ARRAY_LENGTH(kpufreq_freqs); i++) {
        if (kpufreq == kpufreq_freqs[i]) {
            kpufreq_idx = i;
            break;
        }
    }
    // Frequency is Not supported.
    if ( kpufreq != 0 && kpufreq_idx == -1) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Unsupported KPU frequency!"));
    }

    // Frequency is Not supported.
    if (cpufreq > (CPU_MAX_FREQ / M_1)) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Unsupported CPU frequency!"));
    }

    // Return if frequency hasn't changed.
    if (( 20 > abs(cpufreq - (cpufreq_get_cpuclk()/M_1))) && ( 20 > abs(kpufreq - (sysctl_clock_get_freq(SYSCTL_CLOCK_AI) / M_1)))) {
        return mp_const_true;
    }

    uint32_t store_freq[FREQ_READ_NUM] = {0};
    for(int i = 0; i < FREQ_READ_NUM; i++)
	{
        store_freq[i] = freq_list[i] * M_1;  
	}   
    uint32_t res = 0;
    for(int i = 0; i < FREQ_READ_NUM; i++)
	{
        if(0 == store_freq[i])
            continue;
		res = sys_spiffs_write(FREQ_STORE_ADDR + i*4,4,(uint8_t* )(&store_freq[i]));
	}
    mp_printf(&mp_plat_print, "\r\n");
    sysctl->soft_reset.soft_reset = 1;//reboot
    // if(result == 0)
    //     nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Can not set frequency!"));

    return mp_const_true;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_cpufreq_set_frequency_obj,0, py_cpufreq_set_frequency);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_cpufreq_get_current_frequencies_obj, py_cpufreq_get_current_frequencies);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_kpufreq_get_supported_frequencies_obj, py_kpufreq_get_supported_frequencies);

static const mp_map_elem_t globals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__),                  MP_OBJ_NEW_QSTR(MP_QSTR_cpufreq) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_frequency),             (mp_obj_t)&py_cpufreq_set_frequency_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_current_frequencies),   (mp_obj_t)&py_cpufreq_get_current_frequencies_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_kpu_supported_frequencies), (mp_obj_t)&py_kpufreq_get_supported_frequencies_obj },
    { NULL, NULL },
};
STATIC MP_DEFINE_CONST_DICT(globals_dict, globals_dict_table);

const mp_obj_module_t cpufreq_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_t)&globals_dict,
};
