/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2015 Damien P. George
 * Copyright (c) 2016 Paul Sokolovsky
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timer.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/objtype.h"
#include "mphalport.h"
#include "plic.h"
#include "sysctl.h"
#include "ov2640.h"
#include "dvp.h"
#include "fpioa.h"

#define TIMER_INTR_SEL TIMER_INTR_LEVEL
#define TIMER_DIVIDER  8

// TIMER_BASE_CLK is normally 80MHz. TIMER_DIVIDER ought to divide this exactly
#define TIMER_SCALE    (TIMER_BASE_CLK / TIMER_DIVIDER)

#define TIMER_FLAGS    0
struct dvp_buf
{
	uint32_t* addr[2];
	uint8_t buf_used[2];
	uint8_t buf_sel;
};
typedef struct _machine_ov2640_obj_t {
	mp_obj_base_t base;
	uint32_t active;
	uint16_t device_id;
	uint16_t manuf_id;
	struct dvp_buf buf;
	void* ptr;
    //mp_uint_t repeat;//timer mode
} machine_ov2640_obj_t;

const mp_obj_type_t machine_ov2640_type;

#define K210_DEBUG 0
#if K210_DEBUG==1
#define debug_print(x,arg...) printf("[lichee_debug]"x,##arg)
#else 
#define debug_print(x,arg...) 
#endif
void _ndelay(uint32_t ms)
{
    uint32_t i;

    while (ms && ms--)
    {
        for (i = 0; i < 25; i++)
            __asm__ __volatile__("nop");
    }
}

static int dvp_irq(void *ctx)
{

	machine_ov2640_obj_t* self = ctx;
	
	if (dvp_get_interrupt(DVP_STS_FRAME_FINISH)) {
		debug_print("Enter finish dvp_irq\n");
		dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
		self->buf.buf_used[self->buf.buf_sel] = 1;
		self->buf.buf_sel ^= 0x01;
		dvp_set_display_addr((uint32_t)self->buf.addr[self->buf.buf_sel]);	
	} else {
		debug_print("Enter start dvp_irq\n");
		dvp_clear_interrupt(DVP_STS_FRAME_START);
			if (self->buf.buf_used[self->buf.buf_sel] == 0)
			{
				dvp_start_convert();
			}
	}
	debug_print("self->buf.buf_used[0] = %d\n",self->buf.buf_used[0]);
	debug_print("self->buf.buf_used[1] = %d\n",self->buf.buf_used[1]);
	debug_print("self->buf.buf_sel = %d\n",self->buf.buf_sel);
	return 0;
}

STATIC void machine_ov2640_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_ov2640_obj_t *self = self_in;

    mp_printf(print, "ov2640(%p) ", self);
    mp_printf(print, "ov2640 active = %d, ", self->active);
    mp_printf(print, "manuf_id=%d, ",self->manuf_id);
    mp_printf(print, "device_id=%d", self->device_id);
}

STATIC mp_obj_t machine_ov2640_make_new() {
    
    machine_ov2640_obj_t *self = m_new_obj(machine_ov2640_obj_t);
    self->base.type = &machine_ov2640_type;

    return self;
}

STATIC void machine_ov2640_disable(machine_ov2640_obj_t *self) {
	sysctl_clock_disable(SYSCTL_CLOCK_DVP);
	plic_irq_disable(IRQN_DVP_INTERRUPT);
	plic_irq_deregister(IRQN_DVP_INTERRUPT);
	dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
	free(self->ptr);
	self->active = 0;
}

STATIC mp_obj_t machine_ov2640_init_helper(machine_ov2640_obj_t *self) {
	/*init dvp*/

	//printf("init ov2640\r\n");
	ov2640_init();
	ov2640_read_id(&self->manuf_id, &self->device_id);
	//printf("manuf_id:0x%04x,device_id:0x%04x\r\n", self->manuf_id, self->device_id);
	if(self->manuf_id != 0x7FA2 || self->device_id != 0x2642)
	{
		return mp_obj_new_bool(0);
	}
	/*buffer interrupt*/
	void* ptr = NULL;
	if(ptr == NULL)
	{
		ptr = malloc(sizeof(uint8_t) * 320 * 240 * (2 * 2) + 127);
		self->ptr = ptr;
	}
	if (ptr == NULL)
		return mp_obj_new_bool(1);
	self->buf.addr[0] = (uint32_t *)(((uint32_t)ptr + 127) & 0xFFFFFF80);
	self->buf.addr[1] = (uint32_t *)((uint32_t)self->buf.addr[0] + 320 * 240 * 2);
	self->buf.buf_used[0] = 0;
	self->buf.buf_used[1] = 0;
	self->buf.buf_sel = 0;
	/*init interrupt*/
	dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
	dvp_set_display_addr((uint32_t)self->buf.addr[self->buf.buf_sel]);
	plic_set_priority(IRQN_DVP_INTERRUPT, 1);
	plic_irq_register(IRQN_DVP_INTERRUPT, dvp_irq, self);
	plic_irq_enable(IRQN_DVP_INTERRUPT);
	/*start interrupt*/
	dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
	dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);
	
	self->active = 1;

	return mp_obj_new_bool(1);
}


STATIC mp_obj_t machine_ov2640_deinit(mp_obj_t self_in) {
    machine_ov2640_disable(self_in);

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_ov2640_deinit_obj, machine_ov2640_deinit);


STATIC mp_obj_t machine_ov2640_get_image(mp_obj_t self_in,mp_obj_t buf) {
    machine_ov2640_obj_t* self = self_in;
	uint32_t length = 0;
	uint8_t* buf_image = 0;
	//mp_obj_list_get(buf,&length,&item);
	mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_WRITE);
	while(self->buf.buf_used[self->buf.buf_sel] == 0 )_ndelay(50);
	memcpy(bufinfo.buf, self->buf.addr[self->buf.buf_sel], bufinfo.len);
	self->buf.buf_used[self->buf.buf_sel] = 0;
	//printf("[lichee]:get image!\n");
	return mp_obj_new_bool(1);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_ov2640_get_images_obj, machine_ov2640_get_image);


STATIC mp_obj_t machine_ov2640_init(mp_obj_t self_in) {
	machine_ov2640_obj_t* self = self_in;
    return machine_ov2640_init_helper(self);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_ov2640_init_obj, machine_ov2640_init);



STATIC const mp_rom_map_elem_t machine_ov2640_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&machine_ov2640_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_ov2640_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_ov2640_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_image), MP_ROM_PTR(&machine_ov2640_get_images_obj) },
};

STATIC MP_DEFINE_CONST_DICT(machine_ov2640_locals_dict, machine_ov2640_locals_dict_table);

const mp_obj_type_t machine_ov2640_type = {
    { &mp_type_type },
    .name = MP_QSTR_ov2640,
    .print = machine_ov2640_print,
    .make_new = machine_ov2640_make_new,
    .locals_dict = (mp_obj_t)&machine_ov2640_locals_dict,
};



















