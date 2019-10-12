
/*
 * This file is part of the MaixPy project.
 * Copyright (c) 2019 Sipeed Ltd
 * This work is licensed under the Apache2.0 license, see the file LICENSE for details.
 *
 * OV3660 driver.
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cambus.h"
#include "omv_boardconfig.h"
#include "sensor.h"
#include "sleep.h"
#include "cambus.h"
#include "dvp.h"
#include "ov3660.h"
#include "mphalport.h"

#define SVGA_HSIZE     (800)
#define SVGA_VSIZE     (600)

#define UXGA_HSIZE     (1600)
#define UXGA_VSIZE     (1200)
#define REG_DLY  0xffff

typedef struct{
    uint16_t reg;
    uint8_t  data;
}cmd_a16_d8_t;

static const cmd_a16_d8_t ov3660_default[] = { //k210 
    {0x3008, 0x82},// software reset
    {REG_DLY,0x0a},//delay 10ms

    {0x3103, 0x13},
    {0x3008, 0x42}, // power down
    {0x3017, 0xff}, // D[9:0]/VSYNC/HREF/PCLK output
    {0x3018, 0xff}, // D[9:0] output
    {0x302c, 0xc3}, // outpu drive capability 4x, FSIN input enable, STROBE input enable
    {0x4740, 0x21},
                
    {0x3611, 0x01},
    {0x3612, 0x2d},
                
    {0x3032, 0x00},
    {0x3614, 0x80},
    {0x3618, 0x00},
    {0x3619, 0x75},
    {0x3622, 0x80},
    {0x3623, 0x00},
    {0x3624, 0x03},
    {0x3630, 0x52},
    {0x3632, 0x07},
    {0x3633, 0xd2},
    {0x3704, 0x80},
    {0x3708, 0x66},
    {0x3709, 0x12},
    {0x370b, 0x12},
    {0x3717, 0x00},
    {0x371b, 0x60},
    {0x371c, 0x00},
    {0x3901, 0x13},
                
    {0x3600, 0x08},
    {0x3620, 0x43},
    {0x3702, 0x20},
    {0x3739, 0x48},
    {0x3730, 0x20},
    {0x370c, 0x0c},
                
    {0x3a18, 0x00},
    {0x3a19, 0xf8},
                
    {0x3000, 0x10}, // reset for individual blocks
    {0x3002, 0x1c}, // reset for individual blocks
    {0x3004, 0xef}, // clock enable for individual blocks
    {0x3006, 0xc3}, // clock enable for individual blocks
                
    {0x6700, 0x05},
    {0x6701, 0x19},
    {0x6702, 0xfd},
    {0x6703, 0xd1},
    {0x6704, 0xff},
    {0x6705, 0xff},
                
    {0x3800, 0x00},
    {0x3801, 0x00},
    {0x3802, 0x00},
    {0x3803, 0x00},
    {0x3804, 0x08},
    {0x3805, 0x1f},
    {0x3806, 0x06},
    {0x3807, 0x09},
    {0x3808, 0x03},
    {0x3809, 0x20},
    {0x380a, 0x02},
    {0x380b, 0x58},
    {0x3810, 0x00},
    {0x3811, 0x08},
    {0x3812, 0x00},
    {0x3813, 0x02},
    {0x3814, 0x31},
    {0x3815, 0x31},
                
    {0x3826, 0x23},
    {0x303a, 0x00}, // PLLS bypass off
    {0x303b, 0x17}, // PLLS multiplier
    {0x303c, 0x11}, // PLLS charge pump , PLLS system divider, default 0x11
    {0x303d, 0x30}, // PLLS predivider[5:4], PLLS root divider[2], PLLS seld5[1:0], default 0x30
    {0x3824, 0x02}, // PCLK divider ratio
    {0x460c, 0x22}, // PCLK divider controled by 0x3824[4:0]
                
    {0x380c, 0x09},//25fps
    {0x380d, 0x30},
    {0x380e, 0x03},
    {0x380f, 0x10},
                
    {0x3c01, 0x80},
    {0x3c00, 0x04},
    {0x3a08, 0x00},
    {0x3a09, 0xc4},
    {0x3a0e, 0x04},
    {0x3a0a, 0x00},
    {0x3a0b, 0xa4},
    {0x3a0d, 0x04},
                
    {0x3a00, 0x3c},
    {0x3a14, 0x09},
    {0x3a15, 0x30},
    {0x3a02, 0x09},
    {0x3a03, 0x30},
                
    {0x4300, 0x32}, // UYVY
    {0x440e, 0x08},
    {0x4520, 0x0b},
    {0x460b, 0x37},
    {0x4713, 0x02},
    {0x471c, 0xd0},
    {0x5086, 0x00},
                
    {0x5001, 0x03},
    {0x5002, 0x00},
    {0x501f, 0x00}, // YUV422
                
    {0x3820, 0x01},
    {0x3821, 0x01},
    {0x4514, 0xaa},
    {0x3008, 0x02}, // power on
                
    {0x5180, 0xff},
    {0x5181, 0xf2},
    {0x5182, 0x00},
    {0x5183, 0x14},
    {0x5184, 0x25},
    {0x5185, 0x24},
    {0x5186, 0x16},
    {0x5187, 0x16},
    {0x5188, 0x16},
    {0x5189, 0x68},
    {0x518a, 0x60},
    {0x518b, 0xe0},
    {0x518c, 0xb2},
    {0x518d, 0x42},
    {0x518e, 0x35},
    {0x518f, 0x56},
    {0x5190, 0x56},
    {0x5191, 0xf8},
    {0x5192, 0x04},
    {0x5193, 0x70},
    {0x5194, 0xf0},
    {0x5195, 0xf0},
    {0x5196, 0x03},
    {0x5197, 0x01},
    {0x5198, 0x04},
    {0x5199, 0x12},
    {0x519a, 0x04},
    {0x519b, 0x00},
    {0x519c, 0x06},
    {0x519d, 0x82},
    {0x519e, 0x38},
                
    {0x5381, 0x1d},
    {0x5382, 0x60},
    {0x5383, 0x03},
    {0x5384, 0x0b},
    {0x5385, 0x6c},
    {0x5386, 0x77},
    {0x5387, 0x70},
    {0x5388, 0x60},
    {0x5389, 0x12},
    {0x538a, 0x01},
    {0x538b, 0x98},
                
    {0x5481, 0x05},//
    {0x5482, 0x09},//
    {0x5483, 0x10},//
    {0x5484, 0x3a},//
    {0x5485, 0x4c},//
    {0x5486, 0x5a},//
    {0x5487, 0x68},//
    {0x5488, 0x74},//
    {0x5489, 0x80},//
    {0x548a, 0x8e},//
    {0x548b, 0xa4},//
    {0x548c, 0xb4},//
    {0x548d, 0xc8},//
    {0x548e, 0xde},//
    {0x548f, 0xf0},//
    {0x5490, 0x15},//
                
    {0x5000, 0xa7},
    {0x5800, 0x0C},
    {0x5801, 0x09},
    {0x5802, 0x0C},
    {0x5803, 0x0C},
    {0x5804, 0x0D},
    {0x5805, 0x17},
    {0x5806, 0x06},
    {0x5807, 0x05},
    {0x5808, 0x04},
    {0x5809, 0x06},
    {0x580a, 0x09},
    {0x580b, 0x0E},
    {0x580c, 0x05},
    {0x580d, 0x01},
    {0x580e, 0x01},
    {0x580f, 0x01},
    {0x5810, 0x05},
    {0x5811, 0x0D},
    {0x5812, 0x05},
    {0x5813, 0x01},
    {0x5814, 0x01},
    {0x5815, 0x01},
    {0x5816, 0x05},
    {0x5817, 0x0D},
    {0x5818, 0x08},
    {0x5819, 0x06},
    {0x581a, 0x05},
    {0x581b, 0x07},
    {0x581c, 0x0B},
    {0x581d, 0x0D},
    {0x581e, 0x12},
    {0x581f, 0x0D},
    {0x5820, 0x0E},
    {0x5821, 0x10},
    {0x5822, 0x10},
    {0x5823, 0x1E},
    {0x5824, 0x53},
    {0x5825, 0x15},
    {0x5826, 0x05},
    {0x5827, 0x14},
    {0x5828, 0x54},
    {0x5829, 0x25},
    {0x582a, 0x33},
    {0x582b, 0x33},
    {0x582c, 0x34},
    {0x582d, 0x16},
    {0x582e, 0x24},
    {0x582f, 0x41},
    {0x5830, 0x50},
    {0x5831, 0x42},
    {0x5832, 0x15},
    {0x5833, 0x25},
    {0x5834, 0x34},
    {0x5835, 0x33},
    {0x5836, 0x24},
    {0x5837, 0x26},
    {0x5838, 0x54},
    {0x5839, 0x25},
    {0x583a, 0x15},
    {0x583b, 0x25},
    {0x583c, 0x53},
    {0x583d, 0xCF},
                
    {0x3a0f, 0x48},
    {0x3a10, 0x40},
    {0x3a1b, 0x48},
    {0x3a1e, 0x40},
    {0x3a11, 0x90},
    {0x3a1f, 0x20},
                
    {0x5302, 0x28},
    {0x5303, 0x18},
    {0x5306, 0x1c},//0x18
    {0x5307, 0x28},
                
    {0x4002, 0xc5},
    {0x4003, 0x81},
    {0x4005, 0x12},
                
    {0x5688, 0x11},
    {0x5689, 0x11},
    {0x568a, 0x11},
    {0x568b, 0x11},
    {0x568c, 0x11},
    {0x568d, 0x11},
    {0x568e, 0x11},
    {0x568f, 0x11},
                
    {0x5001, 0xa3},
    {0x5580, 0x06},
    {0x5588, 0x00},
    {0x5583, 0x40},
    {0x5584, 0x2c},
    {0,0}
};
static const cmd_a16_d8_t qvga_config[] = { //k210
    {0x3008, 0x42}, // power down

    {0x3503, 0x00},
    {0x3a00, 0x78},
    {0x3a02, 0x03},
    {0x3a03, 0x10},
    {0x3a14, 0x03},
    {0x3a15, 0x10},

    {0x5302, 0x30},
    {0x5303, 0x10},
    {0x5306, 0x0c},
    {0x5307, 0x1c},

    {0x3800, 0x00},
    {0x3801, 0x00},
    {0x3802, 0x00},
    {0x3803, 0x00},
    {0x3804, 0x08},
    {0x3805, 0x1f},
    {0x3806, 0x06},
    {0x3807, 0x09},
    {0x3808, 0x01},
    {0x3809, 0x40},
    {0x380a, 0x00},
    {0x380b, 0xf0},
    {0x3810, 0x00},
    {0x3811, 0x08},
    {0x3812, 0x00},
    {0x3813, 0x02},
    {0x3814, 0x31},
    {0x3815, 0x31},

    {0x3826, 0x23},
    {0x303a, 0x00},
    {0x303b, 0x0a},
    {0x303c, 0x11},// PLLS charge pump , PLLS system divider, default 0x11
    {0x303d, 0x30},
    {0x3824, 0x02},
    {0x460c, 0x22},

    {0x380c, 0x08},//12.5fps
    {0x380d, 0x02},
    {0x380e, 0x03},
    {0x380f, 0x10},

    {0x3a08, 0x00},
    {0x3a09, 0x62},
    {0x3a0e, 0x08},
    {0x3a0a, 0x00},
    {0x3a0b, 0x52},
    {0x3a0d, 0x09},

    {0x3820, 0x01}, // vertical binning mode=true
    {0x3821, 0x01}, // horizontal binning mode=true
    {0x4514, 0xaa},
    {0x3618, 0x00},
    {0x3708, 0x66},
    {0x3709, 0x12},
    {0x4520, 0x0b},

    {0x5001, 0xa3}, // scale enable[7]

    {0x3008, 0x02}, // power on
    {0, 0}
};


// #define NUM_BRIGHTNESS_LEVELS (5)
// static const uint8_t brightness_regs[NUM_BRIGHTNESS_LEVELS + 1][5] = {
//     { BPADDR, BPDATA, BPADDR, BPDATA, BPDATA },
//     { 0x00, 0x04, 0x09, 0x00, 0x00 }, /* -2 */
//     { 0x00, 0x04, 0x09, 0x10, 0x00 }, /* -1 */
//     { 0x00, 0x04, 0x09, 0x20, 0x00 }, /*  0 */
//     { 0x00, 0x04, 0x09, 0x30, 0x00 }, /* +1 */
//     { 0x00, 0x04, 0x09, 0x40, 0x00 }, /* +2 */
// };

// #define NUM_CONTRAST_LEVELS (5)
// static const uint8_t contrast_regs[NUM_CONTRAST_LEVELS + 1][7] = {
//     { BPADDR, BPDATA, BPADDR, BPDATA, BPDATA, BPDATA, BPDATA },
//     { 0x00, 0x04, 0x07, 0x20, 0x18, 0x34, 0x06 }, /* -2 */
//     { 0x00, 0x04, 0x07, 0x20, 0x1c, 0x2a, 0x06 }, /* -1 */
//     { 0x00, 0x04, 0x07, 0x20, 0x20, 0x20, 0x06 }, /*  0 */
//     { 0x00, 0x04, 0x07, 0x20, 0x24, 0x16, 0x06 }, /* +1 */
//     { 0x00, 0x04, 0x07, 0x20, 0x28, 0x0c, 0x06 }, /* +2 */
// };

// #define NUM_SATURATION_LEVELS (5)
// static const uint8_t saturation_regs[NUM_SATURATION_LEVELS + 1][5] = {
//     { BPADDR, BPDATA, BPADDR, BPDATA, BPDATA },
//     { 0x00, 0x02, 0x03, 0x28, 0x28 }, /* -2 */
//     { 0x00, 0x02, 0x03, 0x38, 0x38 }, /* -1 */
//     { 0x00, 0x02, 0x03, 0x48, 0x48 }, /*  0 */
//     { 0x00, 0x02, 0x03, 0x58, 0x58 }, /* +1 */
//     { 0x00, 0x02, 0x03, 0x58, 0x58 }, /* +2 */
// };

static int reset(sensor_t *sensor)
{	
    int i=0;
    const cmd_a16_d8_t* regs;

    regs = ov3660_default;
    while (regs[i].reg) {
        if(regs[i].reg == REG_DLY)
        {
            mp_hal_delay_ms(regs[i].data);
            i++;
            continue;
        }
        cambus_writeb(sensor->slv_addr, regs[i].reg, regs[i].data);
        i++;
    }
    i = 0;
    regs = qvga_config;
    while (regs[i].reg) {
        if(regs[i].reg == REG_DLY)
        {
            mp_hal_delay_ms(regs[i].data);
            i++;
            continue;
        }
        cambus_writeb(sensor->slv_addr, regs[i].reg, regs[i].data);
        i++;
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
    return cambus_writeb(sensor->slv_addr, reg_addr, (uint8_t)reg_data);
}


static int set_pixformat(sensor_t *sensor, pixformat_t pixformat)
{
    // int i=0;
    // const uint8_t (*regs)[2]=NULL;

    /* read pixel format reg */
    // switch (pixformat) {
    //     case PIXFORMAT_RGB565:
    //         regs = rgb565_regs;
    //         break;
    //     case PIXFORMAT_YUV422:
    //     case PIXFORMAT_GRAYSCALE:
    //         regs = yuv422_regs;
    //         break;
    //     case PIXFORMAT_JPEG:
    //         regs = jpeg_regs;

    //         break;
    //     default:
    //         return -1;
    // }

    // /* Write initial regsiters */
    // while (regs[i][0]) {
    //     cambus_writeb(sensor->slv_addr, regs[i][0], regs[i][1]);
    //     i++;
    // }
    switch (pixformat) {
        case PIXFORMAT_RGB565:
	// 		dvp_set_image_format(DVP_CFG_RGB_FORMAT);
    //         break;
        case PIXFORMAT_YUV422:
            dvp_set_image_format(DVP_CFG_YUV_FORMAT);
            break;
        case PIXFORMAT_GRAYSCALE:
			dvp_set_image_format(DVP_CFG_Y_FORMAT);
            break;
    //     case PIXFORMAT_JPEG:
	// 		dvp_set_image_format(DVP_CFG_RGB_FORMAT);
    //         break;
        default:
            return -1;
    }
    /* delay n ms */
    mp_hal_delay_ms(30);
    return 0;
}

static int set_framesize(sensor_t *sensor, framesize_t framesize)
{
    int ret=0;
//     uint8_t clkrc;
//     uint16_t w = resolution[framesize][0];
//     uint16_t h = resolution[framesize][1];

//     int i=0;
//     const uint8_t (*regs)[2];

//     if ((w <= 800) && (h <= 600)) {
//         clkrc =0x80;
//         regs = svga_config;
// //		regs = ov2640_config;
//     } else {
//         clkrc =0x81;
//         regs = uxga_regs;
//     }

//     /* Disable DSP */
//     ret |= cambus_writeb(sensor->slv_addr, BANK_SEL, BANK_SEL_DSP);
//     ret |= cambus_writeb(sensor->slv_addr, R_BYPASS, R_BYPASS_DSP_BYPAS);

//     /* Set CLKRC */
// 	if(clkrc == 0x81)
// 	{
// 	    ret |= cambus_writeb(sensor->slv_addr, BANK_SEL, BANK_SEL_SENSOR);
// 	    ret |= cambus_writeb(sensor->slv_addr, CLKRC, clkrc);
// 	}

//     /* Write DSP input regsiters */
//     while (regs[i][0]) {
//         cambus_writeb(sensor->slv_addr, regs[i][0], regs[i][1]);
//         i++;
//     }
// 	 /* Write output width */
// 	ret |= cambus_writeb(sensor->slv_addr,0xe0,0x04 ); /* OUTH[8]/OUTW[9:8] */
//     ret |= cambus_writeb(sensor->slv_addr, ZMOW, (w>>2)&0xFF); /* OUTW[7:0] (real/4) */
//     ret |= cambus_writeb(sensor->slv_addr, ZMOH, (h>>2)&0xFF); /* OUTH[7:0] (real/4) */
//     ret |= cambus_writeb(sensor->slv_addr, ZMHH, ((h>>8)&0x04)|((w>>10)&0x03)); /* OUTH[8]/OUTW[9:8] */
// 	ret |= cambus_writeb(sensor->slv_addr,0xe0,0x00 ); /* OUTH[8]/OUTW[9:8] */

//     /* Enable DSP */
//     ret |= cambus_writeb(sensor->slv_addr, BANK_SEL, BANK_SEL_DSP);
//     ret |= cambus_writeb(sensor->slv_addr, R_BYPASS, R_BYPASS_DSP_EN);

//     /* delay n ms */
//     mp_hal_delay_ms(30);
// 	dvp_set_image_size(w, h);
    return ret;
}

static int set_framerate(sensor_t *sensor, framerate_t framerate)
{
    return -1;
}

static int set_contrast(sensor_t *sensor, int level)
{
    int ret=0;

    // level += (NUM_CONTRAST_LEVELS / 2 + 1);
    // if (level < 0 || level > NUM_CONTRAST_LEVELS) {
    //     return -1;
    // }

    // /* Switch to DSP register bank */
    // ret |= cambus_writeb(sensor->slv_addr, BANK_SEL, BANK_SEL_DSP);

    // /* Write contrast registers */
    // for (int i=0; i<sizeof(contrast_regs[0])/sizeof(contrast_regs[0][0]); i++) {
    //     ret |= cambus_writeb(sensor->slv_addr, contrast_regs[0][i], contrast_regs[level][i]);
    // }

    return ret;
}

static int set_brightness(sensor_t *sensor, int level)
{
    int ret=0;

    // level += (NUM_BRIGHTNESS_LEVELS / 2 + 1);
    // if (level < 0 || level > NUM_BRIGHTNESS_LEVELS) {
    //     return -1;
    // }

    // /* Switch to DSP register bank */
    // ret |= cambus_writeb(sensor->slv_addr, BANK_SEL, BANK_SEL_DSP);

    // /* Write brightness registers */
    // for (int i=0; i<sizeof(brightness_regs[0])/sizeof(brightness_regs[0][0]); i++) {
    //     ret |= cambus_writeb(sensor->slv_addr, brightness_regs[0][i], brightness_regs[level][i]);
    // }

    return ret;
}

static int set_saturation(sensor_t *sensor, int level)
{
    int ret=0;

    // level += (NUM_SATURATION_LEVELS / 2 + 1);
    // if (level < 0 || level > NUM_SATURATION_LEVELS) {
    //     return -1;
    // }

    // /* Switch to DSP register bank */
    // ret |= cambus_writeb(sensor->slv_addr, BANK_SEL, BANK_SEL_DSP);

    // /* Write contrast registers */
    // for (int i=0; i<sizeof(saturation_regs[0])/sizeof(saturation_regs[0][0]); i++) {
    //     ret |= cambus_writeb(sensor->slv_addr, saturation_regs[0][i], saturation_regs[level][i]);
    // }

    return ret;
}

static int set_gainceiling(sensor_t *sensor, gainceiling_t gainceiling)
{
    int ret =0;

    // /* Switch to SENSOR register bank */
    // ret |= cambus_writeb(sensor->slv_addr, BANK_SEL, BANK_SEL_SENSOR);

    // /* Write gain ceiling register */
    // ret |= cambus_writeb(sensor->slv_addr, COM9, COM9_AGC_SET(gainceiling));

    return ret;
}

static int set_quality(sensor_t *sensor, int qs)
{
    int ret=0;

    // /* Switch to DSP register bank */
    // ret |= cambus_writeb(sensor->slv_addr, BANK_SEL, BANK_SEL_DSP);

    // /* Write QS register */
    // ret |= cambus_writeb(sensor->slv_addr, QS, qs);

    return ret;
}

static int set_colorbar(sensor_t *sensor, int enable)
{
    int ret = 0;
    uint8_t reg;
    /* Switch to SENSOR register bank */
    // int ret = cambus_writeb(sensor->slv_addr, BANK_SEL, BANK_SEL_SENSOR);

    // /* Update COM7 */
    // ret |= cambus_readb(sensor->slv_addr, COM7, &reg);

    // if (enable) {
    //     reg |= COM7_COLOR_BAR;
    // } else {
    //     reg &= ~COM7_COLOR_BAR;
    // }

    // return cambus_writeb(sensor->slv_addr, COM7, reg) | ret;
    return ret;
}

static int set_auto_gain(sensor_t *sensor, int enable, float gain_db, float gain_db_ceiling)
{
   uint8_t reg;
   int ret = 0;

//    int ret = cambus_readb(sensor->slv_addr, BANK_SEL, &reg);
//    ret |= cambus_writeb(sensor->slv_addr, BANK_SEL, reg | BANK_SEL_SENSOR);
//    ret |= cambus_readb(sensor->slv_addr, COM8, &reg);
//    ret |= cambus_writeb(sensor->slv_addr, COM8, (reg & (~COM8_AGC_EN)) | ((enable != 0) ? COM8_AGC_EN : 0));

//    if ((enable == 0) && (!isnanf(gain_db)) && (!isinff(gain_db))) {
//        float gain = IM_MAX(IM_MIN(fast_expf((gain_db / 20.0) * fast_log(10.0)), 32.0), 1.0);

//        int gain_temp = fast_roundf(fast_log2(IM_MAX(gain / 2.0, 1.0)));
//        int gain_hi = 0xF >> (4 - gain_temp);
//        int gain_lo = IM_MIN(fast_roundf(((gain / (1 << gain_temp)) - 1.0) * 16.0), 15);

//        ret |= cambus_writeb(sensor->slv_addr, GAIN, (gain_hi << 4) | (gain_lo << 0));
//    } else if ((enable != 0) && (!isnanf(gain_db_ceiling)) && (!isinff(gain_db_ceiling))) {
//        float gain_ceiling = IM_MAX(IM_MIN(fast_expf((gain_db_ceiling / 20.0) * fast_log(10.0)), 128.0), 2.0);

//        ret |= cambus_readb(sensor->slv_addr, COM9, &reg);
//        ret |= cambus_writeb(sensor->slv_addr, COM9, (reg & 0x1F) | ((fast_ceilf(fast_log2(gain_ceiling)) - 1) << 5));
//    }

   return ret;
}

static int get_gain_db(sensor_t *sensor, float *gain_db)
{
    uint8_t reg, gain;
    int ret = 0;
    // int ret = cambus_readb(sensor->slv_addr, BANK_SEL, &reg);
    // ret |= cambus_writeb(sensor->slv_addr, BANK_SEL, reg | BANK_SEL_SENSOR);
    // ret |= cambus_readb(sensor->slv_addr, COM8, &reg);

    // // DISABLED
    // // if (reg & COM8_AGC_EN) {
    // //     ret |= cambus_writeb(sensor->slv_addr, COM8, reg & (~COM8_AGC_EN));
    // // }
    // // DISABLED

    // ret |= cambus_readb(sensor->slv_addr, GAIN, &gain);

    // // DISABLED
    // // if (reg & COM8_AGC_EN) {
    // //     ret |= cambus_writeb(sensor->slv_addr, COM8, reg | COM8_AGC_EN);
    // // }
    // // DISABLED

    // int hi_gain = 1 << (((gain >> 7) & 1) + ((gain >> 6) & 1) + ((gain >> 5) & 1) + ((gain >> 4) & 1));
    // float lo_gain = 1.0 + (((gain >> 0) & 0xF) / 16.0);
    // *gain_db = 20.0 * (fast_log(hi_gain * lo_gain) / fast_log(10.0));

    return ret;
}

static int set_auto_exposure(sensor_t *sensor, int enable, int exposure_us)
{
    uint8_t reg;
    int ret = 0;
    // int ret = cambus_readb(sensor->slv_addr, BANK_SEL, &reg);
    // ret |= cambus_writeb(sensor->slv_addr, BANK_SEL, reg | BANK_SEL_SENSOR);
    // ret |= cambus_readb(sensor->slv_addr, COM8, &reg);
    // ret |= cambus_writeb(sensor->slv_addr, COM8, COM8_SET_AEC(reg, (enable != 0)));

    // if ((enable == 0) && (exposure_us >= 0)) {
    //     ret |= cambus_readb(sensor->slv_addr, COM7, &reg);
    //     int t_line = 0;

    //     if (COM7_GET_RES(reg) == COM7_RES_UXGA) t_line = 1600 + 322;
    //     if (COM7_GET_RES(reg) == COM7_RES_SVGA) t_line = 800 + 390;
    //     if (COM7_GET_RES(reg) == COM7_RES_CIF) t_line = 400 + 195;

    //     ret |= cambus_readb(sensor->slv_addr, CLKRC, &reg);
    //     int pll_mult = (reg & CLKRC_DOUBLE) ? 2 : 1;
    //     int clk_rc = ((reg & CLKRC_DIVIDER_MASK) + 1) * 2;

    //     ret |= cambus_readb(sensor->slv_addr, BANK_SEL, &reg);
    //     ret |= cambus_writeb(sensor->slv_addr, BANK_SEL, reg & (~BANK_SEL_SENSOR));
    //     ret |= cambus_readb(sensor->slv_addr, IMAGE_MODE, &reg);
    //     int t_pclk = 0;

    //     if (IMAGE_MODE_GET_FMT(reg) == IMAGE_MODE_YUV422) t_pclk = 2;
    //     if (IMAGE_MODE_GET_FMT(reg) == IMAGE_MODE_RAW10) t_pclk = 1;
    //     if (IMAGE_MODE_GET_FMT(reg) == IMAGE_MODE_RGB565) t_pclk = 2;

    //     int exposure = IM_MAX(IM_MIN(((exposure_us*(((OMV_XCLK_FREQUENCY/clk_rc)*pll_mult)/1000000))/t_pclk)/t_line,0xFFFF),0x0000);

    //     ret |= cambus_readb(sensor->slv_addr, BANK_SEL, &reg);
    //     ret |= cambus_writeb(sensor->slv_addr, BANK_SEL, reg | BANK_SEL_SENSOR);

    //     ret |= cambus_readb(sensor->slv_addr, REG04, &reg);
    //     ret |= cambus_writeb(sensor->slv_addr, REG04, (reg & 0xFC) | ((exposure >> 0) & 0x3));

    //     ret |= cambus_readb(sensor->slv_addr, AEC, &reg);
    //     ret |= cambus_writeb(sensor->slv_addr, AEC, (reg & 0x00) | ((exposure >> 2) & 0xFF));

    //     ret |= cambus_readb(sensor->slv_addr, REG04, &reg);
    //     ret |= cambus_writeb(sensor->slv_addr, REG04, (reg & 0xC0) | ((exposure >> 10) & 0x3F));
    // }

    return ret;
}

static int get_exposure_us(sensor_t *sensor, int *exposure_us)
{
    uint8_t reg, aec_10, aec_92, aec_1510;
    int ret = 0;

    // int ret = cambus_readb(sensor->slv_addr, BANK_SEL, &reg);
    // ret |= cambus_writeb(sensor->slv_addr, BANK_SEL, reg | BANK_SEL_SENSOR);
    // ret |= cambus_readb(sensor->slv_addr, COM8, &reg);

    // // DISABLED
    // // if (reg & COM8_AEC_EN) {
    // //     ret |= cambus_writeb(sensor->slv_addr, COM8, reg & (~COM8_AEC_EN));
    // // }
    // // DISABLED

    // ret |= cambus_readb(sensor->slv_addr, REG04, &aec_10);
    // ret |= cambus_readb(sensor->slv_addr, AEC, &aec_92);
    // ret |= cambus_readb(sensor->slv_addr, REG45, &aec_1510);

    // // DISABLED
    // // if (reg & COM8_AEC_EN) {
    // //     ret |= cambus_writeb(sensor->slv_addr, COM8, reg | COM8_AEC_EN);
    // // }
    // // DISABLED

    // ret |= cambus_readb(sensor->slv_addr, COM7, &reg);
    // int t_line = 0;

    // if (COM7_GET_RES(reg) == COM7_RES_UXGA) t_line = 1600 + 322;
    // if (COM7_GET_RES(reg) == COM7_RES_SVGA) t_line = 800 + 390;
    // if (COM7_GET_RES(reg) == COM7_RES_CIF) t_line = 400 + 195;

    // ret |= cambus_readb(sensor->slv_addr, CLKRC, &reg);
    // int pll_mult = (reg & CLKRC_DOUBLE) ? 2 : 1;
    // int clk_rc = ((reg & CLKRC_DIVIDER_MASK) + 1) * 2;

    // ret |= cambus_readb(sensor->slv_addr, BANK_SEL, &reg);
    // ret |= cambus_writeb(sensor->slv_addr, BANK_SEL, reg & (~BANK_SEL_SENSOR));
    // ret |= cambus_readb(sensor->slv_addr, IMAGE_MODE, &reg);
    // int t_pclk = 0;

    // if (IMAGE_MODE_GET_FMT(reg) == IMAGE_MODE_YUV422) t_pclk = 2;
    // if (IMAGE_MODE_GET_FMT(reg) == IMAGE_MODE_RAW10) t_pclk = 1;
    // if (IMAGE_MODE_GET_FMT(reg) == IMAGE_MODE_RGB565) t_pclk = 2;

    // uint16_t exposure = ((aec_1510 & 0x3F) << 10) + ((aec_92 & 0xFF) << 2) + ((aec_10 & 0x3) << 0);
    // *exposure_us = (exposure*t_line*t_pclk)/(((OMV_XCLK_FREQUENCY/clk_rc)*pll_mult)/1000000);

    return ret;
}

static int set_auto_whitebal(sensor_t *sensor, int enable, float r_gain_db, float g_gain_db, float b_gain_db)
{
    uint8_t reg;
    int ret = 0;

    // int ret = cambus_readb(sensor->slv_addr, BANK_SEL, &reg);
    // ret |= cambus_writeb(sensor->slv_addr, BANK_SEL, reg & (~BANK_SEL_SENSOR));
    // ret |= cambus_readb(sensor->slv_addr, CTRL1, &reg);
    // ret |= cambus_writeb(sensor->slv_addr, CTRL1, (reg & (~CTRL1_AWB)) | ((enable != 0) ? CTRL1_AWB : 0));

    // if ((enable == 0) && (!isnanf(r_gain_db)) && (!isnanf(g_gain_db)) && (!isnanf(b_gain_db))
    //                   && (!isinff(r_gain_db)) && (!isinff(g_gain_db)) && (!isinff(b_gain_db))) {
    // }

    return ret;
}

static int get_rgb_gain_db(sensor_t *sensor, float *r_gain_db, float *g_gain_db, float *b_gain_db)
{
    uint8_t reg;
    int ret = 0;

    // int ret = cambus_readb(sensor->slv_addr, BANK_SEL, &reg);
    // ret |= cambus_writeb(sensor->slv_addr, BANK_SEL, reg & (~BANK_SEL_SENSOR));
    // ret |= cambus_readb(sensor->slv_addr, CTRL1, &reg);

    // // DISABLED
    // // if (reg & CTRL1_AWB) {
    // //     ret |= cambus_writeb(sensor->slv_addr, CTRL1, reg & (~CTRL1_AWB));
    // // }
    // // DISABLED

    // // DISABLED
    // // if (reg & CTRL1_AWB) {
    // //     ret |= cambus_writeb(sensor->slv_addr, CTRL1, reg | CTRL1_AWB);
    // // }
    // // DISABLED

    return ret;
}

static int set_hmirror(sensor_t *sensor, int enable)
{
    uint8_t reg;
    int ret = 0;

    // int ret = cambus_readb(sensor->slv_addr, BANK_SEL, &reg);
    // ret |= cambus_writeb(sensor->slv_addr, BANK_SEL, reg | BANK_SEL_SENSOR);
    // ret |= cambus_readb(sensor->slv_addr, REG04, &reg);

    // if (enable) {
    //     reg |= REG04_HFLIP_IMG;
    // } else {
    //     reg &= ~REG04_HFLIP_IMG;
    // }

    // ret |= cambus_writeb(sensor->slv_addr, REG04, reg);

    return ret;
}

static int set_vflip(sensor_t *sensor, int enable)
{
    uint8_t reg;
    int ret = 0;

    // int ret = cambus_readb(sensor->slv_addr, BANK_SEL, &reg);
    // ret |= cambus_writeb(sensor->slv_addr, BANK_SEL, reg | BANK_SEL_SENSOR);
    // ret |= cambus_readb(sensor->slv_addr, REG04, &reg);

    // if (enable) {
    //     reg |= REG04_VFLIP_IMG;
    //     reg |= REG04_VREF_EN;
    // } else {
    //     reg &= ~REG04_VFLIP_IMG;
    //     reg &= ~REG04_VREF_EN;
    // }

    // ret |= cambus_writeb(sensor->slv_addr, REG04, reg);

    return ret;
}


int ov3660_init(sensor_t *sensor)
{
    // Initialize sensor structure.
    sensor->gs_bpp              = 2;
    sensor->reset               = reset;
    sensor->read_reg            = read_reg;
    sensor->write_reg           = write_reg;
    sensor->set_pixformat       = set_pixformat;
    sensor->set_framesize       = set_framesize;
    sensor->set_framerate       = set_framerate;
    sensor->set_contrast        = set_contrast;
    sensor->set_brightness      = set_brightness;
    sensor->set_saturation      = set_saturation;
    sensor->set_gainceiling     = set_gainceiling;
    sensor->set_quality         = set_quality;
    sensor->set_colorbar        = set_colorbar;
    sensor->set_auto_gain       = set_auto_gain;
    sensor->get_gain_db         = get_gain_db;
    sensor->set_auto_exposure   = set_auto_exposure;
    sensor->get_exposure_us     = get_exposure_us;
    sensor->set_auto_whitebal   = set_auto_whitebal;
    sensor->get_rgb_gain_db     = get_rgb_gain_db;
    sensor->set_hmirror         = set_hmirror;
    sensor->set_vflip           = set_vflip;

    // Set sensor flags
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_VSYNC, 0);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_HSYNC, 0);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_PIXCK, 1);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_FSYNC, 0);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_JPEGE, 1);

    return 0;
}

