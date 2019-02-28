
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
    // printf("movi offset: %d %x\n", avi->offset_movi, avi->offset_movi);
    // offset = avi_srarch_id(buf, read_size, (uint8_t*)"movi");
    err = avi_get_streaminfo(buf+avi->offset_movi+4, avi);
    if( err != 0)
    {
        video_stop_play(avi);
        return err;
    }
    // printf("----2--:%d %d\n", avi->stream_id, avi->stream_size);
    vfs_internal_seek(file, avi->offset_movi+12, VFS_SEEK_SET, &err);
    if(avi->audio_sample_rate)//init audio device
    {
        video_hal_audio_init(avi);
    }
    avi->file = (void*)file;
    avi->video_buf = buf;
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
        // printf("save index:%d\n", avi->index_buf_save);

        while( !avi->audio_buf[avi->index_buf_save].empty );//buffer full, wait for play complete
        // printf("save index:%d ok\n", avi->index_buf_save);
        vfs_internal_read(avi->file, avi->audio_buf[avi->index_buf_save].buf, avi->stream_size+8, &err);
        if( err != 0)
        {
            video_stop_play(avi);
            return err;
        }
        avi->audio_buf[avi->index_buf_save].len = avi->stream_size;
        for(uint32_t i = 0; i< avi->audio_buf[avi->index_buf_save].len/2; ++i)
        {
            ((int16_t*)avi->audio_buf[avi->index_buf_save].buf)[i] = (int16_t)( ((int16_t*)avi->audio_buf[avi->index_buf_save].buf)[i] * (avi->volume/100.0) );
        }
        avi->audio_buf[avi->index_buf_save].empty = false;
        if(avi->audio_count==0)//first once play
        {
            ++avi->index_buf_play;
            if(!avi->audio_buf[avi->index_buf_play].empty)
            {
                video_hal_audio_play(avi->audio_buf[avi->index_buf_play].buf, avi->audio_buf[avi->index_buf_play].len);
                // printf("play index:%d\n", avi->index_buf_play);
            }
        }
        else if(avi->index_buf_play == avi->index_buf_save)//play complete already, restart play, play index no change
        {
            if(!avi->audio_buf[avi->index_buf_play].empty)
            {
                video_hal_audio_play(avi->audio_buf[avi->index_buf_play].buf, avi->audio_buf[avi->index_buf_play].len);
                // printf("play index:%d\n", avi->index_buf_play);
            }
        }
        ++avi->audio_count;
        pbuf = avi->audio_buf[avi->index_buf_save].buf;
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


/////////////////////////////////////////////////////////////////////////////////////

/**
  * @avi_config: config: sec_per_frame, max_byte_sec, width, height,
  *                      audio_sample_rate, audio_channels, audio_format
  */
int video_record_avi_init(const char* path, avi_t* avi_config)
{
    
    int err, tmp;
    uint32_t offset;
    uint8_t* buf = VIDEO_BUFF();
    mp_obj_t file = vfs_internal_open(path, "wb", &err);
    uint32_t header_size;

    if(file==MP_OBJ_NULL || err!=0)
        return err;
    err = avi_record_header_init(buf, VIDEO_AVI_BUFF_SIZE, avi_config);
    if( err != 0 )
    {
        vfs_internal_close(file, &tmp);
        return err;
    }
    header_size = avi_config->offset_movi + 4;
    mp_uint_t ret = vfs_internal_write(file, buf, header_size, &err);
    if( (ret != header_size) || (err!=0) )
    {
        vfs_internal_close(file, &tmp);
        return err;
    }
    avi_config->file = file;

    return 0;
}

void video_avi_record_fail(avi_t* avi)
{
    int err;
    vfs_internal_close(avi->file, &err);
}

void video_avi_record_success(avi_t* avi)
{
    int err;
    vfs_internal_close(avi->file, &err);
}


