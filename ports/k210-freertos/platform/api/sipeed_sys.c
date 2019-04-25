#include "platform.h"
#include "sysctl.h"
#include "utils.h"
#include "math.h"
#include "sleep.h"
#include "sipeed_sys.h"


void sipeed_sys_reset()
{
	sysctl->soft_reset.soft_reset = 1;
}

