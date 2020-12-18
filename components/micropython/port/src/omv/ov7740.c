/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * OV7725 driver.
 *
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cambus.h"
#include "ov7740.h"
#include "omv_boardconfig.h"
#include "sensor.h"
#include "sleep.h"
#include "cambus.h"
#include "dvp.h"

static const uint8_t default_regs[][2] = {
	{0x47, 0x02}  ,
	{0x17, 0x27}  ,
	{0x04, 0x40}  ,
	{0x1B, 0x81}  ,
	{0x29, 0x17}  ,
	{0x5F, 0x03}  ,
	{0x3A, 0x09}  ,
	{0x33, 0x44}  ,
	{0x68, 0x1A}  ,
	{0x14, 0x38}  ,
	{0x5F, 0x04}  ,
	{0x64, 0x00}  ,
	{0x67, 0x90}  ,
	{0x27, 0x80}  ,
	{0x45, 0x41}  ,
	{0x4B, 0x40}  ,
	{0x36, 0x2f}  ,
	{0x11, 0x00}  ,  // 60fps
	{0x36, 0x3f}  ,
	// {0x0c, 0x12}  , // default YUYV
	{0x12, 0x00}  ,
	{0x17, 0x25}  ,
	{0x18, 0xa0}  ,
	{0x1a, 0xf0}  ,
	{0x31, 0x50}  ,
	{0x32, 0x78}  ,
	{0x82, 0x3f}  ,
	{0x85, 0x08}  ,
	{0x86, 0x02}  ,
	{0x87, 0x01}  ,
	{0xd5, 0x10}  ,
	{0x0d, 0x34}  ,
	{0x19, 0x03}  ,
	{0x2b, 0xf8}  ,
	{0x2c, 0x01}  ,
	{0x53, 0x00}  ,
	{0x89, 0x30}  ,
	{0x8d, 0x30}  ,
	{0x8f, 0x85}  ,
	{0x93, 0x30}  ,
	{0x95, 0x85}  ,
	{0x99, 0x30}  ,
	{0x9b, 0x85}  ,
	{0xac, 0x6E}  ,
	{0xbe, 0xff}  ,
	{0xbf, 0x00}  ,
	{0x38, 0x14}  ,
	{0xe9, 0x00}  ,
	{0x3D, 0x08}  ,
	{0x3E, 0x80}  ,
	{0x3F, 0x40}  ,
	{0x40, 0x7F}  ,
	{0x41, 0x6A}  ,
	{0x42, 0x29}  ,
	{0x49, 0x64}  ,
	{0x4A, 0xA1}  ,
	{0x4E, 0x13}  ,
	{0x4D, 0x50}  ,
	{0x44, 0x58}  ,
	{0x4C, 0x1A}  ,
	{0x4E, 0x14}  ,
	{0x38, 0x11}  ,
	{0x84, 0x70}  ,
	{0,0}
};

#define NUM_BRIGHTNESS_LEVELS (9)
static const uint8_t brightness_regs[NUM_BRIGHTNESS_LEVELS][2] = {
    {0x06, 0x40}, /* -4 */
    {0x06, 0x30}, /* -3 */
    {0x06, 0x20}, /* -2 */
    {0x06, 0x10}, /* -1 */
    {0x0E, 0x00}, /*  0 */
    {0x0E, 0x10}, /* +1 */
    {0x0E, 0x20}, /* +2 */
    {0x0E, 0x30}, /* +3 */
    {0x0E, 0x40}, /* +4 */
};

#define NUM_CONTRAST_LEVELS (9)
static const uint8_t contrast_regs[NUM_CONTRAST_LEVELS][3] = {
    {0x20, 0x10, 0xD0}, /* -4 */
    {0x20, 0x14, 0x80}, /* -3 */
    {0x20, 0x18, 0x48}, /* -2 */
    {0x20, 0x1C, 0x20}, /* -1 */
    {0x20, 0x20, 0x00}, /*  0 */
    {0x20, 0x24, 0x00}, /* +1 */
    {0x20, 0x28, 0x00}, /* +2 */
    {0x20, 0x2C, 0x00}, /* +3 */
    {0x20, 0x30, 0x00}, /* +4 */
};

#define NUM_SATURATION_LEVELS (9)
static const uint8_t saturation_regs[NUM_SATURATION_LEVELS][2] = {
    {0x00, 0x00}, /* -4 */
    {0x10, 0x10}, /* -3 */
    {0x20, 0x20}, /* -2 */
    {0x30, 0x30}, /* -1 */
    {0x40, 0x40}, /*  0 */
    {0x50, 0x50}, /* +1 */
    {0x60, 0x60}, /* +2 */
    {0x70, 0x70}, /* +3 */
    {0x80, 0x80}, /* +4 */
};

static int reset(sensor_t *sensor)
{
    // // Reset all registers
    int ret;
    ret = cambus_writeb(sensor->slv_addr, 0x12, 0x80);

    // Delay 2 ms
    msleep(2);

    // Write default regsiters
    for (int i = 0; default_regs[i][0]; i++) {
        ret |= cambus_writeb(sensor->slv_addr, default_regs[i][0], default_regs[i][1]);
    }

    return ret;
}

static int ov7740_sleep(sensor_t *sensor, int enable)
{
    if(enable)
    {
        DCMI_PWDN_HIGH();
    }
    else
    {
        DCMI_PWDN_LOW();
    }
    return 0;
}

static int read_reg(sensor_t *sensor, uint8_t reg_addr)
{
    uint8_t reg_data;
    if (cambus_readb(sensor->slv_addr, reg_addr, &reg_data) != 0) {
        return -1;
    }
    return reg_data;
}

static int write_reg(sensor_t *sensor, uint8_t reg_addr, uint16_t reg_data)
{
    return cambus_writeb(sensor->slv_addr, reg_addr, reg_data);
}

static int set_pixformat(sensor_t *sensor, pixformat_t pixformat)
{
    return 0;
}

static int set_framesize(sensor_t *sensor, framesize_t framesize)
{
    int ret=0;
    uint16_t w = resolution[framesize][0];
    uint16_t h = resolution[framesize][1];

    // VGA
    if ((w > 320) || (h > 240))
    {
        ret |= cambus_writeb(sensor->slv_addr, 0x31, 0xA0);
        ret |= cambus_writeb(sensor->slv_addr, 0x32, 0xF0);
        ret |= cambus_writeb(sensor->slv_addr, 0x82, 0x32);
    }
    // QVGA
    else if( ((w <= 320) && (h <= 240)) && ((w > 160) || (h > 120)) )
    {
        ret |= cambus_writeb(sensor->slv_addr, 0x31, 0x50);
        ret |= cambus_writeb(sensor->slv_addr, 0x32, 0x78);
        ret |= cambus_writeb(sensor->slv_addr, 0x82, 0x3F);
    }
    // QQVGA
    else
    {
        ret |= cambus_writeb(sensor->slv_addr, 0x31, 0x28);
        ret |= cambus_writeb(sensor->slv_addr, 0x32, 0x3c);
        ret |= cambus_writeb(sensor->slv_addr, 0x82, 0x3F);
    }

    return ret;
}

static int set_framerate(sensor_t *sensor, framerate_t framerate)
{
    int ret = 0;
    switch(framerate)
    {
        case FRAMERATE_60FPS:
            ret |= cambus_writeb(sensor->slv_addr, 0x11, 0x00);
            ret |= cambus_writeb(sensor->slv_addr, 0x55, 0x40);
            ret |= cambus_writeb(sensor->slv_addr, 0x2b, 0xF0);
            ret |= cambus_writeb(sensor->slv_addr, 0x2c, 0x01);
            break;
        case FRAMERATE_30FPS:
            ret |= cambus_writeb(sensor->slv_addr, 0x11, 0x01);
            ret |= cambus_writeb(sensor->slv_addr, 0x55, 0x40);
            ret |= cambus_writeb(sensor->slv_addr, 0x2b, 0xF0);
            ret |= cambus_writeb(sensor->slv_addr, 0x2c, 0x01);
            break;
        case FRAMERATE_25FPS:
            ret |= cambus_writeb(sensor->slv_addr, 0x11, 0x01);
            ret |= cambus_writeb(sensor->slv_addr, 0x55, 0x40);
            ret |= cambus_writeb(sensor->slv_addr, 0x2b, 0x5E);
            ret |= cambus_writeb(sensor->slv_addr, 0x2c, 0x02);
            break;
        case FRAMERATE_15FPS:
            ret |= cambus_writeb(sensor->slv_addr, 0x11, 0x03);
            ret |= cambus_writeb(sensor->slv_addr, 0x55, 0x40);
            ret |= cambus_writeb(sensor->slv_addr, 0x2b, 0xF0);
            ret |= cambus_writeb(sensor->slv_addr, 0x2c, 0x01);
            break;
        default:
            return -1;
    }
    return ret;
}

static int set_contrast(sensor_t *sensor, int level)
{
    int ret=0;
    uint8_t tmp = 0;

    level += (NUM_CONTRAST_LEVELS / 2);
    if (level < 0 || level >= NUM_CONTRAST_LEVELS) {
        return -1;
    }
    ret |= cambus_readb(sensor->slv_addr, 0x81,&tmp);
    tmp |= 0x20;
    ret |= cambus_writeb(sensor->slv_addr, 0x81, tmp);
    ret |= cambus_readb(sensor->slv_addr, 0xDA,&tmp);
    tmp |= 0x04;
    ret |= cambus_writeb(sensor->slv_addr, 0xDA, tmp);
    ret |= cambus_writeb(sensor->slv_addr, 0xE1, contrast_regs[level][0]);
    ret |= cambus_writeb(sensor->slv_addr, 0xE2, contrast_regs[level][1]);
    ret |= cambus_writeb(sensor->slv_addr, 0xE3, contrast_regs[level][2]);
    ret |= cambus_readb(sensor->slv_addr, 0xE4,&tmp);
    tmp &= 0xFB;
    ret |= cambus_writeb(sensor->slv_addr, 0xE4, tmp);

    return ret;
}

static int set_brightness(sensor_t *sensor, int level)
{
    int ret=0;
    uint8_t tmp = 0;

    level += (NUM_BRIGHTNESS_LEVELS / 2);
    if (level < 0 || level >= NUM_BRIGHTNESS_LEVELS) {
        return -1;
    }
    ret |= cambus_readb(sensor->slv_addr, 0x81,&tmp);
    tmp |= 0x20;
    ret |= cambus_writeb(sensor->slv_addr, 0x81, tmp);
    ret |= cambus_readb(sensor->slv_addr, 0xDA,&tmp);
    tmp |= 0x04;
    ret |= cambus_writeb(sensor->slv_addr, 0xDA, tmp);
    ret |= cambus_writeb(sensor->slv_addr, 0xE4, brightness_regs[level][0]);
    ret |= cambus_writeb(sensor->slv_addr, 0xE3, brightness_regs[level][1]);
    return ret;
}

static int set_saturation(sensor_t *sensor, int level)
{
    int ret=0;
    uint8_t tmp = 0;

    level += (NUM_SATURATION_LEVELS / 2 );
    if (level < 0 || level >= NUM_SATURATION_LEVELS) {
        return -1;
    }
    ret |= cambus_readb(sensor->slv_addr, 0x81,&tmp);
    tmp |= 0x20;
    ret |= cambus_writeb(sensor->slv_addr, 0x81, tmp);
    ret |= cambus_readb(sensor->slv_addr, 0xDA,&tmp);
    tmp |= 0x02;
    ret |= cambus_writeb(sensor->slv_addr, 0xDA, tmp);
    ret |= cambus_writeb(sensor->slv_addr, 0xDD, saturation_regs[level][0]);
    ret |= cambus_writeb(sensor->slv_addr, 0xDE, saturation_regs[level][1]);
    return ret;
}

static int set_gainceiling(sensor_t *sensor, gainceiling_t gainceiling)
{
    int ret=0;
    uint8_t tmp = 0;
    uint8_t ceiling = (uint8_t)gainceiling;
    if(ceiling > GAINCEILING_32X)
        ceiling = GAINCEILING_32X;
    tmp = (ceiling & 0x07) << 4;
    ret |= cambus_writeb(sensor->slv_addr, 0x14, tmp);
    return ret;
}

static int set_colorbar(sensor_t *sensor, int enable)
{
    int ret=0;
    if(enable)
    {
        ret |= cambus_writeb(sensor->slv_addr, 0x38, 0x07);
        ret |= cambus_writeb(sensor->slv_addr, 0x84, 0x02);
    }
    else
    {
        ret |= cambus_writeb(sensor->slv_addr, 0x38, 0x07);
        ret |= cambus_writeb(sensor->slv_addr, 0x84, 0x00);
    }
    return ret;
}

static int set_auto_gain(sensor_t *sensor, int enable, float gain_db, float gain_db_ceiling)
{
    int ret=0;
    uint8_t tmp = 0;
    uint16_t gain = (uint16_t)gain_db;
    uint8_t ceiling = (uint8_t)gain_db_ceiling;

    ret |= cambus_readb(sensor->slv_addr, 0x13, &tmp);
    if(enable != 0)
    {
        ret |= cambus_writeb(sensor->slv_addr, 0x13, tmp | 0x04);
    }
    else
    {
        ret |= cambus_writeb(sensor->slv_addr, 0x13, tmp & 0xFB);
        if(gain!=0xFFFF && (uint16_t)gain_db_ceiling!=0xFFFF)
        {
            ret |= cambus_readb(sensor->slv_addr, 0x15, &tmp);
            tmp = (tmp & 0xFC) | (gain>>8 & 0x03);
            ret |= cambus_writeb(sensor->slv_addr, 0x15, tmp);
            tmp = gain & 0xFF;
            ret |= cambus_writeb(sensor->slv_addr, 0x00, tmp);
            tmp = (ceiling & 0x07) << 4;
            ret |= cambus_writeb(sensor->slv_addr, 0x14, tmp);
        }
    }
    return ret;
}

static int get_gain_db(sensor_t *sensor, float *gain_db)
{
    int ret=0;
    uint8_t tmp = 0;
    uint16_t gain;

    ret |= cambus_readb(sensor->slv_addr, 0x00, &tmp);
    gain = tmp;
    ret |= cambus_readb(sensor->slv_addr, 0x15, &tmp);
    gain |= ((uint16_t)(tmp & 0x03))<<8;
    *gain_db = (float)gain;
    return ret;
}

static int set_auto_exposure(sensor_t *sensor, int enable, int exposure_us)
{
    int ret=0;
    uint8_t tmp = 0;

    ret |= cambus_readb(sensor->slv_addr, 0x13, &tmp);
    if(enable != 0)
    {
        ret |= cambus_writeb(sensor->slv_addr, 0x13, tmp | 0x01);
    }
    else
    {
        ret |= cambus_writeb(sensor->slv_addr, 0x13, tmp & 0xFE);
        ret |= cambus_writeb(sensor->slv_addr, 0x0F, (uint8_t)(exposure_us>>8));
        ret |= cambus_writeb(sensor->slv_addr, 0x10, (uint8_t)exposure_us);
    }
    return ret;
}

static int get_exposure_us(sensor_t *sensor, int *exposure_us)
{
    int ret=0;
    uint8_t tmp = 0;

    ret |= cambus_readb(sensor->slv_addr, 0x0F, &tmp);
    *exposure_us = tmp<<8 & 0xFF00;
    ret |= cambus_readb(sensor->slv_addr, 0x10, &tmp);
    *exposure_us = tmp | *exposure_us;
    return ret;
}

static int set_auto_whitebal(sensor_t *sensor, int enable, float r_gain_db, float g_gain_db, float b_gain_db)
{
    int ret=0;
    uint8_t tmp = 0;

    ret |= cambus_readb(sensor->slv_addr, 0x80, &tmp);
    if(enable != 0)
    {
        ret |= cambus_writeb(sensor->slv_addr, 0x80, tmp | 0x14);
    }
    else
    {
        if((uint16_t)r_gain_db!= 0xFFFF && (uint16_t)g_gain_db!=0xFFFF && (uint16_t)b_gain_db!=0xFFFF)
        {
            ret |= cambus_writeb(sensor->slv_addr, 0x80, tmp & 0xEF);
            ret |= cambus_writeb(sensor->slv_addr, 0x01, (uint8_t)b_gain_db);
            ret |= cambus_writeb(sensor->slv_addr, 0x02, (uint8_t)r_gain_db);
            ret |= cambus_writeb(sensor->slv_addr, 0x03, (uint8_t)g_gain_db);
        }
        else
        {
            ret |= cambus_writeb(sensor->slv_addr, 0x80, tmp & 0xEB);
        }
    }
    return ret;
}

static int get_rgb_gain_db(sensor_t *sensor, float *r_gain_db, float *g_gain_db, float *b_gain_db)
{
    int ret=0;
    uint8_t tmp = 0;

    ret |= cambus_readb(sensor->slv_addr, 0x02, &tmp);
    *r_gain_db = (float)tmp;
    ret |= cambus_readb(sensor->slv_addr, 0x03, &tmp);
    *g_gain_db = (float)tmp;
    ret |= cambus_readb(sensor->slv_addr, 0x01, &tmp);
    *b_gain_db = (float)tmp;

    return ret;
}

static int set_hmirror(sensor_t *sensor, int enable)
{
    uint8_t reg;
    int ret = cambus_readb(sensor->slv_addr, 0x0C, &reg);
    ret |= cambus_writeb(sensor->slv_addr, 0x0C, OV7740_SET_MIRROR(reg, enable)) ;

    //Sensor Horizontal Output Start Point
    ret = cambus_readb(sensor->slv_addr, 0x16, &reg);
    ret |= cambus_writeb(sensor->slv_addr, 0x16, OV7740_SET_SP(reg, enable));

    return ret;
}

static int set_vflip(sensor_t *sensor, int enable)
{
    uint8_t reg;
    int ret = cambus_readb(sensor->slv_addr, 0x0C, &reg);
    ret |= cambus_writeb(sensor->slv_addr, 0x0C, OV7740_SET_FLIP(reg, enable));

    return ret;
}

static int set_special_effect(sensor_t *sensor, sde_t sde)
{
    int ret;
    uint8_t reg;
    switch (sde)
    {
        case SDE_NORMAL:
            ret = cambus_readb(sensor->slv_addr, 0x81, &reg);
            ret |= cambus_writeb(sensor->slv_addr, 0x81, reg & 0xFE);
            ret = cambus_readb(sensor->slv_addr, 0xDA, &reg);
            ret |= cambus_writeb(sensor->slv_addr, 0xDA, reg & 0xBF);
            break;
        case SDE_NEGATIVE:
            ret = cambus_readb(sensor->slv_addr, 0x81, &reg);
            ret |= cambus_writeb(sensor->slv_addr, 0x81, reg | 0x01);
            ret = cambus_readb(sensor->slv_addr, 0xDA, &reg);
            ret |= cambus_writeb(sensor->slv_addr, 0xDA, reg | 0x40);
            break;

        default:
            return -1;
    }
    return ret;
}

static int set_lens_correction(sensor_t *sensor, int enable, int radi, int coef)
{
    return -1;
}

int ov7740_init(sensor_t *sensor)
{
    // Initialize sensor structure.
    sensor->gs_bpp              = 2;
    sensor->reset               = reset;
    sensor->sleep               = ov7740_sleep;
    sensor->read_reg            = read_reg;
    sensor->write_reg           = write_reg;
    sensor->set_pixformat       = set_pixformat;
    sensor->set_framesize       = set_framesize;
    sensor->set_framerate       = set_framerate;
    sensor->set_contrast        = set_contrast;
    sensor->set_brightness      = set_brightness;
    sensor->set_saturation      = set_saturation;
    sensor->set_gainceiling     = set_gainceiling;
    sensor->set_colorbar        = set_colorbar;
    sensor->set_auto_gain       = set_auto_gain;
    sensor->get_gain_db         = get_gain_db;
    sensor->set_auto_exposure   = set_auto_exposure;
    sensor->get_exposure_us     = get_exposure_us;
    sensor->set_auto_whitebal   = set_auto_whitebal;
    sensor->get_rgb_gain_db     = get_rgb_gain_db;
    sensor->set_hmirror         = set_hmirror;
    sensor->set_vflip           = set_vflip;
    sensor->set_special_effect  = set_special_effect;
    sensor->set_lens_correction = set_lens_correction;

    // Set sensor flags
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_VSYNC, 1);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_HSYNC, 0);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_PIXCK, 1);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_FSYNC, 1);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_JPEGE, 0);

    return 0;
}
