#include <unistd.h>
#include "py/mpconfig.h"
#include "uarths.h"
#include "sleep.h"

#include "py/runtime.h"
#include "extmod/misc.h"
#include "lib/utils/pyexec.h"
/*
 * Core UART functions to implement for a port
 */

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = 0;
    uarths_receive_data(&c,1);
    return c;
}
void mp_hal_debug_tx_strn_cooked(void *env, const char *str, uint32_t len);
const mp_print_t mp_debug_print = {NULL, mp_hal_debug_tx_strn_cooked};

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
        uarths_send_data(str,len);
}
void mp_hal_debug_tx_strn_cooked(void *env, const char *str, uint32_t len) {
    (void)env;
    while (len--) {
        if (*str == '\n') {
            uarths_putchar('\r');
        }
        uarths_putchar( *str++);
    }
}
