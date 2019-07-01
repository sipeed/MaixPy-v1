#include "fb_alloc.h"
#include "omv.h"
#include "ide_dbg.h"
#include "sensor.h"

bool omv_init_once()
{
    return true;
}

bool omv_init()
{
    bool ret = true;
    fb_alloc_init0();
    sensor_init0();
    ret = ide_debug_init0();
    return ret;
}
