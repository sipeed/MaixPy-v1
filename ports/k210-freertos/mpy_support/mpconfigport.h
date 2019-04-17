#include <stdint.h>
#include <stdbool.h>
// options to control how MicroPython is built

// You can disable the built-in MicroPython compiler by setting the following
// config option to 0.  If you do this then you won't get a REPL prompt, but you
// will still be able to execute pre-compiled scripts, compiled with mpy-cross.

#ifndef _MPCONFIGPORT_H_
#define _MPCONFIGPORT_H_


// object representation and NLR handling
#define MICROPY_OBJ_BASE_ALIGNMENT  __attribute__((aligned(8)))

#define MICROPY_OBJ_REPR                    (MICROPY_OBJ_REPR_A)
#define MICROPY_NLR_SETJMP                  (1)
#define MICROPY_READER_VFS                  (1)

#define MICROPY_HW_UART_REPL                (1)
// MCU definition
#define MP_ENDIANNESS_LITTLE                (1)
#define MICROPY_NO_ALLOCA                   (1)


// optimisations
#define MICROPY_OPT_COMPUTED_GOTO           (1)
#define MICROPY_OPT_MPZ_BITWISE             (1)


#define MICROPY_ENABLE_COMPILER     (1)

#define MICROPY_QSTR_BYTES_IN_LEN           (1)
#define MICROPY_QSTR_BYTES_IN_HASH          (1)
#define MICROPY_ALLOC_PATH_MAX      (128)
#define MICROPY_ALLOC_PARSE_CHUNK_INIT (16)
#define MICROPY_EMIT_X64            (0)
#define MICROPY_EMIT_THUMB          (0)
#define MICROPY_EMIT_INLINE_THUMB   (0)

#define MICROPY_COMP_MODULE_CONST   (1)
#define MICROPY_COMP_CONST          (1)
#define MICROPY_COMP_DOUBLE_TUPLE_ASSIGN (1)
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN (10)

#define MICROPY_ENABLE_SCHEDULER            (1)
#define MICROPY_SCHEDULER_DEPTH             (8)

#define MICROPY_ENABLE_FINALISER            (1)
#define MICROPY_STACK_CHECK                 (0)
#define MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF (1)
#define MICROPY_KBD_EXCEPTION               (1)
#define MICROPY_REPL_EMACS_KEYS             (1)
#define MICROPY_REPL_AUTO_INDENT            (1)
#define MICROPY_HAL_HAS_VT100               (1)

#define MICROPY_CPYTHON_COMPAT              (1)
#define MICROPY_STREAMS_NON_BLOCK           (1)
#define MICROPY_STREAMS_POSIX_API           (0)
#define MICROPY_MODULE_BUILTIN_INIT         (1)
#define MICROPY_MODULE_WEAK_LINKS           (1)

#define MICROPY_PERSISTENT_CODE_LOAD        (1)
#define MICROPY_PERSISTENT_CODE_SAVE (0)
#define MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE (0)

#define MICROPY_COMP_RETURN_IF_EXPR         (1)

#define MICROPY_PY_COLLECTIONS              (1)
#define MICROPY_PY_COLLECTIONS_DEQUE        (1)
#define MICROPY_PY_COLLECTIONS_ORDEREDDICT  (1)

extern const struct _mp_print_t mp_debug_print;
extern const struct _mp_print_t mp_debug_print;
#define MICROPY_DEBUG_VERBOSE       (0)
#define MICROPY_DEBUG_PRINTER       (&mp_debug_print)


#define MICROPY_MEM_STATS           (1)
#define MICROPY_DEBUG_PRINTERS      (0)
#define MICROPY_ENABLE_GC           (1)
#define MICROPY_GC_ALLOC_THRESHOLD  (1)
#define MICROPY_REPL_EVENT_DRIVEN   (0)
#define MICROPY_MALLOC_USES_ALLOCATED_SIZE (1)
#define MICROPY_HELPER_REPL         (1)
#define MICROPY_HELPER_LEXER_UNIX   (1)
#define MICROPY_ENABLE_SOURCE_LINE  (1)
#define MICROPY_ENABLE_DOC_STRING   (0)
#define MICROPY_ERROR_REPORTING     (MICROPY_ERROR_REPORTING_DETAILED)
#define MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG (0)
#define MICROPY_PY_ASYNC_AWAIT      (0)
#define MICROPY_PY_BUILTINS_BYTEARRAY (1)
#define MICROPY_PY_BUILTINS_MEMORYVIEW (1)
#define MICROPY_PY_BUILTINS_FROZENSET (1)
#define MICROPY_PY_BUILTINS_SET     (1)
#define MICROPY_PY_BUILTINS_PROPERTY (1)
#define MICROPY_PY_BUILTINS_MIN_MAX  (1)
#define MICROPY_PY_BUILTINS_STR_OP_MODULO (1)
#define MICROPY_PY_GC               (1)
#define MICROPY_MODULE_FROZEN_STR           (0)
#define MICROPY_MODULE_FROZEN_MPY           (1)
#define MICROPY_LONGINT_IMPL                (MICROPY_LONGINT_IMPL_MPZ)

#define MICROPY_FLOAT_IMPL                  (MICROPY_FLOAT_IMPL_DOUBLE)
#define MICROPY_PY_BUILTINS_HELP            (1)
#define MICROPY_PY_BUILTINS_HELP_TEXT       kendryte_k210_help_text
#define MICROPY_PY_BUILTINS_HELP_MODULES    (1)
#define MICROPY_PY_BUILTINS_COMPLEX         (1)
#define MICROPY_PY_BUILTINS_FLOAT           (1)


// control over Python builtins
#define MICROPY_PY_STR_BYTES_CMP_WARN       (1)
#define MICROPY_PY_BUILTINS_STR_UNICODE     (1)
#define MICROPY_PY_BUILTINS_STR_CENTER      (1)
#define MICROPY_PY_BUILTINS_STR_PARTITION   (1)
#define MICROPY_PY_BUILTINS_STR_SPLITLINES  (1)
#define MICROPY_PY_BUILTINS_SLICE           (1)
#define MICROPY_PY_BUILTINS_SLICE_ATTRS     (1)
#define MICROPY_PY_BUILTINS_RANGE_ATTRS     (1)
#define MICROPY_PY_BUILTINS_ROUND_INT       (1)
#define MICROPY_PY_BUILTINS_TIMEOUTERROR    (1)
#define MICROPY_PY_ALL_SPECIAL_METHODS      (1)
#define MICROPY_PY_BUILTINS_COMPILE         (1)
#define MICROPY_PY_BUILTINS_ENUMERATE       (1)
#define MICROPY_PY_BUILTINS_EXECFILE        (1)
#define MICROPY_PY_BUILTINS_FILTER          (1)
#define MICROPY_PY_BUILTINS_REVERSED        (1)
#define MICROPY_PY_BUILTINS_NOTIMPLEMENTED  (1)
#define MICROPY_PY_BUILTINS_INPUT           (1)
#define MICROPY_PY_BUILTINS_POW3            (1)
#define MICROPY_PY___FILE__                 (1)
#define MICROPY_PY_MICROPYTHON_MEM_INFO     (1)
#define MICROPY_PY_ARRAY                    (1)
#define MICROPY_PY_ARRAY_SLICE_ASSIGN       (1)
#define MICROPY_PY_MATH                     (1)
#define MICROPY_PY_MATH_SPECIAL_FUNCTIONS   (1)
#define MICROPY_PY_CMATH                    (1)
#define MICROPY_PY_IO                       (1)
#define MICROPY_PY_IO_IOBASE                (1)
#define MICROPY_PY_IO_FILEIO                (1)
#define MICROPY_PY_IO_BYTESIO               (1)
#define MICROPY_PY_IO_BUFFEREDWRITER        (1)
#define MICROPY_PY_STRUCT                   (1)
#define MICROPY_PY_SYS                      (1)
#define MICROPY_PY_SYS_MAXSIZE              (1)
#define MICROPY_PY_SYS_MODULES              (1)
#define MICROPY_PY_SYS_EXIT                 (1)
#define MICROPY_PY_SYS_STDFILES             (1)
#define MICROPY_PY_SYS_STDIO_BUFFER         (1)
#define MICROPY_PY_UERRNO                   (1)
#define MICROPY_PY_USELECT                  (0)
#define MICROPY_PY_UTIME_MP_HAL             (1)

//thread todo
#define MICROPY_PY_THREAD                   (1)
#define MICROPY_PY_THREAD_GIL               (1)
#define MICROPY_PY_THREAD_GIL_VM_DIVISOR    (32)

#define MICROPY_FATFS_ENABLE_LFN            (1)
#define MICROPY_FATFS_LFN_CODE_PAGE         (437) /* 1=SFN/ANSI 437=LFN/U.S.(OEM) */
#define mp_type_fileio                      mp_type_vfs_spiffs_fileio
#define mp_type_textio                      mp_type_vfs_spiffs_textio
#define MICROPY_VFS                         (1)
#define MICROPY_VFS_SPIFFS                  (1)
#define MICROPY_VFS_FAT                     (1)
#define MICROPY_FATFS_MULTI_PARTITION       (1)
#define MICROPY_FATFS_RPATH            (2)
#define MP_SSIZE_MAX (0x7fffffff)

#define _USE_MKFS 1
#define _FS_READONLY 0

// use vfs's functions for import stat and builtin open
#define mp_import_stat mp_vfs_import_stat
#define mp_builtin_open mp_vfs_open
#define mp_builtin_open_obj mp_vfs_open_obj
#define MICROPY_PY_ATTRTUPLE                (1)

#define MICROPY_PY_FUNCTION_ATTRS           (1)
#define MICROPY_PY_DESCRIPTORS              (1)


// extended modules
#define MICROPY_PY_UCTYPES                  (1)
#define MICROPY_PY_UZLIB                    (1)
#define MICROPY_PY_UJSON                    (1)
#define MICROPY_PY_URE                      (1)
#define MICROPY_PY_URE_SUB                  (1)
#define MICROPY_PY_UHEAPQ                   (1)
#define MICROPY_PY_UTIMEQ                   (1)
#define MICROPY_PY_UHASHLIB                 (0)
#define MICROPY_PY_UHASHLIB_SHA1            (0)
#define MICROPY_PY_UHASHLIB_SHA256          (0)
#define MICROPY_PY_UCRYPTOLIB               (0)
#define MICROPY_PY_UBINASCII                (1)
#define MICROPY_PY_UBINASCII_CRC32          (1)
#define MICROPY_PY_URANDOM                  (1)
#define MICROPY_PY_URANDOM_EXTRA_FUNCS      (1)
#define MICROPY_PY_OS_DUPTERM               (1)

#define MICROPY_PY_MACHINE_PULSE            (1)
#define MICROPY_PY_MACHINE_I2C              (0) // not use mpy internal soft i2c
#define MICROPY_PY_MACHINE_HW_I2C           (1) // enable hardware i2c
#define MICROPY_PY_MACHINE_SPI              (0) // disable soft spi
#define MICROPY_PY_MACHINE_HW_SPI           (1) // enable hardware spi
#define MICROPY_PY_USSL                     (0)
#define MICROPY_SSL_MBEDTLS                 (0)
#define MICROPY_PY_USSL_FINALISER           (0)
#define MICROPY_PY_WEBSOCKET                (1)
#define MICROPY_PY_WEBREPL                  (1)
#define MICROPY_PY_FRAMEBUF                 (0)
#define MICROPY_PY_USOCKET_EVENTS           (MICROPY_PY_WEBREPL)
#define MICROPY_PY_NETWORK                  (1)
#define MICROPY_PY_USOCKET                  (1)
#define MICROPY_PY_LWIP                     (0)
#define MICROPY_PY_UHASHLIB_MAIX            (1)
#define MICROPY_PY_UHASHLIB_SHA256_MAIX     (1)
#define MICROPY_PY_UCRYPTOLIB_MAIX          (1)


//disable ext str pool
#define MICROPY_QSTR_EXTRA_POOL             mp_qstr_frozen_const_pool

// type definitions for the specific machine

#define MICROPY_MAKE_POINTER_CALLABLE(p) ((void*)((mp_uint_t)(p) | 1))

// This port is intended to be 32-bit, but unfortunately, int32_t for
// different targets may be defined in different ways - either as int
// or as long. This requires different printf formatting specifiers
// to print such value. So, we avoid int32_t and use int directly.
#define UINT_FMT "%u"
#define INT_FMT "%d"
typedef int64_t mp_int_t; // must be pointer size
typedef uint64_t mp_uint_t; // must be pointer size

typedef long mp_off_t;

#define MP_PLAT_PRINT_STRN(str, len) mp_hal_stdout_tx_strn_cooked(str, len)

// extra built in names to add to the global namespace
#define MICROPY_PORT_BUILTINS \
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&mp_builtin_open_obj) },

// extern const struct _mp_obj_module_t socket_module;
extern const struct _mp_obj_module_t uos_module;
extern const struct _mp_obj_module_t utime_module;
extern const struct _mp_obj_module_t maix_module;
extern const struct _mp_obj_module_t machine_module;
extern const struct _mp_obj_module_t network_module;
extern const struct _mp_obj_module_t socket_module;
extern const struct _mp_obj_module_t image_module;
extern const struct _mp_obj_module_t sensor_module;
extern const struct _mp_obj_module_t lcd_module;
extern const struct _mp_obj_module_t cpufreq_module;
extern const struct _mp_obj_module_t kpu_module;
extern const struct _mp_obj_module_t audio_module;
extern const struct _mp_obj_module_t mp_module_uhashlib_maix;
extern const struct _mp_obj_module_t mp_module_ucryptolib;


// openmv minimum
#ifndef MAIXPY_OMV_MINIMUM_FUNCTION
#define MAIXPY_OMV_MINIMUM_FUNCTION         (0) // Minimum function
#endif //MAIXPY_OMV_MINIMUM_FUNCTION

// video record play
#ifndef MAIXPY_VIDEO_SUPPORT
#define MAIXPY_VIDEO_SUPPORT                (0) // avi video support
#endif //MAIXPY_VIDEO_SUPPORT
#if MAIXPY_VIDEO_SUPPORT
extern const struct _mp_obj_module_t video_module;
#define MAIXPY_PY_VIDEO_DEF \
    { MP_OBJ_NEW_QSTR(MP_QSTR_video), (mp_obj_t)&video_module },
#else
#define MAIXPY_PY_VIDEO_DEF 
#endif

// nes game emulator
#ifndef MAIXPY_NES_EMULATOR_SUPPORT
#define MAIXPY_NES_EMULATOR_SUPPORT         (0) // NES gamer emulator
#endif //MAIXPY_NES_EMULATOR_SUPPORT
#if MAIXPY_NES_EMULATOR_SUPPORT
extern const struct _mp_obj_module_t nes_module;
#define MAIXPY_PY_NES_DEF \
    { MP_OBJ_NEW_QSTR(MP_QSTR_nes), (mp_obj_t)&nes_module },
#else
#define MAIXPY_PY_NES_DEF 
#endif

// lvgl GUI lib
#ifndef MAIXPY_LVGL_SUPPORT
#define MAIXPY_LVGL_SUPPORT                 (0) // lvgl GUI lib
#endif
#if MAIXPY_LVGL_SUPPORT
#undef MAIXPY_LVGL_SUPPORT
#define MAIXPY_LVGL_SUPPORT                 (1) // lvgl GUI lib
extern const struct _mp_obj_module_t mp_module_lvgl;
extern const struct _mp_obj_module_t mp_module_lvgl_helper;
#define MAIXPY_PY_LVGL_DEF \
    { MP_OBJ_NEW_QSTR(MP_QSTR_lvgl), (mp_obj_t)&mp_module_lvgl }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_lvgl_helper), (mp_obj_t)&mp_module_lvgl_helper },
#else
#define MAIXPY_PY_LVGL_DEF
#endif // MAIXPY_LVGL_SUPPORT

// touchscreen
#ifndef MAIXPY_TOUCH_SCREEN_SUPPORT
#define MAIXPY_TOUCH_SCREEN_SUPPORT          (0)
#endif
#if MAIXPY_TOUCH_SCREEN_SUPPORT
extern const struct _mp_obj_module_t mp_module_touchscreen;
#define MAIXPY_PY_TOUCHSCREEN_DEF \
    { MP_OBJ_NEW_QSTR(MP_QSTR_touchscreen), (mp_obj_t)&mp_module_touchscreen },
#else
#define MAIXPY_PY_TOUCHSCREEN_DEF 
#endif

/////////////////////////////////////////////////////////////////////////////////

#define MICROPY_PORT_BUILTIN_MODULES \
    { MP_OBJ_NEW_QSTR(MP_QSTR_uos), (mp_obj_t)&uos_module }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_time), (mp_obj_t)&utime_module }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_utime), (mp_obj_t)&utime_module }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_Maix), (mp_obj_t)&maix_module },\
    { MP_OBJ_NEW_QSTR(MP_QSTR_machine), (mp_obj_t)&machine_module },\
    { MP_OBJ_NEW_QSTR(MP_QSTR_network), (mp_obj_t)&network_module },\
    { MP_OBJ_NEW_QSTR(MP_QSTR_usocket), (mp_obj_t)&socket_module }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_socket), (mp_obj_t)&socket_module }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_image), (mp_obj_t)&image_module }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_sensor), (mp_obj_t)&sensor_module }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_lcd), (mp_obj_t)&lcd_module }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_cpufreq), (mp_obj_t)&cpufreq_module }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_KPU), (mp_obj_t)&kpu_module }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_audio), (mp_obj_t)&audio_module }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_uhashlib), (mp_obj_t)&mp_module_uhashlib_maix }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_ucryptolib), (mp_obj_t)&mp_module_ucryptolib }, \
    MAIXPY_PY_NES_DEF \
    MAIXPY_PY_VIDEO_DEF \
    MAIXPY_PY_LVGL_DEF \
    MAIXPY_PY_TOUCHSCREEN_DEF




#define MICROPY_PORT_BUILTIN_MODULE_WEAK_LINKS \
    { MP_OBJ_NEW_QSTR(MP_QSTR_binascii), (mp_obj_t)&mp_module_ubinascii }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_collections), (mp_obj_t)&mp_module_collections }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_errno), (mp_obj_t)&mp_module_uerrno }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_heapq), (mp_obj_t)&mp_module_uheapq }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_json), (mp_obj_t)&mp_module_ujson }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_math), (mp_obj_t)&mp_module_math }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_os), (mp_obj_t)&uos_module }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_random), (mp_obj_t)&mp_module_urandom }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_re), (mp_obj_t)&mp_module_ure }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_struct), (mp_obj_t)&mp_module_ustruct }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_zlib), (mp_obj_t)&mp_module_uzlib }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_hashlib), (mp_obj_t)&mp_module_uhashlib_maix }, \

#define MICROPY_PY_MACHINE                  (1)
#define MICROPY_PY_MACHINE_PIN_MAKE_NEW     mp_pin_make_new

// We need to provide a declaration/definition of alloca()
#include <alloca.h>

#define MICROPY_HW_BOARD_NAME "Sipeed_M1"
#define MICROPY_HW_MCU_NAME "kendryte-k210"
#define MICROPY_PY_SYS_PLATFORM "MaixPy"

#ifdef __linux__
#define MICROPY_MIN_USE_STDOUT (1)
#endif

//#ifdef __thumb__
//#define MICROPY_MIN_USE_CORTEX_CPU (1)
//#define MICROPY_MIN_USE_STM32_MCU (1)
//#endif

#if MICROPY_PY_THREAD
#define MICROPY_EVENT_POLL_HOOK \
    do { \
        extern void mp_handle_pending(void); \
        mp_handle_pending(); \
        MP_THREAD_GIL_EXIT(); \
        MP_THREAD_GIL_ENTER(); \
    } while (0);
#else
#define MICROPY_EVENT_POLL_HOOK 
#endif


#define MP_STATE_PORT MP_STATE_VM


#define MICROPY_PORT_ROOT_POINTERS \
    const char *readline_hist[16];  \
    struct _machine_uart_obj_t *Maix_stdio_uart; \
	struct _nic_obj_t *modnetwork_nic; \





#endif // _MPCONFIGPORT_H_

