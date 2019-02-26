#ifndef __VIDEO_H
#define __VIDEO_H

#include "avi.h"

//////////////////// VIDEO BUFF ///////////////////////
#include "framebuffer.h"
#define   VIDEO_BUFF()  fb_framebuffer->pixels  // we use omv module's framebuff here, size: OMV_INIT_W * OMV_INIT_H * 2
#define   VIDEO_AVI_BUFF_SIZE  1024*60          // must <= OMV_INIT_W * OMV_INIT_H * 2

#include "omv_boardconfig.h"
extern uint8_t g_lcd_buf[OMV_INIT_W * OMV_INIT_H * 2];
#define   IMAGE_BUFF()  g_lcd_buf

#define LCD_W 320
#define LCD_H 240


//////////////////////////////////////////////////////

#include "imlib.h" // need image_t related


typedef struct{
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} video_display_roi_t;

int video_play_avi_init(const char* path, avi_t* avi);
int video_play_avi(avi_t* avi);
int video_stop_play();
int video_display(image_t* img, video_display_roi_t img_roi);

#endif


