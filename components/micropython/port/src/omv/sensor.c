/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Sensor abstraction layer.
 *
 */
#include <stdlib.h>
#include <string.h>
#include "mp.h"
#include "cambus.h"
#include "ov2640.h"
#include "sensor.h"
#include "framebuffer.h"
#include "omv_boardconfig.h"
#include "dvp.h"
#include "mphalport.h"
#include "plic.h"
#include "fpioa.h"
#include "syslog.h"
#include "ff.h"
#include "gc0328.h"
#include "ov7740.h"
#include "mphalport.h"
#include "ov3660.h"

extern volatile dvp_t* const dvp;

#define OV_CHIP_ID      (0x0A)
#define OV_CHIP_ID2     (0x0B)
#define ON_CHIP_ID      (0x00)
#define OV_CHIP_ID_16BIT      (0x300A)
#define OV_CHIP_ID2_16BIT     (0x300B)
#define MAX_XFER_SIZE   (0xFFFC*4)
#define systick_sleep mp_hal_delay_ms

sensor_t  sensor     = {0};
volatile static uint8_t g_dvp_finish_flag = 0;


static volatile int line = 0;
uint8_t _line_buf;

const int resolution[][2] = {
    {0,    0   },
    // C/SIF Resolutions
    {88,   72  },    /* QQCIF     */
    {176,  144 },    /* QCIF      */
    {352,  288 },    /* CIF       */
    {88,   60  },    /* QQSIF     */
    {176,  120 },    /* QSIF      */
    {352,  240 },    /* SIF       */
    // VGA Resolutions
    {40,   30  },    /* QQQQVGA   */
    {80,   60  },    /* QQQVGA    */
    {160,  120 },    /* QQVGA     */
    {320,  240 },    /* QVGA      */
    {640,  480 },    /* VGA       */
    {60,   40  },    /* HQQQVGA   */
    {120,  80  },    /* HQQVGA    */
    {240,  160 },    /* HQVGA     */
    // FFT Resolutions
    {64,   32  },    /* 64x32     */
    {64,   64  },    /* 64x64     */
    {128,  64  },    /* 128x64    */
    {128,  128 },    /* 128x128    */
    {240,  240 },    /* 240x240    */
    // Other
    {128,  160 },    /* LCD       */
    {128,  160 },    /* QQVGA2    */
    {720,  480 },    /* WVGA      */
    {752,  480 },    /* WVGA2     */
    {800,  600 },    /* SVGA      */
    {1280, 1024},    /* SXGA      */
    {1600, 1200},    /* UXGA      */
};

void _ndelay(uint32_t ns)
{
    uint32_t i;

    while (ns && ns--)
    {
        for (i = 0; i < 25; i++)
            __asm__ __volatile__("nop");
    }
}

static int sensor_irq(void *ctx)
{
	// sensor_t *sensor = ctx;
	if (dvp_get_interrupt(DVP_STS_FRAME_FINISH)) {	//frame end
		dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
		g_dvp_finish_flag = 1;
	} else {	//frame start
        if(g_dvp_finish_flag == 0)  //only we finish the convert, do transmit again
            dvp_start_convert();	//so we need deal img ontime, or skip one framebefore next
		dvp_clear_interrupt(DVP_STS_FRAME_START);
	}

	return 0;
}


void sensor_init_fb()
{
#if  !defined(OMV_MINIMUM)|| CONFIG_MAIXPY_IDE_SUPPORT
    // Init FB mutex
    mutex_init(&JPEG_FB()->lock);

    // Save fb_enabled flag state
	JPEG_FB()->w=0;
    JPEG_FB()->h=0;
	JPEG_FB()->size=0;
    // Set default quality
    JPEG_FB()->quality = 35;
#endif

    // Clear framebuffers
	MAIN_FB()->x=0;
    MAIN_FB()->y=0;
	MAIN_FB()->w=0;
    MAIN_FB()->h=0;
	MAIN_FB()->u=0;
    MAIN_FB()->v=0;
	MAIN_FB()->bpp=0;
    if(MAIN_FB()->pixels)
        free(MAIN_FB()->pixels);
    if(MAIN_FB()->pix_ai)
        free(MAIN_FB()->pix_ai);
	MAIN_FB()->pixels = NULL;
	MAIN_FB()->pix_ai = NULL;
}

void sensor_init0()
{
    sensor_init_fb();
}


//-------------------------------Monocular--------------------------------------
int sensro_ov_detect(sensor_t* sensor)
{
    int init_ret = 0;
    /* Reset the sensor */
    DCMI_RESET_HIGH();
    mp_hal_delay_ms(10);
    DCMI_RESET_LOW();
    mp_hal_delay_ms(30);

    /* Probe the ov sensor */
    sensor->slv_addr = cambus_scan();
    if (sensor->slv_addr == 0) {
        /* Sensor has been held in reset,
           so the reset line is active low */
        sensor->reset_pol = ACTIVE_LOW;

        /* Pull the sensor out of the reset state,systick_sleep() */
        /* Need set PWDN and RST again for some sensor*/
        DCMI_PWDN_HIGH();
        mp_hal_delay_ms(10);
        DCMI_PWDN_LOW();
        mp_hal_delay_ms(10);
        DCMI_RESET_HIGH();
        mp_hal_delay_ms(30);

        /* Probe again to set the slave addr */
        sensor->slv_addr = cambus_scan();
        if (sensor->slv_addr == 0) {
            sensor->pwdn_pol = ACTIVE_LOW;
            /* Need set PWDN and RST again for some sensor*/
            DCMI_PWDN_HIGH();
            mp_hal_delay_ms(10);
            DCMI_RESET_LOW();
            mp_hal_delay_ms(10);
            DCMI_RESET_HIGH();
            mp_hal_delay_ms(30);

            sensor->slv_addr = cambus_scan();
            if (sensor->slv_addr == 0) {
                sensor->reset_pol = ACTIVE_HIGH;

                /* Need set PWDN and RST again for some sensor*/
                DCMI_PWDN_LOW();
                mp_hal_delay_ms(10);
                DCMI_PWDN_HIGH();
                mp_hal_delay_ms(10);
                DCMI_RESET_LOW();
                mp_hal_delay_ms(30);

                sensor->slv_addr = cambus_scan();
                if(sensor->slv_addr == 0) {
                    //should do something?
                    return -2;
                }
            }
        }
    }

    // Clear sensor chip ID.
    sensor->chip_id = 0;

    // Set default snapshot function.
    sensor->snapshot = sensor_snapshot;
	sensor->flush = sensor_flush;
    if (sensor->slv_addr == LEPTON_ID) {
        sensor->chip_id = LEPTON_ID;
		/*set LEPTON xclk rate*/
		/*lepton_init*/
    } else {
        // Read ON semi sensor ID.
        cambus_readb(sensor->slv_addr, ON_CHIP_ID, (uint8_t*)&sensor->chip_id);
        if (sensor->chip_id == MT9V034_ID) {
			/*set MT9V034 xclk rate*/
			/*mt9v034_init*/
        } else { // Read OV sensor ID.
            uint8_t tmp;
            uint8_t reg_width = cambus_reg_width();
            uint16_t reg_addr, reg_addr2;
            if(reg_width == 8){
                reg_addr = OV_CHIP_ID;
                reg_addr2 = OV_CHIP_ID2;
            }else{
                reg_addr = OV_CHIP_ID_16BIT;
                reg_addr2 = OV_CHIP_ID2_16BIT;
            }
            cambus_readb(sensor->slv_addr, reg_addr, &tmp);
            sensor->chip_id = tmp<<8;
            cambus_readb(sensor->slv_addr, reg_addr2, &tmp);
            sensor->chip_id |= tmp;
            // Initialize sensor struct.
            switch (sensor->chip_id) {
                case OV9650_ID:
					/*ov9650_init*/
                    break;
                case OV2640_ID:
                    mp_printf(&mp_plat_print, "[CyberEye]: find ov2640\n");
                    init_ret = ov2640_init(sensor);
                    break;
                // case OV7725_ID:
				// 	/*ov7725_init*/
                //     printk("find ov7725\r\n");
                //     init_ret = ov7725_init(sensor);
                //     break;
                case OV7740_ID:
                    mp_printf(&mp_plat_print, "[CyberEye]: find ov7740\n");
                    init_ret = ov7740_init(sensor);
                    break;
                case OV3660_ID:
                    mp_printf(&mp_plat_print, "[CyberEye]: find ov3660\n");
                    init_ret = ov3660_init(sensor);
                    break;
                default:
                    // Sensor is not supported.
                    return -3;
            }
        }
    }

    if (init_ret != 0 ) {
        // Sensor init failed.
        return -4;
    }
    return 0;
}

int sensro_gc_detect(sensor_t* sensor, bool pwnd)
{
    if(pwnd)
        DCMI_PWDN_LOW();//enable gc0328 要恢复 normal 工作模式，需将 PWDN pin 接入低电平即可，同时写入初始化寄存器即可
    DCMI_RESET_LOW();//reset gc3028
    mp_hal_delay_ms(10);
    DCMI_RESET_HIGH();
    mp_hal_delay_ms(10);
    uint8_t id = cambus_scan_gc0328();
    if(0 == id)
    {
        return -3;
    }
    else
    {
        // mp_printf(&mp_plat_print, "[CyberEye]: gc0328 id = %x\n",id); 
        sensor->slv_addr = GC0328_ADDR;
        sensor->chip_id = id;
        sensor->snapshot = sensor_snapshot;
	    sensor->flush = sensor_flush;
        gc0328_init(sensor);
    }
    return 0;
}
int sensor_init_dvp(mp_int_t freq)
{
    int init_ret = 0;
	
	fpioa_set_function(47, FUNC_CMOS_PCLK);
	fpioa_set_function(46, FUNC_CMOS_XCLK);
	fpioa_set_function(45, FUNC_CMOS_HREF);
	fpioa_set_function(44, FUNC_CMOS_PWDN);
	fpioa_set_function(43, FUNC_CMOS_VSYNC);
	fpioa_set_function(42, FUNC_CMOS_RST);
    
	// fpioa_set_function(41, FUNC_SCCB_SCLK);
	// fpioa_set_function(40, FUNC_SCCB_SDA);

    // Initialize the camera bus, 8bit reg
    // cambus_init(8, -2, 41, 40, 0, 0);
    cambus_init(8, 2, 41, 40, 0, 0);
	 // Initialize dvp interface
	dvp_set_xclk_rate(freq);

    /* Some sensors have different reset polarities, and we can't know which sensor
    is connected before initializing cambus and probing the sensor, which in turn
    requires pulling the sensor out of the reset state. So we try to probe the
    sensor with both polarities to determine line state. */
    sensor.pwdn_pol = ACTIVE_HIGH;
    sensor.reset_pol = ACTIVE_HIGH;
    DCMI_PWDN_HIGH();
    mp_hal_delay_ms(10);
    DCMI_PWDN_LOW();
    mp_hal_delay_ms(10);

    cambus_set_writeb_delay(10);
    if(0 == sensro_ov_detect(&sensor)){//find ov sensor
        // mp_printf(&mp_plat_print, "[CyberEye]: find ov sensor\n");
    }
    else if(0 == sensro_gc_detect(&sensor, true)){//find gc0328 sensor
        mp_printf(&mp_plat_print, "[CyberEye]: find gc3028\n");
        cambus_set_writeb_delay(2);
    }
    else
    {
        mp_printf(&mp_plat_print, "[CyberEye]: no sensor\n");
        init_ret = -1;
    }
    dvp_set_image_format(DVP_CFG_YUV_FORMAT);
    dvp_enable_burst();
	dvp_disable_auto();
	dvp_set_output_enable(0, 1);	//enable to AI
	dvp_set_output_enable(1, 1);	//enable to lcd
    if(sensor.size_set)
    {
        dvp_set_image_size(MAIN_FB()->w_max, MAIN_FB()->h_max);
        dvp_set_ai_addr((uint32_t)MAIN_FB()->pix_ai, (uint32_t)(MAIN_FB()->pix_ai + MAIN_FB()->w * MAIN_FB()->h), (uint32_t)(MAIN_FB()->pix_ai + MAIN_FB()->w * MAIN_FB()->h * 2));
        dvp_set_display_addr((uint32_t)MAIN_FB()->pixels);
    }

    return init_ret;
}
int sensor_init_irq()
{
	dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
	plic_set_priority(IRQN_DVP_INTERRUPT, 2);
    /* set irq handle */
	plic_irq_register(IRQN_DVP_INTERRUPT, sensor_irq, (void*)&sensor);

	plic_irq_disable(IRQN_DVP_INTERRUPT);
	dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
	dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);

	return 0;
}

int sensor_reset(mp_int_t freq)
{
    sensor.reset_set = false;
	sensor_init_fb();		//init FB
    if (sensor_init_dvp(freq) != 0) { //init pins, scan I2C, do ov2640 init
      return -1;
    }
    // Reset the sesnor state
    sensor.sde         = 0;
    sensor.pixformat   = 0;
    sensor.framesize   = 0;
    sensor.framerate   = 0;
    sensor.gainceiling = 0;
    if(sensor.reset == NULL)
    {
        // mp_printf(&mp_plat_print, "[CyberEye]: sensor reset function is null\n");
        return -1;
    }
    // Call sensor-specific reset function
    if (sensor.reset(&sensor) != 0) {	//rst reg, set default cfg.
        return -1;
    }
    // Disable dvp  IRQ before all cfg done 
    sensor_init_irq();
    mp_hal_delay_ms(20);
    sensor.reset_set = true;
    if(sensor.size_set)
    {
        sensor_run(1);
    }
	// mp_printf(&mp_plat_print, "[CyberEye]: exit sensor_reset\n");
    return 0;
}

void sensor_deinit()
{
    sensor_run(0);
    dvp_set_image_size(0, 0);
    dvp_set_ai_addr(0, 0, 0);
    dvp_set_display_addr(0);
    if(MAIN_FB()->pixels)
        free(MAIN_FB()->pixels);
    if(MAIN_FB()->pix_ai)
        free(MAIN_FB()->pix_ai);
    MAIN_FB()->pixels = NULL;
    MAIN_FB()->pix_ai = NULL;
    MAIN_FB()->w = 0;
    MAIN_FB()->h = 0;
    MAIN_FB()->w_max = 0;
    MAIN_FB()->h_max = 0;
    MAIN_FB()->u = 0;
    MAIN_FB()->v = 0;
    MAIN_FB()->bpp = -1;
    MAIN_FB()->x = 0;
    MAIN_FB()->y = 0;
    memset(&sensor, 0, sizeof(sensor));
}

//-------------------------------Binocular--------------------------------------

int binocular_sensor_scan()
{
    int init_ret = 0;
    //reset both sensor
    mp_hal_delay_ms(10);
    DCMI_PWDN_HIGH();
    DCMI_RESET_LOW();
    mp_hal_delay_ms(10);
    DCMI_RESET_HIGH();
    mp_hal_delay_ms(10);

    DCMI_PWDN_LOW();
    mp_hal_delay_ms(10);
    DCMI_RESET_LOW();
    mp_hal_delay_ms(10);
    DCMI_RESET_HIGH();
    mp_hal_delay_ms(10);

    /* Probe the first sensor */
    DCMI_PWDN_HIGH();
    mp_hal_delay_ms(10);   
    sensor.slv_addr = cambus_scan();
    if (sensor.slv_addr == 0) {
        mp_printf(&mp_plat_print, "[CyberEye]: Can not find sensor first\n");
        /* Sensor has been held in reset,
           so the reset line is active low */
        sensor.reset_pol = ACTIVE_LOW;
        /* Pull the sensor out of the reset state,systick_sleep() */
        DCMI_RESET_HIGH();
        mp_hal_delay_ms(10);
        /* Probe again to set the slave addr */
        sensor.slv_addr = cambus_scan();
        if (sensor.slv_addr == 0) {
            mp_printf(&mp_plat_print, "[CyberEye]: No sensor\n");
            return -1;
        }
    }
            
    /* find  the second sensor */
    DCMI_PWDN_LOW();
    mp_hal_delay_ms(10);
    if(sensor.slv_addr != cambus_scan())
    {
        mp_printf(&mp_plat_print, "[CyberEye]: sensors don't match\n");
        return -2;
    }
    // Clear sensor chip ID.
    sensor.chip_id = 0;

    // Set default snapshot function.
    sensor.snapshot = sensor_snapshot;
	sensor.flush = sensor_flush;
    if (sensor.slv_addr == LEPTON_ID) {
        sensor.chip_id = LEPTON_ID;
		/*set LEPTON xclk rate*/
		/*lepton_init*/
    } else {
        // Read ON semi sensor ID.
        cambus_readb(sensor.slv_addr, ON_CHIP_ID, (uint8_t*)&sensor.chip_id);
        if (sensor.chip_id == MT9V034_ID) {
			/*set MT9V034 xclk rate*/
			/*mt9v034_init*/
        } else { // Read OV sensor ID.
            uint8_t tmp;
            cambus_readb(sensor.slv_addr, OV_CHIP_ID, &tmp);
            sensor.chip_id = tmp<<8;
            cambus_readb(sensor.slv_addr, OV_CHIP_ID2, &tmp);
            sensor.chip_id |= tmp;
            // Initialize sensor struct.
            switch (sensor.chip_id) {
                case OV9650_ID:
					/*ov9650_init*/
                    break;
                case OV2640_ID:
                    mp_printf(&mp_plat_print, "[CyberEye]: find ov2640\n");
                    init_ret = ov2640_init(&sensor);
                    break;
                case OV7740_ID:
                    mp_printf(&mp_plat_print, "[CyberEye]: find ov7740\n");
                    init_ret = ov7740_init(&sensor);
                    break;
                case OV7725_ID:
					/*ov7725_init*/
                default:
                    // Sensor is not supported.
                    return -3;
            }
        }
    }

    if (init_ret != 0 ) {
        // Sensor init failed.
        return -4;
    }
    return 0;
}

int binocular_sensor_reset(mp_int_t freq)
{
	sensor_init_fb();		//init FB
    fpioa_set_function(47, FUNC_CMOS_PCLK);
	fpioa_set_function(46, FUNC_CMOS_XCLK);
	fpioa_set_function(45, FUNC_CMOS_HREF);
	fpioa_set_function(44, FUNC_CMOS_PWDN);
	fpioa_set_function(43, FUNC_CMOS_VSYNC);
	fpioa_set_function(42, FUNC_CMOS_RST);

    /* Do a power cycle */
    DCMI_PWDN_HIGH();
    mp_hal_delay_ms(10);

    DCMI_PWDN_LOW();
    mp_hal_delay_ms(10);

    // Initialize the camera bus, 8bit reg
    cambus_init(8, 2, 41, 40, 0, 0);
	 // Initialize dvp interface
	dvp_set_xclk_rate(freq);

    if(0 == binocular_sensor_scan()){//scan I2C, do ov2640 init
    }
    else{
        DCMI_PWDN_HIGH();
        if(0 == sensro_gc_detect(&sensor, false)){//find gc0328 sensor
            mp_printf(&mp_plat_print, "[CyberEye]: sensor1 find gc3028\n");
            cambus_set_writeb_delay(2);
        }
        else{
            mp_printf(&mp_plat_print, "[CyberEye]: scan sensor1 error\n");
            return -1;
        }
        DCMI_PWDN_LOW();
        if(0 == sensro_gc_detect(&sensor, false)){//find gc0328 sensor
            mp_printf(&mp_plat_print, "[CyberEye]: sensor2 find gc3028\n");
            cambus_set_writeb_delay(2);
        }
        else{
            mp_printf(&mp_plat_print, "[CyberEye]: scan sensor2 error\n");
            return -1;
        }
    }
    
    dvp_set_image_format(DVP_CFG_YUV_FORMAT);

	dvp_enable_burst();
	dvp_disable_auto();
	dvp_set_output_enable(0, 1);	//enable to AI
	dvp_set_output_enable(1, 1);	//enable to lcd
    if(sensor.size_set)
    {
        dvp_set_image_size(MAIN_FB()->w_max, MAIN_FB()->h_max);
        dvp_set_ai_addr((uint32_t)MAIN_FB()->pix_ai, (uint32_t)(MAIN_FB()->pix_ai + MAIN_FB()->w * MAIN_FB()->h), (uint32_t)(MAIN_FB()->pix_ai + MAIN_FB()->w * MAIN_FB()->h * 2));
        dvp_set_display_addr((uint32_t)(MAIN_FB()->pixels));
    }
    /* Some sensors have different reset polarities, and we can't know which sensor
       is connected before initializing cambus and probing the sensor, which in turn
       requires pulling the sensor out of the reset state. So we try to probe the
       sensor with both polarities to determine line state. */
    sensor.pwdn_pol = ACTIVE_BINOCULAR;
    sensor.reset_pol = ACTIVE_HIGH;


    // Reset the sesnor state
    sensor.sde         = 0;
    sensor.pixformat   = 0;
    sensor.framesize   = 0;
    sensor.framerate   = 0;
    sensor.gainceiling = 0;

    //select first sensor ,  Call sensor-specific reset function
    DCMI_PWDN_HIGH();
    mp_hal_delay_ms(10);
    DCMI_RESET_LOW();
    mp_hal_delay_ms(10);
    DCMI_RESET_HIGH();
    mp_hal_delay_ms(10); 

    if (sensor.reset(&sensor) != 0) {	//rst reg, set default cfg.
        mp_printf(&mp_plat_print, "[CyberEye]: First sensor reset failed\n");
        return -1;
    }

    //select second sensor ,  Call sensor-specific reset function
    DCMI_PWDN_LOW();
    mp_hal_delay_ms(10);
    DCMI_RESET_LOW();
    mp_hal_delay_ms(10);
    DCMI_RESET_HIGH();
    mp_hal_delay_ms(10);

    if (sensor.reset(&sensor) != 0) {	//rst reg, set default cfg.
        mp_printf(&mp_plat_print, "[CyberEye]: Second sensor reset failed\n");
        return -1;
    }

    // Disable dvp  IRQ before all cfg done 
    sensor_init_irq();
    sensor.reset_set = true;
	// mp_printf(&mp_plat_print, "[CyberEye]: exit sensor_reset\n");
    return 0;
}


int sensor_get_id()
{
    return sensor.chip_id;
}

int sensor_sleep(int enable)
{
    if (sensor.sleep == NULL
        || sensor.sleep(&sensor, enable) != 0) {
        // Operation not supported
        return -1;
    }
    return 0;
}


int sensor_shutdown(int enable)
{
    if (enable) {
        DCMI_PWDN_HIGH();
    } else {
        DCMI_PWDN_LOW();
    }

    systick_sleep(10);
    return 0;
}


int sensor_read_reg(uint8_t reg_addr)
{
    if (sensor.read_reg == NULL) {
        // Operation not supported
        return -1;
    }
    return sensor.read_reg(&sensor, reg_addr);
}

int sensor_write_reg(uint8_t reg_addr, uint16_t reg_data)
{
    if (sensor.write_reg == NULL) {
        // Operation not supported
        return -1;
    }
    return sensor.write_reg(&sensor, reg_addr, reg_data);
}

int sensor_set_pixformat(pixformat_t pixformat)
{
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
    //     case PIXFORMAT_BAYER:
    //         break;
        default:
            return -1;
    }
    if (sensor.set_pixformat == NULL
        || sensor.set_pixformat(&sensor, pixformat) != 0) {
        // Operation not supported
        return -1;
    }
    // Set pixel format
    sensor.pixformat = pixformat;
    // Skip the first frame.
    MAIN_FB()->bpp = -1;

    return 0;
}

int sensor_set_framesize(framesize_t framesize)
{
    sensor.size_set = false;

    int w_old = MAIN_FB()->w;
    int h_old = MAIN_FB()->h;
    // Call the sensor specific function
    if (sensor.set_framesize == NULL
        || sensor.set_framesize(&sensor, framesize) != 0) {
        // Operation not supported
        return EIO;
    }
    // Set framebuffer size
    sensor.framesize = framesize;
    // Skip the first frame.
    MAIN_FB()->bpp = -1;
    // Set MAIN FB x, y offset.
    MAIN_FB()->x = 0;
    MAIN_FB()->y = 0;
    // Set MAIN FB width and height.
    MAIN_FB()->w = resolution[framesize][0];
    MAIN_FB()->h = resolution[framesize][1];
    MAIN_FB()->w_max = MAIN_FB()->w;
    MAIN_FB()->h_max = MAIN_FB()->h;
    if(MAIN_FB()->w != w_old || MAIN_FB()->h != h_old)
    {
        if(MAIN_FB()->pixels)
            free(MAIN_FB()->pixels);
        if(MAIN_FB()->pix_ai)
            free(MAIN_FB()->pix_ai);
        MAIN_FB()->pixels = (uint8_t*)malloc( (MAIN_FB()->w * MAIN_FB()->h * OMV_INIT_BPP + 127)/128*128 );
        if(!MAIN_FB()->pixels)
            return ENOMEM;
        MAIN_FB()->pix_ai = (uint8_t*)malloc( (MAIN_FB()->w * MAIN_FB()->h * 3 + 63)/64*64);
        if(!MAIN_FB()->pix_ai)
        {
            free(MAIN_FB()->pixels);
            MAIN_FB()->pixels = NULL;
            return ENOMEM;
        }
    }
    if(sensor.reset_set)
    {
        dvp_set_image_size(MAIN_FB()->w_max, MAIN_FB()->h_max);
        dvp_set_ai_addr((uint32_t)MAIN_FB()->pix_ai, (uint32_t)(MAIN_FB()->pix_ai + MAIN_FB()->w * MAIN_FB()->h), (uint32_t)(MAIN_FB()->pix_ai + MAIN_FB()->w * MAIN_FB()->h * 2));
        dvp_set_display_addr((uint32_t)MAIN_FB()->pixels);
        sensor_run(1);
    }
    // Set MAIN FB backup width and height.
    MAIN_FB()->u = resolution[framesize][0];
    MAIN_FB()->v = resolution[framesize][1];
    sensor.size_set = true;
    return 0;
}

int sensor_set_framerate(framerate_t framerate)
{

    /* call the sensor specific function */
    if (sensor.set_framerate == NULL
        || sensor.set_framerate(&sensor, framerate) != 0) {
        /* operation not supported */
        return -1;
    }

    /* set the frame rate */
    sensor.framerate = framerate;

    return 0;
}

int sensor_set_windowing(int x, int y, int w, int h)
{	//TODO： set camera windows
    MAIN_FB()->x = x;
    MAIN_FB()->y = y;
    MAIN_FB()->w = MAIN_FB()->u = w;
    MAIN_FB()->h = MAIN_FB()->v = h;
	dvp_set_image_size(w, h);	//set QVGA default
	dvp_set_ai_addr((uint32_t)MAIN_FB()->pix_ai, (uint32_t)(MAIN_FB()->pix_ai + MAIN_FB()->w * MAIN_FB()->h), (uint32_t)(MAIN_FB()->pix_ai + MAIN_FB()->w * MAIN_FB()->h * 2));
    return 0;
}

int sensor_set_contrast(int level)
{
    if (sensor.set_contrast != NULL) {
        return sensor.set_contrast(&sensor, level);
    }
    return -1;
}

int sensor_set_brightness(int level)
{
    if (sensor.set_brightness != NULL) {
        return sensor.set_brightness(&sensor, level);
    }
    return -1;
}

int sensor_set_saturation(int level)
{
    if (sensor.set_saturation != NULL) {
        return sensor.set_saturation(&sensor, level);
    }
    return -1;
}

int sensor_set_gainceiling(gainceiling_t gainceiling)
{

    /* call the sensor specific function */
    if (sensor.set_gainceiling == NULL
        || sensor.set_gainceiling(&sensor, gainceiling) != 0) {
        /* operation not supported */
        return -1;
    }

    sensor.gainceiling = gainceiling;
    return 0;
}

int sensor_set_quality(int qs)
{
    /* call the sensor specific function */
    if (sensor.set_quality == NULL
        || sensor.set_quality(&sensor, qs) != 0) {
        /* operation not supported */
        return -1;
    }
    return 0;
}

int sensor_set_colorbar(int enable)
{
    /* call the sensor specific function */
    if (sensor.set_colorbar == NULL
        || sensor.set_colorbar(&sensor, enable) != 0) {
        /* operation not supported */
        return -1;
    }
    return 0;
}

int sensor_set_auto_gain(int enable, float gain_db, float gain_db_ceiling)
{
    /* call the sensor specific function */
    if (sensor.set_auto_gain == NULL
        || sensor.set_auto_gain(&sensor, enable, gain_db, gain_db_ceiling) != 0) {
        /* operation not supported */
        return -1;
    }
    return 0;
}

int sensor_get_gain_db(float *gain_db)
{
    /* call the sensor specific function */
    if (sensor.get_gain_db == NULL
        || sensor.get_gain_db(&sensor, gain_db) != 0) {
        /* operation not supported */
        return -1;
    }
    return 0;
}

int sensor_set_auto_exposure(int enable, int exposure_us)
{
    /* call the sensor specific function */
    if (sensor.set_auto_exposure == NULL
        || sensor.set_auto_exposure(&sensor, enable, exposure_us) != 0) {
        /* operation not supported */
        return -1;
    }
    return 0;
}

int sensor_get_exposure_us(int *exposure_us)
{
    /* call the sensor specific function */
    if (sensor.get_exposure_us == NULL
        || sensor.get_exposure_us(&sensor, exposure_us) != 0) {
        /* operation not supported */
        return -1;
    }
    return 0;
}

int sensor_set_auto_whitebal(int enable, float r_gain_db, float g_gain_db, float b_gain_db)
{
    /* call the sensor specific function */
    if (sensor.set_auto_whitebal == NULL
        || sensor.set_auto_whitebal(&sensor, enable, r_gain_db, g_gain_db, b_gain_db) != 0) {
        /* operation not supported */
        return -1;
    }
    return 0;
}

int sensor_get_rgb_gain_db(float *r_gain_db, float *g_gain_db, float *b_gain_db)
{
    /* call the sensor specific function */
    if (sensor.get_rgb_gain_db == NULL
        || sensor.get_rgb_gain_db(&sensor, r_gain_db, g_gain_db, b_gain_db) != 0) {
        /* operation not supported */
        return -1;
    }
    return 0;
}

int sensor_set_hmirror(int enable)
{
    /* call the sensor specific function */
    if (sensor.set_hmirror == NULL
        || sensor.set_hmirror(&sensor, enable) != 0) {
        /* operation not supported */
        return -1;
    }
    return 0;
}

int sensor_set_vflip(int enable)
{
    /* call the sensor specific function */
    if (sensor.set_vflip == NULL
        || sensor.set_vflip(&sensor, enable) != 0) {
        /* operation not supported */
        return -1;
    }
    return 0;
}

int sensor_set_special_effect(sde_t sde)
{

    /* call the sensor specific function */
    if (sensor.set_special_effect == NULL
        || sensor.set_special_effect(&sensor, sde) != 0) {
        /* operation not supported */
        return -1;
    }

    sensor.sde = sde;
    return 0;
}

int sensor_set_lens_correction(int enable, int radi, int coef)
{
    /* call the sensor specific function */
    if (sensor.set_lens_correction == NULL
        || sensor.set_lens_correction(&sensor, enable, radi, coef) != 0) {
        /* operation not supported */
        return -1;
    }

    return 0;
}

/*
int sensor_set_vsync_output(GPIO_TypeDef *gpio, uint32_t pin)
{
    sensor.vsync_pin  = pin;
    sensor.vsync_gpio = gpio;
    // Enable VSYNC EXTI IRQ
    NVIC_SetPriority(DCMI_VSYNC_IRQN, IRQ_PRI_EXTINT);
    HAL_NVIC_EnableIRQ(DCMI_VSYNC_IRQN);
    return 0;
}
*/

int sensor_run(int enable)
{
	if(enable)
	{
		dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
		plic_irq_enable(IRQN_DVP_INTERRUPT);
		dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);
	}
	else{
		plic_irq_disable(IRQN_DVP_INTERRUPT);
		dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
		dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);
	}
    return 0;
}

static void sensor_check_buffsize()
{
    int bpp=0;
    switch (sensor.pixformat) {
        case PIXFORMAT_BAYER:
        case PIXFORMAT_GRAYSCALE:
            bpp = 1;
            break;
        case PIXFORMAT_YUV422:
        case PIXFORMAT_RGB565:
            bpp = 2;
            break;
        default:
            break;
    }

    if ((MAIN_FB()->w * MAIN_FB()->h * bpp) > (MAIN_FB()->w_max * MAIN_FB()->h_max * OMV_INIT_BPP)) {
		mp_printf(&mp_plat_print, "%s: Image size too big to fit into buf!\n", __func__);
        if (sensor.pixformat == PIXFORMAT_GRAYSCALE) {
            // Crop higher GS resolutions to QVGA
            sensor_set_windowing(190, 120, 320, 240);
        } else if (sensor.pixformat == PIXFORMAT_RGB565) {
            // Switch to BAYER if the frame is too big to fit in RAM.
            sensor_set_pixformat(PIXFORMAT_BAYER);
        }
    }

}

int exchang_data_byte(uint8_t* addr,uint32_t length)
{
  if(NULL == addr)
    return -1;
  uint8_t data = 0;
  for(int i = 0 ; i < length ;i = i + 2)
  {
    data = addr[i];
    addr[i] = addr[i + 1];
    addr[i + 1] = data;
  }
  return 0;
}
int exchang_pixel(uint16_t* addr,uint32_t resoltion)
{
  if(NULL == addr)
    return -1;
  uint16_t data = 0;
  for(int i = 0 ; i < resoltion ;i = i + 2)
  {
    data = addr[i];
    addr[i] = addr[i + 1];
    addr[i + 1] = data;
  }
  return 0;
}

typedef int (*dual_func_t)(int);
extern volatile dual_func_t dual_func;
static uint32_t* g_pixs = NULL;
static uint32_t g_pixs_size = 0;

static int reverse_u32pixel_2(int core)
{
    uint32_t data;
    uint32_t* pend = g_pixs+g_pixs_size;
    for(;g_pixs<pend;g_pixs++)
    {
        data = *(g_pixs);
        *(g_pixs) = ((data & 0x000000FF) << 24) | ((data & 0x0000FF00) << 8) | 
                ((data & 0x00FF0000) >> 8) | ((data & 0xFF000000) >> 24) ;
    }
    return 0;
}

int reverse_u32pixel(uint32_t* addr,uint32_t length)
{
  if(NULL == addr)
    return -1;

  uint32_t data;
  g_pixs_size = length/2;
  uint32_t* pend = addr+g_pixs_size;
  g_pixs = pend;
  dual_func = reverse_u32pixel_2;
  for(;addr<pend;addr++)
  {
	  data = *(addr);
	  *(addr) = ((data & 0x000000FF) << 24) | ((data & 0x0000FF00) << 8) | 
                ((data & 0x00FF0000) >> 8) | ((data & 0xFF000000) >> 24) ;
  }  //1.7ms
  while(dual_func){}
  
  
  return 0;
}

int sensor_flush(void)
{	//flush old frame, let dvp capture new image
	//use it when you don't snap for a while.
	g_dvp_finish_flag = 0;
    fb_update_jpeg_buffer();
	return 0;
}

int sensor_snapshot(sensor_t *sensor, image_t *image, streaming_cb_t streaming_cb)
{	
    if(!sensor->reset_set || !sensor->size_set)
        return -2;
    bool streaming = (streaming_cb != NULL); // Streaming mode.
	if(image == NULL) return -1;
    // Compress the framebuffer for the IDE preview, only if it's not the first frame,
    // the framebuffer is enabled and the image sensor does not support JPEG encoding.
    // Note: This doesn't run unless the IDE is connected and the framebuffer is enabled.
    fb_update_jpeg_buffer();

    // Make sure the raw frame fits into the FB. If it doesn't it will be cropped if
    // the format is set to GS, otherwise the pixel format will be swicthed to BAYER.
    sensor_check_buffsize();
	
    // The user may have changed the MAIN_FB width or height on the last image so we need
    // to restore that here. We don't have to restore bpp because that's taken care of
    // already in the code below. Note that we do the JPEG compression above first to save
    // the FB of whatever the user set it to and now we restore.
    MAIN_FB()->w = MAIN_FB()->u;
    MAIN_FB()->h = MAIN_FB()->v;

    if (streaming_cb) {
        image->pixels = NULL;
    }

    do {
        if (streaming_cb  && image->pixels != NULL) {  //&& doublebuf
            // Call streaming callback function with previous frame.
            // Note: Image pointer should Not be NULL in streaming mode.
            streaming = streaming_cb(image);
        }
	    // Fix the BPP
        switch (sensor->pixformat) {
            case PIXFORMAT_GRAYSCALE:
                MAIN_FB()->bpp = 1;
                break;
            case PIXFORMAT_YUV422:
            case PIXFORMAT_RGB565:
                MAIN_FB()->bpp = 2;
                break;
            case PIXFORMAT_BAYER:
                MAIN_FB()->bpp = 3;
                break;
            case PIXFORMAT_JPEG:
                // Read the number of data items transferred
                MAIN_FB()->bpp = MAX_XFER_SIZE * 4;
                break;
            default:
                break;
        }
		//
		if(MAIN_FB()->bpp > 3)
		{
			mp_printf(&mp_plat_print, "[CyberEye]: %s | bpp error\n",__func__);
			return -1;
		}

		//wait for new frame
		g_dvp_finish_flag = 0;
        uint32_t start =  systick_current_millis();
		while (g_dvp_finish_flag == 0)
        {
            _ndelay(50);
            if(systick_current_millis() - start > 300)//wait for 30ms
                return -1;
        }
        // Set the user image.
		image->w = MAIN_FB()->w;
		image->h = MAIN_FB()->h;
		image->bpp = MAIN_FB()->bpp;
		image->pix_ai = MAIN_FB()->pix_ai;
		//as data come in is in u32 LE format, we need exchange its order
		//unsigned long t0,t1;
		//t0=read_cycle();
		//exchang_data_byte((image->pixels), (MAIN_FB()->w)*(MAIN_FB()->h)*2);
		//exchang_pixel((image->pixels), (MAIN_FB()->w)*(MAIN_FB()->h)); //cost 3ms@400M
        if(sensor->pixformat == PIXFORMAT_GRAYSCALE)
        {
            image->pixels = image->pix_ai;
        }
        else
        {
            image->pixels = MAIN_FB()->pixels;
		    reverse_u32pixel((uint32_t*)(image->pixels), (MAIN_FB()->w)*(MAIN_FB()->h)/2);
        }
		//t1=read_cycle();
		//mp_printf(&mp_plat_print, "%ld-%ld=%ld, %ld us!\r\n",t1,t0,(t1-t0),((t1-t0)*1000000/400000000)); 
		if (streaming_cb) {
			// In streaming mode, either switch frame buffers in double buffer mode,
			// or call the streaming callback with the main FB in single buffer mode.
				// In single buffer mode, call streaming callback.
				streaming = streaming_cb(image);
		}
    } while (streaming == true);

    return 0;
}
