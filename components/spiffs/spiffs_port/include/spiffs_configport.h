

#include "global_config.h"

// Set generic spiffs debug output call.
#if CONFIG_SPIFFS_DBG
    #define SPIFFS_DBG(_f, ...) printf(_f, ## __VA_ARGS__)
#else
    #define SPIFFS_DBG(_f, ...)
#endif

// Set spiffs debug output call for garbage collecting.
#if CONFIG_SPIFFS_GC_DBG
    #define SPIFFS_GC_DBG(_f, ...) printf(_f, ## __VA_ARGS__)
#else
    #define SPIFFS_GC_DBG(_f, ...)
#endif

// Set spiffs debug output call for caching.
#if CONFIG_SPIFFS_CACHE_DBG
    #define SPIFFS_CACHE_DBG(_f, ...) printf(_f, ## __VA_ARGS__)
#else
    #define SPIFFS_CACHE_DBG(_f, ...)
#endif

// Set spiffs debug output call for system consistency checks.
#if CONFIG_SPIFFS_CHECK_DBG
    #define SPIFFS_CHECK_DBG(_f, ...) printf(_f, ## __VA_ARGS__)
#else
    #define SPIFFS_CHECK_DBG(_f, ...)
#endif

// Set spiffs debug output call for all api invocations.
#if CONFIG_SPIFFS_API_DBG
    #define SPIFFS_API_DBG(_f, ...) printf(_f, ## __VA_ARGS__)
#else
    #define SPIFFS_API_DBG(_f, ...)
#endif

#define  FS_PATCH_LENGTH (SPIFFS_OBJ_NAME_LEN*20+20)

typedef signed int s32_t;
typedef unsigned int u32_t;
typedef signed short s16_t;
typedef unsigned short u16_t;
typedef signed char s8_t;
typedef unsigned char u8_t;


