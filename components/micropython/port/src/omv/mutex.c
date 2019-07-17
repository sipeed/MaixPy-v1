/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Mutex.
 *
 */
#include "mutex.h"
#include "task.h"
#include "semphr.h"

// This is a standard implementation of mutexs on ARM processors following the ARM guide.
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0321a/BIHEJCHB.html

//TODO: optimize mutex

void mutex_init(mutex_t *mutex)
{
	mutex->lock = 0;
	mutex->tid = 0;
}

void mutex_lock(mutex_t *mutex, uint32_t tid)
{
	// Wait for mutex to be unlocked
	while(mutex->lock != 0);
	mutex->lock = 1;
	mutex->tid = tid;

}


int mutex_try_lock(mutex_t *mutex, uint32_t tid)
{
    // If mutex is already locked by the current thread then
    // release the Kraken err.. the mutex, else attempt to lock it.
    if (mutex->tid == tid) {
        mutex_unlock(mutex, tid);
    } //TODO: complete mutex
	else if (mutex->lock == 0) 
	{
		mutex->lock = 1;
        mutex->tid = tid;
		return 1;
	}
    return 0;
}


void mutex_unlock(mutex_t *mutex, uint32_t tid)
{
	if(mutex->tid == tid)
	{
		mutex->lock = 0;
		mutex->tid = 0;
	}
}

