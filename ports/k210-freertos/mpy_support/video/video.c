
#include "video.h"
#include "vfs_internal.h"

int video_play_avi_init(const char* path, avi_t* avi)
{
    int err, tmp;
    uint8_t* data = VIDEO_BUFF();
    mp_obj_t file = vfs_internal_open(path, "rb", &err);
    if(file==MP_OBJ_NULL || err!=0)
        return err;
    mp_uint_t ret = vfs_internal_read(file, data, VIDEO_AVI_BUFF_SIZE, &err);
    if(err != 0 )
    {
        vfs_internal_close(file, &tmp);
        return err;
    }
    err = avi_init(data, VIDEO_AVI_BUFF_SIZE, &avi);
    if(err != 0)
    {
        vfs_internal_close(file, &tmp);
        return err;
    }
    avi_debug_info(&avi);
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

