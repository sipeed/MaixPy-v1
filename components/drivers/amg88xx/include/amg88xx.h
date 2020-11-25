#ifndef _AMG88XX_H
#define _AMG88XX_H

// 09-10-2020
// Author: Vinicio Valbuena

#include "stdint.h"
#include "sipeed_i2c.h"

#define AMG88XX_WIDTH  8
#define AMG88XX_HEIGHT 8

// Power Control Register
#define AMG88XX_ADDR_PCTL   0x00 // [R/W]
// Reset Register
#define AMG88XX_ADDR_RST    0x01 // [W]
// Frame Rate Register
#define AMG88XX_ADDR_FPSC   0x02 // [R/W]
// Interrupt Control Register
#define AMG88XX_ADDR_INTC   0x03 // [R/W]
// Status Register
#define AMG88XX_ADDR_STAT   0x04 // [R]
// Status Clear Register
#define AMG88XX_ADDR_SCLR   0x05 // [W]
// ADDR 0x06 RESERVED
// Average Register
#define AMG88XX_ADDR_AVE    0x07 // [R/W]
// Interrupt Level Register
#define AMG88XX_ADDR_INTHL  0x08 // [R/W]
#define AMG88XX_ADDR_INTHH  0x09 // [R/W]
#define AMG88XX_ADDR_INTLL  0x0A // [R/W]
#define AMG88XX_ADDR_INTLH  0x0B // [R/W]
#define AMG88XX_ADDR_IHYSL  0x0C // [R/W]
#define AMG88XX_ADDR_IHYSH  0x0D // [R/W]
// Thermistor Register
#define AMG88XX_ADDR_TTHL   0x0E // [R]
#define AMG88XX_ADDR_TTHH   0x0F // [R]
// Interrupt Table Register
#define AMG88XX_ADDR_INT0   0x10 // [R]
#define AMG88XX_ADDR_INT1   0x11 // [R]
#define AMG88XX_ADDR_INT2   0x12 // [R]
#define AMG88XX_ADDR_INT3   0x13 // [R]
#define AMG88XX_ADDR_INT4   0x14 // [R]
#define AMG88XX_ADDR_INT5   0x15 // [R]
#define AMG88XX_ADDR_INT6   0x16 // [R]
#define AMG88XX_ADDR_INT7   0x17 // [R]

// Temperature Register
#define AMG88XX_TABLEOFFSET 0x80 // [R]

// Power Control Register | Operating mode
#define AMG88XX_PCTL_NORMAL   0x00
#define AMG88XX_PCTL_SLEEP    0x10
#define AMG88XX_PCTL_STAND_60 0x20
#define AMG88XX_PCTL_STAND_10 0x21

// Reset Register | Operating mode
#define AMG88XX_RST_FLAG    0x30
#define AMG88XX_RST_INITIAL 0x3F

// Frame Rate Register
#define AMG88XX_FPSC_10FPS  0x00
#define AMG88XX_FPSC_1FPS   0x01

// Interrupt Control Register   [ NO TE OLVIDES DE MI ]
#define AMG88XX_INTC_MOD_DIFFERENCE 0x00
#define AMG88XX_INTC_MOD_ABSOLUTE   0x01
#define AMG88XX_INTC_EN_REACTIVE    0x00
#define AMG88XX_INTC_EN_ACTIVE      0x01

typedef union {

	uint8_t value;

	struct {
		// 0 = INT output reactive (Hi-Z)
		// 1 = INT output active
		uint8_t INTEN : 1;

		// 0 = Difference interrupt mode
		// 1 = absolute value interrupt mode
		uint8_t INTMOD : 1;
	};

} amg88xx_intc_t;


typedef struct {
	i2c_device_number_t  i2c_num;
	uint8_t              i2c_addr;         // 0x68
	uint32_t             i2c_freq;         // 100*1000
	uint8_t              scl_pin;
	uint8_t              sda_pin;
	int32_t              v[64];            // 8 * 8
	uint8_t              temp[128];        // 64 * 2
	uint8_t              len;              // 64

	amg88xx_intc_t       intc;

	bool                 is_init;
} amg88xx_t;


int  amg88xx_init(amg88xx_t *obj,
		i2c_device_number_t i2c_num, uint8_t i2c_addr, uint32_t i2c_freq);
void amg88xx_destroy(amg88xx_t *obj);
int  amg88xx_snapshot(amg88xx_t *obj, int16_t **pixels);

#endif
