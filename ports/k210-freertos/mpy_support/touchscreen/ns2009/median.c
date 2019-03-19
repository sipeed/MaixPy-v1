/*
 * libc/filter/median.c
 */

#include <math.h>
#include <stddef.h>
#include <median.h>
#include "touchscreen.h"

struct median_filter_t *median_alloc(int length)
{
    struct median_filter_t *filter;

    if (length <= 0)
        return NULL;
    filter = touchscreen_malloc(sizeof(struct median_filter_t));
    if (!filter)
        return NULL;

    filter->buffer = touchscreen_malloc(sizeof(int) * length);
    filter->index = touchscreen_malloc(sizeof(int) * length);
    if (!filter->buffer || !filter->index)
    {
        if (filter->buffer)
            touchscreen_free(filter->buffer);
        if (filter->index)
            touchscreen_free(filter->index);
        touchscreen_free(filter);
        return NULL;
    }
    filter->length = length;
    filter->position = 0;
    filter->count = 0;

    return filter;
}

void median_free(struct median_filter_t *filter)
{
    if (filter)
    {
        if (filter->buffer)
            touchscreen_free(filter->buffer);
        if (filter->index)
            touchscreen_free(filter->index);
        touchscreen_free(filter);
    }
}

int median_update(struct median_filter_t *filter, int value)
{
    int pos = filter->position;
    int cnt = filter->count;
    int *idx;
    int cidx;
    int oidx;
    int oval;
    int result;
    if (cnt > 0)
    {
        if (cnt == filter->length)
        {
            oidx = 0;
            while (filter->index[oidx] != pos)
                ++oidx;
            oval = filter->buffer[pos];
        }
        else
        {
            filter->index[pos] = pos;
            oidx = pos;
            oval = INT_MAX;
        }

        filter->buffer[pos] = value;
        idx = &filter->index[oidx];
        if (oval < value)
        {
            while (++oidx != cnt)
            {
                cidx = *(++idx);
                if (filter->buffer[cidx] < value)
                {
                    *idx = *(idx - 1);
                    *(idx - 1) = cidx;
                }
                else
                {
                    break;
                }
            }
        }
        else if (oval > value)
        {
            while (oidx-- != 0)
            {
                cidx = *(--idx);
                if (filter->buffer[cidx] > value)
                {
                    *idx = *(idx + 1);
                    *(idx + 1) = cidx;
                }
                else
                {
                    break;
                }
            }
        }
        result = filter->buffer[filter->index[cnt / 2]];
    }
    else
    {
        filter->buffer[0] = value;
        filter->index[0] = 0;
        filter->position = 0;
        filter->count = 0;
        result = value;
    }

    pos++;
    filter->position = (pos == filter->length) ? 0 : pos;
    if (cnt < filter->length)
        filter->count++;

    return result;
}

void median_clear(struct median_filter_t *filter)
{
    if (filter)
    {
        filter->position = 0;
        filter->count = 0;
    }
}