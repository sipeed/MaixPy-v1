/**
 * Copyright 2020 Sipeed Co.,Ltd.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *  http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>

#include "gc2145_regs.h"
#include "gc2145.h"

#include "dvp.h"
#include "plic.h"
#include "sleep.h"
#include "printf.h"

#include "sensor.h"
#include "cambus.h"

#include "mphalport.h"

int gc2145_reset(sensor_t *sensor)
{
    uint16_t index = 0;

    cambus_writeb(GC2145_ADDR, 0xFE, 0x01);
    for (index = 0; sensor_gc2145_default_regs[index][0]; index++)
    {
        if (sensor_gc2145_default_regs[index][0] == 0xff)
        {
            mp_hal_delay_ms(sensor_gc2145_default_regs[index][1]);
            continue;
        }
        // mp_printf(&mp_plat_print, "0x12,0x%02x,0x%02x,\r\n", sensor_gc2145_default_regs[index][0], sensor_gc2145_default_regs[index][1]);//debug
        cambus_writeb(GC2145_ADDR, sensor_gc2145_default_regs[index][0], sensor_gc2145_default_regs[index][1]);
        // mp_hal_delay_ms(1);
    }
    return 0;
}

static int gc2145_read_reg(sensor_t *sensor, uint8_t reg_addr)
{
    uint8_t reg_data;
    if (cambus_readb(sensor->slv_addr, reg_addr, &reg_data) != 0)
    {
        return -1;
    }
    return reg_data;
}

static int gc2145_write_reg(sensor_t *sensor, uint8_t reg_addr, uint16_t reg_data)
{
    return cambus_writeb(sensor->slv_addr, reg_addr, (uint8_t)reg_data);
}

static int gc2145_set_pixformat(sensor_t *sensor, pixformat_t pixformat)
{
    // TODO:
    return 0;
}

static int gc2145_set_framesize(sensor_t *sensor, framesize_t framesize)
{
    int ret = 0;
    uint16_t w = resolution[framesize][0];
    uint16_t h = resolution[framesize][1];

    int i = 0;
    const uint8_t(*regs)[2];

    if (framesize == FRAMESIZE_QQVGA)
    {
        regs = sensor_gc2145_qqvga_config;
    }
    else if ((w <= 320) && (h <= 240))
    {
        regs = sensor_gc2145_qvga_config;
    }
    else
    {
        regs = sensor_gc2145_vga_config;
    }

    while (regs[i][0])
    {
        cambus_writeb(sensor->slv_addr, regs[i][0], regs[i][1]);
        i++;
    }
    /* delay n ms */
    mp_hal_delay_ms(30);
    dvp_set_image_size(w, h);
    return ret;
}

static int gc2145_set_framerate(sensor_t *sensor, framerate_t framerate)
{
    // TODO:
    return 0;
}

#define NUM_CONTRAST_LEVELS (5)
static uint8_t contrast_regs[NUM_CONTRAST_LEVELS][2] = {
    {0x80, 0x00},
    {0x80, 0x20},
    {0x80, 0x40},
    {0x80, 0x60},
    {0x80, 0x80}};

static int gc2145_set_contrast(sensor_t *sensor, int level)
{
    int ret = 0;

    level += (NUM_CONTRAST_LEVELS / 2);
    if (level < 0 || level > NUM_CONTRAST_LEVELS)
    {
        return -1;
    }
    cambus_writeb(sensor->slv_addr, 0xfe, 0x00);
    cambus_writeb(sensor->slv_addr, 0xd4, contrast_regs[level][0]);
    cambus_writeb(sensor->slv_addr, 0xd3, contrast_regs[level][1]);
    return ret;
}

static int gc2145_set_brightness(sensor_t *sensor, int level)
{
    int ret = 0;
    // TODO:
    return ret;
}

#define NUM_SATURATION_LEVELS (5)
static uint8_t saturation_regs[NUM_SATURATION_LEVELS][3] = {
    {0x00, 0x00, 0x00},
    {0x10, 0x10, 0x10},
    {0x20, 0x20, 0x20},
    {0x30, 0x30, 0x30},
    {0x40, 0x40, 0x40},
};
static int gc2145_set_saturation(sensor_t *sensor, int level)
{
    int ret = 0;
    level += (NUM_CONTRAST_LEVELS / 2);
    if (level < 0 || level > NUM_CONTRAST_LEVELS)
    {
        return -1;
    }
    cambus_writeb(sensor->slv_addr, 0xfe, 0x00);
    cambus_writeb(sensor->slv_addr, 0xd0, saturation_regs[level][0]);
    cambus_writeb(sensor->slv_addr, 0xd1, saturation_regs[level][1]);
    cambus_writeb(sensor->slv_addr, 0xd2, saturation_regs[level][2]);
    return ret;
}

static int gc2145_set_gainceiling(sensor_t *sensor, gainceiling_t gainceiling)
{
    int ret = 0;
    // TODO:
    return ret;
}

static int gc2145_set_quality(sensor_t *sensor, int qs)
{
    int ret = 0;
    // TODO:
    return ret;
}

static int gc2145_set_colorbar(sensor_t *sensor, int enable)
{
    int ret = 0;
    // TODO:
    return ret;
}

static int gc2145_set_auto_gain(sensor_t *sensor, int enable, float gain_db, float gain_db_ceiling)
{
    int ret = 0;
    // TODO:
    return ret;
}

static int gc2145_get_gain_db(sensor_t *sensor, float *gain_db)
{
    int ret = 0;
    // TODO:
    return ret;
}

static int gc2145_set_auto_exposure(sensor_t *sensor, int enable, int exposure_us)
{
    int ret = 0;
    uint8_t temp;
    cambus_writeb(sensor->slv_addr, 0xfe, 0x00);
    cambus_readb(sensor->slv_addr, 0x4f, &temp);
    if (enable != 0)
    {
        cambus_writeb(sensor->slv_addr, 0x4f, temp | 0x01); // enable
        if (exposure_us != -1)
        {
            cambus_writeb(sensor->slv_addr, 0xfe, 0x01);
            cambus_writeb(sensor->slv_addr, 0x2b, (uint8_t)(((uint16_t)exposure_us) >> 8));
            cambus_writeb(sensor->slv_addr, 0x2c, (uint8_t)(((uint16_t)exposure_us)));
        }
    }
    else
    {
        cambus_writeb(sensor->slv_addr, 0x4f, temp & 0xfe); // disable
        if (exposure_us != -1)
        {
            cambus_writeb(sensor->slv_addr, 0xfe, 0x01);
            cambus_writeb(sensor->slv_addr, 0x2b, (uint8_t)(((uint16_t)exposure_us) >> 8));
            cambus_writeb(sensor->slv_addr, 0x2c, (uint8_t)(((uint16_t)exposure_us)));
        }
    }
    return ret;
}

static int gc2145_get_exposure_us(sensor_t *sensor, int *exposure_us)
{
    int ret = 0;
    // TODO:
    return ret;
}

static int gc2145_set_auto_whitebal(sensor_t *sensor, int enable, float r_gain_db, float g_gain_db, float b_gain_db)
{
    int ret = 0;
    uint8_t temp;
    cambus_writeb(sensor->slv_addr, 0xfe, 0x00);
    cambus_readb(sensor->slv_addr, 0x42, &temp);
    if (enable != 0)
    {
        cambus_writeb(sensor->slv_addr, 0x42, temp | 0x02); // enable
        if (!isnanf(r_gain_db))
            cambus_writeb(sensor->slv_addr, 0x80, (uint8_t)r_gain_db); //limit
        if (!isnanf(g_gain_db))
            cambus_writeb(sensor->slv_addr, 0x81, (uint8_t)g_gain_db);
        if (!isnanf(b_gain_db))
            cambus_writeb(sensor->slv_addr, 0x82, (uint8_t)b_gain_db);
    }
    else
    {
        cambus_writeb(sensor->slv_addr, 0x42, temp & 0xfd); // disable
        if (!isnanf(r_gain_db))
            cambus_writeb(sensor->slv_addr, 0x77, (uint8_t)r_gain_db);
        if (!isnanf(g_gain_db))
            cambus_writeb(sensor->slv_addr, 0x78, (uint8_t)g_gain_db);
        if (!isnanf(b_gain_db))
            cambus_writeb(sensor->slv_addr, 0x79, (uint8_t)b_gain_db);
    }
    return ret;
}

static int gc2145_get_rgb_gain_db(sensor_t *sensor, float *r_gain_db, float *g_gain_db, float *b_gain_db)
{
    int ret = 0;
    // TODO:
    return ret;
}

static int gc2145_set_hmirror(sensor_t *sensor, int enable)
{
    uint8_t data;
    cambus_readb(GC2145_ADDR, 0x17, &data);
    data = data & 0xFC;
    if (enable)
    {
        data = data | 0x01 | (sensor->vflip ? 0x02 : 0x00);
    }
    else
    {
        data = data | (sensor->vflip ? 0x02 : 0x00);
    }
    return cambus_writeb(GC2145_ADDR, 0x17, data);
}

static int gc2145_set_vflip(sensor_t *sensor, int enable)
{
    uint8_t data;
    cambus_readb(GC2145_ADDR, 0x17, &data);
    data = data & 0xFC;
    if (enable)
    {
        data = data | 0x02 | (sensor->hmirror ? 0x01 : 0x00);
    }
    else
    {
        data = data | (sensor->hmirror ? 0x01 : 0x00);
    }
    return cambus_writeb(GC2145_ADDR, 0x17, data);
}

int gc2145_init(sensor_t *sensor)
{
    //Initialize sensor structure.
    sensor->gs_bpp = 2;
    sensor->reset = gc2145_reset;
    sensor->read_reg = gc2145_read_reg;
    sensor->write_reg = gc2145_write_reg;
    sensor->set_pixformat = gc2145_set_pixformat;
    sensor->set_framesize = gc2145_set_framesize;
    sensor->set_framerate = gc2145_set_framerate;
    sensor->set_contrast = gc2145_set_contrast;
    sensor->set_brightness = gc2145_set_brightness;
    sensor->set_saturation = gc2145_set_saturation;
    sensor->set_gainceiling = gc2145_set_gainceiling;
    sensor->set_quality = gc2145_set_quality;
    sensor->set_colorbar = gc2145_set_colorbar;
    sensor->set_auto_gain = gc2145_set_auto_gain;
    sensor->get_gain_db = gc2145_get_gain_db;
    sensor->set_auto_exposure = gc2145_set_auto_exposure;
    sensor->get_exposure_us = gc2145_get_exposure_us;
    sensor->set_auto_whitebal = gc2145_set_auto_whitebal;
    sensor->get_rgb_gain_db = gc2145_get_rgb_gain_db;
    sensor->set_hmirror = gc2145_set_hmirror;
    sensor->set_vflip = gc2145_set_vflip;

    // Set sensor flags
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_VSYNC, 0);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_HSYNC, 0);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_PIXCK, 1);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_FSYNC, 0);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_JPEGE, 1);

    return 0;
}