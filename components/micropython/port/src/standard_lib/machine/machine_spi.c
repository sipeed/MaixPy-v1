/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 "Eric Poulsen" <eric@zyxod.com>
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
#include <stdint.h>
#include <string.h>

#include "py/runtime.h"
#include "py/stream.h"
#include "py/mphal.h"
#include "extmod/machine_spi.h"
#include "modmachine.h"

#include "spi.h"
#include "fpioa.h"
#include "sipeed_spi.h"

#if MICROPY_PY_MACHINE_HW_SPI

typedef enum{
    MACHINE_SPI_MODE_MASTER   = 0,
    MACHINE_SPI_MODE_MASTER_2,
    MACHINE_SPI_MODE_MASTER_4,
    MACHINE_SPI_MODE_MASTER_8,
    MACHINE_SPI_MODE_SLAVE   ,
    MACHINE_SPI_MODE_MAX
}machine_spi_mode_t;

typedef enum{
    MACHINE_SPI_CS0 = 0,
    MACHINE_SPI_CS1    ,
    MACHINE_SPI_CS2    ,
    MACHINE_SPI_CS3    ,
    MACHINE_SPI_MAX
}machine_spi_cs_t;

// SPI protocol
typedef struct {
    void (*init)(mp_obj_base_t *obj, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
    void (*deinit)(mp_obj_base_t *obj); // can be NULL
    void (*transfer)(mp_obj_base_t *obj, size_t len, const uint8_t *src, uint8_t *dest, int cs);
} mp_machine_hw_spi_p_t;

typedef enum{
    MACHINE_SPI_FIRSTBIT_MSB = 0,
    MACHINE_SPI_FIRSTBIT_LSB = 1,
    MACHINE_SPI_FIRSTBIT_MAX
}machine_spi_firstbit_t;

STATIC int check_pin(mp_obj_t pin)
{
    int pin_int;
    if( pin == mp_const_none || pin == MP_OBJ_NULL)
        return -1;
    if( mp_obj_is_integer(pin) )
    {
        pin_int = mp_obj_get_int(pin);
        if(pin_int<0 || pin_int>47)
            return -1;
    }
    else
    {//TODO: Maybe support Pin object
        return -2;
    }
    return pin_int;
}

#define MP_HW_SPI_MAX_XFER_BYTES (4092)
#define MP_HW_SPI_MAX_XFER_BITS (MP_HW_SPI_MAX_XFER_BYTES * 8) // Has to be an even multiple of 8

typedef struct _machine_hw_spi_obj_t {
    mp_obj_base_t      base;
    spi_device_num_t   id;
    machine_spi_mode_t mode;
    uint32_t           baudrate;
    uint8_t            polarity;
    uint8_t            phase;
    uint8_t            bits;
    uint8_t            firstbit;
    int8_t             pin_sck;
    int8_t             pin_cs[4];
    int8_t             pin_d[8];
    enum {
        MACHINE_HW_SPI_STATE_NONE,
        MACHINE_HW_SPI_STATE_INIT,
        MACHINE_HW_SPI_STATE_DEINIT
    } state;
} machine_hw_spi_obj_t;

STATIC void machine_hw_spi_deinit_internal(machine_hw_spi_obj_t *self) {
    sipeed_spi_deinit(self->id);
}

STATIC void machine_hw_spi_deinit(mp_obj_base_t *self_in) {
    machine_hw_spi_obj_t *self = (machine_hw_spi_obj_t *) self_in;
    if (self->state == MACHINE_HW_SPI_STATE_INIT) {
        self->state = MACHINE_HW_SPI_STATE_DEINIT;
        machine_hw_spi_deinit_internal(self);
    }
}

STATIC void machine_hw_spi_transfer(mp_obj_base_t *self_in, size_t len, const uint8_t *src, uint8_t *dest, int cs) {
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (self->state == MACHINE_HW_SPI_STATE_DEINIT) {
        mp_raise_msg(&mp_type_OSError, "[MAIXPY]SPI: transfer on deinitialized SPI");
        return;
    }
    if(dest==NULL)
        sipeed_spi_transfer_data_standard(self->id, cs, src, NULL, len, 0);
    else
        sipeed_spi_transfer_data_standard(self->id, cs, src, dest, len, len);
}

/******************************************************************************/
// MicroPython bindings for hw_spi

STATIC void machine_hw_spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    char* temp = m_new(char, 300);
    if(!temp)
        mp_raise_OSError(ENOMEM);
    if(self->mode == MACHINE_SPI_MODE_MASTER)
    {
        snprintf(temp,300,"[MAIXPY]SPI:(%p) id=%u, mode=%u, baudrate=%u, polarity=%u, phase=%u, bits=%u, firstbit=%u, sck=%d, mosi=%d, miso=%d",
                self, self->id, self->mode, self->baudrate, self->polarity,
                self->phase, self->bits, self->firstbit,
                self->pin_sck, self->pin_d[0], self->pin_d[1]);
        uint16_t len;
        for(uint8_t i=0; i<4; ++i)
        {
             len = strlen(temp);
            if(self->pin_cs[i] >= 0)
            {
                snprintf(temp+len,300-len,", cs%d=%d", i, self->pin_cs[i]);
            }
        }
    }
    else//TODO:
    {

    }
    mp_printf(print,temp);
}

STATIC void machine_hw_spi_init(mp_obj_base_t *self_in, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    machine_hw_spi_obj_t *self = (machine_hw_spi_obj_t *) self_in;

    enum {  ARG_id,
            ARG_mode,
            ARG_baudrate,
            ARG_polarity,
            ARG_phase,
            ARG_bits,
            ARG_firstbit,
            ARG_sck,
            ARG_mosi,
            ARG_miso,
            ARG_cs0,
            ARG_cs1,
            ARG_cs2,
            ARG_cs3,
            ARG_d0,
            ARG_d1,
            ARG_d2,
            ARG_d3,
            ARG_d4,
            ARG_d5,
            ARG_d6,
            ARG_d7,
            // ARG_pins//TODO: pins dict( tuple in pyboard or esp8266) support 
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,       MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_mode,     MP_ARG_INT, {.u_int = MACHINE_SPI_MODE_MASTER} },
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 500000} },
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MACHINE_SPI_FIRSTBIT_MSB} },
        { MP_QSTR_sck,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_mosi,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_miso,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_cs0,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_cs1,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_cs2,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_cs3,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_d0,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_d1,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_d2,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_d3,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_d4,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_d5,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_d6,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_d7,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        // { MP_QSTR_pins,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args),
                     allowed_args, args);
    //check args
    if(args[ARG_id].u_int >= SPI_DEVICE_3 || args[ARG_id].u_int<0)
        mp_raise_ValueError("[MAIXPY]SPI: spi id error(0~2)");
    if(args[ARG_id].u_int == SPI_DEVICE_2) //TODO: slave mode support
        mp_raise_NotImplementedError("[MAIXPY]SPI: SPI2 only for slave mode");
    if(args[ARG_mode].u_int < MACHINE_SPI_MODE_MASTER || args[ARG_mode].u_int>=MACHINE_SPI_MODE_MAX)
        mp_raise_ValueError("[MAIXPY]SPI: spi mode error");
    if( (args[ARG_mode].u_int==MACHINE_SPI_MODE_SLAVE)&& (args[ARG_id].u_int!=SPI_DEVICE_2) )
        mp_raise_ValueError("[MAIXPY]SPI: slave mode only for SPI2");
    if( args[ARG_mode].u_int != MACHINE_SPI_MODE_MASTER) //TODO: support 2/4/8(dual quad octal) lines mode
        mp_raise_NotImplementedError("[MAIXPY]SPI: only standard mode supported yet");
    if( args[ARG_baudrate].u_int<=0)
        mp_raise_ValueError("[MAIXPY]SPI: baudrate(freq) value error");
    if( args[ARG_polarity].u_int != 0 && args[ARG_polarity].u_int != 1)
        mp_raise_ValueError("[MAIXPY]SPI: polarity should be 0 or 1");
    if( args[ARG_phase].u_int != 0 && args[ARG_phase].u_int != 1)
        mp_raise_ValueError("[MAIXPY]SPI: ARG_phase should be 0 or 1");
    if( args[ARG_bits].u_int <4 || args[ARG_bits].u_int > 32)
        mp_raise_ValueError("[MAIXPY]SPI: bits should be 4~32");
    if( args[ARG_firstbit].u_int != MACHINE_SPI_FIRSTBIT_LSB && args[ARG_firstbit].u_int != MACHINE_SPI_FIRSTBIT_MSB)
        mp_raise_ValueError("[MAIXPY]SPI: firstbit should be SPI.LSB or SPI.MSB");
    if( args[ARG_firstbit].u_int == MACHINE_SPI_FIRSTBIT_LSB)//TODO: support LSB mode
        mp_raise_NotImplementedError("[MAIXPY]SPI: only support MSB mode now");
    
    //check sck cs pin
    int sck=-1;
    int cs[4] = {-1, -1, -1, -1};
    int d[8] = {-1,-1,-1,-1,-1,-1,-1,-1};
    int ret;
    bool is_set_fpioa = false;
    sck   = check_pin(args[ARG_sck].u_obj);
    cs[0] = check_pin(args[ARG_cs0].u_obj);
    cs[1] = check_pin(args[ARG_cs1].u_obj);
    cs[2] = check_pin(args[ARG_cs2].u_obj);
    cs[3] = check_pin(args[ARG_cs3].u_obj);
    // if( sck >=0 && (cs[0]>=0 || cs[1]>=0 || cs[2]>=0 || cs[3]>=0) ) // sck and cs set
    // {
    //     valid = true;
    //     is_set_fpioa = true;
    // }
    // if( sck < 0 && cs[0]<0 && cs[1]<0 && cs[2]<0 && cs[3]<0 ) // not set
    //     valid = true; 
    // if(!valid)
    //     mp_raise_ValueError("[MAIXPY]SPI: sck and cs(n) pin should be set or all not set");
    //check data pins
    if( args[ARG_mode].u_int == MACHINE_SPI_MODE_MASTER)// standard spi mode
    {
        if(is_set_fpioa)
        {
            is_set_fpioa = false;
            ret = check_pin(args[ARG_mosi].u_obj);
            if(ret >= 0)//has mosi
            {
                d[0] = ret;
                is_set_fpioa = true;
                ret = check_pin(args[ARG_miso].u_obj);
                if(ret >= 0)
                {
                    d[1] = ret;
                }
            }
            else// no mosi, check d0 
            {
                ret = check_pin(args[ARG_d0].u_obj);
                if(ret >= 0)
                {
                    d[0] = ret;
                    is_set_fpioa = true;
                    ret = check_pin(args[ARG_d1].u_obj);
                    if(ret >= 0)
                    {
                        d[1] = ret;
                    }
                }
            }
        }
    }
    else//TODO: nonstandard mode support 
    {

    }

    self->id = args[ARG_id].u_int;
    self->mode      = args[ARG_mode].u_int;
    self->baudrate  = args[ARG_baudrate].u_int;
    self->polarity  = args[ARG_polarity].u_int;
    self->phase     = args[ARG_phase].u_int;
    self->bits      = args[ARG_bits].u_int;
    self->firstbit  = args[ARG_firstbit].u_int;
    self->pin_sck   = sck;
    for( uint8_t i=0; i<4; ++i)
        self->pin_cs[i] = cs[i];
    for( uint8_t i=0; i<8; ++i)
        self->pin_d[i] = d[i];
    
    // Init SPI
    if(  self->mode == MACHINE_SPI_MODE_MASTER)// standard spi mode
    {
        if(is_set_fpioa)
        {
            if(self->id  == SPI_DEVICE_0)
            {
                fpioa_set_function(sck, FUNC_SPI0_SCLK);
                for(uint8_t i=0; i<4; ++i)
                {
                    if(self->pin_cs[i] >= 0)
                        fpioa_set_function(self->pin_cs[i], FUNC_SPI0_SS0+i);
                }
                for(uint8_t i=0; i<2; ++i)
                {
                    if(self->pin_d[i] >= 0)
                        fpioa_set_function(self->pin_d[i], FUNC_SPI0_D0+i);
                }
            }
            else if( self->id == SPI_DEVICE_1)
            {
                fpioa_set_function(self->pin_sck, FUNC_SPI1_SCLK);
                for(uint8_t i=0; i<4; ++i)
                {
                    if(self->pin_cs[i] >= 0)
                        fpioa_set_function(self->pin_cs[i], FUNC_SPI1_SS0+i);
                }
                for(uint8_t i=0; i<2; ++i)
                {
                    if(self->pin_d[i] >= 0)
                        fpioa_set_function(self->pin_d[i], FUNC_SPI1_D0+i);
                }
            }
        }
        int work_mode = self->phase|(self->polarity<<1);
        spi_init(self->id, work_mode, SPI_FF_STANDARD, self->bits, 0);
        spi_set_clk_rate(self->id, self->baudrate);
    }
    else//TODO:
    {

    }
    self->state = MACHINE_HW_SPI_STATE_INIT;
}

mp_obj_t machine_hw_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    machine_hw_spi_obj_t *self = m_new_obj(machine_hw_spi_obj_t);
    self->base.type = &machine_hw_spi_type;

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
	machine_hw_spi_init((mp_obj_base_t*)self, n_args, all_args, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}


/******************************************************************************/
// MicroPython bindings for generic machine.SPI

STATIC mp_obj_t machine_spi_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    mp_obj_base_t *s = (mp_obj_base_t*)MP_OBJ_TO_PTR(args[0]);
    mp_machine_hw_spi_p_t *spi_p = (mp_machine_hw_spi_p_t*)s->type->protocol;
    spi_p->init(s, n_args - 1, args + 1, kw_args);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_spi_init_obj, 1, machine_spi_init);

STATIC mp_obj_t machine_spi_deinit(mp_obj_t self) {
    mp_obj_base_t *s = (mp_obj_base_t*)MP_OBJ_TO_PTR(self);
    mp_machine_hw_spi_p_t *spi_p = (mp_machine_hw_spi_p_t*)s->type->protocol;
    if (spi_p->deinit != NULL) {
        spi_p->deinit(s);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_spi_deinit_obj, machine_spi_deinit);

STATIC void mp_machine_spi_transfer(mp_obj_t self, size_t len, const void *src, void *dest, int cs) {
    mp_obj_base_t *s = (mp_obj_base_t*)MP_OBJ_TO_PTR(self);
    mp_machine_hw_spi_p_t *spi_p = (mp_machine_hw_spi_p_t*)s->type->protocol;
    spi_p->transfer(s, len, src, dest, cs);
}

STATIC mp_obj_t mp_machine_spi_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum {
        ARG_write,
        ARG_cs,
    };
    static const mp_arg_t machine_spi_read_allowed_args[] = {
        { MP_QSTR_write, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_cs, MP_ARG_INT, {.u_int = 0} },
    };
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    mp_arg_val_t args[MP_ARRAY_SIZE(machine_spi_read_allowed_args)];
    mp_arg_parse_all(n_args - 2, pos_args + 2, kw_args,
        MP_ARRAY_SIZE(machine_spi_read_allowed_args), machine_spi_read_allowed_args, args);

    int cs = args[ARG_cs].u_int;
    // bool cs_valid = true;
    // if(cs>=0 && cs<4)
    // {
    //     if(self->pin_cs[cs] < 0)
    //         cs_valid =false;
    // }
    // else
    //     cs_valid = false;
    // if(!cs_valid)
    //     mp_raise_ValueError("[MAIXPY]SPI: cs value error");

    vstr_t vstr;
    vstr_init_len(&vstr, mp_obj_get_int(pos_args[1]));
    memset(vstr.buf, args[ARG_write].u_int, vstr.len);
    mp_machine_spi_transfer(self, vstr.len, vstr.buf, vstr.buf, cs);
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_machine_spi_read_obj, 2, mp_machine_spi_read);

STATIC mp_obj_t mp_machine_spi_readinto(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
        enum {
        ARG_write,
        ARG_cs,
    };
    static const mp_arg_t machine_spi_read_allowed_args[] = {
        { MP_QSTR_write, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_cs, MP_ARG_INT, {.u_int = 0} },
    };
    // machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    mp_arg_val_t args[MP_ARRAY_SIZE(machine_spi_read_allowed_args)];
    mp_arg_parse_all(n_args - 2, pos_args + 2, kw_args,
        MP_ARRAY_SIZE(machine_spi_read_allowed_args), machine_spi_read_allowed_args, args);

    int cs = args[ARG_cs].u_int;
    // bool cs_valid = true;
    // if(cs>=0 && cs<4)
    // {
    //     if(self->pin_cs[cs] < 0)
    //         cs_valid =false;
    // }
    // else
    //     cs_valid = false;
    // if(!cs_valid)
    //     mp_raise_ValueError("[MAIXPY]SPI: cs value error");

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(pos_args[1], &bufinfo, MP_BUFFER_WRITE);
    memset(bufinfo.buf, args[ARG_write].u_int, bufinfo.len);
    mp_machine_spi_transfer(pos_args[0], bufinfo.len, bufinfo.buf, bufinfo.buf, cs);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_machine_spi_readinto_obj, 2, mp_machine_spi_readinto);

STATIC mp_obj_t mp_machine_spi_write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum {
        ARG_cs
    };
    static const mp_arg_t machine_spi_write_allowed_args[] = {
        { MP_QSTR_cs, MP_ARG_INT, {.u_int = 0} },
    };
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    mp_arg_val_t args[MP_ARRAY_SIZE(machine_spi_write_allowed_args)];
    mp_arg_parse_all(n_args - 2, pos_args + 2, kw_args,
        MP_ARRAY_SIZE(machine_spi_write_allowed_args), machine_spi_write_allowed_args, args);

    int cs = args[ARG_cs].u_int;
    // bool cs_valid = true;
    // if(cs>=0 && cs<4)
    // {
    //     if(self->pin_cs[cs] < 0)
    //         cs_valid =false;
    // }
    // else
    //     cs_valid = false;
    // if(!cs_valid)
    //     mp_raise_ValueError("[MAIXPY]SPI: cs value error");

    mp_buffer_info_t src;
    if(mp_obj_get_type(pos_args[1]) == &mp_type_int)
    {
        uint8_t data = (uint8_t)mp_obj_get_int(pos_args[1]);
        mp_machine_spi_transfer(self, 1, (const uint8_t*)&data, NULL, cs);
    }
    else
    {
        mp_get_buffer_raise(pos_args[1], &src, MP_BUFFER_READ);
        if(src.len==0)
            mp_raise_ValueError("len must > 0");
        mp_machine_spi_transfer(self, src.len, (const uint8_t*)src.buf, NULL, cs);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_machine_hw_spi_write_obj, 2, mp_machine_spi_write);

STATIC mp_obj_t mp_machine_spi_write_readinto(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum {
        ARG_cs
    };
    static const mp_arg_t machine_spi_write_allowed_args[] = {
        { MP_QSTR_cs, MP_ARG_INT, {.u_int = 0} },
    };
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    mp_arg_val_t args[MP_ARRAY_SIZE(machine_spi_write_allowed_args)];
    mp_arg_parse_all(n_args - 3, pos_args + 3, kw_args,
        MP_ARRAY_SIZE(machine_spi_write_allowed_args), machine_spi_write_allowed_args, args);

    int cs = args[ARG_cs].u_int;
    // bool cs_valid = true;
    // if(cs>=0 && cs<4)
    // {
    //     if(self->pin_cs[cs] < 0)
    //         cs_valid =false;
    // }
    // else
    //     cs_valid = false;
    // if(!cs_valid)
    //     mp_raise_ValueError("[MAIXPY]SPI: cs value error");

    mp_buffer_info_t src;
    mp_get_buffer_raise(pos_args[1], &src, MP_BUFFER_READ);
    mp_buffer_info_t dest;
    mp_get_buffer_raise(pos_args[2], &dest, MP_BUFFER_WRITE);
    if (src.len != dest.len) {
        mp_raise_ValueError("[MAIXPY]SPI: buffers must be the same length");
    }
    mp_machine_spi_transfer(self, src.len, src.buf, dest.buf, cs);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_machine_hw_spi_write_readinto_obj, 2, mp_machine_spi_write_readinto);

STATIC const mp_rom_map_elem_t machine_spi_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_spi_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_spi_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_machine_spi_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_machine_spi_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_machine_hw_spi_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_readinto), MP_ROM_PTR(&mp_machine_hw_spi_write_readinto_obj) },

    //spi
    { MP_ROM_QSTR(MP_QSTR_SPI0), MP_ROM_INT(SPI_DEVICE_0) },
    { MP_ROM_QSTR(MP_QSTR_SPI1), MP_ROM_INT(SPI_DEVICE_1) },
    { MP_ROM_QSTR(MP_QSTR_SPI2), MP_ROM_INT(SPI_DEVICE_2) },
    //not support SPI3 currently for SPI3 used by flash
    // { MP_ROM_QSTR(MP_QSTR_SPI3), MP_ROM_INT(SPI_DEVICE_3) },
    //firstbit
    { MP_ROM_QSTR(MP_QSTR_MSB), MP_ROM_INT(MACHINE_SPI_FIRSTBIT_MSB) },
    { MP_ROM_QSTR(MP_QSTR_LSB), MP_ROM_INT(MACHINE_SPI_FIRSTBIT_LSB) },
    //mode
    { MP_ROM_QSTR(MP_QSTR_MODE_MASTER), MP_ROM_INT(MACHINE_SPI_MODE_MASTER) },
    { MP_ROM_QSTR(MP_QSTR_MODE_MASTER_2), MP_ROM_INT(MACHINE_SPI_MODE_MASTER_2) },
    { MP_ROM_QSTR(MP_QSTR_MODE_MASTER_4), MP_ROM_INT(MACHINE_SPI_MODE_MASTER_4) },
    { MP_ROM_QSTR(MP_QSTR_MODE_MASTER_8), MP_ROM_INT(MACHINE_SPI_MODE_MASTER_8) },
    { MP_ROM_QSTR(MP_QSTR_MODE_SLAVE), MP_ROM_INT(MACHINE_SPI_MODE_SLAVE) },
    //cs(chip select)
    { MP_ROM_QSTR(MP_QSTR_CS0), MP_ROM_INT(MACHINE_SPI_CS0) },
    { MP_ROM_QSTR(MP_QSTR_CS1), MP_ROM_INT(MACHINE_SPI_CS1) },
    { MP_ROM_QSTR(MP_QSTR_CS2), MP_ROM_INT(MACHINE_SPI_CS2) },
    { MP_ROM_QSTR(MP_QSTR_CS3), MP_ROM_INT(MACHINE_SPI_CS3) },
};

MP_DEFINE_CONST_DICT(mp_machine_spi_locals_dict, machine_spi_locals_dict_table);

STATIC const mp_machine_hw_spi_p_t machine_hw_spi_p = {
    .init = machine_hw_spi_init,
    .deinit = machine_hw_spi_deinit,
    .transfer = machine_hw_spi_transfer,
};

const mp_obj_type_t machine_hw_spi_type = {
    { &mp_type_type },
    .name = MP_QSTR_SPI,
    .print = machine_hw_spi_print,
    .make_new = machine_hw_spi_make_new,
    .protocol = &machine_hw_spi_p,
    .locals_dict = (mp_obj_dict_t *) &mp_machine_spi_locals_dict,
};

#endif //MICROPY_PY_MACHINE_HW_SPI