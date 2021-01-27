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

//画线
//x1,y1:起点坐标
//x2,y2:终点坐标
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, uRow, uCol;
    delta_x = x2 - x1; //计算坐标增量
    delta_y = y2 - y1;
    uRow = x1;
    uCol = y1;
    if (delta_x > 0)
        incx = 1; //设置单步方向
    else if (delta_x == 0)
        incx = 0; //垂直线
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }
    if (delta_y > 0)
        incy = 1;
    else if (delta_y == 0)
        incy = 0; //水平线
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }
    if (delta_x > delta_y)
        distance = delta_x; //选取基本增量坐标轴
    else
        distance = delta_y;
    for (t = 0; t <= distance + 1; t++) //画线输出
    {
        lcd->draw_point(uRow, uCol, color); //画点
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance)
        {
            xerr -= distance;
            uRow += incx;
        }
        if (yerr > distance)
        {
            yerr -= distance;
            uCol += incy;
        }
    }
}

//在指定位置画一个指定大小的圆
//(x,y):中心点
//r    :半径
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    int a, b;
    int di;
    a = 0;
    b = r;
    di = 3 - (r << 1); //判断下个点位置的标志
    while (a <= b)
    {
        lcd->draw_point(x0 + a, y0 - b, color); //5
        lcd->draw_point(x0 + b, y0 - a, color); //0
        lcd->draw_point(x0 + b, y0 + a, color); //4
        lcd->draw_point(x0 + a, y0 + b, color); //6
        lcd->draw_point(x0 - a, y0 + b, color); //1
        lcd->draw_point(x0 - b, y0 + a, color);
        lcd->draw_point(x0 - a, y0 - b, color); //2
        lcd->draw_point(x0 - b, y0 - a, color); //7
        a++;
        //使用Bresenham算法画圆
        if (di < 0)
            di += 4 * a + 6;
        else
        {
            di += 10 + 4 * (a - b);
            b--;
        }
    }
}

static void lcd_draw_cross(int x, int y, uint16_t color)
{
    lcd_draw_line(x - 12, y, x + 13, y, color); //横线
    lcd_draw_line(x, y - 12, x, y + 13, color); //竖线
    lcd->draw_point(x + 1, y + 1, color);
    lcd->draw_point(x - 1, y + 1, color);
    lcd->draw_point(x + 1, y - 1, color);
    lcd->draw_point(x - 1, y - 1, color);
    lcd_draw_circle(x, y, 6, color); //画中心圈
}

static void cairo_draw_string(int x, int y, const char *title)
{
    // lcd->draw_string(x, y, (char*)title, WHITE);
}

int do_tscal(struct ts_ns2009_pdata_t *ts_ns2009_pdata, int width, int height, int* c)
{
    struct tscal_t cal;

    // char buffer[256];
    int index;
    c[0] = 1;
    c[1] = 0;
    c[2] = 0;
    c[3] = 0;
    c[4] = 1;
    c[5] = 0;
    c[6] = 1;

    ts_ns2009_set_calibration(ts_ns2009_pdata, NS2009_IOCTL_SET_CALBRATION, c);


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
    lcd->clear(BLACK);
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
                    // sprintf(buffer, "[%d,%d,%d,%d,%d,%d,%d]", cal.a[0], cal.a[1], cal.a[2], cal.a[3], cal.a[4], cal.a[5], cal.a[6]);
                }
                else
                {
                    // sprintf(buffer, "%s", "calibration failed");
                }
                lcd->clear(BLACK);
                // cairo_draw_string(0, height / 2, buffer);
                memcpy(c, cal.a, 7*sizeof(int));
                ts_ns2009_pdata->event->type = TOUCH_NONE;
                break;
            }
            lcd->clear(BLACK);
            lcd_draw_cross(cal.xfb[index], cal.yfb[index], RED);
        }
        ts_ns2009_pdata->event->type = TOUCH_NONE;
    }

    return 0;
}
