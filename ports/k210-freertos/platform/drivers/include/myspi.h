#ifndef __MYSPI_H
#define __MYSPI_H

#include "gpiohs.h"
#include <stdint.h>

/* clang-format off */
#define SOFT_SPI                1
//out
#define SS_GPIONUM              10
/* clang-format on */

//ss
#define SOFT_SPI_CS_SET()                \
    {                                    \
        gpiohs->output_val.bits.b10 = 1; \
    }
#define SOFT_SPI_CS_CLR()                \
    {                                    \
        gpiohs->output_val.bits.b10 = 0; \
    }

#if SOFT_SPI

/* clang-format off */
//out
#define SCLK_GPIONUM            11
#define MOSI_GPIONUM            12
//in
#define MISO_GPIONUM            13
/* clang-format on */

//clk
#define SOFT_SPI_CLK_SET()               \
    {                                    \
        gpiohs->output_val.bits.b11 = 1; \
    }
#define SOFT_SPI_CLK_CLR()               \
    {                                    \
        gpiohs->output_val.bits.b11 = 0; \
    }

//mosi
#define SOFT_SPI_MOSI_SET()              \
    {                                    \
        gpiohs->output_val.bits.b12 = 1; \
    }
#define SOFT_SPI_MOSI_CLR()              \
    {                                    \
        gpiohs->output_val.bits.b12 = 0; \
    }
#else

#define SPI_SS 0

#endif

void soft_spi_init(void);
uint8_t soft_spi_rw(uint8_t data);
void soft_spi_rw_len(uint8_t *send, uint8_t *recv, uint8_t len);

uint64_t get_millis(void);

#endif
