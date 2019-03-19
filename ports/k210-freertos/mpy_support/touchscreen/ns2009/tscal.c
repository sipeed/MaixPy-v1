#include "ns2009.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "lcd.h"
#include "tscal.h"
#include "sleep.h"

static int perform_calibration(struct tscal_t *cal)
{
    float n, x, y, x2, y2, xy, z, zx, zy;
    float det, a, b, c, e, f, i;
    float scaling = 65536.0;
    int j;

    n = x = y = x2 = y2 = xy = 0;
    for (j = 0; j < 5; j++)
    {
        n += 1.0;
        x += (float)cal->x[j];
        y += (float)cal->y[j];
        x2 += (float)(cal->x[j] * cal->x[j]);
        y2 += (float)(cal->y[j] * cal->y[j]);
        xy += (float)(cal->x[j] * cal->y[j]);
    }

    det = n * (x2 * y2 - xy * xy) + x * (xy * y - x * y2) + y * (x * xy - y * x2);
    if (det < 0.1 && det > -0.1)
        return 0;

    a = (x2 * y2 - xy * xy) / det;
    b = (xy * y - x * y2) / det;
    c = (x * xy - y * x2) / det;
    e = (n * y2 - y * y) / det;
    f = (x * y - n * xy) / det;
    i = (n * x2 - x * x) / det;

    z = zx = zy = 0;
    for (j = 0; j < 5; j++)
    {
        z += (float)cal->xfb[j];
        zx += (float)(cal->xfb[j] * cal->x[j]);
        zy += (float)(cal->xfb[j] * cal->y[j]);
    }

    cal->a[0] = (int)((b * z + e * zx + f * zy) * (scaling));
    cal->a[1] = (int)((c * z + f * zx + i * zy) * (scaling));
    cal->a[2] = (int)((a * z + b * zx + c * zy) * (scaling));

    z = zx = zy = 0;
    for (j = 0; j < 5; j++)
    {
        z += (float)cal->yfb[j];
        zx += (float)(cal->yfb[j] * cal->x[j]);
        zy += (float)(cal->yfb[j] * cal->y[j]);
    }

    cal->a[3] = (int)((b * z + e * zx + f * zy) * (scaling));
    cal->a[4] = (int)((c * z + f * zx + i * zy) * (scaling));
    cal->a[5] = (int)((a * z + b * zx + c * zy) * (scaling));

    cal->a[6] = (int)scaling;
    return 1;
}

static void lcd_draw_cross(int x, int y, uint16_t color)
{
    lcd_draw_line(x - 12, y, x + 13, y, color); //横线
    lcd_draw_line(x, y - 12, x, y + 13, color); //竖线
    lcd_draw_point(x + 1, y + 1, color);
    lcd_draw_point(x - 1, y + 1, color);
    lcd_draw_point(x + 1, y - 1, color);
    lcd_draw_point(x - 1, y - 1, color);
    lcd_draw_circle(x, y, 6, color); //画中心圈
}

static void cairo_draw_string(int x, int y, const char *title)
{
    lcd_draw_string(x, y, title, BLUE);
}

int do_tscal(struct ts_ns2009_pdata_t *ts_ns2009_pdata)
{
    struct tscal_t cal;
    lcd_ctl_t lcd_ctl;

    char buffer[256];
    int c[7] = {1, 0, 0, 0, 1, 0, 1};
    int width, height;
    int index;

    printf("%s\r\n", __func__);

    ts_ns2009_set_calibration(ts_ns2009_pdata, NS2009_IOCTL_SET_CALBRATION, &c[0]);

    //FIXME 需要更优雅的实现
    lcd_get_info(&lcd_ctl);

    width = lcd_ctl.width;
    height = lcd_ctl.height;

    cal.xfb[0] = 50;
    cal.yfb[0] = 50;

    cal.xfb[1] = width - 50;
    cal.yfb[1] = 50;

    cal.xfb[2] = width - 50;
    cal.yfb[2] = height - 50;

    cal.xfb[3] = 50;
    cal.yfb[3] = height - 50;

    cal.xfb[4] = width / 2;
    cal.yfb[4] = height / 2;

    index = 0;

    lcd_draw_cross(cal.xfb[index], cal.yfb[index], RED);

    while (1)
    {
        ts_ns2009_poll(ts_ns2009_pdata);
        if (ts_ns2009_pdata->event->type == TOUCH_END)
        {
            cal.x[index] = ts_ns2009_pdata->event->x;
            cal.y[index] = ts_ns2009_pdata->event->y;

            if (++index >= 5)
            {
                if (perform_calibration(&cal))
                {
                    ts_ns2009_set_calibration(ts_ns2009_pdata, NS2009_IOCTL_SET_CALBRATION, &cal.a[0]);
                    sprintf(buffer, "[%d, %d, %d, %d, %d, %d, %d]", cal.a[0], cal.a[1], cal.a[2], cal.a[3], cal.a[4], cal.a[5], cal.a[6]);
                }
                else
                {
                    sprintf(buffer, "%s", "calibration failed");
                }
                cairo_draw_string(50, height / 2, buffer);
                printf("%s\r\n", buffer);
                break;
            }
            lcd_draw_cross(cal.xfb[index], cal.yfb[index], RED);
        }
    }

    return 0;
}
