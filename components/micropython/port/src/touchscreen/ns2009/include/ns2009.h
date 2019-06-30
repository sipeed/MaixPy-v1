#ifndef __NS2009_H
#define __NS2009_H

#include <stdio.h>

/* clang-format off */
#define NS2009_SLV_ADDR                     0x48
#define NS2009_IOCTL_SET_CALBRATION         0x1
/* clang-format on */

enum
{
    NS2009_LOW_POWER_READ_X = 0xc0,
    NS2009_LOW_POWER_READ_Y = 0xd0,
    NS2009_LOW_POWER_READ_Z1 = 0xe0,
    NS2009_LOW_POWER_READ_Z2 = 0xf0,
};

enum event_type
{
    TOUCH_NONE = 0,
    TOUCH_BEGIN,
    TOUCH_MOVE,
    TOUCH_END
};

struct ts_ns2009_event_t
{
    enum event_type type;
    int x, y;
} __attribute__((aligned(8)));

struct ts_ns2009_pdata_t
{
    struct tsfilter_t *filter;
    struct ts_ns2009_event_t *event;
    int x, y;
    int press;
} __attribute__((aligned(8)));

int ts_ns2009_poll(struct ts_ns2009_pdata_t *ts_ns2009_pdata);
int ts_ns2009_set_calibration(struct ts_ns2009_pdata_t *ts_ns2009_pdata, int cmd, void *arg);
struct ts_ns2009_pdata_t *ts_ns2009_probe(int* cal, int* err);
void ts_ns2009_remove(struct ts_ns2009_pdata_t *ts_ns2009_pdata);

int ns2009_hal_i2c_recv(const uint8_t *send_buf, size_t send_buf_len, uint8_t *receive_buf,
                  size_t receive_buf_len);

#endif