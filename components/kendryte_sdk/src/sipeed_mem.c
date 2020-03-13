#include "stdint.h"
#include <stdlib.h>
#include "iomem.h"
#include "sipeed_mem.h"

extern char _heap_start[];
extern char _heap_end[];

extern char *_heap_cur;
extern char *_heap_line;
extern char *_ioheap_line;

typedef struct _iomem_malloc_t
{
    void (*init)();
    uint32_t (*unused)();
    uint8_t *membase;
    uint32_t memsize;
    uint32_t memtblsize;
    uint16_t *memmap;
    uint8_t  memrdy;
    _lock_t *lock;
} iomem_malloc_t;

extern iomem_malloc_t malloc_cortol;


void sipeed_reset_sys_mem()
{
    //TODO:
    //FIXME:
}

size_t get_free_heap_size2(void)
{
    size_t unused=0;
    unused = (uintptr_t)_ioheap_line + 0x40000000 - (uintptr_t)_heap_cur;
    return unused;
}

