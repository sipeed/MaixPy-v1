#ifndef __TOUCHSCREEN_H_
#define __TOUCHSCREEN_H_


typedef enum{
    TOUCHSCREEN_TYPE_IDLE =0,
    TOUCHSCREEN_TYPE_RELEASE,
    TOUCHSCREEN_TYPE_PRESS
} touchscreen_type_t;

int touchscreen_init(void* arg);
int touchscreen_read(int* type, int* x, int* y);
int touchscreen_deinit();
int touchscreen_calibrate();

/////////////// HAL ////////////////////

#include "machine_i2c.h"
#include "gc.h"
#include "mp.h"

#define CALIBRATION_SIZE 7

typedef struct 
{
    machine_hard_i2c_obj_t* i2c;
    int calibration[CALIBRATION_SIZE];          // 7 Bytes
} touchscreen_config_t;

//TODO: replace heap with GC
// #define touchscreen_malloc(p) m_malloc(p)
// #define touchscreen_free(p) m_free(p, gc_nbytes(p))
#define touchscreen_malloc(p) malloc(p)
#define touchscreen_free(p) free(p)

#endif

