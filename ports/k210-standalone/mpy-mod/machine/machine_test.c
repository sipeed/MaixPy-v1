#include <stdio.h>
#include <stdlib.h>



#include "py/obj.h"
#include "py/runtime.h"
#include "modmachine.h"
#include "mphalport.h"

#include "sysctl.h"
#include "plic.h"
#include "dmac.h"
#include "lcd.h"
#include "dvp.h"
#include "ov2640.h"
#include "ov5640.h"
#include "uarths.h"
#include "fpioa.h"
#include "gpiohs.h"
#include "st7789.h"
#include "sleep.h"
/*
#define OV2640

uint8_t buf_sel;
volatile uint8_t buf_used[2];
uint32_t *lcd_buf[2];

typedef struct _machine_test_obj_t {
	mp_obj_base_t base;
} machine_test_obj_t;
*/

/**
 *	IO7-IO35
 *	FUNC_GPIOHS3...FUNC_GPIOHS31
 *	通过STM32串口控制IO电平
 *	ON\r\n
 *	OFF\r\n
 * */
 
 /*
void init_gpio_for_test(void)
{
	printf("%s\r\n",__func__);
	for(uint8_t index = 0; index < 28; index++)
	{
		if(17 == index + 8|| 6 == index)continue;
		printf("index:%d IO%d --> FUNC_GPIOHS%d\r\n", index,index + 8, index + 4);
    	fpioa_set_function(index + 8, FUNC_GPIOHS4 + index);
    	gpiohs_set_drive_mode(4 + index, GPIO_DM_OUTPUT);
    	gpiohs_set_pin(4 + index, GPIO_PV_LOW);
	}
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_test_init_obj, init_gpio_for_test);

void set_gpio_val(uint8_t val)
{
	if(val)
	{
		gpiohs->output_val.bits.b4 = 1;
		gpiohs->output_val.bits.b5 = 1;
		gpiohs->output_val.bits.b6 = 1;
		gpiohs->output_val.bits.b7 = 1;

		gpiohs->output_val.bits.b8 = 1;
		gpiohs->output_val.bits.b9 = 1;
		gpiohs->output_val.bits.b10 = 1;
		gpiohs->output_val.bits.b11 = 1;
		gpiohs->output_val.bits.b12 = 1;
		gpiohs->output_val.bits.b13 = 1;

		gpiohs->output_val.bits.b14 = 1;
		gpiohs->output_val.bits.b15 = 1;
		gpiohs->output_val.bits.b16 = 1;
		gpiohs->output_val.bits.b17 = 1;
		gpiohs->output_val.bits.b18 = 1;
		gpiohs->output_val.bits.b19 = 1;

		gpiohs->output_val.bits.b20 = 1;
		gpiohs->output_val.bits.b21 = 1;
		gpiohs->output_val.bits.b22 = 1;
		gpiohs->output_val.bits.b23 = 1;
		gpiohs->output_val.bits.b24 = 1;
		gpiohs->output_val.bits.b25 = 1;

		gpiohs->output_val.bits.b26 = 1;
		gpiohs->output_val.bits.b27 = 1;
		gpiohs->output_val.bits.b28 = 1;
		gpiohs->output_val.bits.b29 = 1;
		gpiohs->output_val.bits.b30 = 1;
		gpiohs->output_val.bits.b31 = 1;
	}
	else
	{
		gpiohs->output_val.bits.b4 = 0;
		gpiohs->output_val.bits.b5 = 0;
		gpiohs->output_val.bits.b6 = 0;
		gpiohs->output_val.bits.b7 = 0;

		gpiohs->output_val.bits.b8 = 0;
		gpiohs->output_val.bits.b9 = 0;
		gpiohs->output_val.bits.b10 = 0;
		gpiohs->output_val.bits.b11 = 0;
		gpiohs->output_val.bits.b12 = 0;
		gpiohs->output_val.bits.b13 = 0;

		gpiohs->output_val.bits.b14 = 0;
		gpiohs->output_val.bits.b15 = 0;
		gpiohs->output_val.bits.b16 = 0;
		gpiohs->output_val.bits.b17 = 0;
		gpiohs->output_val.bits.b18 = 0;
		gpiohs->output_val.bits.b19 = 0;

		gpiohs->output_val.bits.b20 = 0;
		gpiohs->output_val.bits.b21 = 0;
		gpiohs->output_val.bits.b22 = 0;
		gpiohs->output_val.bits.b23 = 0;
		gpiohs->output_val.bits.b24 = 0;
		gpiohs->output_val.bits.b25 = 0;

		gpiohs->output_val.bits.b26 = 0;
		gpiohs->output_val.bits.b27 = 0;
		gpiohs->output_val.bits.b28 = 0;
		gpiohs->output_val.bits.b29 = 0;
		gpiohs->output_val.bits.b30 = 0;
		gpiohs->output_val.bits.b31 = 0;
	}
}


int machine_test_make_new(void)
{
	machine_test_obj_t *self = m_new_obj(machine_test_obj_t);
	self->base.type = &machine_test_type;

	return self;
}
int machine_test_gpio(void)
{
	uint32_t val = 0;
	uint32_t last_val = 0;
	fpioa_set_function(17, FUNC_GPIOHS3);
	gpiohs_set_drive_mode(3, GPIO_DM_INPUT);
	val = gpiohs_get_pin(3);
	if(val != last_val)
	{
		if(val == 0x01)
		{
			printf("set all gpiohs output 1\r\n");
			set_gpio_val(1);
		}
		else
		{
			printf("set all gpiohs output 0\r\n");
			set_gpio_val(0);
		}
		last_val = val;
	}
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_test_gpio_obj, machine_test_gpio);
*/
const mp_obj_type_t machine_test_type;
#define OV2640
	
uint8_t buf_sel;
volatile uint8_t buf_used[2];
uint32_t *lcd_buf[2];

static int dvp_irq(void *ctx)
{
	if (dvp_get_interrupt(DVP_STS_FRAME_FINISH)) {
		dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
		buf_used[buf_sel] = 1;
		buf_sel ^= 0x01;
		dvp_set_display_addr((uint32_t)lcd_buf[buf_sel]);
	} else {
		dvp_clear_interrupt(DVP_STS_FRAME_START);
		if (buf_used[buf_sel] == 0)
		{
			dvp_start_convert();
		}
	}
	return 0;
}
	
#if 0
#define UART_BUF_LENGTH_MAX 256
	
	uint8_t *uart_buf;
	uint8_t uart_status;
	static uint8_t start_recv = 0;
	static uint8_t recv_cnt = 0;
	
	static int uarths_irq(void *ctx)
	{
		static uint16_t cnt, len;
	
		while (uarths->ip.rxwm == 1) {
			if(start_recv)
			{
				uart_buf[recv_cnt++] = uarths->rxdata.data;
				if(uart_buf[recv_cnt-1] == 0x05)
				{
					uart_status = 1;
					start_recv = 0;
				}
			}
			else if(uarths->rxdata.data == 0xaa && uart_status == 0)
			{
				start_recv = 1;
				recv_cnt = 0;
				uart_buf[recv_cnt++] = 0xaa;	//也许是个bug，如果这里写uarths->rxdata.data，会数据错误
			}
		}
		return 0;
	}
	
	void uart_int_init(void)
	{
		printf("%s\r\n",__func__);
		uart_buf = (uint8_t *)malloc(UART_BUF_LENGTH_MAX);
		if (uart_buf == NULL)
			return;
		uart_status = 0;
	
		plic_set_priority(IRQN_UARTHS_INTERRUPT, 1);
		plic_irq_register(IRQN_UARTHS_INTERRUPT, uarths_irq, NULL);
		plic_irq_enable(IRQN_UARTHS_INTERRUPT);
		// enable global interrupt
		set_csr(mstatus, MSTATUS_MIE);
	}
#endif
	/**
	 *	IO7-IO35
	 *	FUNC_GPIOHS3...FUNC_GPIOHS31
	 *	通过STM32串口控制IO电平
	 *	ON\r\n
	 *	OFF\r\n
	 * */
void init_gpio_for_test(void)
{
	
	printf("%s\r\n",__func__);
	for(uint8_t index = 0; index < 28; index++)
	{
		printf("index:%d IO%d --> FUNC_GPIOHS%d\r\n", index,index + 8, index + 4);
		fpioa_set_function(index + 8, FUNC_GPIOHS4 + index);
		gpiohs_set_drive_mode(4 + index, GPIO_DM_OUTPUT);
		gpiohs_set_pin(4 + index, GPIO_PV_LOW);
	}
	
}

void set_gpio_val(uint8_t val)
{
	if(val)
	{
		
		gpiohs->output_val.bits.b4 = 1;
		gpiohs->output_val.bits.b5 = 1;
		gpiohs->output_val.bits.b6 = 1;
		gpiohs->output_val.bits.b7 = 1;
		
		gpiohs->output_val.bits.b8 = 1;
		gpiohs->output_val.bits.b9 = 1;
		gpiohs->output_val.bits.b10 = 1;
		gpiohs->output_val.bits.b11 = 1;
		gpiohs->output_val.bits.b12 = 1;
		gpiohs->output_val.bits.b13 = 1;
		
		gpiohs->output_val.bits.b14 = 1;
		gpiohs->output_val.bits.b15 = 1;
		gpiohs->output_val.bits.b16 = 1;
		gpiohs->output_val.bits.b17 = 1;
		gpiohs->output_val.bits.b18 = 1;
		gpiohs->output_val.bits.b19 = 1;
		
		gpiohs->output_val.bits.b20 = 1;
		gpiohs->output_val.bits.b21 = 1;
		gpiohs->output_val.bits.b22 = 1;
		gpiohs->output_val.bits.b23 = 1;
		gpiohs->output_val.bits.b24 = 1;
		gpiohs->output_val.bits.b25 = 1;

		gpiohs->output_val.bits.b26 = 1;
		gpiohs->output_val.bits.b27 = 1;
		gpiohs->output_val.bits.b28 = 1;
		gpiohs->output_val.bits.b29 = 1;
		gpiohs->output_val.bits.b30 = 1;
		gpiohs->output_val.bits.b31 = 1;
	}
	else
	{
		
		gpiohs->output_val.bits.b4 = 0;
		gpiohs->output_val.bits.b5 = 0;
		gpiohs->output_val.bits.b6 = 0;
		gpiohs->output_val.bits.b7 = 0;
		
		gpiohs->output_val.bits.b8 = 0;
		gpiohs->output_val.bits.b9 = 0;
		gpiohs->output_val.bits.b10 = 0;
		
		gpiohs->output_val.bits.b11 = 0;
		gpiohs->output_val.bits.b12 = 0;
		gpiohs->output_val.bits.b13 = 0;
		
		gpiohs->output_val.bits.b14 = 0;
		gpiohs->output_val.bits.b15 = 0;
		gpiohs->output_val.bits.b16 = 0;
		gpiohs->output_val.bits.b17 = 0;
		gpiohs->output_val.bits.b18 = 0;
		gpiohs->output_val.bits.b19 = 0;
		
		gpiohs->output_val.bits.b20 = 0;
		gpiohs->output_val.bits.b21 = 0;
		gpiohs->output_val.bits.b22 = 0;
		gpiohs->output_val.bits.b23 = 0;
		gpiohs->output_val.bits.b24 = 0;
		gpiohs->output_val.bits.b25 = 0;

		gpiohs->output_val.bits.b26 = 0;
		gpiohs->output_val.bits.b27 = 0;
		gpiohs->output_val.bits.b28 = 0;
		gpiohs->output_val.bits.b29 = 0;
		gpiohs->output_val.bits.b30 = 0;
		gpiohs->output_val.bits.b31 = 0;
	}
}

int machine_test_make_new(void)
{
	uint64_t core_id = current_coreid();
	uint16_t manuf_id, device_id;
	volatile uint8_t val = 0, last_val = 0;

	void *ptr;

	if (core_id == 0)
	{
		sysctl_pll_set_freq(SYSCTL_CLOCK_PLL0,320000000);
		uarths_init();
		printf("pll0 freq:%d\r\n",sysctl_clock_get_freq(SYSCTL_CLOCK_PLL0));

		sysctl->power_sel.power_mode_sel6 = 1;
		sysctl->power_sel.power_mode_sel7 = 1;
		sysctl->misc.spi_dvp_data_enable = 1;

		plic_init();
		dmac->reset = 0x01;
		while (dmac->reset);
		dmac->cfg = 0x03;

		// LCD init
		printf("LCD init\r\n");
		lcd_init();
		lcd_clear(BLUE);
		// DVP init
		printf("DVP init\r\n");
#if 0
		uart_int_init();
#endif
#ifdef OV2640
		do {
			printf("init ov2640\r\n");
			ov2640_init();
			ov2640_read_id(&manuf_id, &device_id);
			printf("manuf_id:0x%04x,device_id:0x%04x\r\n", manuf_id, device_id);
		} while (manuf_id != 0x7FA2 || device_id != 0x2642);
#else
		do{
			myov5640_init();
			device_id = ov5640_read_id();
			printf("ov5640 id:%d\r\n",device_id);
		}
		while (device_id!=0x5640);
#endif

		ptr = malloc(sizeof(uint8_t) * 320 * 240 * (2 * 2) + 127);
		if (ptr == NULL)
			return;

		lcd_buf[0] = (uint32_t *)(((uint32_t)ptr + 127) & 0xFFFFFF80);
		lcd_buf[1] = (uint32_t *)((uint32_t)lcd_buf[0] + 320 * 240 * 2);
		buf_used[0] = 0;
		buf_used[1] = 0;
		buf_sel = 0;

		dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
		dvp_set_display_addr((uint32_t)lcd_buf[buf_sel]);
		plic_set_priority(IRQN_DVP_INTERRUPT, 1);
		plic_irq_register(IRQN_DVP_INTERRUPT, dvp_irq, NULL);
		plic_irq_enable(IRQN_DVP_INTERRUPT);
		// system start
		printf("system start\n");
		set_csr(mstatus, MSTATUS_MIE);
		dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
		dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);



		//初始化IO
		init_gpio_for_test();
		fpioa_set_function(11, FUNC_GPIOHS3);
		gpiohs_set_drive_mode(3, GPIO_DM_INPUT);
		while(1)
		{
			// printf("i am alive 1\r\n");
			while(buf_used[buf_sel]==0);
			// printf("i am alive 2\r\n");
			lcd_draw_picture(0, 0, 320, 240, lcd_buf[buf_sel]);
			// printf("i am alive 3\r\n");
			while (tft_busy());
			buf_used[buf_sel] = 0;

			val = gpiohs_get_pin(3);
			
			if(val != last_val)
			{	
				
				if(val == 0x01)
				{
					printf("set all gpiohs output 1\r\n");
					set_gpio_val(1);
				}
				else
				{
					printf("set all gpiohs output 0\r\n");
					set_gpio_val(0);
				}
				
				last_val = val;
			}
			
		}
	}
	while(1);
}

STATIC const mp_rom_map_elem_t machine_test_locals_dict_table[] = {
	//{ MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_test_init_obj) },
	//{ MP_ROM_QSTR(MP_QSTR_gpio), MP_ROM_PTR(&machine_test_gpio_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_test_locals_dict, machine_test_locals_dict_table);

const mp_obj_type_t machine_test_type = {
    { &mp_type_type },
    .name = MP_QSTR_Test,
    .make_new = machine_test_make_new,
    .locals_dict = (mp_obj_t)&machine_test_locals_dict,
};

