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
#include "py/mpprint.h"
#define I2C_FREQUENCY   (100000)
#define I2C_TIMEOUT     (1000)

//static I2C_HandleTypeDef I2CHandle;

int cambus_init(uint8_t reg_wid)
{
	dvp_init(reg_wid);
    return 0;
}
int cambus_read_id(uint8_t addr,uint16_t *manuf_id, uint16_t *device_id)
{
	dvp_sccb_send_data(addr, 0xFF, 0x01);
	*manuf_id = (dvp_sccb_receive_data(addr, 0x1C) << 8) | dvp_sccb_receive_data(addr, 0x1D);
	*device_id = (dvp_sccb_receive_data(addr, 0x0A) << 8) | dvp_sccb_receive_data(addr, 0x0B);
	return 0;
}

int cambus_scan()
{

	uint16_t manuf_id = 0;
	uint16_t device_id = 0;
    for (uint8_t addr=0x08; addr<=0x77; addr++) {
		cambus_read_id(addr ,&manuf_id,&device_id);
		if(0xffff != device_id)
		{
			return addr ;
		}
    }
    return 0;
}
int cambus_scan_gc0328(void)
{
    dvp_sccb_send_data(GC0328_ADDR, 0xFE, 0x00);
    uint8_t id = dvp_sccb_receive_data(GC0328_ADDR, 0xf0);
    if (id != 0x9d)
    {
        mp_printf(&mp_plat_print, "error gc0328 detect, ret id is 0x%x\r\n", id);
        return 0;
    }
    return id;
}
int cambus_readb(uint8_t slv_addr, uint8_t reg_addr, uint8_t *reg_data)
{

    int ret = 0;
	*reg_data = dvp_sccb_receive_data(slv_addr, reg_addr);

	if(0xff == *reg_data)
		ret = -1;

    return ret;

}

int cambus_writeb(uint8_t slv_addr, uint8_t reg_addr, uint8_t reg_data)
{

    int ret=0;
	dvp_sccb_send_data(slv_addr,reg_addr,reg_data);
	return 0;
}

int cambus_readw(uint8_t slv_addr, uint8_t reg_addr, uint16_t *reg_data)
{
    return 0;
}

int cambus_writew(uint8_t slv_addr, uint8_t reg_addr, uint16_t reg_data)
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
