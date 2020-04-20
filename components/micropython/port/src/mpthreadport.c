/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George on behalf of Pycom Ltd
 * Copyright (c) 2017 Pycom Limited
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>

#include "py/mpconfig.h"
#include "py/mpstate.h"
#include "py/gc.h"
#include "py/mpthread.h"
#include "mpthreadport.h"


#if MICROPY_PY_THREAD

#include "task.h"


// this structure forms a linked list, one node per active thread
typedef struct _thread_t {
    TaskHandle_t id;        // system id of thread
    int ready;              // whether the thread is ready and running
    void *arg;              // thread Python args, a GC root pointer
    void *stack;            // pointer to the stack
    size_t stack_len;       // number of words in the stack
    struct _thread_t *next;
} __attribute__((aligned(8))) thread_t;

// the mutex controls access to the linked list
STATIC volatile mp_thread_mutex_t thread_mutex;
STATIC volatile thread_t thread_entry0;
STATIC volatile thread_t *thread; // root pointer, handled by mp_thread_gc_others
STATIC volatile uint32_t thread_num; // 

void mp_thread_init(void *stack, uint32_t stack_len) {
    mp_thread_set_state(&mp_state_ctx.thread);
    // create the first entry in the linked list of all threads
    thread = &thread_entry0;
    thread->id = xTaskGetCurrentTaskHandle();
    thread->ready = 1;
    thread->arg = NULL;
    thread->stack = stack;
    thread->stack_len = stack_len;
    thread->next = NULL;
    mp_thread_mutex_init(&thread_mutex);
}

void mp_thread_gc_others(void) {
    mp_thread_mutex_lock(&thread_mutex, 1);
    for (thread_t *th = thread; th != NULL; th = th->next) {
        gc_collect_root((void**)&th, 1);
        gc_collect_root(&th->arg, 1); // probably not needed
        if (th->id == xTaskGetCurrentTaskHandle()) {
            continue;
        }
        if (!th->ready) {
            continue;
        }
        gc_collect_root(th->stack, th->stack_len); // probably not needed
    }
    mp_thread_mutex_unlock(&thread_mutex);

    // int n_th = 0;
    // void **ptrs;
    // mp_state_thread_t *state;

    // mp_thread_mutex_lock(&thread_mutex, 1);
    // for (thread_t *th = thread; th != NULL; th = th->next) {
    //     if (!th->ready) continue;                               // thread not ready
	// 	//if (th->type == THREAD_TYPE_SERVICE) continue;          // Only scan PYTHON threads
    //     if (th->id == xTaskGetCurrentTaskHandle()) continue;    // Do not process the running thread

    //     //state = (mp_state_thread_t *)th->state_thread;
    //     n_th++;

    //     // Mark the root pointers on thread
    //     //gc_collect_root((void **)state->dict_locals, 1);

    //     if (th->arg) {
    //         // Mark the pointers on thread arguments
    //         ptrs = (void**)(void*)&th->arg;
    //         gc_collect_root(ptrs, 1);
    //     }

    //     #if MICROPY_ENABLE_PYSTACK
    //     // Mark the pointers on thread pystack
    //     //ptrs = (void**)(void*)state->pystack_start;
    //     //gc_collect_root(ptrs, (state->pystack_cur - state->pystack_start) / sizeof(void*));
    //     #endif

    //     // If PyStack is used, no pointers to MPy heap are placed on tasks stack
    //     #if !MICROPY_ENABLE_PYSTACK
    //     // Mark the pointers on thread stack
    //     //gc_collect_root(th->curr_sp, ((void *)state->stack_top - th->curr_sp) / sizeof(void*)); // probably not needed
    //     #endif
    // }
    // mp_thread_mutex_unlock(&thread_mutex);
    // return n_th;
	
}

mp_state_thread_t *mp_thread_get_state(void) {
    return pvTaskGetThreadLocalStoragePointer(NULL, 0);
}

void mp_thread_set_state(struct _mp_state_thread_t *state) {
    vTaskSetThreadLocalStoragePointer(NULL, 0, state);
}

void mp_thread_start(void) {
    mp_thread_mutex_lock(&thread_mutex, 1);
    for (thread_t *th = thread; th != NULL; th = th->next) {
        if (th->id == xTaskGetCurrentTaskHandle()) {
            th->ready = 1;
            break;
        }
    }
    mp_thread_mutex_unlock(&thread_mutex);
}

STATIC void *(*ext_thread_entry)(void*) = NULL;
STATIC void freertos_entry(void *arg) {
    if (ext_thread_entry) {
        ext_thread_entry(arg);
    }
    vTaskDelete(NULL);
    for (;;);
}

#define MP_THREAD_MIN_STACK_SIZE                        (4 * 1024)
#define MP_THREAD_DEFAULT_STACK_SIZE                    (MP_THREAD_MIN_STACK_SIZE + 1024)
#define MP_THREAD_PRIORITY                              4

void mp_thread_create_ex(void *(*entry)(void*), void *arg, size_t *stack_size, int priority, char *name) {
    // store thread entry function into a global variable so we can access it
    ext_thread_entry = entry;

    if (*stack_size == 0) {
        *stack_size = MP_THREAD_DEFAULT_STACK_SIZE; // default stack size
    } else if (*stack_size < MP_THREAD_MIN_STACK_SIZE) {
        *stack_size = MP_THREAD_MIN_STACK_SIZE; // minimum stack size
    }

    // allocate TCB, stack and linked-list node (must be outside thread_mutex lock)
    StackType_t *stack = NULL;

    thread_t *th = m_new_obj(thread_t);

    mp_thread_mutex_lock(&thread_mutex, 1);

    // create thread
    TaskHandle_t thread_id;
    TaskStatus_t task_status;
	//todo add schedule processor
    xTaskCreateAtProcessor(0, // processor
						   freertos_entry, // function entry
						   name, //task name
						   *stack_size / sizeof(StackType_t), //stack_deepth
						   arg, //function arg
						   priority, //task priority,please don't change this parameter,because it will impack function running
						   &thread_id);//task handle
    //mp_printf(&mp_plat_print, "[MAIXPY]: thread_id %p created \n",thread_id);
    if (thread_id == NULL) {
        m_del_obj(thread_t,th);
        mp_thread_mutex_unlock(&thread_mutex);
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "can't create thread"));
    }
	vTaskGetInfo(thread_id,&task_status,(BaseType_t)pdTRUE,(eTaskState)eInvalid);
	stack = task_status.pxStackBase;
    // adjust the stack_size to provide room to recover from hitting the limit
    *stack_size -= 1024;

    // add thread to linked list of all threads
    th->id = thread_id;
    th->ready = 0;
    th->arg = arg;
    th->stack = stack;
	//stack_len may be a bug,because that k210 addr width is 64 bit ,but addr width is 32bit
	//the StackType_t type is a type of the uintprt_t,uintprt_t in k210 is 64bit 
    th->stack_len = *stack_size / sizeof(StackType_t);
    th->next = thread;
    thread = th;
    mp_thread_mutex_unlock(&thread_mutex);

}

void mp_thread_create(void *(*entry)(void*), void *arg, size_t *stack_size) {
    uint8_t thread_name[15] = {0};
    sprintf(thread_name,"mp_thread%d",thread_num);
    mp_thread_create_ex(entry, arg, stack_size, MP_THREAD_PRIORITY, thread_name);
    thread_num++;
}

void mp_thread_finish(void) {
    mp_thread_mutex_lock(&thread_mutex, 1);
    thread_t *th = thread;
    thread_t *pre_th = NULL;
    for (th = thread; th != NULL; th = th->next) {
        if (th->id == xTaskGetCurrentTaskHandle()) {
            th->ready = 0;
            break;
        }
        pre_th = th;
    }
    mp_thread_mutex_unlock(&thread_mutex);
}

void mp_thread_mutex_init(mp_thread_mutex_t *mutex) {
    mutex->handle = xSemaphoreCreateMutexStatic(&mutex->buffer);
}

int mp_thread_mutex_lock(mp_thread_mutex_t *mutex, int wait) {
    return (pdTRUE == xSemaphoreTake(mutex->handle, wait ? portMAX_DELAY : 0));
}

void mp_thread_mutex_unlock(mp_thread_mutex_t *mutex) {
    xSemaphoreGive(mutex->handle);
}


void mp_thread_deinit(void) {

    mp_thread_mutex_lock(&thread_mutex, 1);
    for (thread_t *th = thread; th != NULL; th = th->next) {
        // don't delete the current task
        if (th->id == xTaskGetCurrentTaskHandle()) {
            continue;
        }
        vTaskDelete(th->id);
        m_del_obj(thread_t,th);
    }
    mp_thread_mutex_unlock(&thread_mutex);
    // allow FreeRTOS to clean-up the threads
    vTaskDelay(2);

}

// free mem, need set CONFIG_STATIC_TASK_CLEAN_UP_ENABLE=y in Kconfig
void vPortCleanUpTCB(void *tcb) {
    if (thread == NULL) {
        // threading not yet initialised
        return;
    }
    thread_t *prev = NULL;
    mp_thread_mutex_lock(&thread_mutex, 1);
    for (thread_t *th = thread; th != NULL; prev = th, th = th->next) {
        // unlink the node from the list
        if ((void*)th->id == tcb) {
            if (prev != NULL) {
                prev->next = th->next;
            } else {
                // move the start pointer
                thread = th->next;
            }
            // explicitly release all its memory
            m_del(thread_t, th, 1);
            break;
        }
    }
    mp_thread_mutex_unlock(&thread_mutex);
}
#else
void vPortCleanUpTCB(void *tcb) {

}
#endif // MICROPY_PY_THREAD
