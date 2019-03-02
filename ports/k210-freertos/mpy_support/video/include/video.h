#ifndef __VIDEO_H
#define __VIDEO_H

#include "avi.h"

//////////////////// VIDEO BUFF ///////////////////////
#include "framebuffer.h"
#include "omv_boardconfig.h"
extern uint8_t g_jpg_buf[OMV_JPEG_BUF_SIZE];
#define   VIDEO_BUFF()  g_jpg_buf  // we use omv module's framebuff here
#define   VIDEO_AVI_BUFF_SIZE  OMV_JPEG_BUF_SIZE

extern uint8_t g_dvp_buf[OMV_INIT_W * OMV_INIT_H * 2];
#define   IMAGE_BUFF()  g_dvp_buf

#define LCD_W 320
#define LCD_H 240

// #define VIDEO_DEBUG
//////////////////////////////////////////////////////

#include "imlib.h" // need image_t related

typedef enum{
    VIDEO_HAL_FILE_SEEK_SET = 0,
    VIDEO_HAL_FILE_SEEK_CUR,
    VIDEO_HAL_FILE_SEEK_END
} video_hal_file_seek_t;

typedef enum{
    VIDEO_STATUS_PLAY_END = 0,
    VIDEO_STATUS_PLAYING = -1,
    VIDEO_STATUS_RESUME = -2,
    VIDEO_STATUS_DECODE_VIDEO = -3,
    VIDEO_STATUS_DECODE_AUDIO = -4
}video_status_t;

typedef struct{
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} video_display_roi_t;

int video_play_avi_init(const char* path, avi_t* avi);
int video_play_avi(avi_t* avi);
int video_stop_play();
int video_hal_display(image_t* img, video_display_roi_t img_roi);
uint64_t video_hal_ticks_us(void);
int video_hal_audio_init(avi_t* avi);
int video_hal_audio_play(uint8_t* data, uint32_t len);
void video_avi_record_fail(avi_t* avi);
void video_avi_record_success(avi_t* avi);
int video_hal_file_open(avi_t* avi, const char* path, bool write);
int video_hal_file_write(avi_t* avi, uint8_t* data, uint32_t len);
int video_hal_file_read(avi_t* avi, uint8_t* data, uint32_t len);
int video_hal_file_close(avi_t* avi);
int video_hal_file_seek(avi_t* avi, long offset, uint8_t whence);
/**
 * 
 * @return mjpeg file size, <0 if error occurred
 */
int video_hal_image_encode_mjpeg(avi_t* avi, image_t* img);
uint8_t* video_hal_malloc(uint32_t size);
uint8_t* video_hal_free(uint8_t* ptr);

#endif


