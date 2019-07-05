#include "boards.h"

#if CONFIG_BOARD_M5STICK
#include "m5stick.h"
#endif


int boards_init()
{
#if CONFIG_BOARD_M5STICK
    m5stick_init();
#endif
}

