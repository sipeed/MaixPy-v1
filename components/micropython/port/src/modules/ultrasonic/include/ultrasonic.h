#ifndef __ULTRASONIC_H
#define __ULTRASONIC_H

#include "stdint.h"

/**
 * @param gpio: gpio resource, not pin
 * @attention register fpioa first
 * @return -1: param error
 *          0: timeout
 *         >0: distance
 */
long ultrasonic_measure_cm(uint8_t gpio, uint32_t timeout_us);
long ultrasonic_measure_inch(uint8_t gpio, uint32_t timeout_us);


#endif

