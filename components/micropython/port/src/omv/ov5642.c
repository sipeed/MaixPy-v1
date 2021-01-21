#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "syslog.h"
#include "cambus.h"
#include "ov5642.h"
#include "ov5642_regs.h"
#include "omv_boardconfig.h"
#include "mphalport.h"
#include "dvp.h"

static int reset(sensor_t *sensor)
{
    int i=0;
    // Write default regsiters
    for (i=0; OV5642_ovga_30fps_regs[i][0]; i++) {
        cambus_writeb(sensor->slv_addr, OV5642_ovga_30fps_regs[i][0], OV5642_ovga_30fps_regs[i][1]);
    }
    mp_hal_delay_ms(100);

    return 0;
}

static int sleep(sensor_t *sensor, int enable)
{
    uint8_t reg;
    if (enable) {
        reg = 0x42;
    } else {
        reg = 0x02;
    }
    // Write back register
    return cambus_writeb(sensor->slv_addr, 0x3008, reg);
}

static int read_reg(sensor_t *sensor, uint16_t reg_addr)
{
    uint8_t reg_data;
    if (cambus_readb(sensor->slv_addr, reg_addr, &reg_data) != 0) {
        return -1;
    }
    return reg_data;
}

static int write_reg(sensor_t *sensor, uint16_t reg_addr, uint16_t reg_data)
{
    return cambus_writeb(sensor->slv_addr, reg_addr, reg_data);
}

static int set_pixformat(sensor_t *sensor, pixformat_t pixformat)
{
    int ret=0;
    switch (pixformat) {
        case PIXFORMAT_RGB565:
            cambus_writeb(sensor->slv_addr, 0x501f, 0x01);//ISP RGB
            cambus_writeb(sensor->slv_addr, 0x5000, 0x2f);//RAW Gamma enable
            cambus_writeb(sensor->slv_addr, 0x5002, 0xf8);//YUV to RGB and Dithering enable
            cambus_writeb(sensor->slv_addr, 0x4300, 0x61);//RGB
            break;
        case PIXFORMAT_YUV422:
        case PIXFORMAT_GRAYSCALE:
            cambus_writeb(sensor->slv_addr, 0x501f, 0x00);//ISP YUV
            cambus_writeb(sensor->slv_addr, 0x5000, 0x6f);//YUV Gamma and RAW Gamma enable
            cambus_writeb(sensor->slv_addr, 0x4300, 0x32);//UYVY...         
            break;
        case PIXFORMAT_BAYER:
            //reg = 0x00;//TODO: fix order
            break;
        default:
            return -1;
    }
    return ret;
}

static int set_framesize(sensor_t *sensor, framesize_t framesize)
{
    int ret=0;
    uint16_t w = resolution[framesize][0];
    uint16_t h = resolution[framesize][1];

    ret |= cambus_writeb(sensor->slv_addr, 0x3808, w>>8);
    ret |= cambus_writeb(sensor->slv_addr, 0x3809,  w);
    ret |= cambus_writeb(sensor->slv_addr, 0x380a, h>>8);
    ret |= cambus_writeb(sensor->slv_addr, 0x380b,  h);
    return ret;
}

static int set_framerate(sensor_t *sensor, framerate_t framerate)
{
    return 0;
}

static int set_contrast(sensor_t *sensor, int level)
{
    return 0;
}

static int set_brightness(sensor_t *sensor, int level)
{
    return 0;
}

static int set_saturation(sensor_t *sensor, int level)
{
    return 0;
}

static int set_gainceiling(sensor_t *sensor, gainceiling_t gainceiling)
{
    return 0;
}

static int set_colorbar(sensor_t *sensor, int enable)
{
    return 0;
}

static int set_auto_gain(sensor_t *sensor, int enable, float gain_db, float gain_db_ceiling)
{
    return 0;
}

static int get_gain_db(sensor_t *sensor, float *gain_db)
{
    return 0;
}

static int set_auto_exposure(sensor_t *sensor, int enable, int exposure_us)
{
    return 0;
}

static int get_exposure_us(sensor_t *sensor, int *exposure_us)
{
    return 0;
}

static int set_auto_whitebal(sensor_t *sensor, int enable, float r_gain_db, float g_gain_db, float b_gain_db)
{
    return 0;
}

static int get_rgb_gain_db(sensor_t *sensor, float *r_gain_db, float *g_gain_db, float *b_gain_db)
{
    return 0;
}

static int set_hmirror(sensor_t *sensor, int enable)
{
    uint8_t reg,reg3621;
    int ret = cambus_readb(sensor->slv_addr, 0x3818, &reg);
    if (enable){
        ret |= cambus_writeb(sensor->slv_addr, 0x3818, reg|0x40);        
    } else {
        ret |= cambus_writeb(sensor->slv_addr, 0x3818, reg&0xbf);    
        ret |= cambus_writeb(sensor->slv_addr, 0x3801, 0x50);
        cambus_readb(sensor->slv_addr, 0x3621, &reg3621);
        ret |= cambus_writeb(sensor->slv_addr, 0x3621, reg3621|0x20);
    }
    return ret;
}

static int set_vflip(sensor_t *sensor, int enable)
{
    uint8_t reg,reg3621;
    int ret = cambus_readb(sensor->slv_addr, 0x3818, &reg);
    if (enable){
        ret |= cambus_writeb(sensor->slv_addr, 0x3818, reg|0x20);
    } else {
        ret |= cambus_writeb(sensor->slv_addr, 0x3818, reg&0xdf);
    }
    return ret;
}

static int set_special_effect(sensor_t *sensor, sde_t sde)
{
    return 0;
}

static int set_lens_correction(sensor_t *sensor, int enable, int radi, int coef)
{
    return 0;
}

int ov5642_init(sensor_t *sensor)
{
    // Initialize sensor structure.
    sensor->gs_bpp              = 1;
    sensor->reset               = reset;
    sensor->sleep               = sleep;
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
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_VSYNC, 0);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_HSYNC, 0);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_PIXCK, 1);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_FSYNC, 1);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_JPEGE, 0);

    return 0;
}
