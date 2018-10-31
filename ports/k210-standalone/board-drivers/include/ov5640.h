/**
 * @file      ov5640.h
 * @brief     OV5640 Controller
 */

#ifndef _OV5640_H
#define _OV5640_H

#include <stdint.h>

uint8_t myov5640_init(void);
uint16_t ov5640_read_id(void);

#endif /* _OV5640_H */
