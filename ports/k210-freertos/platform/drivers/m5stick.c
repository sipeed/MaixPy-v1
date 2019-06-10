
#ifdef MAIXPY_M5STICK

#include "sipeed_i2c.h"
#include "stdint.h"
#include "stdbool.h"
#include "fpioa.h"
#include "sysctl.h"


#define AXP192_I2C_PIN_SCL  28
#define AXP192_I2C_PIN_SDA  29
#define AXP192_ADDR         0x34

/**
 * 
 * @attention Will use I2C0, and release after init complete
 * 
 */
bool m5stick_init()
{
    uint8_t cmd[2];
    int ret = 0;

    sysctl_set_power_mode(SYSCTL_POWER_BANK3,SYSCTL_POWER_V33);

    //init power manager //TODO: pack API for power saving
    fpioa_set_function(AXP192_I2C_PIN_SCL, FUNC_I2C0_SCLK);
    fpioa_set_function(AXP192_I2C_PIN_SDA, FUNC_I2C0_SDA);
    maix_i2c_init(I2C_DEVICE_0, 7, 400000);
    ret = maix_i2c_recv_data(I2C_DEVICE_0, AXP192_ADDR, NULL, 0, cmd, 1);
    if (ret != 0)
        goto end;
    cmd[0] = 0x91;
    cmd[1] = 0xF0;
    ret = maix_i2c_send_data(I2C_DEVICE_0, AXP192_ADDR, cmd, 2);
    if(ret!=0)
        goto end;
    cmd[0] = 0x90;
    cmd[1] = 0x02;
    ret = maix_i2c_send_data(I2C_DEVICE_0, AXP192_ADDR, cmd, 2);
    if(ret!=0)
        goto end;
    cmd[0] = 0x28;
    cmd[1] = 0xA0;
    ret = maix_i2c_send_data(I2C_DEVICE_0, AXP192_ADDR, cmd, 2);
    if(ret!=0)
        goto end;
    cmd[0] = 0x27;
    cmd[1] = 0x2C;
    ret = maix_i2c_send_data(I2C_DEVICE_0, AXP192_ADDR, cmd, 2);
    if(ret!=0)
        goto end;
    cmd[0] = 0x12;
    cmd[1] = 0x1F;
    ret = maix_i2c_send_data(I2C_DEVICE_0, AXP192_ADDR, cmd, 2);
    if(ret!=0)
        goto end;
    cmd[0] = 0x10;
    cmd[1] = 0x01;
    ret = maix_i2c_send_data(I2C_DEVICE_0, AXP192_ADDR, cmd, 2);
    if(ret!=0)
        goto end;
    cmd[0] = 0x23;
    cmd[1] = 0x08;
    ret = maix_i2c_send_data(I2C_DEVICE_0, AXP192_ADDR, cmd, 2);
    if(ret!=0)
        goto end;
    
end:
    maix_i2c_deinit(I2C_DEVICE_0);
    return ret==0;
}


#endif

