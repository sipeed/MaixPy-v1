
#include "video.h"
#include "lcd.h"
#include "encoding.h"
#include "sysctl.h"

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
