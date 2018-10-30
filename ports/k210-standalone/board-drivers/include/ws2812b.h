#ifndef _WS2812B_H
#define _WS2812B_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * LED control:
 * WS2812B_SetLedRGB(); // First LED (Connected to MCU)
 * WS2812B_SetLedRGB(); // Second LED
 * WS2812B_SetLedRGB(); // Third LED (If exist)
 * WS2812B_TxRes()      // Send reset signal
 *
 * WS2812B's protocol:
 * The data is sent in a sequence containing 24 of those bits – 8 bits for each color
 * Highest bit first, followed by a low “reset” pulse of at least 50µs.
 * 0 is encoded as: T0H = 0.4uS ToL = 0.85uS (Tolerance 0.15uS)
 * 1 is encoded as: T1H = 0.85uS T1L = 0.4uS (Tolerance 0.15uS)
 * A valid reset: Hold data line low for at least 50µs.
 *
 *                                                           |-->Update color
 *                                _________________ | >50uS  |
 * START_______IDLE_______________|1st LED|2nd LED|___Reset_______IDLE_____________
 **/

void WS2812B_SetLedRGB(uint8_t r, uint8_t g, uint8_t b,int gpio_num);
void WS2812B_TxRes(int gpio_num);
void  WS2812B_SetLednRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t num,int gpio_num);
void init_nop_cnt(void);

#ifdef __cplusplus
}
#endif

#endif /* _WS2812B_H */
