#include "fb_alloc.h"
#include "omv.h"
#include "ide_dbg.h"


bool omv_init_once()
{
    return true;
}

bool omv_init()
{
    fb_alloc_init0();
    sensor_init0();
    ide_debug_init0();
    return true;
}
