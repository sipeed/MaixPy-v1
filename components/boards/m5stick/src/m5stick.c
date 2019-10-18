#include "sipeed_i2c.h"
#include "stdint.h"
#include "stdbool.h"
#include "fpioa.h"
#include "sysctl.h"
#include "global_config.h"
#include "fpioa.h"
#include "gpiohs.h"
#include "sleep.h"

#ifdef CONFIG_BOARD_M5STICK

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
    ret = maix_i2c_recv_data(I2C_DEVICE_0, AXP192_ADDR, NULL, 0, cmd, 1, 10);
    if (ret != 0)
        goto end;
    cmd[0] = 0x23;
    cmd[1] = 0x08; //K210_VCore(DCDC2) set to 0.9V
    ret = maix_i2c_send_data(I2C_DEVICE_0, AXP192_ADDR, cmd, 2, 10);
    if (ret != 0)
        goto end;
    cmd[0] = 0x33;
    cmd[1] = 0xC1; //190mA Charging Current
    ret = maix_i2c_send_data(I2C_DEVICE_0, AXP192_ADDR, cmd, 2, 10);
    if(ret!=0)
        goto end;
    cmd[0] = 0x36;
    cmd[1] = 0x6C; //4s shutdown
    ret = maix_i2c_send_data(I2C_DEVICE_0, AXP192_ADDR, cmd, 2, 10);
    if(ret!=0)
        goto end;
    cmd[0] = 0x91;
    cmd[1] = 0xF0; //LCD Backlight: GPIO0 3.3V
    ret = maix_i2c_send_data(I2C_DEVICE_0, AXP192_ADDR, cmd, 2, 10);
    if(ret!=0)
        goto end;
    cmd[0] = 0x90;
    cmd[1] = 0x02; //GPIO LDO mode
    ret = maix_i2c_send_data(I2C_DEVICE_0, AXP192_ADDR, cmd, 2, 10);
    if(ret!=0)
        goto end;
    cmd[0] = 0x28;
    cmd[1] = 0xF0; //VDD2.8V net: LDO2 3.3V,  VDD 1.5V net: LDO3 1.8V
    ret = maix_i2c_send_data(I2C_DEVICE_0, AXP192_ADDR, cmd, 2, 10);
    if(ret!=0)
        goto end;
    cmd[0] = 0x27;
    cmd[1] = 0x2C; //VDD1.8V net:  DC-DC3 1.8V
    ret = maix_i2c_send_data(I2C_DEVICE_0, AXP192_ADDR, cmd, 2, 10);
    if(ret!=0)
        goto end;
    cmd[0] = 0x12;
    cmd[1] = 0xFF; //open all power and EXTEN
    ret = maix_i2c_send_data(I2C_DEVICE_0, AXP192_ADDR, cmd, 2, 10);
    if(ret!=0)
        goto end;
    cmd[0] = 0x23;
    cmd[1] = 0x08; //VDD 0.9v net: DC-DC2 0.9V
    ret = maix_i2c_send_data(I2C_DEVICE_0, AXP192_ADDR, cmd, 2, 10);
    if(ret!=0)
        goto end;
    cmd[0] = 0x31;
    cmd[1] = 0x03; //Cutoff voltage 3.2V
    ret = maix_i2c_send_data(I2C_DEVICE_0, AXP192_ADDR, cmd, 2, 10);
    if(ret!=0)
        goto end;
    cmd[0] = 0x39;
    cmd[1] = 0xFC; //Turnoff Temp Protect (Sensor not exist!)
    ret = maix_i2c_send_data(I2C_DEVICE_0, AXP192_ADDR, cmd, 2, 10);
    if(ret!=0)
        goto end;
    fpioa_set_function(23, FUNC_GPIOHS0 + 26);
    gpiohs_set_drive_mode(26, GPIO_DM_OUTPUT);
    gpiohs_set_pin(26, GPIO_PV_HIGH); //Disable VBUS As Input, BAT->5V Boost->VBUS->Charing Cycle

    msleep(20);
end:
    maix_i2c_deinit(I2C_DEVICE_0);
    return ret==0;
}


#endif

