#ifndef __TSCAL_H
#define __TSCAL_H

#include <stdio.h>

struct tscal_t
{
    int x[5], xfb[5];
    int y[5], yfb[5];
    int a[7];
};

int do_tscal(struct ts_ns2009_pdata_t *ts_ns2009_pdata);

#endif