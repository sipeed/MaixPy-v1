
#include "video.h"
#include "lcd.h"
#include "encoding.h"
#include "sysctl.h"
#include "i2s.h"
#include "stdlib.h"
#include <errno.h>
#include "io.h"
#include "lcd.h"
#include "vfs_internal.h"

extern volatile i2s_t *const i2s[3]; //TODO: remove register, replace with function

int video_hal_display(image_t* img, video_display_roi_t img_roi)
{
	int is_cut;
    uint16_t lcd_width  = lcd->get_width();
    uint16_t lcd_height = lcd->get_height();
    int l_pad = 0;//, r_pad = 0;
    int t_pad = 0;//, b_pad = 0;

    if (img_roi.w > lcd_width) {
        int adjust = img_roi.w - lcd_width;
        img_roi.w -= adjust;
        img_roi.x += adjust / 2;
    } else if (img_roi.w < lcd_width) {
        int adjust = lcd_width - img_roi.w;
        l_pad = adjust / 2;
        // r_pad = (adjust + 1) / 2;
    }
    if (img_roi.h > lcd_height) {
        int adjust = img_roi.h - lcd_height;
        img_roi.h -= adjust;
        img_roi.y += adjust / 2;
    } else if (img_roi.h < lcd_height) {
        int adjust = lcd_height - img_roi.h;
        t_pad = adjust / 2;
        // b_pad = (adjust + 1) / 2;
    }

	is_cut =((img_roi.x != 0) || (img_roi.y != 0) || \
			(img_roi.w != img->w) || (img_roi.h != img->h));
    if(is_cut){	//cut from img
        if (IM_IS_GS(img)) {
            lcd->draw_pic_grayroi(l_pad, t_pad, img->w, img->h, img_roi.x, img_roi.y, img_roi.w, img_roi.h, (uint8_t *)(img->pixels));
        }
        else {
            lcd->draw_pic_roi(l_pad, t_pad, img->w, img->h, img_roi.x, img_roi.y, img_roi.w, img_roi.h, (uint32_t *)(img->pixels));
        }
    }
    else{	//no cut
        if (IM_IS_GS(img)) {
            lcd->draw_pic_gray(l_pad, t_pad, img_roi.w, img_roi.h, (uint8_t *)(img->pixels));
        }
        else {
            lcd->draw_picture(l_pad, t_pad, img_roi.w, img_roi.h, (uint32_t *)(img->pixels));
        }
    }
    return 0;
}

uint64_t inline video_hal_ticks_us(void)
{
    return (uint64_t)(read_csr(mcycle)/(sysctl_clock_get_freq(SYSCTL_CLOCK_CPU)/1000000));
}


static int on_irq_dma4(void *ctx)
{
    avi_t* avi = (avi_t*)ctx;

    avi->audio_buf[avi->index_buf_play].empty = true;
    if(++avi->index_buf_play > 3)
        avi->index_buf_play = 0;
    if( !avi->audio_buf[avi->index_buf_play].empty )
    {
        video_hal_audio_play(avi->audio_buf[avi->index_buf_play].buf, avi->audio_buf[avi->index_buf_play].len, (uint8_t)avi->audio_channels);
    }
    return 0;
}

int video_hal_display_init()
{
    // ! don't init here, init on system start up
    // so we can use `lcd.freq(freq)` to set lcd spi frequency 
    return 0;
}


int video_hal_audio_init(avi_t* avi)
{
    // //TODO:optimize
    // i2s_init(I2S_DEVICE_0, I2S_TRANSMITTER, 0x0C);
    // i2s_tx_channel_config(I2S_DEVICE_0, I2S_CHANNEL_1,
    //     RESOLUTION_16_BIT, SCLK_CYCLES_32,
    //     /*TRIGGER_LEVEL_1*/TRIGGER_LEVEL_4,
    //     RIGHT_JUSTIFYING_MODE
    //     );
    i2s_set_sample_rate(I2S_DEVICE_0, avi->audio_sample_rate);
    dmac_set_irq(DMAC_CHANNEL4, on_irq_dma4, (void*)avi, 1);
    avi->audio_buf[0].buf = (uint8_t*)malloc(avi->audio_buf_size+8);
    if( !avi->audio_buf[0].buf )
        return ENOMEM;
    avi->audio_buf[1].buf = (uint8_t*)malloc(avi->audio_buf_size+8);
    if( !avi->audio_buf[1].buf )
    {
        free(avi->audio_buf[0].buf);
        return ENOMEM;
    }
    avi->audio_buf[2].buf = (uint8_t*)malloc(avi->audio_buf_size+8);
    if( !avi->audio_buf[2].buf )
    {
        free(avi->audio_buf[0].buf);
        free(avi->audio_buf[1].buf);
        return ENOMEM;
    }
    avi->audio_buf[3].buf = (uint8_t*)malloc(avi->audio_buf_size+8);
    if( !avi->audio_buf[3].buf )
    {
        free(avi->audio_buf[0].buf);
        free(avi->audio_buf[1].buf);
        free(avi->audio_buf[2].buf);
        return ENOMEM;
    }
    avi->index_buf_save = 0;
    avi->index_buf_play = 0;
    avi->audio_buf[0].len = 0;
    avi->audio_buf[1].len = 0;
    avi->audio_buf[2].len = 0;
    avi->audio_buf[3].len = 0;
    avi->audio_buf[0].empty = true;
    avi->audio_buf[1].empty = true;
    avi->audio_buf[2].empty = true;
    avi->audio_buf[3].empty = true;
    return 0;
}

int video_hal_audio_deinit(avi_t* avi)
{
    if(avi->audio_buf[0].buf)
        free(avi->audio_buf[0].buf);
    if(avi->audio_buf[1].buf)
        free(avi->audio_buf[1].buf);
    if(avi->audio_buf[2].buf)
        free(avi->audio_buf[2].buf);
    if(avi->audio_buf[3].buf)
        free(avi->audio_buf[3].buf);
    avi->audio_buf[0].buf = NULL;
    avi->audio_buf[1].buf = NULL;
    avi->audio_buf[2].buf = NULL;
    avi->audio_buf[3].buf = NULL;
    //TODO: replace register version with function
    ier_t u_ier;
    u_ier.reg_data = readl(&i2s[I2S_DEVICE_0]->ier);
    u_ier.ier.ien = 0;
    writel(u_ier.reg_data, &i2s[I2S_DEVICE_0]->ier);
    ccr_t u_ccr;
    u_ccr.reg_data = readl(&i2s[I2S_DEVICE_0]->ccr);
    u_ccr.ccr.dma_tx_en = 0;
    writel(u_ccr.reg_data, &i2s[I2S_DEVICE_0]->ccr);
    return 0;
}

int video_hal_audio_play(uint8_t* data, uint32_t len, uint8_t channels)
{
    i2s_play(I2S_DEVICE_0, DMAC_CHANNEL4, data, len, 1024, 16, channels);
    return 0;
}

    
/**
 * 
 * @return <0 if error, 0 if success
 */
int video_hal_file_open(avi_t* avi, const char* path, bool write)
{
    int err;

    if( avi->file )
        return -EPERM;
    if(write)
        avi->file = (void*)vfs_internal_open(path, "wb", &err);
    else
        avi->file = (void*)vfs_internal_open(path, "rb", &err);
    if( err!=0 )
        return -err;
    return 0;
}


int video_hal_file_write(avi_t* avi, uint8_t* data, uint32_t len)
{
    int err, ret;

    if( !avi->file )
        return -EPERM;
    ret = vfs_internal_write( (mp_obj_t)avi->file, data, len, &err );
    if( err<0 )
        return -err;
    return ret;
}

int video_hal_file_read(avi_t* avi, uint8_t* data, uint32_t len)
{
    int err;

    if( !avi->file )
        return -EPERM;
    vfs_internal_read( (mp_obj_t)avi->file, data, len, &err );
    if( err!=0 )
        return -err;
    return 0;
}

int video_hal_file_close(avi_t* avi)
{
    int err;

    if( !avi->file )
        return -EPERM;
    vfs_internal_close( (mp_obj_t)avi->file, &err );
    avi->file = NULL;
    if( err!=0 )
        return -err;
    return 0;
}

int video_hal_file_seek(avi_t* avi, long offset, uint8_t whence)
{
    int err;

    if( !avi->file )
        return -EPERM;
    vfs_internal_seek( (mp_obj_t)avi->file, (mp_int_t)offset, whence, &err );
    if( err!=0 )
        return -err;
    return 0;
}

int video_hal_file_size(avi_t* avi)
{
    return vfs_internal_size((mp_obj_t)avi->file);
}

/**
 * 
 * @return mjpeg file size, <0 if error occurred
 */
int video_hal_image_encode_mjpeg(avi_t* avi, image_t* img)
{
    uint64_t size;

    fb_alloc_mark();
    uint8_t *buffer = fb_alloc_all(&size);
    image_t out = { .w=img->w, .h=img->h, .bpp=size - 8, .pixels=buffer };
    // When jpeg_compress needs more memory than in currently allocated it
    // will try to realloc. MP will detect that the pointer is outside of
    // the heap and return NULL which will cause an out of memory error.
    jpeg_compress(img, &out, avi->mjpeg_quality, false);
    if(out.bpp%8)//align
    {
        out.bpp += (8 - out.bpp%8);
    }
    int ret = video_hal_file_write(avi, out.pixels, out.bpp);
    fb_free();
    fb_alloc_free_till_mark();
    if(ret < 0)
        return ret;
    if( ret != out.bpp)
        return EIO;
    return ret;
}

uint8_t* video_hal_malloc(uint32_t size)
{
    return (uint8_t*)malloc(size);
}

void video_hal_free(uint8_t* ptr)
{
    free(ptr);
}


