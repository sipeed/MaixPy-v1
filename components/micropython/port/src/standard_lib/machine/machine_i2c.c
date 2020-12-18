/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016-2018 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
// #include "extmod/machine_i2c.h"
#include "py/mpconfig.h"
#include "sleep.h"
#include "machine_i2c.h"

// I2C protocol
// the first 4 methods can be NULL, meaning operation is not supported
typedef struct _mp_machine_i2c_p_t {
    int (*start)(mp_obj_base_t *obj);
    int (*stop)(mp_obj_base_t *obj);
    int (*read)(mp_obj_base_t *obj, uint8_t *dest, size_t len, bool nack);
    int (*write)(mp_obj_base_t *obj, const uint8_t *src, size_t len);
    int (*readfrom)(mp_obj_base_t *obj, uint16_t addr, uint8_t *dest, size_t len, bool stop);
    int (*writeto)(mp_obj_base_t *obj, uint16_t addr, const uint8_t *src, size_t len, bool stop);
} mp_machine_i2c_p_t;

#if MICROPY_PY_MACHINE_SW_I2C

#include "gpiohs.h"

typedef struct _mp_machine_i2c_buf_t {
    size_t len;
    uint8_t *buf;
} mp_machine_i2c_buf_t;

typedef enum _soft_i2c_device_number
{
    I2C_DEVICE_3 = 3,
    I2C_DEVICE_4,
    I2C_DEVICE_5,
    SOFT_I2C_DEVICE_MAX,
} soft_i2c_device_number_t;

#define mp_hal_delay_us_fast(s) mp_hal_delay_us(s)
#define MP_MACHINE_I2C_FLAG_STOP (0x02)
#define MP_MACHINE_I2C_FLAG_READ (0x01) // if not set then it's a write

STATIC void mp_hal_i2c_delay(machine_hard_i2c_obj_t *self) {
    // We need to use an accurate delay to get acceptable I2C
    // speeds (eg 1us should be not much more than 1us).
    mp_hal_delay_us(self->us_delay);
}

STATIC int mp_hal_i2c_scl_release(machine_hard_i2c_obj_t *self) {
    uint32_t count = self->timeout;

    // mp_hal_pin_od_high(self->pin_scl);
    gpiohs_set_drive_mode(self->pin_scl, GPIO_DM_OUTPUT);
    gpiohs_set_pin(self->pin_scl, 1);

    mp_hal_i2c_delay(self);
    // For clock stretching, wait for the SCL pin to be released, with timeout.
    gpiohs_set_drive_mode(self->pin_scl, GPIO_DM_INPUT);

    // for (; mp_hal_pin_read(self->pin_scl) == 0 && count; --count) {
    for (; gpiohs_get_pin(self->pin_scl) == 0 && count; --count) {

        mp_hal_delay_us_fast(1);
    }
    if (count == 0) {
        return -MP_ETIMEDOUT;
    }

    return 0; // success
}

STATIC void mp_hal_i2c_scl_low(machine_hard_i2c_obj_t *self) {
    // mp_hal_pin_od_low(self->pin_scl);

    gpiohs_set_drive_mode(self->pin_scl, GPIO_DM_OUTPUT);
    gpiohs_set_pin(self->pin_scl, 0);
}

STATIC void mp_hal_i2c_sda_low(machine_hard_i2c_obj_t *self) {
    // mp_hal_pin_od_low(self->pin_sda);
    gpiohs_set_pin(self->pin_sda, 0);
    gpiohs_set_drive_mode(self->pin_sda, GPIO_DM_OUTPUT);
}

STATIC void mp_hal_i2c_sda_release(machine_hard_i2c_obj_t *self) {
    // mp_hal_pin_od_high(self->pin_sda);
    gpiohs_set_pin(self->pin_sda, 1);
    gpiohs_set_drive_mode(self->pin_sda, GPIO_DM_OUTPUT);
    gpiohs_set_drive_mode(self->pin_sda, GPIO_DM_INPUT);
}

STATIC int mp_hal_i2c_sda_read(machine_hard_i2c_obj_t *self) {
    gpiohs_set_drive_mode(self->pin_sda, GPIO_DM_INPUT);
    return gpiohs_get_pin(self->pin_sda);
    // return mp_hal_pin_read(self->pin_sda);
}

STATIC int mp_hal_i2c_start(machine_hard_i2c_obj_t *self) {
    mp_hal_i2c_sda_release(self);
    mp_hal_i2c_delay(self);
    int ret = mp_hal_i2c_scl_release(self);
    if (ret != 0) {
        return ret;
    }
    mp_hal_i2c_sda_low(self);
    mp_hal_i2c_delay(self);
    return 0; // success
}

STATIC int mp_hal_i2c_stop(machine_hard_i2c_obj_t *self) {
    mp_hal_i2c_delay(self);
    mp_hal_i2c_sda_low(self);
    mp_hal_i2c_delay(self);
    int ret = mp_hal_i2c_scl_release(self);
    mp_hal_i2c_sda_release(self);
    mp_hal_i2c_delay(self);
    return ret;
}

STATIC int mp_hal_i2c_write_byte(machine_hard_i2c_obj_t *self, uint8_t val) {
    mp_hal_i2c_delay(self);
    mp_hal_i2c_scl_low(self);

    for (int i = 7; i >= 0; i--) {
        if ((val >> i) & 1) {
            mp_hal_i2c_sda_release(self);
        } else {
            mp_hal_i2c_sda_low(self);
        }
        mp_hal_i2c_delay(self);
        int ret = mp_hal_i2c_scl_release(self);
        if (ret != 0) {
            mp_hal_i2c_sda_release(self);

            return ret;
        }
        mp_hal_i2c_scl_low(self);
    }

    mp_hal_i2c_sda_release(self);
    mp_hal_i2c_delay(self);
    int ret = mp_hal_i2c_scl_release(self);
    if (ret != 0) {
        return ret;
    }

    int ack = mp_hal_i2c_sda_read(self);
    mp_hal_i2c_delay(self);
    mp_hal_i2c_scl_low(self);
    
    return ack;
}

STATIC int mp_hal_i2c_read_byte(machine_hard_i2c_obj_t *self, uint8_t *val, int nack) {
    mp_hal_i2c_delay(self);
    mp_hal_i2c_scl_low(self);
    mp_hal_i2c_delay(self);
    uint8_t data = 0;
    for (int i = 7; i >= 0; i--) {
        int ret = mp_hal_i2c_scl_release(self);
        if (ret != 0) {
            return ret;
        }
        data = (data << 1) | mp_hal_i2c_sda_read(self);
        mp_hal_i2c_scl_low(self);
        mp_hal_i2c_delay(self);
    }
    *val = data;

    // send ack/nack bit
    if (!nack) {
        mp_hal_i2c_sda_low(self);
    }
    mp_hal_i2c_delay(self);
    int ret = mp_hal_i2c_scl_release(self);
    if (ret != 0) {
        mp_hal_i2c_sda_release(self);
        return ret;
    }
    mp_hal_i2c_scl_low(self);
    mp_hal_i2c_sda_release(self);
    return 0; // success
}

int mp_machine_soft_i2c_transfer(mp_obj_base_t *self_in, uint16_t addr, size_t n, mp_machine_i2c_buf_t *bufs, unsigned int flags) {
    machine_hard_i2c_obj_t *self = (machine_hard_i2c_obj_t *)self_in;
    // start the I2C transaction

    int ret = mp_hal_i2c_start(self);

    // int ret=0;
    if (ret != 0) {
        return ret;
    }

    // write the slave address
    ret = mp_hal_i2c_write_byte(self, (addr << 1) | (flags & MP_MACHINE_I2C_FLAG_READ));
    
    if (ret < 0) {
        return ret;
    } else if (ret != 0) {
        // nack received, release the bus cleanly
        mp_hal_i2c_stop(self);
        return -MP_ENODEV;
    }

    int transfer_ret = 0;
    for (; n--; ++bufs) {
        size_t len = bufs->len;
        uint8_t *buf = bufs->buf;
        if (flags & MP_MACHINE_I2C_FLAG_READ) {
            // read bytes from the slave into the given buffer(s)
            while (len--) {
                ret = mp_hal_i2c_read_byte(self, buf++, (n | len) == 0);
                    // mp_printf(&mp_plat_print, "debug:4\r\n");

                if (ret != 0) {
                    return ret;
                }
            }
        } else {
            // write bytes from the given buffer(s) to the slave
            while (len--) {
                ret = mp_hal_i2c_write_byte(self, *buf++);

                if (ret < 0) {
                    return ret;
                } else if (ret != 0) {
                    // nack received, stop sending
                    n = 0;
                    break;
                }
                ++transfer_ret; // count the number of acks
            }
        }
    }

    // finish the I2C transaction
    if (flags & MP_MACHINE_I2C_FLAG_STOP) {
        ret = mp_hal_i2c_stop(self);
        if (ret != 0) {
            return ret;
        }
    }

    return transfer_ret;
}

STATIC int mp_machine_i2c_writeto(mp_obj_base_t *self, uint16_t addr, const uint8_t *src, size_t len, bool stop) {
    mp_machine_i2c_buf_t buf = {.len = len, .buf = (uint8_t *)src};
    unsigned int flags = stop ? MP_MACHINE_I2C_FLAG_STOP : 0;
    return mp_machine_soft_i2c_transfer(self, addr, 1, &buf, flags);
}

STATIC int mp_machine_i2c_readfrom(mp_obj_base_t *self, uint16_t addr, uint8_t *dest, size_t len, bool stop) {
    mp_machine_i2c_buf_t buf = {.len = len, .buf = dest};
    unsigned int flags = MP_MACHINE_I2C_FLAG_READ | (stop ? MP_MACHINE_I2C_FLAG_STOP : 0);
    return mp_machine_soft_i2c_transfer(self, addr, 1, &buf, flags);
}

#endif

#if MICROPY_PY_MACHINE_HW_I2C

const mp_obj_type_t machine_hard_i2c_type;

STATIC i2c_slave_handler_t slave_callback[I2C_DEVICE_MAX];
STATIC machine_hard_i2c_obj_t* i2c_obj[I2C_DEVICE_MAX]={NULL, NULL, NULL};

STATIC bool check_i2c_device(uint32_t i2c_id)
{
    if(i2c_id >= I2C_DEVICE_MAX
#if MICROPY_PY_MACHINE_SW_I2C
         + SOFT_I2C_DEVICE_MAX
#endif
        )
        return false;
    return true;
}  

STATIC bool check_i2c_mode(uint32_t mode)
{
    if(mode >= MACHINE_I2C_MODE_MAX)
        return false;
    return true;
}

STATIC bool check_i2c_freq(uint32_t freq)
{
    return true;
}

STATIC bool check_i2c_timeout(uint32_t timeout)
{
    return true;
}

STATIC bool check_addr_size(uint32_t addr_size)
{
    if( addr_size!=7 && addr_size!=10)
        return false;
    return true;
}

STATIC bool check_pin(uint32_t scl, uint32_t sda)
{
    if(scl > 47)
        return false;
    if(sda > 47)
        return false;
    return true;
}

STATIC bool check_addr(uint32_t addr,uint32_t addr_size)
{
    if( addr_size!=7 && addr_size!=10)
        return false;
    if( (addr_size == 7) && (addr>127) )
        return false;
    if( (addr_size == 10) && (addr>1023) )
        return false;
    return true;
}
///////////////////////////////////
void on_i2c0_receive(uint32_t data)
{
    if(i2c_obj[0] != NULL)
        mp_call_function_1(i2c_obj[0]->on_receive, mp_obj_new_int(data));
}

uint32_t on_i2c0_transmit()
{
    mp_obj_t ret;

    if(i2c_obj[0] != NULL)
    {
        ret = mp_call_function_0(i2c_obj[0]->on_transmit);
        return mp_obj_get_int(ret) & 0xFF;
    }
    return 0;
}

void on_i2c0_event(i2c_event_t event)
{
    if(i2c_obj[0] != NULL)
        mp_call_function_1(i2c_obj[0]->on_event, mp_obj_new_int(event));
}
///////////////////////////////////
void on_i2c1_receive(uint32_t data)
{
    if(i2c_obj[1] != NULL)
        mp_call_function_1(i2c_obj[1]->on_receive, mp_obj_new_int(data));
}

uint32_t on_i2c1_transmit()
{
    mp_obj_t ret;

    if(i2c_obj[1] != NULL)
    {
        ret = mp_call_function_0(i2c_obj[1]->on_transmit);
        return mp_obj_get_int(ret) & 0xFF;
    }
    return 0;
}

void on_i2c1_event(i2c_event_t event)
{
    if(i2c_obj[1] != NULL)
        mp_call_function_1(i2c_obj[1]->on_event, mp_obj_new_int(event));
}
/////////////////////////////////////
void on_i2c2_receive(uint32_t data)
{
    if(i2c_obj[2] != NULL)
        mp_call_function_1(i2c_obj[2]->on_receive, mp_obj_new_int(data));
}

uint32_t on_i2c2_transmit()
{
    mp_obj_t ret;

    if(i2c_obj[2] != NULL)
    {
        ret = mp_call_function_0(i2c_obj[2]->on_transmit);
        return mp_obj_get_int(ret) & 0xFF;
    }
    return 0;
}

void on_i2c2_event(i2c_event_t event)
{
    if(i2c_obj[2] != NULL)
        mp_call_function_1(i2c_obj[2]->on_event, mp_obj_new_int(event));
}




STATIC void machine_hard_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_hard_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if(self->mode == MACHINE_I2C_MODE_MASTER || self->mode == MACHINE_I2C_MODE_MASTER_SOFT)
    {
        mp_printf(print, "[MAIXPY]I2C:(%p) I2C=%d, mode=%d, freq=%u, addr_size=%u, scl=%d, sda=%d",
            self, self->i2c, self->mode, self->freq, self->addr_size, self->pin_scl, self->pin_sda);
    }
    else
    {
        mp_printf(print, "[MAIXPY]I2C:(%p) I2C=%d, mode=%d, addr_size=%u, addr=%02x, scl=%d, sda=%d, on_receive=%p, on_transmit=%p, on_event=%p",
            self, self->i2c, self->mode, self->addr_size, self->addr, self->pin_scl, self->pin_sda, self->on_receive, self->on_transmit, self->on_event);
    }
}


STATIC int machine_hard_i2c_readfrom(mp_obj_base_t *self_in, uint16_t addr, uint8_t *dest, size_t len, bool stop) {
    // mp_obj_base_t *self = (mp_obj_base_t*)MP_OBJ_TO_PTR(self_in);
    machine_hard_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);

#if MICROPY_PY_MACHINE_SW_I2C
    if (self->mode == MACHINE_I2C_MODE_MASTER_SOFT) {
        unsigned int flags = MP_MACHINE_I2C_FLAG_READ | (stop ? MP_MACHINE_I2C_FLAG_STOP : 0);
        return mp_machine_i2c_readfrom(self, addr, dest, len, flags);
    }
#endif

    //TODO: stop not implement
    int ret = maix_i2c_recv_data(self->i2c, addr, NULL, 0, dest, len, 100);
    if(ret < 0)
        ret = -EIO;
    else if( ret == 0)
        ret = len;
    return ret;
}

STATIC int machine_hard_i2c_writeto(mp_obj_base_t *self_in, uint16_t addr, const uint8_t *src, size_t len, bool stop) {
    // mp_obj_base_t *self = (mp_obj_base_t*)MP_OBJ_TO_PTR(self_in);
    machine_hard_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
#if MICROPY_PY_MACHINE_SW_I2C
    if (self->mode == MACHINE_I2C_MODE_MASTER_SOFT) {
        return mp_machine_i2c_writeto(self, addr, src, len, stop);
    }
#endif
    //TODO: stop not implement
    //TODO: send 0 byte date support( only send start, slave address, and wait ack, stop at last)
    int ret = maix_i2c_send_data(self->i2c, addr, src, len, 20);
    if(ret != 0)
        ret = -EIO;
    else
        ret = len;//TODO: get sent length actually
    return ret;
}


/******************************************************************************/
/* MicroPython bindings for machine API                                       */

STATIC mp_obj_t machine_i2c_init_helper(machine_hard_i2c_obj_t* self, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // parse args
    enum { 
        ARG_id,
        ARG_mode,
        ARG_scl,
        ARG_sda,
        ARG_freq,
        ARG_timeout,
        ARG_addr,
        ARG_addr_size,
        ARG_on_receive,
        ARG_on_transmit,
        ARG_on_event
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_int = I2C_DEVICE_0} },
        { MP_QSTR_mode, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MACHINE_I2C_MODE_MASTER} },
        { MP_QSTR_scl, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_sda, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_freq, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 400000} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1000} },
        { MP_QSTR_addr, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_addr_size, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 7} },
        { MP_QSTR_on_receive, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_on_transmit, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_on_event, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_us_delay, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // work out i2c bus
    int i2c_id = 0, freq = 0, timeout = 0, mode = 0, addr = 0, addr_size = 0;
    i2c_id = mp_obj_get_int(args[ARG_id].u_obj);
    mode = args[ARG_mode].u_int;
    freq = args[ARG_freq].u_int;
    timeout = args[ARG_freq].u_int;
    addr = args[ARG_addr].u_int;
    addr_size = args[ARG_addr_size].u_int;
    if(!check_i2c_device(i2c_id)) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,
            "[MAIXPY]I2C: I2C(%d) doesn't exist", i2c_id));
    }
    if(!check_i2c_mode(mode))
    {
        mp_raise_ValueError("[MAIXPY]I2C: mode error!");
    }
    if(!check_i2c_freq(freq)){ // just check for master mode
        mp_raise_ValueError("[MAIXPY]I2C: freq value error!");
    }
    if(!check_i2c_timeout(timeout))
    {
        mp_raise_ValueError("[MAIXPY]I2C: timeout value error!");
    }
    if(!check_addr_size(addr_size))
    {
        mp_raise_ValueError("[MAIXPY]I2C: addr size error!");
    }
    if( mode == MACHINE_I2C_MODE_SLAVE)
    {
        if(!check_addr(addr, addr_size))
        {
            mp_raise_ValueError("[MAIXPY]I2C: addr/addr size error!");
        }
        if( !mp_obj_is_callable(args[ARG_on_receive].u_obj)||
            !mp_obj_is_callable(args[ARG_on_transmit].u_obj)||
            !mp_obj_is_callable(args[ARG_on_event].u_obj) )
        {
            mp_raise_ValueError("[MAIXPY]I2C: callback error!"); 
        }
    }

    if (args[ARG_scl].u_obj != MP_OBJ_NULL || args[ARG_sda].u_obj != MP_OBJ_NULL) {
        if( !check_pin( mp_obj_get_int(args[ARG_scl].u_obj), mp_obj_get_int(args[ARG_sda].u_obj) ) )
            mp_raise_ValueError("[MAIXPY]I2C: pin(scl/sda) error!"); 
        self->pin_scl = mp_obj_get_int(args[ARG_scl].u_obj);
        self->pin_sda = mp_obj_get_int(args[ARG_sda].u_obj);
    }

    // set param
    self->i2c = i2c_id;
    self->mode = mode;
    self->freq = freq;
    self->timeout = timeout;
    self->addr = addr;
    self->addr_size = addr_size;
    self->on_receive = args[ARG_on_receive].u_obj;
    self->on_transmit = args[ARG_on_transmit].u_obj;
    self->on_event = args[ARG_on_event].u_obj;

#if MICROPY_PY_MACHINE_SW_I2C
    if(self->i2c == (i2c_device_number_t)I2C_DEVICE_3 
        || self->i2c == (i2c_device_number_t)I2C_DEVICE_4 
        || self->i2c == (i2c_device_number_t)I2C_DEVICE_5) {
        self->mode = MACHINE_I2C_MODE_MASTER_SOFT;
        if( mp_obj_get_int(args[ARG_scl].u_obj) >= 32 || mp_obj_get_int(args[ARG_sda].u_obj) >= 32)
            mp_raise_ValueError("[MAIXPY]SW_I2C: pin(scl/sda) < GPIOHS_MAX_PINNO(32)!"); 
    }
#endif

    // initialize hardware i2c
    if( self->mode == MACHINE_I2C_MODE_SLAVE) {
        if(self->i2c==I2C_DEVICE_0) {
            slave_callback[self->i2c].on_receive = on_i2c0_receive;
            slave_callback[self->i2c].on_transmit = on_i2c0_transmit;
            slave_callback[self->i2c].on_event = on_i2c0_event;
        }
        else if(self->i2c==I2C_DEVICE_1) {
            slave_callback[self->i2c].on_receive = on_i2c1_receive;
            slave_callback[self->i2c].on_transmit = on_i2c1_transmit;
            slave_callback[self->i2c].on_event = on_i2c1_event;
        }
        else if(self->i2c==I2C_DEVICE_2) {
            slave_callback[self->i2c].on_receive = on_i2c2_receive;
            slave_callback[self->i2c].on_transmit = on_i2c2_transmit;
            slave_callback[self->i2c].on_event = on_i2c2_event;
        }
        i2c_init_as_slave(self->i2c, self->addr, self->addr_size, slave_callback+self->i2c);   
#if MICROPY_PY_MACHINE_SW_I2C
    } else if (self->mode == MACHINE_I2C_MODE_MASTER_SOFT) {
        if (freq >= 1000000) {
            // allow fastest possible bit-bang rate
            self->us_delay = 0;
        } else {
            self->us_delay = 500000 / freq;
            if (self->us_delay == 0) {
                self->us_delay = 1;
            }
        }
        // mp_hal_pin_open_drain(self->scl);
        // mp_hal_pin_open_drain(self->sda);
        // GPIO_DM_OUTPUT
        fpioa_set_function(self->pin_scl, FUNC_GPIOHS0 + self->pin_scl); 
        gpiohs_set_pin(self->pin_scl, 1);
        gpiohs_set_drive_mode(self->pin_scl, GPIO_DM_OUTPUT);
        
        fpioa_set_function(self->pin_sda, FUNC_GPIOHS0 + self->pin_sda);
        gpiohs_set_pin(self->pin_sda, 1);
        gpiohs_set_drive_mode(self->pin_sda, GPIO_DM_OUTPUT);
        
        mp_hal_i2c_stop(self); // ignore error
#endif
    } else {
        fpioa_set_function(self->pin_scl, FUNC_I2C0_SCLK + i2c_id * 2);
        fpioa_set_function(self->pin_sda, FUNC_I2C0_SDA + i2c_id * 2);
        maix_i2c_init(self->i2c, self->addr_size, self->freq);
    }
    i2c_obj[self->i2c] = self;
    return mp_const_none;
}

mp_obj_t machine_hard_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    machine_hard_i2c_obj_t *self = m_new_obj(machine_hard_i2c_obj_t);
    self->base.type = &machine_hard_i2c_type;
	mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
	machine_i2c_init_helper(self, n_args, args, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t machine_i2c_obj_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    machine_i2c_init_helper(MP_OBJ_TO_PTR(args[0]), n_args - 1, args + 1, kw_args);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_i2c_init_obj, 1, machine_i2c_obj_init);

STATIC mp_obj_t machine_i2c_writeto(size_t n_args, const mp_obj_t *args) {
    mp_obj_base_t *self = (mp_obj_base_t*)MP_OBJ_TO_PTR(args[0]);
    mp_machine_i2c_p_t *i2c_p = (mp_machine_i2c_p_t*)self->type->protocol;
    mp_int_t addr = mp_obj_get_int(args[1]);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[2], &bufinfo, MP_BUFFER_READ);
    bool stop = (n_args == 3) ? true : mp_obj_is_true(args[3]);
    //TODO: support not send stop signal
    if(!stop)
    {
        mp_raise_NotImplementedError("[MAIXPY]I2C: not support stop option yet");
    }
    if(bufinfo.len == 0)
    {
        //TODO: support only send address and not send any bytes
        mp_raise_NotImplementedError("[MAIXPY]I2C: not support send 0 byte data yet");
    }
    int ret = i2c_p->writeto(self, addr,  bufinfo.buf, bufinfo.len, stop);
    if (ret < 0) {
        mp_raise_OSError(-ret);
    }
    // return number of acks received
    return MP_OBJ_NEW_SMALL_INT(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_i2c_writeto_obj, 3, 4, machine_i2c_writeto);

STATIC mp_obj_t machine_i2c_readfrom(size_t n_args, const mp_obj_t *args) {
    mp_obj_base_t *self = (mp_obj_base_t*)MP_OBJ_TO_PTR(args[0]);
    mp_machine_i2c_p_t *i2c_p = (mp_machine_i2c_p_t*)self->type->protocol;
    mp_int_t addr = mp_obj_get_int(args[1]);
    mp_int_t len = mp_obj_get_int(args[2]);
    if(len == 0)
    {
        mp_raise_ValueError("[MAIXPY]I2C: not support receive 0 byte data yet");
    }
    vstr_t vstr;
    vstr_init_len(&vstr, len);
    bool stop = (n_args == 3) ? true : mp_obj_is_true(args[3]);
    //TODO: support not send stop signal
    if(!stop)
    {
        mp_raise_NotImplementedError("[MAIXPY]I2C: not support stop option yet");
    }
    int ret = i2c_p->readfrom(self, addr, (uint8_t*)vstr.buf, vstr.len, stop);
    if (ret < 0) {
        mp_raise_OSError(-ret);
    }
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_i2c_readfrom_obj, 3, 4, machine_i2c_readfrom);

STATIC mp_obj_t machine_i2c_readfrom_into(size_t n_args, const mp_obj_t *args) {
    mp_obj_base_t *self = (mp_obj_base_t*)MP_OBJ_TO_PTR(args[0]);
    mp_machine_i2c_p_t *i2c_p = (mp_machine_i2c_p_t*)self->type->protocol;
    mp_int_t addr = mp_obj_get_int(args[1]);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[2], &bufinfo, MP_BUFFER_WRITE);
    bool stop = (n_args == 3) ? true : mp_obj_is_true(args[3]);
    //TODO: support not send stop signal
    if(!stop)
    {
        mp_raise_NotImplementedError("[MAIXPY]I2C: not support stop option yet");
    }
    int ret = i2c_p->readfrom(self, addr, bufinfo.buf, bufinfo.len, stop);
    if (ret < 0) {
        mp_raise_OSError(-ret);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_i2c_readfrom_into_obj, 3, 4, machine_i2c_readfrom_into);

STATIC int read_mem(mp_obj_t self_in, uint16_t addr, uint32_t memaddr, uint8_t mem_size, uint8_t *buf, size_t len) {
    // mp_obj_base_t *self = (mp_obj_base_t*)MP_OBJ_TO_PTR(self_in);
    machine_hard_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);

    uint8_t send[4];
    // mp_machine_i2c_p_t *i2c_p = (mp_machine_i2c_p_t*)self->base.type->protocol;
    if(len == 0)
    {
        mp_raise_ValueError("[MAIXPY]I2C: not support receive 0 byte data yet");
    }
    mem_size = mem_size/8;
    if(mem_size > 4 || mem_size==0)
    {
        mp_raise_ValueError("mem_size error");
    }
    for(uint8_t i=0; i<mem_size; ++i)
    {
        send[i] = (memaddr >> ((mem_size-1)*8 - i*8)) & 0xFF;
    }
#if MICROPY_PY_MACHINE_SW_I2C
    if (self->mode == MACHINE_I2C_MODE_MASTER_SOFT) {
        int ret = mp_machine_i2c_writeto(self, addr, send, mem_size, false);
        if (ret != mem_size) {
            // must generate STOP
            mp_machine_i2c_writeto(self, addr, NULL, 0, true);
            return ret;
        }
        return mp_machine_i2c_readfrom(self, addr, buf, len, true);
    }
#endif
    int ret = maix_i2c_recv_data(self->i2c, addr, send, mem_size, buf, len, 100);
    if(ret != 0)
        ret = -EIO;
    else
        ret = len; //TODO: get read length actually
    return ret;
}

#define MAX_MEMADDR_SIZE (4)
#define BUF_STACK_SIZE (12)

STATIC int write_mem(mp_obj_t self_in, uint16_t addr, uint32_t memaddr, uint8_t mem_size, const uint8_t *buf, size_t len) {
    mp_obj_base_t *self = (mp_obj_base_t*)MP_OBJ_TO_PTR(self_in);

    mp_machine_i2c_p_t *i2c_p = (mp_machine_i2c_p_t*)self->type->protocol;

    // need some memory to create the buffer to send; try to use stack if possible
    uint8_t buf2_stack[MAX_MEMADDR_SIZE + BUF_STACK_SIZE];
    uint8_t *buf2;
    size_t buf2_alloc = 0;
    if (len <= BUF_STACK_SIZE) {
        buf2 = buf2_stack;
    } else {
        buf2_alloc = MAX_MEMADDR_SIZE + len;
        buf2 = m_new(uint8_t, buf2_alloc);
    }

    // create the buffer to send
    size_t memaddr_len = 0;
    for (int16_t i = mem_size - 8; i >= 0; i -= 8) {
        buf2[memaddr_len++] = memaddr >> i;
    }
    memcpy(buf2 + memaddr_len, buf, len);
    int ret = -1;
#if MICROPY_PY_MACHINE_SW_I2C
    if (((machine_hard_i2c_obj_t *)self)->mode == MACHINE_I2C_MODE_MASTER_SOFT) {
        ret = mp_machine_i2c_writeto(self, addr, buf2, memaddr_len + len, MP_MACHINE_I2C_FLAG_STOP);
    } else {
#endif
        ret = i2c_p->writeto(self, addr, buf2, memaddr_len + len, true);
#if MICROPY_PY_MACHINE_SW_I2C
    }
#endif
    if (buf2_alloc != 0) {
        m_del(uint8_t, buf2, buf2_alloc);
    }
    return ret;
}

STATIC const mp_arg_t machine_i2c_mem_allowed_args[] = {
    { MP_QSTR_addr,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_memaddr, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_arg,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_mem_size, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
};

STATIC mp_obj_t machine_i2c_writeto_mem(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_addr, ARG_memaddr, ARG_buf, ARG_mem_size };
    mp_arg_val_t args[MP_ARRAY_SIZE(machine_i2c_mem_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
        MP_ARRAY_SIZE(machine_i2c_mem_allowed_args), machine_i2c_mem_allowed_args, args);

    // get the buffer to write the data from
    int len;
    uint8_t* data;
    uint8_t temp;
    mp_buffer_info_t bufinfo;
    if(mp_obj_is_integer(args[ARG_buf].u_obj))
    {
        temp = mp_obj_get_int(args[ARG_buf].u_obj);
        data = &temp;
        len = 1;
    }
    else
    {
        mp_get_buffer_raise(args[ARG_buf].u_obj, &bufinfo, MP_BUFFER_READ);
        data = bufinfo.buf;
        len = bufinfo.len;
    }    
    // do the transfer
    int ret = write_mem(pos_args[0], args[ARG_addr].u_int, args[ARG_memaddr].u_int,
        args[ARG_mem_size].u_int, data, len);
    if (ret < 0) {
        mp_raise_OSError(-ret);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_i2c_writeto_mem_obj, 1, machine_i2c_writeto_mem);

STATIC mp_obj_t machine_i2c_readfrom_mem(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_addr, ARG_memaddr, ARG_nbytes, ARG_mem_size };
    mp_arg_val_t args[MP_ARRAY_SIZE(machine_i2c_mem_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
        MP_ARRAY_SIZE(machine_i2c_mem_allowed_args), machine_i2c_mem_allowed_args, args);

    // create the buffer to store data into
    vstr_t vstr;
    vstr_init_len(&vstr, mp_obj_get_int(args[ARG_nbytes].u_obj));

    // do the transfer
    int ret = read_mem(pos_args[0], args[ARG_addr].u_int, args[ARG_memaddr].u_int,
        args[ARG_mem_size].u_int, (uint8_t*)vstr.buf, vstr.len);
    if (ret < 0) {
        mp_raise_OSError(-ret);
    }

    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_i2c_readfrom_mem_obj, 1, machine_i2c_readfrom_mem);

STATIC mp_obj_t machine_i2c_readfrom_mem_into(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_addr, ARG_memaddr, ARG_buf, ARG_mem_size };
    mp_arg_val_t args[MP_ARRAY_SIZE(machine_i2c_mem_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
        MP_ARRAY_SIZE(machine_i2c_mem_allowed_args), machine_i2c_mem_allowed_args, args);

    // get the buffer to store data into
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_buf].u_obj, &bufinfo, MP_BUFFER_WRITE);

    // do the transfer
    int ret = read_mem(pos_args[0], args[ARG_addr].u_int, args[ARG_memaddr].u_int,
        args[ARG_mem_size].u_int, bufinfo.buf, bufinfo.len);
    if (ret < 0) {
        mp_raise_OSError(-ret);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_i2c_readfrom_mem_into_obj, 1, machine_i2c_readfrom_mem_into);

STATIC mp_obj_t machine_i2c_scan(mp_obj_t self_in) {
    machine_hard_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // mp_machine_i2c_p_t *i2c_p = (mp_machine_i2c_p_t*)self->base.type->protocol;
    mp_obj_t list = mp_obj_new_list(0, NULL);
    uint8_t temp;
    // 7-bit addresses 0b0000xxx and 0b1111xxx are reserved
    for (int addr = 0x08; addr < 0x78; ++addr) {
        int ret = -1;
        if (self->mode == MACHINE_I2C_MODE_MASTER) {
            ret = maix_i2c_recv_data(self->i2c, addr, NULL, 0, &temp, 1, 100);
        } 
    #if MICROPY_PY_MACHINE_SW_I2C
        else if (self->mode == MACHINE_I2C_MODE_MASTER_SOFT) {
            ret = mp_machine_i2c_writeto(self, addr, NULL, 0, true);
        }
    #endif
        
        if (ret == 0) {
            mp_obj_list_append(list, MP_OBJ_NEW_SMALL_INT(addr));
        }
    }
    return list;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_i2c_scan_obj, machine_i2c_scan);

STATIC mp_obj_t machine_i2c_deinit(mp_obj_t self_in) {
    machine_hard_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
#if MICROPY_PY_MACHINE_SW_I2C
    if (self->mode == MACHINE_I2C_MODE_MASTER_SOFT) {
        return mp_const_none;
    }
#endif
    maix_i2c_deinit(self->i2c);
    i2c_obj[self->i2c] = NULL;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_i2c_deinit_obj, machine_i2c_deinit);

STATIC const mp_machine_i2c_p_t machine_hard_i2c_p = {
    .readfrom = machine_hard_i2c_readfrom,
    .writeto = machine_hard_i2c_writeto,
};

STATIC const mp_rom_map_elem_t machine_i2c_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_i2c_init_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&machine_i2c_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_i2c_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_scan), MP_ROM_PTR(&machine_i2c_scan_obj) },

    // primitive I2C operations
    /* //TODO: primitive(/raw) operations
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&machine_i2c_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&machine_i2c_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&machine_i2c_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&machine_i2c_write_obj) },
    */
    // standard bus operations
    { MP_ROM_QSTR(MP_QSTR_readfrom), MP_ROM_PTR(&machine_i2c_readfrom_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom_into), MP_ROM_PTR(&machine_i2c_readfrom_into_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeto), MP_ROM_PTR(&machine_i2c_writeto_obj) },

    // memory operations
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem), MP_ROM_PTR(&machine_i2c_readfrom_mem_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem_into), MP_ROM_PTR(&machine_i2c_readfrom_mem_into_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeto_mem), MP_ROM_PTR(&machine_i2c_writeto_mem_obj) },

    // mode
    { MP_ROM_QSTR(MP_QSTR_MODE_MASTER), MP_ROM_INT(MACHINE_I2C_MODE_MASTER) },
    { MP_ROM_QSTR(MP_QSTR_MODE_SLAVE), MP_ROM_INT(MACHINE_I2C_MODE_SLAVE) },
    { MP_ROM_QSTR(MP_QSTR_MODE_MASTER_SOFT), MP_ROM_INT(MACHINE_I2C_MODE_MASTER_SOFT) },

    // devices
    { MP_ROM_QSTR(MP_QSTR_I2C0), MP_ROM_INT(I2C_DEVICE_0) },
    { MP_ROM_QSTR(MP_QSTR_I2C1), MP_ROM_INT(I2C_DEVICE_1) },
    { MP_ROM_QSTR(MP_QSTR_I2C2), MP_ROM_INT(I2C_DEVICE_2) },
#if MICROPY_PY_MACHINE_SW_I2C
    { MP_ROM_QSTR(MP_QSTR_I2C3), MP_ROM_INT(I2C_DEVICE_3) },
    { MP_ROM_QSTR(MP_QSTR_I2C4), MP_ROM_INT(I2C_DEVICE_4) },
    { MP_ROM_QSTR(MP_QSTR_I2C5), MP_ROM_INT(I2C_DEVICE_5) },
#endif

    // slave mode event
    { MP_ROM_QSTR(MP_QSTR_I2C_EV_START),   MP_ROM_INT(I2C_EV_START) },
    { MP_ROM_QSTR(MP_QSTR_I2C_EV_RESTART), MP_ROM_INT(I2C_EV_RESTART) },
    { MP_ROM_QSTR(MP_QSTR_I2C_EV_STOP),    MP_ROM_INT(I2C_EV_STOP) },
    
};

MP_DEFINE_CONST_DICT(mp_machine_soft_i2c_locals_dict, machine_i2c_locals_dict_table);

const mp_obj_type_t machine_hard_i2c_type = {
    { &mp_type_type },
    .name = MP_QSTR_I2C,
    .print = machine_hard_i2c_print,
    .make_new = machine_hard_i2c_make_new,
    .protocol = &machine_hard_i2c_p,
    .locals_dict = (mp_obj_dict_t*)&mp_machine_soft_i2c_locals_dict,
};

#endif // MICROPY_PY_MACHINE_HW_I2C
