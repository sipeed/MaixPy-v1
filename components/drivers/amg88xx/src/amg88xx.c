#include "amg88xx.h"
#include "sipeed_i2c.h"
#include "fpioa.h"
#include "errno.h"
#include "sleep.h"
#include "stdlib.h"


int amg88xx_init(amg88xx_t *obj,
		i2c_device_number_t i2c_num, uint8_t i2c_addr, uint32_t i2c_freq)
{
	obj->i2c_num  = i2c_num;
	obj->i2c_addr = i2c_addr;
	obj->i2c_freq = i2c_freq;
	obj->scl_pin  = fpioa_get_io_by_function(FUNC_I2C0_SCLK + i2c_num * 2);
	obj->sda_pin  = fpioa_get_io_by_function(FUNC_I2C0_SDA  + i2c_num * 2);
	obj->len      = 64;

	// clear var
	obj->intc.value = 0x00;

	maix_i2c_init(i2c_num, 7, i2c_freq);

	// Set POWER CTL MODE
	uint8_t temp[2] = {AMG88XX_ADDR_PCTL, AMG88XX_PCTL_NORMAL};
	int ret = maix_i2c_send_data(i2c_num, i2c_addr, temp, 2, 100);
	if (ret != 0) return EIO;

	// Reset
	temp[0] = AMG88XX_ADDR_RST; temp[1] = AMG88XX_RST_INITIAL;
	ret = maix_i2c_send_data(i2c_num, i2c_addr, temp, 2, 100);
	if (ret != 0) return EIO;

	// Disable Interrupt
	obj->intc.INTEN  = AMG88XX_INTC_EN_REACTIVE;
	temp[0] = AMG88XX_ADDR_INTC; temp[1] = obj->intc.value;
	ret = maix_i2c_send_data(i2c_num, i2c_addr, temp, 2, 100);
	if (ret != 0) return EIO;

	// Set 10 FPS
	temp[0] = AMG88XX_ADDR_FPSC; temp[1] = AMG88XX_FPSC_10FPS;
	ret = maix_i2c_send_data(i2c_num, i2c_addr, temp, 2, 100);
	if (ret != 0) return EIO;

	msleep(65);

	obj->is_init = true;

	return 0;
}

void amg88xx_destroy(amg88xx_t *obj)
{
	obj->is_init = false;
}

int amg88xx_snapshot(amg88xx_t *obj, int16_t **pixels)
{

	if (!pixels) return EINVAL;

	uint8_t * p = obj->temp;

	uint8_t temp[2] = {AMG88XX_TABLEOFFSET, 0};
	int ret = maix_i2c_recv_data(obj->i2c_num, obj->i2c_addr, temp, 1, p, 128, 200);
	if (ret < 0) return EIO;

	uint8_t i = 0;
	uint8_t z = 0;
	int32_t v = 0;

	for (i = 0; i < 128; i+=2)
	{
		v =  0;
		v = p[i+1] << 8 | p[i];
		obj->v[z] = (p[i+1] & 1 << 3) ? v | (0x7FFFF << 13) : v;
		z++;
	}

	*pixels = obj->v;

	return ret;
}
