#ifndef __SIPEED_UID_H
#define __SIPEED_UID_H

#include "stdint.h"
#include "stdbool.h"

/**
 * get chip uid (32 bytes)
 * @param uid uid return value, a 32 bytes uint8_t array
 */
bool sipeed_uid_get(uint8_t* uid);

#endif
