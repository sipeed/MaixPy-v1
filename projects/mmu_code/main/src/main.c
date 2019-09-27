
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
#include "dmac.h"
#include "w25qxx.h"
#include "printf.h"
#include "sysctl.h"

int usermode=0;

int printk2(const char *format, ...);
#define print printk2 //no core lock

#define INSERT_FIELD(val, which, fieldval) (((val) & ~(which)) | ((fieldval) * ((which) & ~((which)-1))))

#define CE_TEXT_FLASH  __attribute__((section(".text_flash")))
#define CE_RODATA_FLASH  __attribute__((section(".rodata_flash")))

#define CE_DEBUG_PRINT(fmt, ...) //print(fmt, ##__VA_ARGS__)
#define CE_ERROR_PRINT(fmt, ...) //print(fmt, ##__VA_ARGS__)

#define CE_PAGE_SIZE (4096)
#define CE_BLOCK_SIZE_IN_PAGES (4)
#define CE_BLOCK_SIZE (CE_PAGE_SIZE * CE_BLOCK_SIZE_IN_PAGES)

#define CE_LV3_PAGE_TABLE_COUNT (32)

#define CE_CACHE_VA_SPACE_SIZE_IN_BLOCKS (512 * CE_LV3_PAGE_TABLE_COUNT)
#define CE_CACHE_VA_SPACE_SIZE_IN_PAGES (CE_CACHE_VA_SPACE_SIZE_IN_BLOCKS * CE_BLOCK_SIZE_IN_PAGES)
#define CE_CACHE_VA_SPACE_SIZE (CE_CACHE_VA_SPACE_SIZE_IN_PAGES * CE_PAGE_SIZE)

#define CE_CACHE_SIZE_IN_BLOCKS ((128 / 4) / CE_BLOCK_SIZE_IN_PAGES)
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

CE_RODATA_FLASH const int test_data[] = {1, 2, 3, 4 ,5 ,6 }; // will be optimized by toolchain
CE_RODATA_FLASH const int test_data2[2048] = {100, 101, 102, 103, 104, 105, 106};

CE_RODATA_FLASH const char ccc[] = "123456789012345678901234567890\r\n";

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
int aaa = 0;
int bbb = 0;
void CE_TEXT_FLASH test2()
{
    aaa = bbb;
    // print(ccc);
    uarths_putchar('a');
    uarths_putchar('a');
    uarths_putchar('a');
    uarths_putchar('a');
    uarths_putchar('\r');
    uarths_putchar('\n');    
}


void ceSetupMMU() {
  asm volatile ("la t0,mretdist");
    asm volatile ("csrw mepc,t0");

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
    msValue |=  ((uint64_t)VM_SV39 << 24);// M S U mode, SV39
    msValue = INSERT_FIELD(msValue, MSTATUS_MPRV, 1);
    msValue = INSERT_FIELD(msValue, MSTATUS_MPP, PRV_U);
    //    print("set mstatus:%x\r\n", msValue);
    write_csr(mstatus, msValue);

    ceResetCacheState();
    asm volatile ("mret");
    asm volatile ("mretdist:");
}

static inline uint32_t ceVAddrToVBlockId(uintptr_t vaddr) {
    return (vaddr - ceVABase) / (CE_BLOCK_SIZE);
}

static void ceMapVBlockToPhysAddr(uint32_t vBlockId, uint32_t physAddr) {
    uint32_t basePageId = vBlockId * CE_BLOCK_SIZE_IN_PAGES;
    uintptr_t vaddr = 0;
    for (uint32_t i = 0 ; i < CE_BLOCK_SIZE_IN_PAGES; i++) {
        ceLv3PageTables[basePageId + i] = physAddr ? ceEncodePTE(physAddr + i * CE_PAGE_SIZE, PTE_V | PTE_R | PTE_X | PTE_G | PTE_U) : 0;
        vaddr = ceVABase + ((uintptr_t)(basePageId + i) * CE_PAGE_SIZE);
        asm volatile("sfence.vm %0" : "=r"(vaddr));
    }
}

uint64_t flash_read_time = 0;

int ceFileReadCallback(uint32_t fileOffset, uint64_t* buf, uint32_t len)
{
    // memset(buf, 0x00, len);
    uint64_t t = read_csr(mcycle)/(sysctl_clock_get_freq(SYSCTL_CLOCK_CPU)/1000000);
    w25qxx_read_data_dma(CONFIG_FIRMWARE_FLASH_ADDR+fileOffset, (uint8_t*)buf,  len, W25QXX_QUAD_FAST);
    flash_read_time = read_csr(mcycle)/(sysctl_clock_get_freq(SYSCTL_CLOCK_CPU)/1000000) - t;
    // CE_DEBUG_PRINT("--flash read time:%ld\r\n", t2 - t);
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
    return 0;
}


static inline int supports_extension(char ext)
{
  return read_csr(misa) & (1 << (ext - 'A'));
}


uint64_t sbi_ticks_us(void)
{
    return SBI_CALL_0(SBI_GET_CYCLE)/(sysctl_clock_get_freq(SYSCTL_CLOCK_CPU)/1000000);
}


int mpymain() ;
int main()
{
    fpioa_set_function(4, FUNC_UARTHS_RX);
    fpioa_set_function(5,  FUNC_UARTHS_TX);
    uarths_init();
    uarths_config(115200, 1);
    print("m:%p\n",malloc(10)); //fix me. bug exist 

    ceSetupMMU();
    usermode = 1;
    
    uint8_t manuf_id, device_id;
    w25qxx_init_dma(3, 0);
    w25qxx_enable_quad_mode_dma();
    w25qxx_read_id_dma(&manuf_id, &device_id);
    print("flash id:%x %x\r\n",manuf_id, device_id);

    int a = 5;
    {
        print("%p\r\n", &a);
        print("--%p\r\n", test);
        uint64_t t = sbi_ticks_us();
        // print("%d %d %d %d\r\n", test_data[0], test_data[1], test_data[2], test_data[3]);
        // print("%d %d %d %d\r\n", test_data2[t%5], test_data2[1], test_data2[2], test_data2[3]);
        uarths_putchar('a');
        print("--putchar time:%d\r\n", sbi_ticks_us() - t);
        bbb = (int)sbi_ticks_us();
        t = sbi_ticks_us();
	test();
        test2();
        print("--time: %dus %dus %d\r\n", flash_read_time, sbi_ticks_us() - t, aaa);
        print("%d %d %d %d\r\n", test_data[0], test_data[1], test_data[2], test_data[3]);
        print("%d %d %d %d\r\n", test_data2[t%5], test_data2[1], test_data2[2], test_data2[3]);
    }
    print("test end\r\n");
    print("test2\r\n");
    //    int core = read_csr(mstatus); // U mode, can not read mstatus
    int core = get_hartid();
    print("sbi ret:%x\r\n", core);
    print("test2 end\r\n");
    mpymain();
    while(1){}
}

unsigned long long sbi_call(unsigned long long which,unsigned long long arg0,unsigned long long arg1,unsigned long long arg2,const char *func,int line){
  unsigned long long res=0;
  //  printk2("[%20s]:%4d which:%2x(%x)==>",func,line,which,arg0);

  if(usermode){
    //    printk2("[u]");
    res = SBI_CALL(which,arg0,arg1,arg2);
  }else{
    //    printk2("[m]");
    res = handlecsr(which,arg0);
  }
  //  printk2("%x\n",res);
  return res;
}

uintptr_t handle_fault_load(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32]) {
    print("fault load\r\n");
    int pu = usermode;//save previous mode
    usermode = 0;
    uintptr_t badAddr = read_csr(mbadaddr);
    if ((badAddr >= ceVABase) && (badAddr < (ceVABase + CE_CACHE_VA_SPACE_SIZE))) {
        if (ceHandlePageFault(badAddr, 0) == 0) {
	  usermode = pu;
	  return epc;
        }
    }
    sys_exit(1337);
    return epc;
}

uintptr_t handle_fault_fetch(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32])
// uintptr_t handle_illegal_instruction(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32])
{
    int pu = usermode;
    print("fault fetch\r\n");
    usermode = 0;
    // uintptr_t badAddr = read_csr(mbadaddr);
    if ((epc >= ceVABase) && (epc < (ceVABase + CE_CACHE_VA_SPACE_SIZE))) {
        if (ceHandlePageFault(epc, 0) == 0) {
	  usermode = pu;
	  return epc;
        }
    }
    dump_core("illegal instruction", cause, epc, regs, fregs);
    sys_exit(1337);
    return epc;
}

unsigned long long handlecsr(unsigned long long cmd,unsigned long long arg0){
  switch (cmd)
    {
    case SBI_GET_HARTID:
      {
	int core = read_csr(mhartid);
	arg0 = core;//a0=coreid
	break;
      }
    case SBI_GET_CYCLE:
      {
	int cycle = read_csr(mcycle);
	arg0 = cycle;//a0=cycle
	break;
      }
    case SBI_GET_MIE:
      {
	int ie = read_csr(mie);
	arg0 = ie;//a0=cycle
	break;
      }
    case SBI_SET_MIE:
      {
	set_csr(mie,arg0);
	break;
      }
    case SBI_CLR_MIE:
      {
	clear_csr(mie,arg0);
	break;
      }
    case SBI_WRI_MIE:
      {
	write_csr(mie,arg0);
	break;
      }
    case SBI_GET_STATUS:
      {
	int status = read_csr(mstatus);
	arg0 = status;//a0=cycle
	break;
      }
    case SBI_SET_STATUS:
      {
	set_csr(mstatus,arg0);
	break;
      }
    case SBI_CLR_STATUS:
      {
	clear_csr(mstatus,arg0);
	break;
      }
    case SBI_GET_MIP:
      {
	int ip = read_csr(mip);
	arg0 = ip;//a0=ip
	break;
      }
    case SBI_SET_MIP:
      {
	set_csr(mip,arg0);
	break;
      }	
    case SBI_CLR_MIP:
      {
	clear_csr(mip,arg0);
	break;
      }	
    case SBI_GET_INSTRET:
      {
	int instret = read_csr(minstret);
	arg0 = instret;//a0=minstret
	break;
      }
    default:
      break;
    }
  return arg0;
}

uintptr_t handle_ecall_u(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32])
{
    register uintptr_t cmd asm ("a7");//get cmd from a7

    regs[10] = handlecsr(cmd,regs[10]); //regs[0]->$a0

    return epc+4;
}

#include "maixpy.h"

int mpymain()
{
    maixpy_main();
    return 0;
}

