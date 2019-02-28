
#include "video.h"
#include "lcd.h"
#include "encoding.h"
#include "sysctl.h"
#include "i2s.h"
#include "stdlib.h"
#include <errno.h>
#include "io.h"

extern volatile i2s_t *const i2s[3]; //TODO: remove register, replace with function

int video_hal_display(image_t* img, video_display_roi_t img_roi)
{
	int is_cut;
    uint16_t lcd_width  = LCD_W;
    uint16_t lcd_height = LCD_H;
    int l_pad = 0, r_pad = 0;
    int t_pad = 0, b_pad = 0;

    if (img_roi.w > lcd_width) {
        int adjust = img_roi.w - lcd_width;
        img_roi.w -= adjust;
        img_roi.x += adjust / 2;
    } else if (img_roi.w < lcd_width) {
        int adjust = lcd_width - img_roi.w;
        l_pad = adjust / 2;
        r_pad = (adjust + 1) / 2;
    }
    if (img_roi.h > lcd_height) {
        int adjust = img_roi.h - lcd_height;
        img_roi.h -= adjust;
        img_roi.y += adjust / 2;
    } else if (img_roi.h < lcd_height) {
        int adjust = lcd_height - img_roi.h;
        t_pad = adjust / 2;
        b_pad = (adjust + 1) / 2;
    }

	is_cut =((img_roi.x != 0) || (img_roi.y != 0) || \
			(img_roi.w != img->w) || (img_roi.h != img->h));
    if(is_cut){	//cut from img
        if (IM_IS_GS(img)) {
            lcd_draw_pic_grayroi(l_pad, t_pad, img->w, img->h, img_roi.x, img_roi.y, img_roi.w, img_roi.h, (uint8_t *)(img->pixels));
        }
        else {
            lcd_draw_pic_roi(l_pad, t_pad, img->w, img->h, img_roi.x, img_roi.y, img_roi.w, img_roi.h, (uint32_t *)(img->pixels));
        }
    }
    else{	//no cut
        if (IM_IS_GS(img)) {
            lcd_draw_pic_gray(l_pad, t_pad, img_roi.w, img_roi.h, (uint8_t *)(img->pixels));
        }
        else {
            lcd_draw_picture(l_pad, t_pad, img_roi.w, img_roi.h, (uint32_t *)(img->pixels));
        }
    }
    return 0;
}

uint64_t inline video_hal_ticks_us(void)
{
    return (uint64_t)(read_csr(mcycle)/(sysctl_clock_get_freq(SYSCTL_CLOCK_CPU)/1000000));
}


static int on_irq_dma3(void *ctx)
{
    avi_t* avi = (avi_t*)ctx;

    // printk("play %d ok\r\n",avi->index_buf_play);
    avi->audio_buf[avi->index_buf_play].empty = true;
    if(++avi->index_buf_play > 3)
        avi->index_buf_play = 0;
    if( !avi->audio_buf[avi->index_buf_play].empty )
    {
        // printk("irq play %d\r\n",avi->index_buf_play);
        video_hal_audio_play(avi->audio_buf[avi->index_buf_play].buf, avi->audio_buf[avi->index_buf_play].len);
    }
    return 0;
}



int video_hal_audio_init(avi_t* avi)
{
    //TODO:optimize
    i2s_init(I2S_DEVICE_0, I2S_TRANSMITTER, 0x0C);
    i2s_tx_channel_config(I2S_DEVICE_0, I2S_CHANNEL_1,
        RESOLUTION_16_BIT, SCLK_CYCLES_32,
        /*TRIGGER_LEVEL_1*/TRIGGER_LEVEL_4,
        RIGHT_JUSTIFYING_MODE
        );
    uint32_t ret = i2s_set_sample_rate(I2S_DEVICE_0, avi->audio_sample_rate/2);//TODO: /2 ?

    dmac_set_irq(DMAC_CHANNEL3, on_irq_dma3, (void*)avi, 1);
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
    //TODO: replace register version with function
    ier_t u_ier;
    u_ier.reg_data = readl(&i2s[I2S_DEVICE_0]->ier);
    u_ier.ier.ien = 0;
    writel(u_ier.reg_data, &i2s[I2S_DEVICE_0]->ier);
    ccr_t u_ccr;
    u_ccr.reg_data = readl(&i2s[I2S_DEVICE_0]->ccr);
    u_ccr.ccr.dma_tx_en = 0;
    writel(u_ccr.reg_data, &i2s[I2S_DEVICE_0]->ccr);
}

int video_hal_audio_play(uint8_t* data, uint32_t len)
{
    i2s_play(I2S_DEVICE_0, DMAC_CHANNEL3, data, len, 1024, 16, 2);
    return 0;
}

