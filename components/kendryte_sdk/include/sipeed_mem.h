#ifndef __SIPEED_MEM_
#define __SIPEED_MEM_

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

void sipeed_reset_sys_mem();
size_t get_free_heap_size2(void);

#ifdef __cplusplus
}
#endif

#endif

