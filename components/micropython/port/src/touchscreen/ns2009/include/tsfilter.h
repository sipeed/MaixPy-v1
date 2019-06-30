#ifndef __TSFILTER_H__
#define __TSFILTER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <median.h>
#include <mean.h>

    struct tsfilter_t
    {
        struct median_filter_t *mx, *my;
        struct mean_filter_t *nx, *ny;
        int cal[7];
    } __attribute__((aligned(8)));

    struct tsfilter_t *tsfilter_alloc(int ml, int nl);
    void tsfilter_free(struct tsfilter_t *filter);
    void tsfilter_setcal(struct tsfilter_t *filter, int *cal);
    void tsfilter_update(struct tsfilter_t *filter, int *x, int *y);
    void tsfilter_clear(struct tsfilter_t *filter);

#ifdef __cplusplus
}
#endif

#endif /* __TSFILTER_H__ */