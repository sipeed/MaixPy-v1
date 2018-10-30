#include <stdint.h>
#include "ws2812b.h"
#include "sleep.h"
#include "gpiohs.h"
#include "sysctl.h"
// Change this according to FPIOA, GPIO, and hardware configuration
#define WS2812B_PIN_CLR(gpio_num)                gpiohs_set_pin(gpio_num,GPIO_PV_LOW);                                   
#define WS2812B_PIN_SET(gpio_num)                gpiohs_set_pin(gpio_num,GPIO_PV_HIGH);  																


volatile static uint16_t ns_300_cnt = 0, ns_800_cnt = 0;

void init_nop_cnt(void)
{
	uint32_t cpu_freq = sysctl_clock_get_freq(SYSCTL_CLOCK_CPU);

	float per_nop_ns = 0.0;

	cpu_freq /= 1000000;
	per_nop_ns = 1000.0 / cpu_freq;

	printf("per_nop_ns:%f cpu_freq:%dMhz\r\n",per_nop_ns,cpu_freq);

	ns_300_cnt = 300.0 / per_nop_ns;
	ns_800_cnt = 800.0 / per_nop_ns;

	printf("ns_300_cnt:%d ns_800_cnt:%d\r\n",ns_300_cnt,ns_800_cnt);
}


// The duration / pulse width depends on the CPU clock frequency!
static inline void WS2812B_Tx0(int gpio_num)
{
	WS2812B_PIN_SET(gpio_num);
	//13--350ns
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");


	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");

	WS2812B_PIN_CLR(gpio_num);
	//45--850ns
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");

		asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
}

static inline void WS2812B_Tx1(int gpio_num)
{
	WS2812B_PIN_SET(gpio_num);
	//45--850ns
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");


	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	WS2812B_PIN_CLR(gpio_num);
	//13--450ns
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");

		asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
}

void WS2812B_TxRes(int gpio_num)
{
	WS2812B_PIN_CLR(gpio_num);
	usleep(60);
}


void WS2812B_SetLedRGB(uint8_t r, uint8_t g, uint8_t b,int gpio_num)
{
	uint64_t frameBuf = (g << 16) | (r << 8) | b;

	for (uint64_t mask = 0x800000; mask > 0; mask >>= 1) {
		if (frameBuf & mask)
		{
			WS2812B_Tx1(gpio_num);
		}
		else
		{
			WS2812B_Tx0(gpio_num);
		}
	}
}

void  WS2812B_SetLednRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t num,int gpio_num)
{
	uint64_t frameBuf = (g << 16) | (r << 8) | b;
	uint8_t cnt;

	for(cnt=0;cnt<num;cnt++)
	{
		for (uint64_t mask = 0x800000; mask > 0; mask >>= 1)
		{
			if (frameBuf & mask)
				WS2812B_Tx1(gpio_num);
			else
				WS2812B_Tx0(gpio_num);
		}
	}
}