
/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _BSP_ATOMIC_H
#define _BSP_ATOMIC_H

#ifdef __cplusplus
extern "C" {
#endif


#define SPINLOCK_INIT \
    {                 \
        0             \
    }

#define CORELOCK_INIT          \
    {                          \
        .lock = SPINLOCK_INIT, \
        .count = 0,            \
        .core = -1             \
    }


/* Defination of memory barrier macro */
#define mb()                          \
    {                                 \
        asm volatile("fence" ::       \
                         : "memory"); \
    }

#define atomic_set(ptr, val) (*(volatile typeof(*(ptr))*)(ptr) = val)
#define atomic_read(ptr) (*(volatile typeof(*(ptr))*)(ptr))

#ifndef __riscv_atomic
#error "atomic extension is required."
#endif
#define atomic_add(ptr, inc) __sync_fetch_and_add(ptr, inc)
#define atomic_or(ptr, inc) __sync_fetch_and_or(ptr, inc)
#define atomic_swap(ptr, swp) __sync_lock_test_and_set(ptr, swp)
#define atomic_cas(ptr, cmp, swp) __sync_val_compare_and_swap(ptr, cmp, swp)

typedef struct _spinlock
{
    int lock;
} spinlock_t;

typedef struct _semaphore
{
    spinlock_t lock;
    int count;
    int waiting;
} semaphore_t;


typedef struct _corelock
{
    spinlock_t lock;
    int count;
    int core;
} corelock_t;

static inline int spinlock_trylock(spinlock_t *lock)
{
    int res = atomic_swap(&lock->lock, -1);
    /* Use memory barrier to keep coherency */
    mb();
    return res;
}

static inline void spinlock_lock(spinlock_t *lock)
{
    while (spinlock_trylock(lock));
}

static inline void spinlock_unlock(spinlock_t *lock)
{
    /* Use memory barrier to keep coherency */
    mb();
    atomic_set(&lock->lock, 0);
    asm volatile ("nop");
}

static inline void semaphore_signal(semaphore_t *semaphore, int i)
{
    spinlock_lock(&(semaphore->lock));
    semaphore->count += i;
    spinlock_unlock(&(semaphore->lock));
}

static inline void semaphore_wait(semaphore_t *semaphore, int i)
{
    atomic_add(&(semaphore->waiting), 1);
    while (1)
    {
        spinlock_lock(&(semaphore->lock));
        if (semaphore->count >= i)
        {
            semaphore->count -= i;
            atomic_add(&(semaphore->waiting), -1);
            spinlock_unlock(&(semaphore->lock));
            break;
        }
        spinlock_unlock(&(semaphore->lock));
    }
}

static inline int semaphore_count(semaphore_t *semaphore)
{
    int res = 0;

    spinlock_lock(&(semaphore->lock));
    res = semaphore->count;
    spinlock_unlock(&(semaphore->lock));
    return res;
}

static inline int semaphore_waiting(semaphore_t *semaphore)
{
    return atomic_read(&(semaphore->waiting));
}

static inline int corelock_trylock(corelock_t *lock)
{
    int res = 0;
    unsigned long core;

    asm volatile("csrr %0, mhartid;"
                 : "=r"(core));
    if(spinlock_trylock(&lock->lock))
    {
        return -1;
    }

    if (lock->count == 0)
    {
        /* First time get lock */
        lock->count++;
        lock->core = core;
        res = 0;
    }
    else if (lock->core == core)
    {
        /* Same core get lock */
        lock->count++;
        res = 0;
    }
    else
    {
        /* Different core get lock */
        res = -1;
    }
    spinlock_unlock(&lock->lock);

    return res;
}

static inline void corelock_lock(corelock_t *lock)
{
    unsigned long core;

    asm volatile("csrr %0, mhartid;"
                 : "=r"(core));
    spinlock_lock(&lock->lock);

    if (lock->count == 0)
    {
        /* First time get lock */
        lock->count++;
        lock->core = core;
    }
    else if (lock->core == core)
    {
        /* Same core get lock */
        lock->count++;
    }
    else
    {
        /* Different core get lock */
        spinlock_unlock(&lock->lock);

        do
        {
            while (atomic_read(&lock->count))
                ;
        } while (corelock_trylock(lock));
        return;
    }
    spinlock_unlock(&lock->lock);
}

static inline void corelock_unlock(corelock_t *lock)
{
    unsigned long core;

    asm volatile("csrr %0, mhartid;"
                 : "=r"(core));
    spinlock_lock(&lock->lock);

    if (lock->core == core)
    {
        /* Same core release lock */
        lock->count--;
        if (lock->count <= 0)
        {
            lock->core = -1;
            lock->count = 0;
        }
    }
    else
    {
        /* Different core release lock */
        spinlock_unlock(&lock->lock);

        register unsigned long a7 asm("a7") = 93;
        register unsigned long a0 asm("a0") = 0;
        register unsigned long a1 asm("a1") = 0;
        register unsigned long a2 asm("a2") = 0;

        asm volatile("scall"
                     : "+r"(a0)
                     : "r"(a1), "r"(a2), "r"(a7));
    }
    spinlock_unlock(&lock->lock);
}

#ifdef __cplusplus
}
#endif

#endif /* _BSP_ATOMIC_H */

