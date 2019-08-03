
#include "stdio.h"
#include "stdint.h"
#include "global_config.h"
#include "syscalls.h"
#include "encoding.h"
#include "printf.h"
#include "assert.h"
#include "uarths.h"
#include "stdlib.h"
#include "fpioa.h"
#include "dump.h"
#include "w25qxx.h"
#include "printf.h"

#define print printk //comment printk's core lock



#define INSERT_FIELD(val, which, fieldval) (((val) & ~(which)) | ((fieldval) * ((which) & ~((which)-1))))


#define CE_TEXT_FLASH  __attribute__((section(".text_flash")))
#define CE_RODATA_FLASH  __attribute__((section(".rodata_flash")))

#define CE_DEBUG_PRINT(fmt, ...) print(fmt, ##__VA_ARGS__)
#define CE_ERROR_PRINT(fmt, ...) print(fmt, ##__VA_ARGS__)

#define CE_PAGE_SIZE (4096)
#define CE_BLOCK_SIZE_IN_PAGES (4)
#define CE_BLOCK_SIZE (CE_PAGE_SIZE * CE_BLOCK_SIZE_IN_PAGES)

#define CE_LV3_PAGE_TABLE_COUNT (32)

#define CE_CACHE_VA_SPACE_SIZE_IN_BLOCKS (512 * CE_LV3_PAGE_TABLE_COUNT)
#define CE_CACHE_VA_SPACE_SIZE_IN_PAGES (CE_CACHE_VA_SPACE_SIZE_IN_BLOCKS * CE_BLOCK_SIZE_IN_PAGES)
#define CE_CACHE_VA_SPACE_SIZE (CE_CACHE_VA_SPACE_SIZE_IN_PAGES * CE_PAGE_SIZE)

#define CE_CACHE_SIZE_IN_BLOCKS ((3072 / 4) / CE_BLOCK_SIZE_IN_PAGES)
#define CE_CACHE_SIZE_IN_PAGES (CE_CACHE_SIZE_IN_BLOCKS * CE_BLOCK_SIZE_IN_PAGES)

uint64_t volatile __attribute__((aligned(CE_PAGE_SIZE)))  ceLv1PageTable[(CE_PAGE_SIZE / 8) * (1)];
uint64_t volatile __attribute__((aligned(CE_PAGE_SIZE)))  ceLv2PageTables[(CE_PAGE_SIZE / 8) * (1)];
uint64_t volatile __attribute__((aligned(CE_PAGE_SIZE)))  ceLv3PageTables[(CE_PAGE_SIZE / 8) * (CE_LV3_PAGE_TABLE_COUNT)];
uint64_t  __attribute__((aligned(CE_PAGE_SIZE)))  ceCacheMemory[(CE_PAGE_SIZE / 8) * CE_CACHE_SIZE_IN_PAGES];

uint16_t ceCacheMemoryBlockAge[CE_CACHE_SIZE_IN_BLOCKS];
uint16_t ceCacheMemoryBlockToVBlockId[CE_CACHE_SIZE_IN_BLOCKS];

static const uint64_t ceVABase = 0xC0000000ULL;
static int ceIsMapWritable;

static uint64_t ceEncodePTE(uint32_t physAddr, uint32_t flags) {
    // assert((physAddr % CE_PAGE_SIZE) == 0);
    return (((uint64_t)physAddr >> 12) << 10) | flags;
}

void ceResetCacheState() {
    memset(ceCacheMemoryBlockAge, 0, sizeof(ceCacheMemoryBlockAge));
    memset((void*)ceLv3PageTables, 0, sizeof(ceLv3PageTables));
    ceIsMapWritable = 0;
    asm volatile ("sfence.vm");
}

CE_RODATA_FLASH int test_data[] = {1, 2, 3, 4 ,5 ,6 };
CE_RODATA_FLASH int test_data2[] = {100, 101, 102, 103, 104, 105, 106 };

CE_RODATA_FLASH char ccc[] = "123456789012345678901234567890\r\n";

void CE_TEXT_FLASH test()
{
    print(ccc);
    uarths_putchar('a');
    uarths_putchar('a');
    uarths_putchar('a');
    uarths_putchar('a');
    uarths_putchar('\r');
    uarths_putchar('\n');    
}

void test2()
{
    print(ccc);
}


void ceSetupMMU() {

    CE_DEBUG_PRINT("setup mmu...\r\n");
    //0 - 3GiB -> mirror to phys
    for (uint32_t i = 0; i < 3; i++) {
        ceLv1PageTable[i] = ceEncodePTE((0x40000000U) * i,  PTE_V | PTE_R | PTE_W | PTE_X | PTE_G | PTE_U);
    }

    //0x100000000 3GiB-4GiB (1GiB) -> lv2
    ceLv1PageTable[3] = ceEncodePTE((uint32_t)ceLv2PageTables, PTE_V | PTE_G | PTE_U  );

    //0x100000000 (2MiB * CE_LV3_PAGE_TABLE_COUNT) -> lv3
    for (uint32_t i = 0; i < CE_LV3_PAGE_TABLE_COUNT; i++) {
        ceLv2PageTables[i] = ceEncodePTE(((uint32_t)ceLv3PageTables) + i * CE_PAGE_SIZE,  PTE_V | PTE_G | PTE_U);
    }

    write_csr(sptbr, (uint64_t)ceLv1PageTable >> 12);

    uint64_t msValue = read_csr(mstatus);
    msValue |= MSTATUS_MPRV | ((uint64_t)VM_SV39 << 24);
    // msValue = INSERT_FIELD(msValue, MSTATUS_MPP, PRV_S);
    write_csr(mstatus, msValue);

    ceResetCacheState();
}

static inline uint32_t ceVAddrToVBlockId(uintptr_t vaddr) {
    return (vaddr - ceVABase) / (CE_BLOCK_SIZE);
}

static void ceMapVBlockToPhysAddr(uint32_t vBlockId, uint32_t physAddr) {
    uint32_t basePageId = vBlockId * CE_BLOCK_SIZE_IN_PAGES;
    uintptr_t vaddr = 0;
    for (uint32_t i = 0 ; i < CE_BLOCK_SIZE_IN_PAGES; i++) {
        ceLv3PageTables[basePageId + i] = physAddr ? ceEncodePTE(physAddr + i * CE_PAGE_SIZE, PTE_V | PTE_R | PTE_X | PTE_G | PTE_U) : 0;
        // ceLv3PageTables[basePageId + i] = ceEncodePTE(test2, PTE_V | PTE_R | PTE_X | PTE_G | PTE_U);
        vaddr = ceVABase + ((uintptr_t)(basePageId + i) * CE_PAGE_SIZE);
        // print("map:%lx %x\r\n", vaddr, physAddr + i * CE_PAGE_SIZE);
        asm volatile("sfence.vm %0" : "=r"(vaddr));
    }
}

int ceFileReadCallback(uint32_t fileOffset, uint64_t* buf, uint32_t len)
{
    // memset(buf, 0x00, len);
    // if(fileOffset == 0)
    //     ((int*)buf)[0] = 123;
    // else
    //     snprintf((char*)buf, len, "hello, this str copied from xxx\r\n");
    // memcpy(buf, test2, 32);
    w25qxx_read_data_dma(0x200000+fileOffset, (uint8_t*)buf,  len, W25QXX_QUAD_FAST);
    // print("read from flash, %d %p %d\r\n",fileOffset, buf, len);
    // uint32_t* p = (uint32_t*)buf;
    // print("%x %x %x %x\r\n", p[0],p[1],p[2],p[3]);
    return 0;
}

static uint32_t ceFindBlockToRetire() {

    uint16_t maxAge = 0;
    uint32_t maxAgeAt = 0;

    for (uint32_t i = 0; i < CE_CACHE_SIZE_IN_BLOCKS; i++) {
        uint16_t age = ceCacheMemoryBlockAge[i];
        if (age == 0) {
            // an empty block!
            return i;
        }
        if (age >= maxAge) {
            maxAge = age;
            maxAgeAt = i;
        }
    }
    return maxAgeAt;
}

static inline int ceCheckAndSetVBlockAccessFlag(uint32_t vBlockId) {
    int hasAccessed = 0;

    uint32_t basePageId = vBlockId * CE_BLOCK_SIZE_IN_PAGES;
    for (uint32_t i = 0; i < CE_BLOCK_SIZE_IN_PAGES; i++) {
        uint64_t pte = ceLv3PageTables[basePageId + i];
        assert(pte & PTE_V);
        if (pte & PTE_A) {
            // TODO: ensure this operation is atomic
            ceLv3PageTables[basePageId + i] &= (~((uint64_t)PTE_A));
            hasAccessed = 1;
        }
    }
    return hasAccessed;
}


void ceUpdateBlockAge() {
    for (uint32_t i = 0; i < CE_CACHE_SIZE_IN_BLOCKS; i++) {
        uint16_t age = ceCacheMemoryBlockAge[i];
        if (age == 0) {
            // an empty block!
            continue;
        }
        int hasAccessed = ceCheckAndSetVBlockAccessFlag(ceCacheMemoryBlockToVBlockId[i]);
        if (!hasAccessed) {
            if (age < UINT16_MAX) {
                age ++;
                ceCacheMemoryBlockAge[i] = age;
            }
        } else {
            age = 1;
            ceCacheMemoryBlockAge[i] = age;
        }
        //CE_DEBUG_PRINT("ceUpdateBlockAge: %d, %d\n", i, age);
    }
}


int ceHandlePageFault(uintptr_t vaddr, int isWrite) {
    if (isWrite) {
        if (!ceIsMapWritable) {
            return -1;
        }
    }

    uint32_t cacheBlockId = ceFindBlockToRetire();
    CE_DEBUG_PRINT("ceHandlePageFault: %p, %d\r\n", (void*)vaddr, cacheBlockId);
    if (ceCacheMemoryBlockAge[cacheBlockId]) {
        CE_DEBUG_PRINT("retire block: %d\r\n", (int) ceCacheMemoryBlockToVBlockId[cacheBlockId]);
        // an used block, free it
        ceMapVBlockToPhysAddr(ceCacheMemoryBlockToVBlockId[cacheBlockId], 0);
    }
    ceCacheMemoryBlockAge[cacheBlockId] = 0;

    uint32_t vBlockId = ceVAddrToVBlockId(vaddr);
    uint32_t physAddr = ((uint32_t) ceCacheMemory) + (CE_BLOCK_SIZE * cacheBlockId);
    
    int ret = ceFileReadCallback(vBlockId * CE_BLOCK_SIZE, (uint64_t*)physAddr, CE_BLOCK_SIZE);
    if (ret != 0) {
        CE_ERROR_PRINT("ceHandlePageFault: file read failed, %p, %d\r\n", (void*)vaddr, ret);
        return -1;
    }

    ceCacheMemoryBlockAge[cacheBlockId] = 1;
    ceCacheMemoryBlockToVBlockId[cacheBlockId] = (uint16_t) vBlockId;
    ceMapVBlockToPhysAddr(vBlockId, physAddr);
    // print("===%p %p %p\r\n", test2, (void*)physAddr, ceCacheMemory);
    // ((void(*)(void))physAddr)();
    return 0;
}



int main()
{
    fpioa_set_function(4, FUNC_UARTHS_RX);
	fpioa_set_function(5,  FUNC_UARTHS_TX);
    uarths_init();
	uarths_config(115200, 1);

    uint8_t manuf_id, device_id;
    w25qxx_init_dma(3, 0);
	w25qxx_enable_quad_mode_dma();
	w25qxx_read_id_dma(&manuf_id, &device_id);
    print("flash id:%x %x\r\n",manuf_id, device_id);
    



    ceSetupMMU();
    // int* p0 = (int*)ceVABase;
    // char* p = ((char*)(ceVABase+2*1024*1024));
    int a = 5;
    // while(a--)
    uarths_putchar('1');
    uarths_putchar('1');
    uarths_putchar('1');
    uarths_putchar('1');
    uarths_putchar('\r');
    uarths_putchar('\n');    
    {
        // print("hello maixpy %p %d %c %c %s\r\n", p, p0[0], p[0], p[1], (char*)p );
        print("%p\r\n", &a);
        print("--%p\r\n", test);
        print("%d %d %d %d\r\n", test_data[0], test_data[1], test_data[2], test_data[3]);
        print("%d %d %d %d\r\n", test_data2[0], test_data2[1], test_data2[2], test_data2[3]);
        test();
        // print("%d\r\n", p0[0]);
        // ((void (*)(void))p0)();

        // print("1111\r\n");
    }
    print("test end\r\n");
    print("test2\r\n");
    int core = current_coreid();
    print("%d\r\n", core);
    print("test2 end\r\n");
    while(1){}
}

uintptr_t handle_fault_load(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32]) {
    uintptr_t badAddr = read_csr(mbadaddr);

    if ((badAddr >= ceVABase) && (badAddr < (ceVABase + CE_CACHE_VA_SPACE_SIZE))) {
        if (ceHandlePageFault(badAddr, 0) == 0) {
            // uint64_t msValue = read_csr(mstatus);
            // msValue |=  MSTATUS_MPP;
            // write_csr(mstatus, msValue);
            return epc;
        }
    }
    sys_exit(1337);
    return epc;
}

uintptr_t handle_illegal_instruction(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32])
{
    // uintptr_t badAddr = read_csr(mbadaddr);
    if ((epc >= ceVABase) && (epc < (ceVABase + CE_CACHE_VA_SPACE_SIZE))) {
        if (ceHandlePageFault(epc, 0) == 0) {
            // uint64_t msValue = read_csr(mstatus);
            // msValue |=  MSTATUS_MPP;
            // write_csr(mstatus, msValue);
            return epc;
        }
    }
    dump_core("illegal instruction", cause, epc, regs, fregs);
    sys_exit(1337);
    return epc;
}


