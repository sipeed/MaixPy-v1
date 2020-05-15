

#include "ns2009.h"
#include "ft52xx.h"
#include <string.h>
#include <stdlib.h>
#include <tsfilter.h>
#include <stdio.h>
#include "fpioa.h"
#include "sleep.h"
#include <math.h>
#include "touchscreen.h"

extern int ft52xx_hal_i2c_recv(const uint8_t *send_buf, size_t send_buf_len, uint8_t *receive_buf, size_t receive_buf_len);

static uint8_t ft52xx_read(uint8_t cmd, uint8_t *buff, size_t len)
{
    uint8_t ret;
    ret = ft52xx_hal_i2c_recv(&cmd, 1, buff, len);
    if (ret != 0)
        return 0;
    return 1;
}

static long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    long divisor = (in_max - in_min);
    if (divisor == 0) {
        return -1;
    }
    return (x - in_min) * (out_max - out_min) / divisor + out_min;
}

int ts_ft52xx_poll(ts_ft52xx_pdata_t *pdata)
{
    int x = 0, y = 0;
    uint8_t buff[16];
    ft52xx_read(DEVIDE_MODE, buff, 16);
    uint8_t state = buff[TD_STATUS];
    if ((state > 2) || (state == 0)) {
        pdata->event->type = TOUCH_END;
        pdata->event->x = 0;
        pdata->event->y = 0;
        pdata->press = 0;
        return 0;
    }

    for (uint8_t i = 0; i < 1; i++) {
        x = buff[TOUCH1_XH + i * 6] & 0x0F;
        x <<= 8;
        x |= buff[TOUCH1_XL + i * 6];
        y = buff[TOUCH1_YH + i * 6] & 0x0F;
        y <<= 8;
        y |= buff[TOUCH1_YL + i * 6];
        x = map(x, 0, 320, 0, 240);
        y = map(y, 0, 320, 0, 240);
    }

    if (!pdata->press) {
        pdata->press = 1;
        pdata->event->type = TOUCH_BEGIN;
    } else if (pdata->event->x != x || pdata->event->y != y) {
        pdata->event->type = TOUCH_MOVE;
    } else {
        pdata->event->type = TOUCH_BEGIN;
    }
    pdata->event->x = x;
    pdata->event->y = y;

    return 1;
}


ts_ft52xx_pdata_t *ts_ft52xx_probe( int *err)
{
    ts_ft52xx_pdata_t *pdata = NULL;

    *err = 0;
    uint8_t val;
    if (!ft52xx_read(FT5206_VENDID_REG, &val, 1)) {
        *err = EIO;
        return NULL;
    }
    pdata = touchscreen_malloc(sizeof(ts_ft52xx_pdata_t));
    if (!pdata) {
        *err = ENOMEM;
        return NULL;
    }
    pdata->x = 0;
    pdata->y = 0;
    pdata->press = 0;
    pdata->event = touchscreen_malloc(sizeof(ts_ft52xx_pdata_t));
    pdata->event->x = 0;
    pdata->event->y = 0;
    pdata->event->type = TOUCH_NONE;
    return pdata;
}

void ts_ft52xx_remove(ts_ft52xx_pdata_t *pdata)
{
    if (pdata->event) {
        touchscreen_free(pdata->event);
        pdata->event = NULL;
    }
    if (pdata) {
        touchscreen_free(pdata);
        pdata = NULL;
    }
}
