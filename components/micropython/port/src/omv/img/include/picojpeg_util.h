#ifndef __PICOJPEG_UTIL_H__
#define __PICOJPEG_UTIL_H__

#include "imlib.h"


int picojpeg_util_read(image_t* img, mp_obj_t file, uint8_t* buf, uint32_t buf_len);

#endif

