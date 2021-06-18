#ifndef __MACHINE_I2C_H_
#define __MACHINE_I2C_H_

#include "i2c.h"
#include "sipeed_i2c.h"
#include "stdint.h"
#include "mp.h"
#include "fpioa.h"

typedef enum{
    MACHINE_I2C_MODE_MASTER = 0,
    MACHINE_I2C_MODE_SLAVE,
#if MICROPY_PY_MACHINE_SW_I2C
    MACHINE_I2C_MODE_MASTER_SOFT,
#endif
    MACHINE_I2C_MODE_MAX
} machine_i2c_mode_t;

typedef struct _machine_hard_i2c_obj_t {
    mp_obj_base_t         base;
    i2c_device_number_t   i2c;
    machine_i2c_mode_t    mode;
    uint32_t              freq;         // Hz
    uint32_t              timeout;      // reserved
    uint16_t              addr;
    uint8_t               addr_size;
    mp_obj_t              on_receive;
    mp_obj_t              on_transmit;
    mp_obj_t              on_event;
    int                   pin_scl;
    int                   pin_sda; 
    int                   gpio_scl;
    int                   gpio_sda;
    int                   us_delay;
} machine_hard_i2c_obj_t;


#endif
