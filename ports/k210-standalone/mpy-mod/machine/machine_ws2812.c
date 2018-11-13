
#include <stdio.h>
#include <stdint.h>

#include "py/nlr.h"
#include "py/runtime.h"
#include "modmachine.h"
#include "mphalport.h"

#include "ws2812b.h"
#include "sleep.h"
#include "gpiohs.h"
#include "sysctl.h"


typedef struct _machine_ws2812_obj_t {
    mp_obj_base_t base;
	uint32_t gpio_num;
	uint32_t pin_num;

} machine_ws2812_obj_t;

const mp_obj_type_t machine_ws2812_type;



void WS2812B_TxRes(int gpio_num);


void stop_irq_all(void)
{
	volatile uint64_t* irq_threshhoud = (volatile uint64_t *)0x0C200000;
	*irq_threshhoud = 0xffffffff;
	//printf("stop_irq_all\r\n");
}
void start_irq_all(void)
{
	volatile uint64_t* irq_threshhoud = (volatile uint64_t *)0x0C200000;
	//printf("start_irq_all\r\n");
	*irq_threshhoud = 0x00000000;
	
}


STATIC void machine_ws2812_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_ws2812_obj_t *self = self_in;

    mp_printf(print, "[MAIXPY]WS2812:(%p) ", self);

    mp_printf(print, "[MAIXPY]WS2812:gpio_hs: %d ", self->gpio_num);
    mp_printf(print, "[MAIXPY]WS2812:pin: %d ", self->pin_num);
}

STATIC mp_obj_t machine_ws2812_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    machine_ws2812_obj_t *self = m_new_obj(machine_ws2812_obj_t);
    self->base.type = &machine_ws2812_type;
    return self;
}

STATIC mp_obj_t machine_ws2812_deinit(machine_ws2812_obj_t *self_in) {
	machine_ws2812_obj_t* self = self_in;
	self->gpio_num = 0;
	self->pin_num = 0;
	gpiohs_set_drive_mode(self->gpio_num,GPIO_DM_OUTPUT);
	gpiohs_set_pin(self->gpio_num,GPIO_PV_LOW);
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_deinit_obj,machine_ws2812_deinit);

STATIC mp_obj_t machine_ws2812_init(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	machine_ws2812_obj_t* self = pos_args[0];
	enum {
		ARG_gpio_num,
		ARG_pin_num,
	};
	static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_gpio_num, 	 MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_pin_num,		 MP_ARG_INT, {.u_int = 0} },
	};
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
	self->gpio_num = args[ARG_gpio_num].u_int;
	self->pin_num = args[ARG_pin_num].u_int;
	init_nop_cnt();
	gpiohs_set_drive_mode(self->gpio_num,GPIO_DM_OUTPUT);
	gpiohs_set_pin(self->gpio_num,GPIO_PV_LOW);
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_init_obj, 1,machine_ws2812_init);

STATIC mp_obj_t machine_ws2812_set_RGB(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	machine_ws2812_obj_t* self = pos_args[0];
	enum {
		ARG_red,
		ARG_green,
		ARG_blue,
	};
	static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_red, 	 MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_green, MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_blue, MP_ARG_INT, {.u_int = 0} },
	};
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	if(args[ARG_red].u_int >255)
	{
		printf("[MAIXPY]WS2812:Error:red value > 255");
		return mp_const_false;
	}
	if(args[ARG_green].u_int >255)
	{
		printf("[MAIXPY]WS2812:Error:green value > 255");
		return mp_const_false;
	}
	if(args[ARG_blue].u_int >255)
	{
		printf("[MAIXPY]WS2812:Error:blue value > 255");
		return mp_const_false;
	}
	stop_irq_all();
	WS2812B_SetLedRGB(args[ARG_red].u_int, args[ARG_green].u_int, args[ARG_blue].u_int, self->gpio_num);
	start_irq_all();
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_set_RGB_obj, 1,machine_ws2812_set_RGB);


STATIC mp_obj_t machine_ws2812_set_RGB_num(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	machine_ws2812_obj_t* self = pos_args[0];
	enum {
		ARG_red,
		ARG_green,
		ARG_blue,
		ARG_num,
	};
	static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_red, 	 MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_green, MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_blue, MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_num, MP_ARG_INT, {.u_int = 0} },
	};
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	if(args[ARG_red].u_int >255)
	{
		printf("[MAIXPY]WS2812:red value > 255");
		return mp_const_false;
	}
	if(args[ARG_green].u_int >255)
	{
		printf("[MAIXPY]WS2812:green value > 255");
		return mp_const_false;
	}
	if(args[ARG_blue].u_int >255)
	{
		printf("[MAIXPY]WS2812:blue value > 255");
		return mp_const_false;
	}
	if(args[ARG_num].u_int <= 0)
	{
		printf("[MAIXPY]WS2812:num value <= 0");
		return mp_const_false;
	}
	stop_irq_all();
	WS2812B_SetLednRGB(args[ARG_red].u_int, args[ARG_green].u_int, args[ARG_blue].u_int, args[ARG_num].u_int,self->gpio_num);
	start_irq_all();
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_set_RGB_num_obj, 1,machine_ws2812_set_RGB_num);

STATIC const mp_rom_map_elem_t machine_ws2812_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_deinit_obj) },
	{ MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_init_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_RGB_num), MP_ROM_PTR(&machine_set_RGB_num_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_RGB), MP_ROM_PTR(&machine_set_RGB_obj) },

};
STATIC MP_DEFINE_CONST_DICT(machine_ws2812_locals_dict, machine_ws2812_locals_dict_table);

const mp_obj_type_t machine_ws2812_type = {
    { &mp_type_type },
    .name = MP_QSTR_Ws2812,
    .print = machine_ws2812_print,
    .make_new = machine_ws2812_make_new,
    .locals_dict = (mp_obj_t)&machine_ws2812_locals_dict,
};
