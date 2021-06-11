/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * SCCB (I2C like) driver.
 *
 */
#include <stdbool.h>
#include "omv_boardconfig.h"
#include "cambus.h"
#include "dvp.h"
#include "gc0328.h"
#include "gc2145.h"
#include "mt9d111.h"
#include "py/mpprint.h"
#include "sysctl.h"
#include "fpioa.h"
#include "sipeed_i2c.h"
#include "mphalport.h"

static uint32_t write_bus_delay = 10; //ms

void cambus_set_writeb_delay(uint32_t delay)
{
    write_bus_delay = delay;
}

/**
 * @i2c -2: hardware sccb i2c, -1: soft, [0,2]: hardware i2c
 */
int sccb_i2c_init(int8_t i2c, uint8_t pin_clk, uint8_t pin_sda, uint8_t gpio_clk, uint8_t gpio_sda, uint32_t freq)
{
    if (i2c == -2)
    {
        // dvp_sccb_set_clk_rate(1000000);
        fpioa_set_function(pin_clk, FUNC_SCCB_SCLK);
        fpioa_set_function(pin_sda, FUNC_SCCB_SDA);
    }
    else if (i2c == -1)
    {
    }
    else
    {
        if (i2c > 2 || i2c < 0)
            return -1;
        mp_printf(&mp_plat_print, "init i2c:%d freq:%d\r\n", i2c, freq);
        fpioa_set_function(pin_clk, FUNC_I2C0_SCLK + i2c * 2);
        fpioa_set_function(pin_sda, FUNC_I2C0_SDA + i2c * 2);
        maix_i2c_init((i2c_device_number_t)i2c, 7, freq);
    }
    return 0;
}

int sccb_i2c_write_byte(int8_t i2c, uint8_t addr, uint16_t reg, uint8_t reg_len, uint8_t data, uint16_t timeout_ms)
{
    if (i2c == -2)
    {
        dvp_sccb_send_data(addr, reg, data);
    }
    else if (i2c == -1)
    {
    }
    else
    {
        if (i2c > 2 || i2c < 0)
            return -1;
        uint8_t tmp[3];
        if (reg_len == 8)
        {
            tmp[0] = reg & 0xFF;
            tmp[1] = data;
            return maix_i2c_send_data((i2c_device_number_t)i2c, addr, tmp, 2, 10);
        }
        else
        {
            tmp[0] = (reg >> 8) & 0xFF;
            tmp[1] = reg & 0xFF;
            tmp[2] = data;
            return maix_i2c_send_data((i2c_device_number_t)i2c, addr, tmp, 3, 10);
        }
    }
    return 0;
}

int sccb_i2c_read_byte(int8_t i2c, uint8_t addr, uint16_t reg, uint8_t reg_len, uint8_t *data, uint16_t timeout_ms)
{
    *data = 0;
    if (i2c == -2)
    {
        *data = dvp_sccb_receive_data(addr, reg);
    }
    else if (i2c == -1)
    {
    }
    else
    {
        if (i2c > 2 || i2c < 0)
            return -1;
        uint8_t tmp[2];
        if (reg_len == 8)
        {
            tmp[0] = reg & 0xFF;
            maix_i2c_send_data((i2c_device_number_t)i2c, addr, tmp, 1, 10);
            int ret = maix_i2c_recv_data((i2c_device_number_t)i2c, addr, NULL, 0, data, 1, 10);
            return ret;
        }
        else
        {
            tmp[0] = (reg >> 8) & 0xFF;
            tmp[1] = reg & 0xFF;
            int ret = maix_i2c_send_data((i2c_device_number_t)i2c, addr, tmp, 2, 10);
            if (ret != 0)
                return ret;
            ret = maix_i2c_recv_data((i2c_device_number_t)i2c, addr, NULL, 0, data, 1, 10);
            return ret;
        }
    }
    return 0;
}

int sccb_i2c_recieve_byte(int8_t i2c, uint8_t addr, uint8_t *data, uint16_t timeout_ms)
{
    if (i2c == -2)
    {
        *data = dvp_sccb_receive_data(addr, 0x00);
        return *data;
    }
    else if (i2c == -1)
    {
    }
    else
    {
        if (i2c > 2 || i2c < 0)
            return -1;
        int ret = maix_i2c_recv_data((i2c_device_number_t)i2c, addr, NULL, 0, data, 1, 10);
        return ret;
    }
    return 0;
}

static uint8_t sccb_reg_width = 8;
static int8_t i2c_device = -2;

/**
 * 
 * @i2c_type 0: sccb, 1:hardware i2c, 2:software i2c
 * 
 */
int cambus_init(uint8_t reg_wid, int8_t i2c, int8_t pin_clk, int8_t pin_sda, uint8_t gpio_clk, uint8_t gpio_sda)
{
    dvp_init(reg_wid);
    sccb_reg_width = reg_wid;
    if (pin_clk < 0 || pin_sda < 0)
        return -1;
    i2c_device = i2c;
    sccb_i2c_init(i2c_device, pin_clk, pin_sda, gpio_clk, gpio_sda, 100000);
    return 0;
}
int cambus_read_id(uint8_t addr, uint16_t *manuf_id, uint16_t *device_id)
{
    *manuf_id = 0;
    *device_id = 0;
    uint8_t tmp = 0;
    int ret = 0;

    // sccb_i2c_write_byte(i2c_device, addr, 0xFF, sccb_reg_width, 0x01, 10);
    ret |= sccb_i2c_read_byte(i2c_device, addr, 0x1C, sccb_reg_width, &tmp, 100);
    *manuf_id = tmp << 8;
    ret |= sccb_i2c_read_byte(i2c_device, addr, 0x1D, sccb_reg_width, &tmp, 100);
    *manuf_id |= tmp;
    ret |= sccb_i2c_read_byte(i2c_device, addr, 0x0A, sccb_reg_width, &tmp, 100);
    *device_id = tmp << 8;
    ret |= sccb_i2c_read_byte(i2c_device, addr, 0x0B, sccb_reg_width, &tmp, 100);
    *device_id |= tmp;
    // printk("ret:%d %04x %04x\r\n",ret, *manuf_id, *device_id);
    return ret;
}

int cambus_read16_id(uint8_t addr, uint16_t *manuf_id, uint16_t *device_id)
{
    *manuf_id = 0;
    *device_id = 0;
    uint8_t tmp = 0;
    int ret = 0;
    //TODO: 0x300A 0x300B maybe just for OV3660
    // sccb_i2c_write_byte(i2c_device, addr, 0xFF, sccb_reg_width, 0x01, 10);
    ret |= sccb_i2c_read_byte(i2c_device, addr, 0x300A, sccb_reg_width, &tmp, 100);
    if (ret != 0)
        return ret;
    *device_id = tmp << 8;
    ret |= sccb_i2c_read_byte(i2c_device, addr, 0x300B, sccb_reg_width, &tmp, 100);
    *device_id |= tmp;
    // printk("ret:%d %04x %04x\r\n",ret, *manuf_id, *device_id);
    return ret;
}

int cambus_scan()
{
    uint16_t manuf_id = 0;
    uint16_t device_id = 0;
    sccb_reg_width = 8;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++)
    {
        if (cambus_read_id(addr, &manuf_id, &device_id) != 0)
            continue;
        if (device_id != 0 && device_id != 0xffff)
        {
            return addr;
        }
    }
    sccb_reg_width = 16;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++)
    {
        if (cambus_read16_id(addr, &manuf_id, &device_id) != 0)
            continue;
        if (device_id != 0 && device_id != 0xffff)
        {
            return addr;
        }
    }
    return 0;
}
int cambus_scan_gc0328(void)
{
    uint8_t id;
    sccb_reg_width = 8;
    sccb_i2c_write_byte(i2c_device, GC0328_ADDR, 0xFE, sccb_reg_width, 0x00, 10);
    sccb_i2c_read_byte(i2c_device, GC0328_ADDR, 0xF0, sccb_reg_width, &id, 10);
    if (id != 0x9d)
    {
        // mp_printf(&mp_plat_print, "error gc0328 detect, ret id is 0x%x\r\n", id);
        return 0;
    }
    return id;
}

int cambus_scan_gc2145(void)
{
    uint8_t id_h, id_l;
    uint16_t id;
    sccb_reg_width = 8;
    sccb_i2c_write_byte(i2c_device, GC2145_ADDR, 0xFE, sccb_reg_width, 0x00, 10);
    sccb_i2c_read_byte(i2c_device, GC2145_ADDR, 0xF0, sccb_reg_width, &id_h, 10);
    sccb_i2c_read_byte(i2c_device, GC2145_ADDR, 0xF1, sccb_reg_width, &id_l, 10);
    id = (id_h<<8) | id_l;
    if (id != 0x2145)
    {
        // mp_printf(&mp_plat_print, "error gc2145 detect, ret id is 0x%x\r\n", id);
        return 0;
    }
    return id;
}

int cambus_scan_mt9d111(void)
{
    uint16_t id = mt9d111_read_id(i2c_device);
    if (id != MT9D111_ID_CODE)
    {
        // mp_printf(&mp_plat_print, "error mt9d111 detect, ret id is 0x%x\r\n", id);
        return 0;
    }
    return id;
}

int cambus_readb(uint8_t slv_addr, uint16_t reg_addr, uint8_t *reg_data)
{

    int ret = 0;
    sccb_i2c_read_byte(i2c_device, slv_addr, reg_addr, sccb_reg_width, reg_data, 10);
    if (0xff == *reg_data)
        ret = -1;

    return ret;
}

int cambus_writeb(uint8_t slv_addr, uint16_t reg_addr, uint8_t reg_data)
{
    sccb_i2c_write_byte(i2c_device, slv_addr, reg_addr, sccb_reg_width, reg_data, 10);
    mp_hal_delay_ms(write_bus_delay);
    return 0;
}

int cambus_readw(uint8_t slv_addr, uint16_t reg_addr, uint16_t *reg_data)
{
    return 0;
}

int cambus_writew(uint8_t slv_addr, uint16_t reg_addr, uint16_t reg_data)
{
    return 0;
}

int cambus_readw2(uint8_t slv_addr, uint16_t reg_addr, uint16_t *reg_data)
{
    return 0;
}

int cambus_writew2(uint8_t slv_addr, uint16_t reg_addr, uint16_t reg_data)
{
    return 0;
}

uint8_t cambus_reg_width()
{
    return sccb_reg_width;
}
