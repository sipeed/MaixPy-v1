/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013-2016 Kwabena W. Agyeman <kwagyeman@openmv.io>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Interface for using extra frame buffer RAM as a stack.
 *
 */
#include <mp.h>
#include "fb_alloc.h"
#include "framebuffer.h"
#include "omv_boardconfig.h"


#define OMV_FB_ALLOC_SIZE 1*1024*1024
char fballoc_start[OMV_FB_ALLOC_SIZE] __attribute__((aligned(64)));;
char* _fballoc = NULL;
static char *pointer = NULL;

#define nlr_raise_for_fb_alloc_mark(obj) nlr_raise(obj)

NORETURN void fb_alloc_fail()
{
    nlr_raise(mp_obj_new_exception_msg(&mp_type_MemoryError,
        "Out of fast Frame Buffer Stack Memory!"
        " Please reduce the resolution of the image you are running this algorithm on to bypass this issue!"));
}

void fb_alloc_init_once()
{
    
}

void fb_alloc_init0()
{
    _fballoc = fballoc_start + OMV_FB_ALLOC_SIZE; 
    pointer = _fballoc;
}

uint64_t fb_avail()
{
    int64_t temp = pointer - fballoc_start - sizeof(uint64_t);

    return (temp < sizeof(uint64_t)) ? 0 : temp;
}

void fb_alloc_mark()
{
    char *new_pointer = pointer - sizeof(uint64_t);

    // Check if allocation overwrites the framebuffer pixels
    if (new_pointer < fballoc_start) {
        nlr_raise_for_fb_alloc_mark(mp_obj_new_exception_msg(&mp_type_MemoryError,
            "Out of fast Frame Buffer Stack Memory!"
            " Please reduce the resolution of the image you are running this algorithm on to bypass this issue!"));
    }

    // fb_alloc does not allow regions which are a size of 0 to be alloced,
    // meaning that the value below is always 8 or more but never 4. So,
    // we will use a size value of 4 as a marker in the alloc stack.
    *((uint64_t *) new_pointer) = sizeof(uint64_t); // Save size.
    pointer = new_pointer;
}

void fb_alloc_free_till_mark()
{
    while (pointer < _fballoc) {
        int size = *((uint64_t *) pointer);
        pointer += size; // Get size and pop.
        if (size == sizeof(uint64_t)) break; // Break on first marker.
    }
}

// returns null pointer without error if size==0
void *fb_alloc(uint64_t size)
{
    if (!size) {
        return NULL;
    }
    size=((size+sizeof(uint64_t)-1)/sizeof(uint64_t))*sizeof(uint64_t);// Round Up
    char *result = pointer - size;
    char *new_pointer = result - sizeof(uint64_t);
    // Check if allocation overwrites the framebuffer pixels
    if (new_pointer < fballoc_start) {
        fb_alloc_fail();
    }
    // size is always 4/8/12/etc. so the value below must be 8 or more.
    *((uint64_t *) new_pointer) = size + sizeof(uint64_t); // Save size.
    pointer = new_pointer;
	//printf("###fb_alloc: p-> 0x%x\n", pointer);
    return result;
}

// returns null pointer without error if passed size==0
void *fb_alloc0(uint64_t size)
{
    void *mem = fb_alloc(size);
    memset(mem, 0, size); // does nothing if size is zero.
    return mem;
}

void *fb_alloc_all(uint64_t *size)
{
	//printf("#size addr=0x%x\n", size);
    int32_t temp = pointer - fballoc_start - sizeof(uint64_t);
    if (temp < sizeof(uint64_t)) {
        *size = 0;
        return NULL;
    }
	temp = (temp / sizeof(uint64_t)) * sizeof(uint64_t);
    *size = (uint64_t)temp; // Round Down
    char *result = pointer - *size;
    char *new_pointer = result - sizeof(uint64_t);

    // size is always 4/8/12/etc. so the value below must be 8 or more.
    *((uint64_t *) new_pointer) = *size + sizeof(uint64_t); // Save size.
    pointer = new_pointer;
    return result;
}

// returns null pointer without error if returned size==0
void *fb_alloc0_all(uint64_t *size)
{
    void *mem = fb_alloc_all(size);
    memset(mem, 0, *size); // does nothing if size is zero.
    return mem;
}

void fb_free()
{
    if (pointer < _fballoc) {
        pointer += *((uint64_t *) pointer); // Get size and pop.
    }
}

void fb_free_all()
{
    while (pointer < _fballoc) {
        pointer += *((uint64_t *) pointer); // Get size and pop.
    }
}
