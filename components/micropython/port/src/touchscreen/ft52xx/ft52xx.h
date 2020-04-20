#pragma once

#include <stdint.h>

#define FT5206_SLAVE_ADDRESS    (0x38)
#define FT5206_MODE_REG         (0x00)
#define FT5206_TOUCHES_REG      (0x02)
#define FT5206_VENDID_REG       (0xA8)
#define FT5206_CHIPID_REG       (0xA3)
#define FT5206_THRESHHOLD_REG   (0x80)
#define FT5206_POWER_REG        (0x87)

#define FT5206_MONITOR         (0x01)
#define FT5206_SLEEP_IN        (0x03)

#define FT5206_VENDID           0x11
#define FT6206_CHIPID           0x06
#define FT6236_CHIPID           0x36
#define FT6236U_CHIPID          0x64
#define FT5206U_CHIPID          0x64

#define DEVIDE_MODE 0x00
#define TD_STATUS   0x02
#define TOUCH1_XH   0x03
#define TOUCH1_XL   0x04
#define TOUCH1_YH   0x05
#define TOUCH1_YL   0x06

typedef struct ts_ns2009_pdata_t  ts_ft52xx_pdata_t;


ts_ft52xx_pdata_t *ts_ft52xx_probe( int* err);

int ts_ft52xx_poll(ts_ft52xx_pdata_t *pdata);

void ts_ft52xx_remove(ts_ft52xx_pdata_t *pdata);

