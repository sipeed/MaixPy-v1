#include <stddef.h>
#include "touchscreen.h"
#include <tsfilter.h>

struct tsfilter_t *tsfilter_alloc(int ml, int nl)
{
    struct tsfilter_t *filter;
    if (ml <= 0 || nl <= 0)
        return NULL;

    filter = touchscreen_malloc(sizeof(struct tsfilter_t));
    if (!filter)
        return NULL;

    filter->mx = median_alloc(ml);
    filter->my = median_alloc(ml);
    filter->nx = mean_alloc(nl);
    filter->ny = mean_alloc(nl);
    filter->cal[0] = 1;
    filter->cal[1] = 0;
    filter->cal[2] = 0;
    filter->cal[3] = 0;
    filter->cal[4] = 1;
    filter->cal[5] = 0;
    filter->cal[6] = 1;

    if (!filter->mx || !filter->my || !filter->nx || !filter->ny)
    {
        if (filter->mx)
            median_free(filter->mx);
        if (filter->my)
            median_free(filter->my);
        if (filter->nx)
            mean_free(filter->nx);
        if (filter->ny)
            mean_free(filter->ny);
    }
    return filter;
}

void tsfilter_free(struct tsfilter_t *filter)
{
    if (filter)
    {
        if (filter->mx)
            median_free(filter->mx);
        if (filter->my)
            median_free(filter->my);
        if (filter->nx)
            mean_free(filter->nx);
        if (filter->ny)
            mean_free(filter->ny);
        touchscreen_free(filter);
    }
}

void tsfilter_setcal(struct tsfilter_t *filter, int *cal)
{
    if (filter)
    {
        filter->cal[0] = cal[0];
        filter->cal[1] = cal[1];
        filter->cal[2] = cal[2];
        filter->cal[3] = cal[3];
        filter->cal[4] = cal[4];
        filter->cal[5] = cal[5];
        filter->cal[6] = cal[6];
    }
}

void tsfilter_update(struct tsfilter_t *filter, int *x, int *y)
{
    int tx, ty;
    tx = median_update(filter->mx, *x);
    ty = median_update(filter->my, *y);
    tx = mean_update(filter->nx, tx);
    ty = mean_update(filter->ny, ty);
    *x = (filter->cal[2] + filter->cal[0] * tx + filter->cal[1] * ty) / filter->cal[6];
    *y = (filter->cal[5] + filter->cal[3] * tx + filter->cal[4] * ty) / filter->cal[6];
}

void tsfilter_clear(struct tsfilter_t *filter)
{
    if (filter)
    {
        if (filter->mx)
            median_clear(filter->mx);
        if (filter->my)
            median_clear(filter->my);
        if (filter->nx)
            mean_clear(filter->nx);
        if (filter->ny)
            mean_clear(filter->ny);
    }
}