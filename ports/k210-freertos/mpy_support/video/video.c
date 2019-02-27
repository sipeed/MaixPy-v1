
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
        video_hal_audio_init(avi);
    }
    avi->file = (void*)file;
    avi->video_buf = buf;
    avi->offset_movi = offset;
    avi->frame_count = 0;
    avi->status = VIDEO_STATUS_RESUME;
    avi->time_us_fps_ctrl = video_hal_ticks_us();
    avi->volume = 80;
    avi->audio_count = 0;
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
    uint8_t tmp_u8;
    uint8_t* pbuf;

    if(avi->status != VIDEO_STATUS_RESUME && avi->status != VIDEO_STATUS_PLAYING && avi->status != VIDEO_STATUS_PLAY_END)
    {
        return avi->status;
    }
    avi->status = VIDEO_STATUS_PLAYING;
    if(avi->stream_id == AVI_VIDS_FLAG) // video
    {
        pbuf = avi->video_buf;
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
        
        if(++avi->index_buf_save > 3)
            avi->index_buf_save = 0;
        do
        {
            tmp_u8 = avi->index_buf_play;
            if(tmp_u8)
                --tmp_u8;
            else
                tmp_u8 = 3;
        }while(avi->index_buf_save == tmp_u8);//buffer full, wait for play complete
        vfs_internal_read(avi->file, avi->audio_buf[avi->index_buf_save], avi->stream_size+8, &err);
        if( err != 0)
        {
            video_stop_play(avi);
            return err;
        }
        avi->audio_buf_len[avi->index_buf_save] = avi->stream_size;
        for(uint32_t i = 0; i< avi->audio_buf_len[avi->index_buf_save]/2; ++i)
        {
            ((int16_t*)avi->audio_buf[avi->index_buf_save])[i] = (int16_t)( ((int16_t*)avi->audio_buf[avi->index_buf_save])[i] * (avi->volume/100.0) );
        }
        if(avi->audio_count == 0)//first once play
        {
            ++avi->index_buf_play;
            video_hal_audio_play(avi->audio_buf[avi->index_buf_play], avi->audio_buf_len[avi->index_buf_play]);
        }
        ++avi->audio_count;
        pbuf = avi->audio_buf[avi->index_buf_save];
        // printf("save:%d %d\n", avi->index_buf_save, avi->audio_buf_len[avi->index_buf_save]);
        status = VIDEO_STATUS_DECODE_AUDIO;
    } 
    err = avi_get_streaminfo(pbuf + avi->stream_size, avi);
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
    video_hal_audio_deinit(avi);
    return 0;
}

