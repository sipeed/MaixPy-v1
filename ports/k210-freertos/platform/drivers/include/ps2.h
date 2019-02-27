#ifndef __PS2_H
#define __PS2_H

#include <stdio.h>
#include <stdint.h>

void ps2_mode_config(void);
void ps2_read_status(uint8_t *pbuff);
void ps2_motor_ctrl(uint8_t motor1, uint8_t motor2);
uint8_t ps2_read_led_stat(void);

#endif
