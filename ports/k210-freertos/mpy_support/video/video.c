
#include "video.h"
#include "vfs_internal.h"
#include "stdio.h"
#include "picojpeg_util.h"


int video_play_avi_init(const char* path, avi_t* avi)
{
    
    int err;
    uint32_t offset;
    uint8_t* buf = VIDEO_BUFF();
    mp_obj_t file = vfs_internal_open(path, "rb", &err);

    if(file==MP_OBJ_NULL || err!=0)
        return err;
    mp_uint_t read_size = vfs_internal_read(file, buf, VIDEO_AVI_BUFF_SIZE, &err);
    if(err != 0 )
    {
        video_stop_play(avi);
        return err;
    }
    err = avi_init(buf, read_size, avi);
    if(err != 0)
    {
        video_stop_play(avi);
        return err;
    }
    //stream
    offset = avi_srarch_id(buf, read_size, (uint8_t*)"movi");
    err = avi_get_streaminfo(buf+offset+4, avi);
    if( err != 0)
    {
        video_stop_play(avi);
        return err;
    }
    vfs_internal_seek(file, offset+12, VFS_SEEK_SET, &err);
    if(avi->audio_sample_rate)//init audio device
    {
        //TODO: init i2s
    }
    avi->file = (void*)file;
    avi->video_buf = buf;
    avi->offset_movi = offset;
    avi->frame_count = 0;
    avi->status = VIDEO_STATUS_RESUME;
    avi->time_us_fps_ctrl = video_hal_ticks_us();
#ifdef VIDEO_DEBUG
    avi_debug_info(avi);
#endif
    return 0;
}

video_status_t video_play_avi(avi_t* avi)
{
    int err = 0;
    image_t img = {
        .w = 0,
        .h = 0,
        .bpp = 0,
        .data = NULL,
        .pix_ai = NULL
    };
    video_display_roi_t roi = {
        .x = 0,
        .y = 0
    };
    int status = VIDEO_STATUS_PLAYING;

    if(avi->status != VIDEO_STATUS_RESUME && avi->status != VIDEO_STATUS_PLAYING && avi->status != VIDEO_STATUS_PLAY_END)
    {
        return avi->status;
    }
    avi->status = VIDEO_STATUS_PLAYING;
    if(avi->stream_id == AVI_VIDS_FLAG) // video
    {
        vfs_internal_read(avi->file, avi->video_buf, avi->stream_size+8, &err);
        if( err != 0)
        {
            video_stop_play(avi);
            return err;
        }
        img.data = IMAGE_BUFF();
        err = picojpeg_util_read(&img, MP_OBJ_NULL, avi->video_buf, avi->stream_size);
        if( err != 0)
        {
            video_stop_play(avi);
            return err;
        }
        roi.w = img.w;
        roi.h = img.h;
        while( video_hal_ticks_us() - avi->time_us_fps_ctrl < avi->sec_per_frame);
        avi->time_us_fps_ctrl = video_hal_ticks_us();
        video_hal_display(&img, roi);
        ++avi->frame_count;
        status = VIDEO_STATUS_DECODE_VIDEO;
    }
    else // audio
    {//TODO:
        vfs_internal_read(avi->file, avi->video_buf, avi->stream_size+8, &err);
        if( err != 0)
        {
            video_stop_play(avi);
            return err;
        }
        status = VIDEO_STATUS_DECODE_AUDIO;
    } 
    err = avi_get_streaminfo(avi->video_buf+avi->stream_size, avi);
    if( err != AVI_STATUS_OK)//read the next frame
    {
        video_stop_play(avi);
        if(avi->frame_count != avi->total_frame)
        {
            printf("frame error \r\n"); 
            avi->status = err;
            return err;
        }
        avi->status = VIDEO_STATUS_PLAY_END;
    }
    return status;
}


int video_stop_play(avi_t* avi)
{
    int err;
    vfs_internal_close(avi->file, &err);
    avi->status = VIDEO_STATUS_PLAY_END;
    return 0;
}

