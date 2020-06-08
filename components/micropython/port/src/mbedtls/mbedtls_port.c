
#include <stdlib.h>
#include <stdio.h>
#include "rng.h"

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

// int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen) {
//     uint32_t val;
//     int n = 0;
//     *olen = len;
//     while (len--) {
//         if (!n) {
//             val = rng_get();
//             n = 4;
//         }
//         *output++ = val;
//         val >>= 8;
//         --n;
//     }
//     return 0;
// }

#include "mphalport.h"

int os_get_random(unsigned char *buf, size_t len)
{
    int i, j;
    unsigned long tmp;
 
    for (i = 0; i < ((len + 3) & ~3) / 4; i++) {
        tmp = rng_get() + systick_current_millis();
 
        for (j = 0; j < 4; j++) {
            if ((i * 4 + j) < len) {
                buf[i * 4 + j] = (uint8_t)(tmp >> (j * 8));
            } else {
                break;
            }
        }
    }
 
    return 0;
}

int mbedtls_hardware_poll( void *data, unsigned char *output, size_t len, size_t *olen )
{
    int res = os_get_random(output, len);
    *olen = len;
    return 0;
}

