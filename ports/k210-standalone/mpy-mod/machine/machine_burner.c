
#include <stdio.h>
#include <string.h>

#include "py/mphal.h"
#include "py/runtime.h"

#include <stdlib.h>
#include "sysctl.h"
#include "plic.h"
#include "dmac.h"
#include "w25qxx.h"
#include "uarths.h"
#include "fpioa.h"
#include "gpiohs.h"
#include "sleep.h"
#include "lcd.h"

#define UART_BUF_LENGTH_MAX 269

typedef struct _machine_burner_obj_t {
	mp_obj_base_t base;
	uint32_t burn_uarths_freq;
	uint32_t baudrate;
} machine_burner_obj_t;

typedef struct {
	uint8_t *uart_buf;
	uint8_t uart_status;
	uint8_t flash_status;
} burner_attributes;

volatile burner_attributes burner_attr;

const mp_obj_type_t machine_burner_type;

static int uarths_irq(void *ctx)
{
	static uint16_t cnt, len;
	uint8_t *buf = burner_attr.uart_buf;
	while (uarths->ip.rxwm == 1) {
		buf[cnt++] = uarths->rxdata.data;
		if (cnt <= 3) {
			if (buf[0] != 0xAA)
				cnt = 0;
			else if (cnt == 3) {
				len = ((uint16_t)buf[1] << 8) | buf[2];
				if (len > UART_BUF_LENGTH_MAX || len < 4)
					cnt = 0;
			}
		} else if (cnt >= len) {
			uint8_t sum = 0;
			for (cnt = 0; cnt < len - 1; cnt++)
				sum += buf[cnt];
			if (buf[len - 1] != sum) {
				cnt = 0;
				continue;
			} else {
				uarths->rxctrl.rxen  = 0;
				burner_attr.uart_status = 1;
				cnt = 0;
				while (uarths->ip.rxwm == 1)
					sum = uarths->rxdata.data;
				return 0;
			}
		}
	}
	return 0;
}

void modle_burn(void){
	lcd_draw_string(116, 110, "Burn Model mode", WHITE);
	burner_attr.uart_buf = (uint8_t *)malloc(UART_BUF_LENGTH_MAX);
	if (burner_attr.uart_buf == NULL)
		return;
	burner_attr.uart_status = 0;
	burner_attr.flash_status = 0;
	plic_set_priority(IRQN_UARTHS_INTERRUPT, 1);
	plic_irq_register(IRQN_UARTHS_INTERRUPT, uarths_irq, NULL);
	plic_irq_enable(IRQN_UARTHS_INTERRUPT);
	// system start
	//printf("system start\r\n");
	// enable global interrupt
	set_csr(mstatus, MSTATUS_MIE);
	while (1) {
		// wait a valid pack
		while (burner_attr.uart_status == 0);//printf("test\n");

		if (burner_attr.uart_buf[3] == 0x01) { // read status cmd
			burner_attr.flash_status = w25qxx_is_busy();
		} else if (burner_attr.uart_buf[3] == 0x02) { // erase cmd
			uint32_t addr;

			addr = ((uint32_t)burner_attr.uart_buf[5] << 24) |
				((uint32_t)burner_attr.uart_buf[6] << 16) |
				((uint32_t)burner_attr.uart_buf[7] << 8) |
				((uint32_t)burner_attr.uart_buf[8] << 0);
			if (burner_attr.uart_buf[4] == 0x00)
				w25qxx_sector_erase(addr);
			else if (burner_attr.uart_buf[4] == 0x01)
				w25qxx_32k_block_erase(addr);
			else
				w25qxx_64k_block_erase(addr);
			burner_attr.flash_status = 1;
		} else if (burner_attr.uart_buf[3] == 0x03) { // write cmd
			uint32_t addr, len;

			addr = ((uint32_t)burner_attr.uart_buf[4] << 24) |
				((uint32_t)burner_attr.uart_buf[5] << 16) |
				((uint32_t)burner_attr.uart_buf[6] << 8) |
				((uint32_t)burner_attr.uart_buf[7] << 0);
			len = ((uint32_t)burner_attr.uart_buf[8] << 24) |
				((uint32_t)burner_attr.uart_buf[9] << 16) |
				((uint32_t)burner_attr.uart_buf[10] << 8) |
				((uint32_t)burner_attr.uart_buf[11] << 0);
			w25qxx_page_program_fun(addr, &burner_attr.uart_buf[12], len);
			burner_attr.flash_status = 1;
		} else { // invalid cmd
			burner_attr.flash_status = 0xFE;
		}
		burner_attr.uart_buf[0] = 0xAA;
		burner_attr.uart_buf[1] = 0x00;
		burner_attr.uart_buf[2] = 0x06;
		burner_attr.uart_buf[3] = 0x00;
		burner_attr.uart_buf[4] = burner_attr.flash_status;
		burner_attr.uart_buf[5] = 0x00;
		for (uint32_t i = 0; i < 5; i++)
			burner_attr.uart_buf[5] += burner_attr.uart_buf[i];
		for (uint32_t i = 0; i < 6; i++) {
			while (uarths->txdata.full)
				;
			uarths->txdata.data = burner_attr.uart_buf[i];
		}
		burner_attr.uart_status = 0;
		uarths->rxctrl.rxen = 1;
	}
}

STATIC mp_obj_t machine_burner_start(machine_burner_obj_t *self_in){

	machine_burner_obj_t* self = self_in;
	sysctl_pll_set_freq(SYSCTL_PLL0,self->burn_uarths_freq);
	uint32_t freq = sysctl_clock_get_freq(SYSCTL_CLOCK_CPU);
    uint16_t div = freq / self->baudrate - 1;
    uarths->div.div = div;
	modle_burn();
	return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_buner_start_obj, machine_burner_start);

STATIC mp_obj_t machine_burner_init_helper(machine_burner_obj_t *self_in) {
	machine_burner_obj_t* self = self_in;
	self->burn_uarths_freq = 320000000;
	self->baudrate = 1500000;
	while (1) {
		uint8_t manuf_id, device_id;
		w25qxx_init(3);
		w25qxx_read_id(&manuf_id, &device_id);
		printf("[MAIXPY]Flash:0x%02x:0x%02x\n", manuf_id, device_id);
		if (manuf_id != 0xFF && manuf_id != 0x00 && device_id != 0xFF && device_id != 0x00)
			break;
	}
	w25qxx_enable_quad_mode();
	return mp_const_none;
}


STATIC mp_obj_t machine_burner_init(mp_obj_t self_in) {
	machine_burner_obj_t* self = self_in;
    return machine_burner_init_helper(self);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_buner_init_obj, machine_burner_init);


STATIC mp_obj_t machine_burner_make_new() {
    
    volatile machine_burner_obj_t *self = m_new_obj(machine_burner_obj_t);
    self->base.type = &machine_burner_type;
    return self;
}


STATIC const mp_rom_map_elem_t pyb_burner_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_buner_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&machine_buner_start_obj) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_burner_locals_dict, pyb_burner_locals_dict_table);

const mp_obj_type_t machine_burner_type = {
    { &mp_type_type },
    .name = MP_QSTR_burner,
    .make_new = machine_burner_make_new,
    .locals_dict = (mp_obj_dict_t*)&pyb_burner_locals_dict,
};

