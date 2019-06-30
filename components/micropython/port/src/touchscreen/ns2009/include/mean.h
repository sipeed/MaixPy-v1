#ifndef __MEAN_H__
#define __MEAN_H__

#ifdef __cplusplus
extern "C"
{
#endif

    struct mean_filter_t
    {
        int *buffer;
        int length;
        int index;
        int count;
        int sum;
    };

    struct mean_filter_t *mean_alloc(int length);
    void mean_free(struct mean_filter_t *filter);
    int mean_update(struct mean_filter_t *filter, int value);
    void mean_clear(struct mean_filter_t *filter);

#ifdef __cplusplus
}
#endif

#endif /* __MEAN_H__ */