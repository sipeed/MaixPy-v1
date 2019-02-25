#ifndef __VIDEO_H
#define __VIDEO_H

#include "avi.h"

//////////////////// VIDEO BUFF ///////////////////////
#include "framebuffer.h"
#define VIDEO_BUFF()  fb_framebuffer->pixels  // we use omv module's framebuff here, size: OMV_INIT_W * OMV_INIT_H * 2
#define   VIDEO_AVI_BUFF_SIZE  1024*60        // must <= OMV_INIT_W * OMV_INIT_H * 2
//////////////////////////////////////////////////////



int video_play_avi_init(const char* path, avi_t* avi);
int video_play_avi(avi_t* avi);
int video_stop_play();

#endif


