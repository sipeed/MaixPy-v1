#include <stdio.h>
#include <stdlib.h>
#include "sysctl.h"
#include "plic.h"
#include "dmac.h"

#include "py/mphal.h"
#include "py/runtime.h"

#include "w25qxx.h"
#include "uarths.h"
#include "fpioa.h"
#include "gpiohs.h"
#include "face_detect.h"
#include "sleep.h"

#define AI_DMA_CHANNEL 1
#define __AI_DMA_INTERRUPT(x)	IRQN_DMA##x##_INTERRUPT
#define _AI_DMA_INTERRUPT(x)	__AI_DMA_INTERRUPT(x)
#define AI_DMA_INTERRUPT	_AI_DMA_INTERRUPT(AI_DMA_CHANNEL)

#define K210_DEBUG 0
#if K210_DEBUG==1
#define debug_print(x,arg...) printf("[lichee_debug]"x,##arg)
#else 
#define debug_print(x,arg...) 
#endif


typedef struct _machine_ai_obj_t {
	mp_obj_base_t base;
	uint32_t buf_status;
	uint8_t *ai_buf;
	uint8_t *image_buf;
	uint32_t size;

} machine_ai_obj_t;

void ndelay(uint32_t ms)
{
    uint32_t i;

    while (ms && ms--)
    {
        for (i = 0; i < 25; i++)
            __asm__ __volatile__("nop");
    }
}

void _mdelay(uint32_t ms)
{
    uint32_t i;

    while (ms && ms--)
    {
        for (i = 0; i < 25000; i++)
            __asm__ __volatile__("nop");
    }
}

const mp_obj_type_t machine_demo_face_detect_type;

static int imge_buf_convert(machine_ai_obj_t* self)
{
		unsigned char* red = NULL;
		unsigned char* green = NULL;
		unsigned char* blue = NULL;
		red = (unsigned char*)self->ai_buf;
		green = (unsigned char*)(self->ai_buf+320*240);
		blue =(unsigned char*) (self->ai_buf+320*240*2);
		int size = self->size;
		int i = 0;
		uint8_t *src = self->image_buf;
		unsigned int twoByte  = 0;
		for(i = 0; i < size; i += 1)
		{
			twoByte = (src[1] << 8) + src[0];
	        red[i] = (src[1] & 248);
	        green[i] = (unsigned char)((twoByte & 2016) >> 3);
	        blue[i] = ((src[0] & 31) * 8);
	        src += 2;
		}
		debug_print("[MAIXPY]Face Detect:imge_buf_convert finish\n");
}


int ai_dma_irq(void *ctx)
{
	machine_ai_obj_t* self = ctx;
	//ai_test("ai_dma_irq");
	if (dmac->channel[AI_DMA_CHANNEL].intstatus & 0x02) {
		if (self->buf_status == 0) { // convert completed, start ai cal
			debug_print("[MAIXPY]Face Detect:I can enter dma irq self->cal_status == %d\n",self->buf_status);
			ai_cal_start();
			self->buf_status = 1; // wait ai cal completed flag
		}
		else if(self->buf_status == 1){
			debug_print("[MAIXPY]Face Detect:I can enter dma irq self->cal_status == %d\n",self->buf_status);
			self->buf_status = 2;
		}
	}
	dmac->channel[AI_DMA_CHANNEL].intclear = 0xFFFFFFFF;
	ndelay(1000);
	debug_print("[MAIXPY]Face Detect:quit ai_dma_irq\n");
	return 0;
}

STATIC mp_obj_t machine_ai_process_image(mp_obj_t self_in,mp_obj_t buf) {
	machine_ai_obj_t* self = self_in;
	struct face_Data data;
	data.x1 = 0;
	data.y1 = 0;
	data.x2 = 0;
	data.y2 = 0;
	/*get image buf*/
	mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_READ|MP_BUFFER_WRITE);
	self->size = 320*240;
	memcpy(self->image_buf,bufinfo.buf, bufinfo.len);
	self->buf_status = 0;
	/*convert image to rgb888*/
	imge_buf_convert(self);
	/*start input dma*/
	ai_data_input((uint32_t)self->ai_buf);
	// wait ai cal completed
	while(1 != self->buf_status)ndelay(50);
	if(1 == self->buf_status)
	{
		ai_cal_first();
		ai_cal_second();
		ai_data_output();
	}
	while(2 != self->buf_status)
	{
		ndelay(50);
		debug_print("[MAIXPY]Face Detect:whiling\n");
	}
	data = ai_draw_label((uint32_t*)bufinfo.buf);
	mp_obj_list_t* list = mp_obj_new_list(4, NULL);
	list->items[0] = MP_OBJ_NEW_SMALL_INT(data.x1);
	list->items[1] = MP_OBJ_NEW_SMALL_INT(data.x2);
	list->items[2] = MP_OBJ_NEW_SMALL_INT(data.y1);
	list->items[3] = MP_OBJ_NEW_SMALL_INT(data.y2);
	
//	mp_obj_tuple_t *ret_tuple = MP_OBJ_TO_PTR(mp_obj_new_tuple(4, NULL));
//	ret_tuple->items[0] = MP_OBJ_NEW_SMALL_INT(data.x1);
//	ret_tuple->items[1] = MP_OBJ_NEW_SMALL_INT(data.x2);
//	ret_tuple->items[2] = MP_OBJ_NEW_SMALL_INT(data.y1);
//	ret_tuple->items[3] = MP_OBJ_NEW_SMALL_INT(data.y2);

	//memset(bufinfo.buf, 0, bufinfo.len);
	debug_print("[MAIXPY]Face Detect:finish ai_draw_label\n");
	debug_print("[MAIXPY]Face Detect:finish ai_cal_first\n");
	debug_print("[MAIXPY]Face Detect:finish process\n");

	return list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_process_image_obj, machine_ai_process_image);


STATIC mp_obj_t machine_ai_init_helper(machine_ai_obj_t *self_in) {
	machine_ai_obj_t* self = self_in;
	void* ptr = NULL;
	/*malloc ai buf*/
	ptr = malloc(sizeof(uint8_t) * 320 * 240 *  3 + 127);
	if (ptr == NULL)
		return;
	self->ai_buf = (uint32_t *)(((uint32_t)ptr + 127) & 0xFFFFFF80);
	/*malloc image buf*/
	ptr = malloc(sizeof(uint8_t) * 320 * 240 * 2 + 127);
	if (ptr == NULL)
		return;
	self->image_buf = (uint8_t *)(((uint32_t)ptr + 127) & 0xFFFFFF80);
	while (1) {
		uint8_t manuf_id, device_id;
		w25qxx_init(3);
		w25qxx_read_id(&manuf_id, &device_id);
		printf("[MAIXPY]Flash:0x%02x:0x%02x\n", manuf_id, device_id);
		if (manuf_id != 0xFF && manuf_id != 0x00 && device_id != 0xFF && device_id != 0x00)
			break;
	}
	w25qxx_enable_quad_mode();
	self->buf_status = 0;
	if (ai_init(0))
	{
		printf("[MAIXPY]Face Detect:ai_init error,please burn a model\n");
		return mp_const_none;
	}
	plic_irq_register(AI_DMA_INTERRUPT, ai_dma_irq, self);
	printf("MAIXPY]Face Detect:init successful!\n");
	return mp_const_none;
}

STATIC mp_obj_t machine_ai_init(mp_obj_t self_in) {
	machine_ai_obj_t* self = self_in;
    return machine_ai_init_helper(self);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_ai_init_obj, machine_ai_init);



STATIC mp_obj_t machine_ai_make_new() {
    
    volatile machine_ai_obj_t *self = m_new_obj(machine_ai_obj_t);
    self->base.type = &machine_demo_face_detect_type;
    return self;
}

STATIC const mp_rom_map_elem_t pyb_ai_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_ai_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_process_image), MP_ROM_PTR(&machine_process_image_obj) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_ai_locals_dict, pyb_ai_locals_dict_table);

const mp_obj_type_t machine_demo_face_detect_type = {
    { &mp_type_type },
    .name = MP_QSTR_face_detect,
    .make_new = machine_ai_make_new,
    .locals_dict = (mp_obj_dict_t*)&pyb_ai_locals_dict,
};

