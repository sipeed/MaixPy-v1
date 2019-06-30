#ifndef __MACHINE_TIMER_H__
#define __MACHINE_TIMER_H__

#include "py/obj.h"
#include "timer.h"

typedef enum{
    MACHINE_TIMER_MODE_ONE_SHOT = 0,
    MACHINE_TIMER_MODE_PERIODIC,
    MACHINE_TIMER_MODE_PWM,
    MACHINE_TIMER_MODE_MAX
}machine_timer_mode_t;

typedef enum{
    MACHINE_TIMER_UNIT_NS = 0 ,
    MACHINE_TIMER_UNIT_US ,
    MACHINE_TIMER_UNIT_MS ,
    MACHINE_TIMER_UNIT_S  ,
    MACHINE_TIMER_UNIT_MAX
}machine_timer_unit_t;

typedef struct _machine_timer_obj_t {
    mp_obj_base_t          base;
    timer_device_number_t  timer;     //[0,2]
    timer_channel_number_t channel;   //[0,3]
	mp_uint_t              period;    //(0,2^32)
    machine_timer_unit_t   unit;      //machine_timer_unit_t
    machine_timer_mode_t   mode;      //machine_timer_mode_t
	mp_obj_t               callback;
    mp_obj_t               arg;
    uint32_t               priority;  //[1,7]
    uint32_t               div;       //freq div [1,8]
    bool                   active;
} machine_timer_obj_t;


#endif

