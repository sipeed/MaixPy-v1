#include "fb_alloc.h"
#include "omv.h"


bool omv_init_once()
{
    return true;
}

bool omv_init()
{
    fb_alloc_init0();
    return true;
}
