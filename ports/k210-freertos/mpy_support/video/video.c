
#include "video.h"
#include "vfs_internal.h"
#include "stdio.h"

int video_play_avi_init(const char* path, avi_t* avi)
{
    
    int err, tmp;
    uint32_t offset;
    uint8_t* buf = VIDEO_BUFF();
    mp_obj_t file = vfs_internal_open(path, "rb", &err);
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

    if(file==MP_OBJ_NULL || err!=0)
        return err;
    mp_uint_t ret = vfs_internal_read(file, buf, VIDEO_AVI_BUFF_SIZE, &err);
    if(err != 0 )
    {
        vfs_internal_close(file, &tmp);
        return err;
    }
    err = avi_init(buf, VIDEO_AVI_BUFF_SIZE, avi);
    if(err != 0)
    {
        vfs_internal_close(file, &tmp);
        return err;
    }
    printf("avi init ok\r\n");
    avi_debug_info(avi);
    //stream
    offset = avi_srarch_id(buf, VIDEO_AVI_BUFF_SIZE, (uint8_t*)"movi");
    printf("offset:%d\n", offset);
    err = avi_get_streaminfo(buf+offset+4, avi);
    if( err != 0)
    {
        vfs_internal_close(file, &tmp);
        return err;
    }
    printf("stream id:%04x(%04x), size:%d\n", avi->stream_id, AVI_VIDS_FLAG, avi->stream_size);
    vfs_internal_seek(file, offset+12, VFS_SEEK_SET, &err);
    if(avi->audio_sample_rate)							//有音频信息,才初始化
    {
        //TODO: init i2s
    }
    
    while(1)
    {
        if(avi->stream_id == AVI_VIDS_FLAG)	//视频流
        {
            printf("-----1---\n");
            buf = VIDEO_BUFF();
            vfs_internal_read(file, buf, avi->stream_size+8, &err);
            if( err != 0)
            {
                vfs_internal_close(file, &tmp);
                return err;
            }
            printf("memset size:%d\n", sizeof(image_t));
            img.data = IMAGE_BUFF();
            printf("-----2---\n");
            err = picojpeg_util_read(&img, MP_OBJ_NULL, buf, avi->stream_size);
            if( err != 0)
            {
                vfs_internal_close(file, &tmp);
                return err;
            }
            roi.w = img.w;
            roi.h = img.h;
            printf("display\n");
            video_display(&img, roi);
            printf("display end\n");
            //TODO: timer
            // while(frameup==0);	//等待时间到达(在TIM6的中断里面设置为1)
            // frameup=0;			//标志清零
            // frame++; 
        }else 	//音频流
        {		  
           
        } 
        if(avi_get_streaminfo(buf+avi->stream_size, avi) != AVI_STATUS_OK)//读取下一帧 流标志
        {
            printf("frame error \r\n"); 
            break; 
        } 		
    }
    return 0;
}

int video_play_avi(avi_t* avi)
{
    return 0;
}


int video_stop_play()
{
    return 0;
}

