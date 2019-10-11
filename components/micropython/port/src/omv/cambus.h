/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Camera bus driver.
 *
 */
#ifndef __CAMBUS_H__
#define __CAMBUS_H__
#include <stdint.h>
int cambus_init(uint8_t reg_wid, int8_t i2c, int8_t pin_clk, int8_t pin_sda, uint8_t gpio_clk, uint8_t gpio_sda);
int cambus_scan();
int cambus_scan_gc0328(void);
int cambus_readb(uint8_t slv_addr, uint16_t reg_addr,  uint8_t *reg_data);
int cambus_writeb(uint8_t slv_addr, uint16_t reg_addr, uint8_t reg_data);
int cambus_readw(uint8_t slv_addr, uint16_t reg_addr,  uint16_t *reg_data);
int cambus_writew(uint8_t slv_addr, uint16_t reg_addr, uint16_t reg_data);
int cambus_readw2(uint8_t slv_addr, uint16_t reg_addr,  uint16_t *reg_data);
int cambus_writew2(uint8_t slv_addr, uint16_t reg_addr, uint16_t reg_data);
uint8_t cambus_reg_width();
#endif // __CAMBUS_H__
