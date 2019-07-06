#include "boards.h"

#if CONFIG_BOARD_M5STICK
#include "m5stick.h"
#endif


int boards_init()
{
    int ret = 0;
#if CONFIG_BOARD_M5STICK
    if( !m5stick_init() )
        ret = -1;
#endif
    return ret;
}

