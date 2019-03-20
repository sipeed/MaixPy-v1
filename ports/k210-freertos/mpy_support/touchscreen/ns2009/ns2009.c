#include "ns2009.h"
#include <string.h>
#include <stdlib.h>
#include <tsfilter.h>
#include <stdio.h>
#include "fpioa.h"
#include "sleep.h"
#include <math.h>
#include "touchscreen.h"

static uint8_t ns2009_read(uint8_t cmd, int *val)
{
    uint8_t ret, buf[2];
    ret = ns2009_hal_i2c_recv(&cmd, 1, buf, 2);
    if (ret != 0)
        return 0;
    if (val)
        *val = (buf[0] << 4) | (buf[1] >> 4);
    return 1;
}

static void push_event_begin(struct ts_ns2009_event_t *ts_event, int x, int y)
{
    ts_event->type = TOUCH_BEGIN;
    ts_event->x = x;
    ts_event->y = y;
}

static void push_event_move(struct ts_ns2009_event_t *ts_event, int x, int y)
{
    ts_event->type = TOUCH_MOVE;
    ts_event->x = x;
    ts_event->y = y;
}

static void push_event_end(struct ts_ns2009_event_t *ts_event, int x, int y)
{
    ts_event->type = TOUCH_END;
    ts_event->x = x;
    ts_event->y = y;
}

static void push_event_none(struct ts_ns2009_event_t *ts_event)
{
    ts_event->type = TOUCH_NONE;
    ts_event->x = 0;
    ts_event->y = 0;
}

int ts_ns2009_poll(struct ts_ns2009_pdata_t *ts_ns2009_pdata)
{
    int x = 0, y = 0, z1 = 0;

    if (ns2009_read(NS2009_LOW_POWER_READ_Z1, &z1))
    {
        if ((z1 > 70) && (z1 < 2000))
        {
            ns2009_read(NS2009_LOW_POWER_READ_X, &x);
            ns2009_read(NS2009_LOW_POWER_READ_Y, &y);
            tsfilter_update(ts_ns2009_pdata->filter, &x, &y);

            if (!ts_ns2009_pdata->press)
            {
                push_event_begin(ts_ns2009_pdata->event, x, y);
                ts_ns2009_pdata->press = 1;
            }
            else
            {
                if ((ts_ns2009_pdata->x != x) || (ts_ns2009_pdata->y != y))
                {
                    push_event_move(ts_ns2009_pdata->event, x, y);
                }
            }
            ts_ns2009_pdata->x = x;
            ts_ns2009_pdata->y = y;
        }
        else
        {
            if (ts_ns2009_pdata->press)
            {
                tsfilter_clear(ts_ns2009_pdata->filter);
                push_event_end(ts_ns2009_pdata->event, ts_ns2009_pdata->x, ts_ns2009_pdata->y);
                ts_ns2009_pdata->press = 0;
            }
        }
    }
    else
    {
        push_event_none(ts_ns2009_pdata->event);
    }

    return 1;
}

int ts_ns2009_set_calibration(struct ts_ns2009_pdata_t *ts_ns2009_pdata, int cmd, void *arg)
{
    int cal[7];

    if (cmd == NS2009_IOCTL_SET_CALBRATION)
    {
        if (!arg)
            return -1;
        memcpy(cal, arg, sizeof(int) * 7);
        tsfilter_setcal(ts_ns2009_pdata->filter, cal);
        return 0;
    }
    return -1;
}

// "ts-ns2009@0" : {
//     "i2c-bus" : "i2c-v3s.0",
//     "slave-address" : 72,
//     "median-filter-length" : 5,
//     "mean-filter-length" : 5,
//     "calibration" : [ 14052, 21, -2411064, -67, 8461, -1219628, 65536 ],
//     "poll-interval-ms" : 10
// },

//[-4278, -68, 16701858, -8, -5930, 21990146, 65536]
//{65, 5853, -1083592, -4292, -15, 16450115, 65536};
struct ts_ns2009_pdata_t *ts_ns2009_probe(int* cal, int* err)
{
    struct ts_ns2009_pdata_t *ts_ns2009_pdata;

    *err = 0;
    if (!ns2009_read(NS2009_LOW_POWER_READ_Z1, NULL))
    {
        *err = EIO;
        return NULL;
    }
    ts_ns2009_pdata = touchscreen_malloc(sizeof(struct ts_ns2009_pdata_t));
    if (!ts_ns2009_pdata)
    {
        *err = ENOMEM;
        return NULL;
    }

    ts_ns2009_pdata->filter = tsfilter_alloc(5, 5);
    tsfilter_setcal(ts_ns2009_pdata->filter, cal);
    //
    ts_ns2009_pdata->x = 0;
    ts_ns2009_pdata->y = 0;
    ts_ns2009_pdata->press = 0;
    //
    ts_ns2009_pdata->event = touchscreen_malloc(sizeof(struct ts_ns2009_event_t));
    ts_ns2009_pdata->event->x = 0;
    ts_ns2009_pdata->event->y = 0;
    ts_ns2009_pdata->event->type = TOUCH_NONE;
    return ts_ns2009_pdata;
}

void ts_ns2009_remove(struct ts_ns2009_pdata_t *ts_ns2009_pdata)
{
    if (ts_ns2009_pdata->filter)
    {
        tsfilter_free(ts_ns2009_pdata->filter);
        ts_ns2009_pdata->filter = NULL;
    }

    if (ts_ns2009_pdata->event)
    {
        touchscreen_free(ts_ns2009_pdata->event);
        ts_ns2009_pdata->filter = NULL;
    }

    if (ts_ns2009_pdata)
    {
        touchscreen_free(ts_ns2009_pdata);
        ts_ns2009_pdata->filter = NULL;
    }
}
