/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Paul Sokolovsky
 * Copyright (c) 2017 Eric Poulsen
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

#include <string.h>

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "mphalport.h"

#include "wdt.h"
#include "sysctl.h"

const mp_obj_type_t machine_wdt_type;

typedef struct _machine_wdt_obj_t {
    mp_obj_base_t base;
    wdt_device_number_t id;
    mp_int_t timeout;
    mp_obj_t callback;
    mp_obj_t context;
    bool is_interrupt;
} machine_wdt_obj_t;

STATIC void machine_wdt_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_wdt_obj_t *self = self_in;
    mp_printf(print, 
        "[MAIXPY]WDT:(%p; id=%d, timeout=%d, callback=%p, context=%p)",
        self, self->id, self->timeout, self->callback, self->context);
}

// wdt stop bug removed after repair
STATIC void patch_wdt_stop(wdt_device_number_t id) {
    wdt_stop(id);
    sysctl_clock_disable(id ? SYSCTL_CLOCK_WDT1 : SYSCTL_CLOCK_WDT0);
}

// #include "printf.h"
STATIC void machine_wdt_isr(void *self_in) {
    machine_wdt_obj_t *self = self_in;
    if (self->callback != mp_const_none) {
        // printk("wdt id is %d\n", self->id);
        if (self->is_interrupt == false) {
            wdt_clear_interrupt(self->id);
            self->is_interrupt = true;
            mp_sched_schedule(self->callback, self);
        }
        mp_hal_wake_main_task_from_isr();
    }
}

STATIC mp_obj_t machine_wdt_make_new(const mp_obj_type_t *type_in, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    machine_wdt_obj_t *self = m_new_obj(machine_wdt_obj_t);

    enum { ARG_id, ARG_timeout, ARG_callback, ARG_context };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_timeout, MP_ARG_INT, {.u_int = 5000} },
        { MP_QSTR_callback, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_context, MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (args[ARG_id].u_int < 0 || args[ARG_id].u_int > 1) { // WDT_DEVICE_MAX
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "WDT(%d) does not exist", args[ARG_id].u_int));
    }

    if (args[ARG_timeout].u_int <= 0) { // milliseconds
        mp_raise_ValueError("WDT timeout too short");
    }

    self->base.type = &machine_wdt_type;
    self->id = args[ARG_id].u_int;
    self->timeout  = args[ARG_timeout].u_int;
    self->callback  = args[ARG_callback].u_obj;
    self->context  = args[ARG_context].u_obj;
    self->is_interrupt = false;
    
    patch_wdt_stop(self->id);
    if (self->callback != mp_const_none) {
        wdt_init(self->id, self->timeout, (plic_irq_callback_t)machine_wdt_isr, (void *)self);
    } else {
        wdt_init(self->id, self->timeout, NULL, NULL);
    }
    return self;
}

STATIC mp_obj_t machine_wdt_context(mp_obj_t self_in) {
    machine_wdt_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return self->context;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_wdt_context_obj, machine_wdt_context);

STATIC mp_obj_t machine_wdt_feed(mp_obj_t self_in) {
    machine_wdt_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // mp_printf(&mp_plat_print, "wdt id is %d\n", self->id);
    wdt_feed(self->id);
    self->is_interrupt = false;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_wdt_feed_obj, machine_wdt_feed);

// STATIC mp_obj_t machine_wdt_start(mp_obj_t self_in) {
//     machine_wdt_obj_t *self = MP_OBJ_TO_PTR(self_in);
//     wdt_start(self->id, self->timeout, self->callback/*, self->context*/);
//     return mp_const_none;
// }
// STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_wdt_start_obj, machine_wdt_start);

STATIC mp_obj_t machine_wdt_stop(mp_obj_t self_in) {
    machine_wdt_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // mp_printf(&mp_plat_print, "wdt id is %d\n", self->id);
    patch_wdt_stop(self->id);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_wdt_stop_obj, machine_wdt_stop);

#ifdef ENABLE_UNIT_TEST
#include "printf.h"
static int wdt0_irq(void *ctx) {
    static int s_wdt_irq_cnt = 0;
    printk("%s\n", __func__);
    s_wdt_irq_cnt ++;
    if(s_wdt_irq_cnt < 2)
        wdt_clear_interrupt((wdt_device_number_t)0);
    else
        while(1);
    return 0;
}

static void unit_test() {
    mp_printf(&mp_plat_print, "wdt start!\n");
    int timeout = 0;
    plic_init();
    sysctl_enable_irq();
    mp_printf(&mp_plat_print, "wdt time is %d ms\n", wdt_init((wdt_device_number_t)0, 4000, wdt0_irq,NULL));
    while(1) {
        vTaskDelay(1000);
        if(timeout++ < 3) {
            wdt_feed((wdt_device_number_t)0);
        } else {
            printf("wdt_stop\n");
            wdt_stop((wdt_device_number_t)0);
            sysctl_clock_disable(0 ? SYSCTL_CLOCK_WDT1 : SYSCTL_CLOCK_WDT0); // patch for fix stop
            while(1) 
            {
                printf("wdt_idle\n");
                sleep(1);
            }
        }
    }
}
STATIC mp_obj_t machine_wdt_unit_test(mp_obj_t self_in) {
    (void)self_in;
    unit_test();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_wdt_unit_test_obj, machine_wdt_unit_test);
#endif

STATIC const mp_rom_map_elem_t machine_wdt_locals_dict_table[] = {
    // { MP_ROM_QSTR(MP_QSTR_unit_test), MP_ROM_PTR(&machine_wdt_unit_test_obj) },
    { MP_ROM_QSTR(MP_QSTR_WDT_DEVICE_0),  MP_ROM_INT(WDT_DEVICE_0) },
    { MP_ROM_QSTR(MP_QSTR_WDT_DEVICE_1),  MP_ROM_INT(WDT_DEVICE_1) },
    { MP_ROM_QSTR(MP_QSTR_feed), MP_ROM_PTR(&machine_wdt_feed_obj) },
    { MP_ROM_QSTR(MP_QSTR_context), MP_ROM_PTR(&machine_wdt_context_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&machine_wdt_stop_obj) },
    // { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&machine_wdt_start_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_wdt_locals_dict, machine_wdt_locals_dict_table);

const mp_obj_type_t machine_wdt_type = {
    { &mp_type_type },
    .name = MP_QSTR_WDT,
    .print = machine_wdt_print,
    .make_new = machine_wdt_make_new,
    .locals_dict = (mp_obj_t)&machine_wdt_locals_dict,
};
