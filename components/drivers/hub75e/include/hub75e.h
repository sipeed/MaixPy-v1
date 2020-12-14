#ifndef _HUB75E_H_
#define _HUB75E_H_

#define HORIZONTAL_PIXELS 64
#define VERTICAL_PIXELS 64
#define VERTICAL_PIXELS_HALF 32
#define PIXELS_COUNT (HORIZONTAL_PIXELS * VERTICAL_PIXELS)
#define PIXELS_COUNT_BYTES (PIXELS_COUNT / 8)
#define HORIZONTAL_PIXELS_BYTES (HORIZONTAL_PIXELS / 8)

#include <stdlib.h>
#include <string.h>

#include "color_table.h"
#include "dmac.h"
#include "fpioa.h"
#include "gpio.h"
#include "gpiohs.h"
#include "io.h"
#include "pwm.h"
#include "spi.h"
#include "sysctl.h"
#include "utils.h"

typedef struct
{
    uint8_t spi;
    uint8_t cs_pin;
    uint8_t r1_pin;
    uint8_t g1_pin;
    uint8_t b1_pin;
    uint8_t r2_pin;
    uint8_t g2_pin;
    uint8_t b2_pin;
    uint8_t a_gpio;
    uint8_t b_gpio;
    uint8_t c_gpio;
    uint8_t d_gpio;
    uint8_t e_gpio;
    uint8_t oe_gpio;
    uint8_t latch_gpio;
    uint8_t clk_pin;
    uint8_t dma_channel;
    uint16_t width;
    uint16_t height;
} hub75e_t;

#define GPIOHS_OUT_HIGH(io) (*(volatile uint32_t *)0x3800100CU) |= (1 << (io))
#define GPIOHS_OUT_LOW(io) (*(volatile uint32_t *)0x3800100CU) &= ~(1 << (io))
#define GET_GPIOHS_VALX(io) (((*(volatile uint32_t *)0x38001000U) >> (io)) & 1)

#define HUB75E_FUN_SPIxDv(x,v) FUNC_SPI##x##_D##v

void hub75e_init(hub75e_t* hub75e_obj);
void hub75e_display_start(hub75e_t* cur_hub75e_obj, uint16_t *cur_image);
void hub75e_display_stop(void);
#endif
