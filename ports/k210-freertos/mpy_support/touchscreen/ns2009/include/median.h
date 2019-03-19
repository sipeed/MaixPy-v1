#ifndef __MEDIAN_H__
#define __MEDIAN_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define INT_MIN (-1 - 0x7fffffff)
#define INT_MAX 0x7fffffff

    struct median_filter_t
    {
        int *buffer;
        int *index;
        int length;
        int position;
        int count;
    };

    struct median_filter_t *median_alloc(int length);
    void median_free(struct median_filter_t *filter);
    int median_update(struct median_filter_t *filter, int value);
    void median_clear(struct median_filter_t *filter);

#ifdef __cplusplus
}
#endif

#endif /* __MEDIAN_H__ */