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
extern volatile dvp_t* const dvp;

#define OV_CHIP_ID      (0x0A)
#define ON_CHIP_ID      (0x00)
#define MAX_XFER_SIZE   (0xFFFC*4)
#define systick_sleep mp_hal_delay_ms

sensor_t  sensor     = {0};

static volatile int line = 0;
uint8_t _line_buf;
static uint8_t *dest_fb = NULL;

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
    {128,  128 },    /* 128x64    */
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
	sensor_t *sensor = ctx;
	if (dvp_get_interrupt(DVP_STS_FRAME_FINISH)) {
		dvp_clear_interrupt(DVP_STS_FRAME_FINISH);
		sensor->image_buf.buf_used[sensor->image_buf.buf_sel] = 1;
		sensor->image_buf.buf_sel ^= 0x01;
		dvp_set_display_addr((uint32_t)sensor->image_buf.addr[sensor->image_buf.buf_sel]);	
	} else {
		dvp_clear_interrupt(DVP_STS_FRAME_START);
		if (sensor->image_buf.buf_used[sensor->image_buf.buf_sel] == 0)
		{
			printk("sensor_irq convert\r\n");
			dvp_start_convert();
		}
	}

	return 0;
}


void sensor_init0()
{
    // Init FB mutex
    mutex_init(&JPEG_FB()->lock);

    // Save fb_enabled flag state
    int fb_enabled = JPEG_FB()->enabled;

    // Clear framebuffers
    memset(MAIN_FB(), 0, sizeof(framebuffer_t));
    memset(JPEG_FB(), 0, sizeof(jpegbuffer_t) + OMV_JPEG_BUF_SIZE);

    // Set default quality
    JPEG_FB()->quality = 35;

    // Set fb_enabled
    JPEG_FB()->enabled = fb_enabled;
}

int sensor_init1()
{
    int init_ret = 0;
	
	fpioa_set_function(47, FUNC_CMOS_PCLK);
	fpioa_set_function(46, FUNC_CMOS_XCLK);
	fpioa_set_function(45, FUNC_CMOS_HREF);
	fpioa_set_function(44, FUNC_CMOS_PWDN);
	fpioa_set_function(43, FUNC_CMOS_VSYNC);
	fpioa_set_function(42, FUNC_CMOS_RST);
	fpioa_set_function(41, FUNC_SCCB_SCLK);
	fpioa_set_function(40, FUNC_SCCB_SDA);

    /* Do a power cycle */
    DCMI_PWDN_HIGH();
    mp_hal_ticks_ms(10);

    DCMI_PWDN_LOW();
    mp_hal_ticks_ms(10);

    // Initialize the camera bus.
    cambus_init(8);
	 // Initialize dvp interface
	dvp_set_xclk_rate(24000000);
	dvp_enable_burst();
	dvp_disable_auto();
	dvp_set_output_enable(0, 1);
	dvp_set_output_enable(1, 1);
	dvp_set_image_format(DVP_CFG_RGB_FORMAT);
	dvp_set_image_size(320, 240);

    /* Some sensors have different reset polarities, and we can't know which sensor
       is connected before initializing cambus and probing the sensor, which in turn
       requires pulling the sensor out of the reset state. So we try to probe the
       sensor with both polarities to determine line state. */
    sensor.pwdn_pol = ACTIVE_HIGH;
    sensor.reset_pol = ACTIVE_HIGH;

    /* Reset the sensor */
    DCMI_RESET_HIGH();
    mp_hal_ticks_ms(10);

    DCMI_RESET_LOW();
    mp_hal_ticks_ms(10);

    /* Probe the sensor */
    sensor.slv_addr = cambus_scan();
    if (sensor.slv_addr == 0) {
        /* Sensor has been held in reset,
           so the reset line is active low */
        sensor.reset_pol = ACTIVE_LOW;

        /* Pull the sensor out of the reset state,systick_sleep() */
        DCMI_RESET_HIGH();
        mp_hal_delay_ms(10);

        /* Probe again to set the slave addr */
        sensor.slv_addr = cambus_scan();
        if (sensor.slv_addr == 0) {
            sensor.pwdn_pol = ACTIVE_LOW;

            DCMI_PWDN_HIGH();
            mp_hal_delay_ms(10);

            sensor.slv_addr = cambus_scan();
            if (sensor.slv_addr == 0) {
                sensor.reset_pol = ACTIVE_HIGH;

                DCMI_RESET_LOW();
                mp_hal_delay_ms(10);

                sensor.slv_addr = cambus_scan();
                if (sensor.slv_addr == 0) {
                    return -2;
                }
            }
        }
    }

    // Clear sensor chip ID.
    sensor.chip_id = 0;

    // Set default snapshot function.
    sensor.snapshot = sensor_snapshot;
    if (sensor.slv_addr == LEPTON_ID) {
        sensor.chip_id = LEPTON_ID;
		/*set LEPTON xclk rate*/
		/*lepton_init*/
    } else {
        // Read ON semi sensor ID.
        cambus_readb(sensor.slv_addr, ON_CHIP_ID, &sensor.chip_id);
        if (sensor.chip_id == MT9V034_ID) {
			/*set MT9V034 xclk rate*/
			/*mt9v034_init*/
        } else { // Read OV sensor ID.
            cambus_readb(sensor.slv_addr, OV_CHIP_ID, &sensor.chip_id);
            // Initialize sensor struct.
            switch (sensor.chip_id) {
                case OV9650_ID:
					/*ov9650_init*/
                    break;
                case OV2640_ID:
                    init_ret = ov2640_init(&sensor);
                    break;
                case OV7725_ID:
					/*ov7725_init*/
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
	
    // Clear fb_enabled flag
    // This is executed only once to initialize the FB enabled flag.
    JPEG_FB()->enabled = 0;

    /* All good! */
	printf("exit sensor_init\n");
    return 0;
}
int sensor_init2()
{
	
	void* ptr = NULL;
	if(ptr == NULL)
	{
		ptr = malloc(sizeof(uint8_t) * 320 * 240 * (2 * 2) + 127);
	}
	MAIN_FB()->pixels = malloc( 320 * 240 * 2);
	sensor.image_buf.addr[0] = (uint32_t *)(((uint32_t)ptr + 127) & 0xFFFFFF80);
	sensor.image_buf.addr[1] = (uint32_t *)((uint32_t)sensor.image_buf.addr[0] + 320 * 240 * 2);
	sensor.image_buf.buf_used[0] = 0;
	sensor.image_buf.buf_used[1] = 0;
	sensor.image_buf.buf_sel = 0;
	
	// Disable IRQ
	dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
	dvp_set_display_addr((uint32_t)sensor.image_buf.addr[sensor.image_buf.buf_sel]);
	plic_set_priority(IRQN_DVP_INTERRUPT, 2);
    /* set irq handle */
	plic_irq_register(IRQN_DVP_INTERRUPT, sensor_irq, (void*)&sensor);
	
	plic_irq_disable(IRQN_DVP_INTERRUPT);
	//plic_irq_disable(IRQN_DVP_INTERRUPT);

	dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
	dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);
	
}

int sensor_reset()
{
	sensor_init0();
	sensor_init1();
    // Reset the sesnor state
    sensor.sde         = 0;
    sensor.pixformat   = 0;
    sensor.framesize   = 0;
    sensor.framerate   = 0;
    sensor.gainceiling = 0;
    // Call sensor-specific reset function
    if (sensor.reset(&sensor) != 0) {
        return -1;
    }

    // Disable dvp  IRQ
    sensor_init2();

	printf("exit sensor_reset\n");
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

    if (sensor.pixformat == pixformat) {
        // No change
        return 0;
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
    if (sensor.framesize == framesize) {
        // No change
        return 0;
    }

    // Call the sensor specific function
    if (sensor.set_framesize == NULL
        || sensor.set_framesize(&sensor, framesize) != 0) {
        // Operation not supported
        return -1;
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

    // Set MAIN FB backup width and height.
    MAIN_FB()->u = resolution[framesize][0];
    MAIN_FB()->v = resolution[framesize][1];
    return 0;
}

int sensor_set_framerate(framerate_t framerate)
{
    if (sensor.framerate == framerate) {
       /* no change */
        return 0;
    }

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
{
    MAIN_FB()->x = x;
    MAIN_FB()->y = y;
    MAIN_FB()->w = MAIN_FB()->u = w;
    MAIN_FB()->h = MAIN_FB()->v = h;
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
    if (sensor.gainceiling == gainceiling) {
        /* no change */
        return 0;
    }

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
    if (sensor.sde == sde) {
        /* no change */
        return 0;
    }

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

int sensor_set_vsync_output()
{
	dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
	plic_irq_enable(IRQN_DVP_INTERRUPT);
	dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);
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

    if ((MAIN_FB()->w * MAIN_FB()->h * bpp) > OMV_RAW_BUF_SIZE) {
        if (sensor.pixformat == PIXFORMAT_GRAYSCALE) {
            // Crop higher GS resolutions to QVGA
            sensor_set_windowing(190, 120, 320, 240);
        } else if (sensor.pixformat == PIXFORMAT_RGB565) {
            // Switch to BAYER if the frame is too big to fit in RAM.
            sensor_set_pixformat(PIXFORMAT_BAYER);
        }
    }

}

// This function is called back after each line transfer is complete,
// with a pointer to the line buffer that was used. At this point the
// DMA transfers the next line to the other half of the line buffer.
// Note:  For JPEG this function is called once (and ignored) at the end of the transfer.

void Image_CpltUser(uint32_t addr)
{
    uint8_t *src = (uint8_t*) addr;
    uint8_t *dst = dest_fb;

    uint16_t *src16 = (uint16_t*) addr;
    uint16_t *dst16 = (uint16_t*) dest_fb;

    // Skip lines outside the window.
    if (line >= MAIN_FB()->y && line <= (MAIN_FB()->y + MAIN_FB()->h)) {
        switch (sensor.pixformat) {
            case PIXFORMAT_BAYER:
                dst += (line - MAIN_FB()->y) * MAIN_FB()->w;
                for (int i=0; i<MAIN_FB()->w; i++) {
                    dst[i] = src[MAIN_FB()->x + i];
                }
                break;
            case PIXFORMAT_GRAYSCALE:
                dst += (line - MAIN_FB()->y) * MAIN_FB()->w;
                if (sensor.gs_bpp == 1) {
                    // 1BPP GRAYSCALE.
                    for (int i=0; i<MAIN_FB()->w; i++) {
                        dst[i] = src[MAIN_FB()->x + i];
                    }
                } else {
                    // Extract Y channel from YUV.
                    for (int i=0; i<MAIN_FB()->w; i++) {
                        dst[i] = src[MAIN_FB()->x * 2 + i * 2];
                    }
                }
                break;
            case PIXFORMAT_YUV422:
            case PIXFORMAT_RGB565:
                dst16 += (line - MAIN_FB()->y) * MAIN_FB()->w;
                for (int i=0; i<MAIN_FB()->w; i++) {
                    dst16[i] = src16[MAIN_FB()->x + i];
                }
                break;
            case PIXFORMAT_JPEG:
                break;
            default:
                break;
        }
    }

    line++;
}

// This is the default snapshot function, which can be replaced in sensor_init functions. This function
// uses the DCMI and DMA to capture frames and each line is processed in the DCMI_DMAConvCpltUser function.
int sensor_snapshot(sensor_t *sensor, image_t *image, streaming_cb_t streaming_cb)
{
	printf("[MaixPy] %s | test\n",__func__);
    uint32_t frame = 0;
    bool streaming = (streaming_cb != NULL); // Streaming mode.
    bool doublebuf = false; // Use double buffers in streaming mode.
    uint32_t addr, length;
	uint64_t tick_start;
    // Compress the framebuffer for the IDE preview, only if it's not the first frame,
    // the framebuffer is enabled and the image sensor does not support JPEG encoding.
    // Note: This doesn't run unless the IDE is connected and the framebuffer is enabled.
    //fb_update_jpeg_buffer();

    // Make sure the raw frame fits into the FB. If it doesn't it will be cropped if
    // the format is set to GS, otherwise the pixel format will be swicthed to BAYER.
    sensor_check_buffsize();

    // Set the current frame buffer target used in the DMA line callback
    // (DCMI_DMAConvCpltUser function), in both snapshot and streaming modes.
    dest_fb = MAIN_FB()->pixels;

    // The user may have changed the MAIN_FB width or height on the last image so we need
    // to restore that here. We don't have to restore bpp because that's taken care of
    // already in the code below. Note that we do the JPEG compression above first to save
    // the FB of whatever the user set it to and now we restore.
    MAIN_FB()->w = MAIN_FB()->u;
    MAIN_FB()->h = MAIN_FB()->v;

    // We use the stored frame size to read the whole frame. Note that cropping is
    // done in the line function using the diemensions stored in MAIN_FB()->x,y,w,h.
    uint32_t w = resolution[sensor->framesize][0];
    uint32_t h = resolution[sensor->framesize][1];

    // Setup the size and address of the transfer
    switch (sensor->pixformat) {
        case PIXFORMAT_RGB565:
        case PIXFORMAT_YUV422:
            // RGB/YUV read 2 bytes per pixel.
            length = (w * h * 2);
            addr = (uint32_t) &_line_buf;
            break;
        case PIXFORMAT_BAYER:
            // BAYER/RAW: 1 byte per pixel
            length = (w * h * 1);
            addr = (uint32_t) &_line_buf;
            break;
        case PIXFORMAT_GRAYSCALE:
            // 1/2BPP Grayscale.
            length = (w * h * sensor->gs_bpp);
            addr = (uint32_t) &_line_buf;
            break;
        case PIXFORMAT_JPEG:
            // Sensor has hardware JPEG set max frame size.
            length = MAX_XFER_SIZE;
            addr = (uint32_t) (MAIN_FB()->pixels);
            break;
        default:
            return -1;
    }

    if (streaming_cb) {
        image->pixels = NULL;
    }

    // If two frames fit in ram, use double buffering in streaming mode.
    doublebuf = ((length*2) <= OMV_RAW_BUF_SIZE);

    do {
        // Clear line counter
        line = 0;
        // Snapshot start tick      
        if (sensor->pixformat == PIXFORMAT_JPEG) {
            // Start a regular transfer
        } else {
            // Start a multibuffer transfer (line by line)
        }

        if (streaming_cb && doublebuf && image->pixels != NULL) {
            // Call streaming callback function with previous frame.
            // Note: Image pointer should Not be NULL in streaming mode.
            streaming = streaming_cb(image);
        }

        // exchange buffer
//        plic_irq_disable(IRQN_DVP_INTERRUPT);
		
        //get dvp interrupt status
//        sensor->irq_flag = 0;
//		tick_start = mp_hal_ticks_ms();
//        while (sensor->irq_flag == 0) {
//            // Wait for interrupt
//            if ((mp_hal_ticks_ms() - tick_start) >= 3000) {
//                // Sensor timeout, most likely a HW issue.
//                // Abort the DMA request.
//                return -1;
//            }
//        }
//		mp_hal_delay_ms(20);
		
		//Image_CpltUser(NULL);//TODO
		while (sensor->image_buf.buf_used[sensor->image_buf.buf_sel] == 0)
				_ndelay(50);
//		MAIN_FB()->pixels = sensor->image_buf.addr[sensor->image_buf.buf_sel];
		memcpy(MAIN_FB()->pixels, sensor->image_buf.addr[sensor->image_buf.buf_sel], 320*240*2);
		sensor->image_buf.buf_used[sensor->image_buf.buf_sel] = 0;

//		MAIN_FB()->pixels = sensor->image_buf.addr[sensor->image_buf.buf_sel ^ 0x01];
		
        // Abort DMA transfer.
        // Note: In JPEG mode the DMA will still be waiting for data since
        // the max frame size is set, so we need to abort the DMA transfer.
        //...

        // Disable DMA IRQ
        //...

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

        // Set the user image.
        if (image != NULL) {
            image->w = MAIN_FB()->w;
            image->h = MAIN_FB()->h;
            image->bpp = MAIN_FB()->bpp;
            image->pixels = MAIN_FB()->pixels;

            if (streaming_cb) {
                // In streaming mode, either switch frame buffers in double buffer mode,
                // or call the streaming callback with the main FB in single buffer mode.
                if (doublebuf == false) {
                    // In single buffer mode, call streaming callback.
                    streaming = streaming_cb(image);
                } else {
                    // In double buffer mode, switch frame buffers.
                    if (frame == 0) {
                        image->pixels = MAIN_FB()->pixels;
                        // Next frame will be transfered to the second half.
                        dest_fb = MAIN_FB()->pixels + length;
                    } else {
                        image->pixels = MAIN_FB()->pixels + length;
                        // Next frame will be transfered to the first half.
                        dest_fb = MAIN_FB()->pixels;
                    }
                    frame ^= 1; // Switch frame buffers.
                }
            }
        }
    } while (streaming == true);

    return 0;
}

