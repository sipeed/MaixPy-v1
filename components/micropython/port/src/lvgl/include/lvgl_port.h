
#ifndef __LVGL_H_
#define __LVGL_H_

#include "lv_gc.h"

typedef struct{
    LV_ROOTS
} lvgl_state_gc_t;

extern lvgl_state_gc_t g_lvgl_state_gc;

#define LVGL_STATE_GC(x) g_lvgl_state_gc.x

#endif

