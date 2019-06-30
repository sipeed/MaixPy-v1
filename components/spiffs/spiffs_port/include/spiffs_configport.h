

#define SPIFFS_ALIGNED_OBJECT_INDEX_TABLES 1

#define open_fs_debug 0
#if open_fs_debug
    // Set generic spiffs debug output call.
    #define SPIFFS_DBG(_f, ...) printf(_f, ## __VA_ARGS__)

    // Set spiffs debug output call for garbage collecting.
    #define SPIFFS_GC_DBG(_f, ...) printf(_f, ## __VA_ARGS__)

    // Set spiffs debug output call for caching.

    #define SPIFFS_CACHE_DBG(_f, ...) printf(_f, ## __VA_ARGS__)

    // Set spiffs debug output call for system consistency checks.
    #define SPIFFS_CHECK_DBG(_f, ...) printf(_f, ## __VA_ARGS__)

    // Set spiffs debug output call for all api invocations.
    #define SPIFFS_API_DBG(_f, ...) printf(_f, ## __VA_ARGS__)
#endif

#define  FS_PATCH_LENGTH (SPIFFS_OBJ_NAME_LEN*20+20)
#define FLASH_CHIP_SIZE  (16*1024*1024)

#define SPIFFS_OBJ_NAME_LEN (255)
#define SPIFFS_SINGLETON(ignore)   (1)
#define SPIFFS_CFG_PHYS_SZ(ignore)         (3 * 1024 * 1024)
#define SPIFFS_CFG_PHYS_ERASE_SZ(ignore)         (4 * 1024)
#define SPIFFS_CFG_PHYS_ADDR(ignore)          (0xD00000)

#define PAGEN_EACH_BLOCK    32
#define SPIFFS_CFG_LOG_BLOCK_SZ(ignore)         (4*32*1024)
#define SPIFFS_CFG_LOG_PAGE_SZ(ignore) (SPIFFS_CFG_LOG_BLOCK_SZ(ignore)/PAGEN_EACH_BLOCK)


#define SPIFFS_USE_MAGIC (1)
#define SPIFFS_USE_MAGIC_LENGTH (1)

#define SPIFFS_READ_ONLY 0

#define SPIFFS_CACHE(ignore)  (1) 
#define SPIFFS_CACHE_WR(ignore)  (1)
#define SPIFFS_TEMPORAL_FD_CACHE(ignore)  (1)
#define SPIFFS_HAL_CALLBACK_EXTRA         (1)

typedef signed int s32_t;
typedef unsigned int u32_t;
typedef signed short s16_t;
typedef unsigned short u16_t;
typedef signed char s8_t;
typedef unsigned char u8_t;


