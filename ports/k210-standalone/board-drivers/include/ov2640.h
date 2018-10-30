/**
 * @file      ov2640.h
 * @brief     OV2640 Controller
 */

#ifndef _OV2640_K210_H
#define _OV2640_K210_H

#include <stdint.h>

int ov2640_init(void);
int ov2640_read_id(uint16_t *manuf_id, uint16_t *device_id);

#endif /* _OV2640_H */
