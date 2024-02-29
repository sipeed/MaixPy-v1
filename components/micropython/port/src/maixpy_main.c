/*
 * Copyright 2019 Sipeed Co.,Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*****std lib****/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
/*****mpy****/
#include "py/mpconfig.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/stackctrl.h"
#include "py/mpstate.h"
#include "py/nlr.h"
#include "py/compile.h"
#include "py/mphal.h"
#include "gccollect.h"
#include "lib/utils/pyexec.h"
#include "lib/mp-readline/readline.h"
#include "lib/utils/interrupt_char.h"
#include "modmachine.h"
#include "mpconfigboard.h"
#include "lib/oofatfs/ff.h"
#include "extmod/vfs_fat.h"
#if MICROPY_PY_THREAD
#include "mpthreadport.h"
#include "py/mpthread.h"
#endif
#include "machine_uart.h"
/*****bsp****/
#include "sleep.h"
#include "encoding.h"
#include "sysctl.h"
#include "plic.h"
#include "printf.h"
#include "syslog.h"
#include "atomic.h"
#include "entry.h"
#include "sipeed_mem.h"
/*****peripheral****/
#include "fpioa.h"
#include "gpio.h"
#include "timer.h"
#include "uarths.h"
#include "rtc.h"
#include "uart.h"
#include "w25qxx.h"
#include "sdcard.h"
#include "lcd.h"
/*****freeRTOS****/
#if MICROPY_PY_THREAD
#include "FreeRTOS.h"
#include "task.h"
#endif
/*******storage********/
#include "vfs_spiffs.h"
#include "spiffs_config.h"
#include "machine_sdcard.h"
#include "machine_uart.h"
/**********omv**********/
#include "omv_boardconfig.h"
#include "framebuffer.h"
#include "sensor.h"
#include "omv.h"
#include "sipeed_conv.h"
#include "ide_dbg.h"
#include "global_config.h"
#include "Maix_config.h"

/********* others *******/
#include "boards.h"

#ifdef CONFIG_MAIXPY_K210_UARTHS_DEBUG
#define MAIXPY_DEBUG_UARTHS_REPL_UART2 // Debug by UARTHS  (use `printk()`) and REPL by UART2
#endif

#define UART_BUF_LENGTH_MAX 269

uint8_t CPU_freq = 0;
uint8_t PLL0_freq = 0;
uint8_t PLL1_freq = 0;
uint8_t PLL2_freq = 0;

uint8_t *_fb_base;
uint8_t *_jpeg_buf;

#if MICROPY_PY_THREAD
#define MP_TASK_PRIORITY 4
#define MP_TASK_STACK_SIZE (32 * 1024)
#define MP_TASK_STACK_LEN (MP_TASK_STACK_SIZE / sizeof(StackType_t))
TaskHandle_t mp_main_task_handle;
#endif

#define FORMAT_FS_FORCE 0
spiffs_user_mount_t spiffs_user_mount_handle;

void do_str(const char *src, mp_parse_input_kind_t input_kind);

STATIC bool init_sdcard_fs(void)
{
  bool first_part = true;
  for (int part_num = 1; part_num <= 4; ++part_num)
  {
    // create vfs object
    fs_user_mount_t *vfs_fat = m_new_obj_maybe(fs_user_mount_t);
    mp_vfs_mount_t *vfs = m_new_obj_maybe(mp_vfs_mount_t);
    if (vfs == NULL || vfs_fat == NULL)
    {
      break;
    }
    vfs_fat->flags = FSUSER_FREE_OBJ;
    sdcard_init_vfs(vfs_fat, part_num);

    // try to mount the partition
    FRESULT res = f_mount(&vfs_fat->fatfs);
    if (res != FR_OK)
    {
      // couldn't mount
      m_del_obj(fs_user_mount_t, vfs_fat);
      m_del_obj(mp_vfs_mount_t, vfs);
    }
    else
    {
      // mounted via FatFs, now mount the SD partition in the VFS
      if (first_part)
      {
        // the first available partition is traditionally called "sd" for simplicity
        vfs->str = "/sd";
        vfs->len = 3;
      }
      else
      {
        // subsequent partitions are numbered by their index in the partition table
        if (part_num == 2)
        {
          vfs->str = "/sd2";
        }
        else if (part_num == 3)
        {
          vfs->str = "/sd3";
        }
        else
        {
          vfs->str = "/sd4";
        }
        vfs->len = 4;
      }
      vfs->obj = MP_OBJ_FROM_PTR(vfs_fat);
      vfs->next = NULL;
      for (mp_vfs_mount_t **m = &MP_STATE_VM(vfs_mount_table);; m = &(*m)->next)
      {
        // 防止同名的文件系统冲突
        if (*m == NULL || strcmp((*m)->str, vfs->str) == 0)
        {
          *m = vfs;
          break;
        }
      }
      if (first_part)
      {
        // use SD card as current directory
        MP_STATE_PORT(vfs_cur) = vfs;
        first_part = false;
        break;
      }
    }
  }

  if (first_part)
  {
    printk("PYB: can't mount SD card\n");
    return false;
  }
  else
  {
    return true;
  }
}

bool flash_init(uint8_t *manuf_id, uint8_t *device_id)
{
  w25qxx_init_dma(3, 0);
  w25qxx_enable_quad_mode_dma();
  w25qxx_read_id_dma(manuf_id, device_id);
  return true;
}

bool peripherals_init()
{
  int ret;
  ret = boards_init();
  return ret == 0;
}

MP_NOINLINE STATIC spiffs_user_mount_t *init_flash_spiffs()
{

  spiffs_user_mount_t *vfs_spiffs = &spiffs_user_mount_handle;
  vfs_spiffs->flags = SYS_SPIFFS;
  vfs_spiffs->base.type = &mp_spiffs_vfs_type;
  vfs_spiffs->fs.user_data = (void *)vfs_spiffs;
  vfs_spiffs->cfg.hal_read_f = spiffs_read_method;
  vfs_spiffs->cfg.hal_write_f = spiffs_write_method;
  vfs_spiffs->cfg.hal_erase_f = spiffs_erase_method;

  vfs_spiffs->cfg.phys_size = CONFIG_SPIFFS_SIZE;
  vfs_spiffs->cfg.phys_addr = CONFIG_SPIFFS_START_ADDR;
  vfs_spiffs->cfg.phys_erase_block = CONFIG_SPIFFS_EREASE_SIZE;
  vfs_spiffs->cfg.log_block_size = CONFIG_SPIFFS_LOGICAL_BLOCK_SIZE;
  vfs_spiffs->cfg.log_page_size = CONFIG_SPIFFS_LOGICAL_PAGE_SIZE;
  int res = SPIFFS_mount(&(vfs_spiffs->fs),
                         &(vfs_spiffs->cfg),
                         spiffs_work_buf,
                         spiffs_fds,
                         sizeof(spiffs_fds),
                         spiffs_cache_buf,
                         sizeof(spiffs_cache_buf),
                         0);
  if (FORMAT_FS_FORCE || res != SPIFFS_OK || res == SPIFFS_ERR_NOT_A_FS)
  {
    SPIFFS_unmount(&vfs_spiffs->fs);
    printk("[MAIXPY]:Spiffs Unmount.\n");
    printk("[MAIXPY]:Spiffs Formating...\n");
    s32_t format_res = SPIFFS_format(&vfs_spiffs->fs);
    printk("[MAIXPY]:Spiffs Format %s \n", format_res ? "failed" : "successful");
    if (0 != format_res)
    {
      return false;
    }
    res = SPIFFS_mount(&vfs_spiffs->fs,
                       &vfs_spiffs->cfg,
                       spiffs_work_buf,
                       spiffs_fds,
                       sizeof(spiffs_fds),
                       spiffs_cache_buf,
                       sizeof(spiffs_cache_buf),
                       0);
    printk("[MAIXPY]:Spiffs Mount %s \n", res ? "failed" : "successful");
    if (!res)
    {
    }
  }
  return vfs_spiffs;
}

STATIC bool mpy_mount_spiffs(spiffs_user_mount_t *spiffs)
{
  mp_vfs_mount_t *vfs = m_new_obj(mp_vfs_mount_t);
  if (vfs == NULL)
  {
    printk("[MaixPy]:can't mount flash\n");
    return false;
  }
  vfs->str = "/flash";
  vfs->len = 6;
  vfs->obj = MP_OBJ_FROM_PTR(spiffs);
  vfs->next = NULL;
  for (mp_vfs_mount_t **m = &MP_STATE_VM(vfs_mount_table);; m = &(*m)->next)
  {
    if (*m == NULL)
    {
      *m = vfs;
      break;
    }
  }
  return true;
}

bool save_config_to_spiffs(config_data_t *config)
{
#if CONFIG_MAIXPY_USE_MEMZIP
  // Config is pulled from env vars, so just return
  return true;
#else
  s32_t ret;
  spiffs_file fd = SPIFFS_open(&spiffs_user_mount_handle.fs, FREQ_STORE_FILE_NAME, SPIFFS_O_WRONLY | SPIFFS_O_CREAT, 0);
  if (fd <= 0)
    return false;
  ret = SPIFFS_write(&spiffs_user_mount_handle.fs, fd, config, sizeof(config_data_t));
  if (ret <= 0)
  {
    SPIFFS_close(&spiffs_user_mount_handle.fs, fd);
    return false;
  }
  SPIFFS_close(&spiffs_user_mount_handle.fs, fd);
  return true;
#endif
}

void load_config_from_spiffs(config_data_t *config)
{
#if CONFIG_MAIXPY_USE_MEMZIP
  // Pull config from env vars, not filesystem
  config->freq_cpu = FREQ_CPU_DEFAULT;
  config->freq_pll1 = FREQ_PLL1_DEFAULT;
  config->kpu_div = 1;
  config->gc_heap_size = CONFIG_MAIXPY_GC_HEAP_SIZE;
  config->freq_cpu = config->freq_cpu > FREQ_CPU_MAX ? FREQ_CPU_MAX : config->freq_cpu;
  config->freq_cpu = config->freq_cpu < FREQ_CPU_MIN ? FREQ_CPU_MIN : config->freq_cpu;
  config->freq_pll1 = config->freq_pll1 > FREQ_PLL1_MAX ? FREQ_PLL1_MAX : config->freq_pll1;
  config->freq_pll1 = config->freq_pll1 < FREQ_PLL1_MIN ? FREQ_PLL1_MIN : config->freq_pll1;
  if (config->kpu_div == 0)
    config->kpu_div = 1;
  if (config->gc_heap_size == 0)
  {
    config->gc_heap_size = CONFIG_MAIXPY_GC_HEAP_SIZE;
  }
#else
  s32_t ret, flash_error = 0;
  spiffs_file fd = SPIFFS_open(&spiffs_user_mount_handle.fs, FREQ_STORE_FILE_NAME, SPIFFS_O_RDONLY, 0);
  // config init
  config->freq_cpu = FREQ_CPU_DEFAULT;
  config->freq_pll1 = FREQ_PLL1_DEFAULT;
  config->kpu_div = 1;
  config->gc_heap_size = CONFIG_MAIXPY_GC_HEAP_SIZE;
  if (fd <= 0)
  {
    // init data;
    flash_error = save_config_to_spiffs(config);
  }
  else
  {
    // memset(config, 0, sizeof(config_data_t));
    ret = SPIFFS_read(&spiffs_user_mount_handle.fs, fd, config, sizeof(config_data_t));
    if (ret <= 0)
    {
      printk("maixpy can't load freq.conf\t\r\n");
      flash_error = false;
    }
    else
    {
      config->freq_cpu = config->freq_cpu > FREQ_CPU_MAX ? FREQ_CPU_MAX : config->freq_cpu;
      config->freq_cpu = config->freq_cpu < FREQ_CPU_MIN ? FREQ_CPU_MIN : config->freq_cpu;
      config->freq_pll1 = config->freq_pll1 > FREQ_PLL1_MAX ? FREQ_PLL1_MAX : config->freq_pll1;
      config->freq_pll1 = config->freq_pll1 < FREQ_PLL1_MIN ? FREQ_PLL1_MIN : config->freq_pll1;
      if (config->kpu_div == 0)
        config->kpu_div = 1;
      if (config->gc_heap_size == 0)
      {
        config->gc_heap_size = CONFIG_MAIXPY_GC_HEAP_SIZE;
      }
    }
  }
  SPIFFS_close(&spiffs_user_mount_handle.fs, fd);
#endif
}

#if MICROPY_ENABLE_COMPILER
void pyexec_str(vstr_t *str)
{
  // gc.collect()
  gc_collect();
  // save context
  mp_obj_dict_t *volatile old_globals = mp_globals_get();
  mp_obj_dict_t *volatile old_locals = mp_locals_get();

  // set new context
  // mp_obj_t globals = mp_obj_new_dict(0);
  mp_obj_t locals = mp_obj_new_dict(0);
  mp_obj_dict_store(locals, MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR___main__));
  mp_globals_set(MP_OBJ_TO_PTR(locals));
  mp_locals_set(MP_OBJ_TO_PTR(locals));

  nlr_buf_t nlr;
  if (nlr_push(&nlr) == 0)
  {
    mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, str->buf, str->len, 0);
    qstr source_name = lex->source_name;
    mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);
    mp_obj_t module_fun = mp_compile(&parse_tree, source_name, MP_EMIT_OPT_NONE, false);
    mp_call_function_0(module_fun);
    nlr_pop();
    mp_globals_set(old_globals);
    mp_locals_set(old_locals);
#if MICROPY_PY_THREAD
    mp_thread_deinit();
#endif
  }
  else
  {
    mp_globals_set(old_globals);
    mp_locals_set(old_locals);
#if MICROPY_PY_THREAD
    mp_thread_deinit();
#endif
    mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
  }
}
#endif

#define SDCARD_CHECK_CONFIG(GOAL, val)                                                                    \
  {                                                                                                       \
    const char key[] = #GOAL;                                                                             \
    mp_map_elem_t *elem = mp_map_lookup(&self->map, mp_obj_new_str(key, sizeof(key) - 1), MP_MAP_LOOKUP); \
    if (elem != NULL)                                                                                     \
    {                                                                                                     \
      *(val) = mp_obj_get_int(elem->value);                                                               \
    }                                                                                                     \
  }

void sd_preinit_config()
{
  extern sdcard_config_t config;
  // printk("[%d:%s]\r\n", __LINE__, __FUNCTION__);

  const char cfg[] = "sdcard";
  mp_obj_t tmp = maix_config_get_value(mp_obj_new_str(cfg, sizeof(cfg) - 1), mp_const_none);
  if (tmp != mp_const_none && mp_obj_is_type(tmp, &mp_type_dict))
  {
    mp_obj_dict_t *self = MP_OBJ_TO_PTR(tmp);

    SDCARD_CHECK_CONFIG(sclk, &config.sclk_pin);
    SDCARD_CHECK_CONFIG(mosi, &config.mosi_pin);
    SDCARD_CHECK_CONFIG(miso, &config.miso_pin);
    SDCARD_CHECK_CONFIG(cs, &config.cs_pin);
    SDCARD_CHECK_CONFIG(cs_gpio, &config.cs_gpio_num);
  }
}

typedef int (*dual_func_t)(int);
corelock_t lock;
volatile dual_func_t dual_func = 0;
void *arg_list[16];

void core2_task(void *arg)
{
  while (1)
  {
    if (dual_func)
    { //corelock_lock(&lock);
      (*dual_func)(1);
      dual_func = 0;
      //corelock_unlock(&lock);
    }

    //usleep(1);
  }
}
int core1_function(void *ctx)
{
  // vTaskStartScheduler();
  core2_task(NULL);
  return 0;
}

volatile bool maixpy_sdcard_loading = true; // There may be deadlocks.
int sd_preload(int core)
{
  bool mounted = false;
  sd_preinit_config();
  if (0 == sd_init())
  {
    bool sd_state = sdcard_is_present();
    if (sd_state)
    {
      // spiffs_stat fno;
      // if there is a file in the flash called "SKIPSD", then we don't mount the SD card
      // if (!mounted_flash || SPIFFS_stat(&spiffs_user_mount_handle.fs, "SKIPSD", &fno) != SPIFFS_OK)
      {
        for (int r = 0; r < 4; r++)
        {
          // fail try again mounted_sdcard.
          if (init_sdcard_fs())
          {
            mounted = true;
            break;
          }
        }
      }
    }
  }

  if (!mounted)
  {
    printk("[maixpy] mount sdcard failed\r\n");
  }

  maixpy_sdcard_loading = false;
  return 0;
}

void mount_sdcard(void){
  // Speed up the system
  maixpy_sdcard_loading = true;
  sd_preload(1);
}

void mp_task(void *pvParameter)
{
  volatile void *tmp;
  volatile void *sp = &tmp;
#if MICROPY_PY_THREAD
  TaskStatus_t task_status;
  vTaskGetInfo(mp_main_task_handle, &task_status, (BaseType_t)pdTRUE, (eTaskState)eInvalid);
  volatile void *mp_main_stack_base = task_status.pxStackBase;
  mp_thread_init((void *)mp_main_stack_base, MP_TASK_STACK_LEN);
#endif
  config_data_t *config = (config_data_t *)pvParameter;
#if MICROPY_ENABLE_GC
  void *gc_heap = malloc(config->gc_heap_size);
  if (!gc_heap)
  {
    printk("GC heap size %d is too large, reset GC heap size\r\n", config->gc_heap_size);
    config->gc_heap_size = 128 * 1024;
    gc_heap = malloc(config->gc_heap_size); // 128KiB
    if (!gc_heap)
    {
      printk("can't malloc 128k, check firmware size\r\n");
      while (1)
      {
      };
    }
    else
    {
      printk("GC size 128k\r\n");
    }
  }
#endif

soft_reset:
  sipeed_reset_sys_mem();
  // initialise the stack pointer for the main thread
  mp_stack_set_top((void *)sp);
#if MICROPY_PY_THREAD
  mp_stack_set_limit(MP_TASK_STACK_SIZE - 1024);
#else
  mp_stack_set_limit(32768); // stack size 32k set in ld
#endif
#if MICROPY_ENABLE_GC
  gc_init(gc_heap, gc_heap + config->gc_heap_size);
  printk("[MaixPy] gc heap=%p-%p(%d)\r\n", gc_heap, gc_heap + config->gc_heap_size, config->gc_heap_size);
#endif
  mp_init();
  mp_obj_list_init(mp_sys_path, 0);
  mp_obj_list_init(mp_sys_argv, 0); //append agrv here
  readline_init0();
  // module init
  if (!omv_init()) //init before uart
  {
    printk("omv init fail\r\n");
  }
#if MICROPY_HW_UART_REPL
  {
    mp_obj_t args[3] = {
#ifdef MAIXPY_DEBUG_UARTHS_REPL_UART2
        MP_OBJ_NEW_SMALL_INT(UART_DEVICE_2),
#else
        MP_OBJ_NEW_SMALL_INT(MICROPY_UARTHS_DEVICE),
#endif
    };
    args[2] = MP_OBJ_NEW_SMALL_INT(8);
    uint32_t freq = sysctl_clock_get_freq(SYSCTL_CLOCK_CPU);
    if (freq < REPL_BAUDRATE_9600_FREQ_THRESHOLD)
    {
      args[1] = MP_OBJ_NEW_SMALL_INT(9600);
    }
    else
    {
      args[1] = MP_OBJ_NEW_SMALL_INT(115200);
    }
#ifdef MAIXPY_DEBUG_UARTHS_REPL_UART2
    fpioa_set_function(9, FUNC_UARTHS_RX);
    fpioa_set_function(10, FUNC_UARTHS_TX);
    fpioa_set_function(4, FUNC_UART2_RX);
    fpioa_set_function(5, FUNC_UART2_TX);
#else
    fpioa_set_function(4, FUNC_UARTHS_RX);
    fpioa_set_function(5, FUNC_UARTHS_TX);
#endif
    MP_STATE_PORT(Maix_stdio_uart) = machine_uart_type.make_new((mp_obj_t)&machine_uart_type, MP_ARRAY_SIZE(args), 0, args);
    uart_attach_to_repl(MP_STATE_PORT(Maix_stdio_uart), true);
  }
#else
  MP_STATE_PORT(Maix_stdio_uart) = NULL;
#endif
  peripherals_init();
  // initialise peripherals
  bool mounted_flash = false;
  mounted_flash = mpy_mount_spiffs(&spiffs_user_mount_handle); //init spiffs of flash
  if (mounted_flash)
  // {  // Uncomment to enable load config from /flash/config.json
  //   maix_config_init();
  // }
  mount_sdcard();
  // mp_printf(&mp_plat_print, "[MaixPy] init end\r\n"); // for maixpy ide
  // run boot-up scripts
  mp_hal_set_interrupt_char(CHAR_CTRL_C);
#if CONFIG_MAIXPY_USE_MEMZIP
  int ret = pyexec_file_if_exists("boot.py");
  if (ret != 0 && !is_ide_dbg_mode()) // user canceled or ide mode
  {
    if (pyexec_mode_kind == PYEXEC_MODE_FRIENDLY_REPL)
    {
      ret = pyexec_file_if_exists("main.py");
    }
  }
#else
  int ret = pyexec_frozen_module("_boot.py");
  if (ret != 0 && !is_ide_dbg_mode()) // user canceled or ide mode
  {
    ret = pyexec_file_if_exists("boot.py");
    if (pyexec_mode_kind == PYEXEC_MODE_FRIENDLY_REPL)
    {
      ret = pyexec_file_if_exists("main.py");
    }
  }
#endif
  do
  {
    ide_dbg_init();
    while ((!ide_dbg_script_ready()) && (!ide_dbg_need_save_file()))
    {
      nlr_buf_t nlr;
      if (nlr_push(&nlr) == 0)
      {
        if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL)
        {
          if (pyexec_raw_repl() != 0)
          {
            break;
          }
        }
        else
        {
          if (pyexec_friendly_repl() != 0)
          {
            break;
          }
        }
      }
      nlr_pop();
    }
    if (ide_dbg_need_save_file())
    {
      ide_save_file();
    }
    if (ide_dbg_script_ready())
    {
      nlr_buf_t nlr;
      if (nlr_push(&nlr) == 0)
      {
        pyexec_str(ide_dbg_get_script());
        nlr_pop();
      }
      else
      {
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
      }
      ide_dbg_on_script_end();
    }
  } while (MP_STATE_PORT(Maix_stdio_uart)->ide_debug_mode);

#if MICROPY_PY_THREAD
  mp_thread_deinit();
#endif
#if MICROPY_ENABLE_GC
  gc_sweep_all();
#endif
  mp_hal_stdout_tx_strn("[MaixPy]: soft reboot\r\n", 23);
  mp_deinit();
  // Zero out the GC/heap
  gc_wipe();
  msleep(10);
  goto soft_reset;
  // sysctl->soft_reset.soft_reset = 1;
}

void fpioa_clear()
{
  for (int index = 0; index < FPIOA_NUM_IO; index++)
    fpioa_set_function_raw(index, FUNC_RESV0);
}

int maixpy_main()
{
  uint8_t manuf_id, device_id;
  config_data_t config;
  sysctl_pll_set_freq(SYSCTL_PLL0, FREQ_PLL0_DEFAULT);
  sysctl_pll_set_freq(SYSCTL_PLL1, FREQ_PLL1_DEFAULT);
  sysctl_pll_set_freq(SYSCTL_PLL2, FREQ_PLL2_DEFAULT);
  fpioa_clear();
  fpioa_set_function(4, FUNC_UARTHS_RX);
  fpioa_set_function(5, FUNC_UARTHS_TX);
  uarths_init();
  uarths_config(115200, 1);
  flash_init(&manuf_id, &device_id);
  init_flash_spiffs();
  load_config_from_spiffs(&config);
  sysctl_cpu_set_freq(config.freq_cpu);
  sysctl_pll_set_freq(SYSCTL_PLL1, config.freq_pll1);
  sysctl_clock_set_threshold(SYSCTL_THRESHOLD_AI, 15);
  dmac_init();
  plic_init();
  uarths_init();
  uarths_config(115200, 1);
  printk("\r\n");
  printk("[MAIXPY] Pll0:freq:%d\r\n", sysctl_clock_get_freq(SYSCTL_CLOCK_PLL0));
  printk("[MAIXPY] Pll1:freq:%d\r\n", sysctl_clock_get_freq(SYSCTL_CLOCK_PLL1));
  printk("[MAIXPY] Pll2:freq:%d\r\n", sysctl_clock_get_freq(SYSCTL_CLOCK_PLL2));
  printk("[MAIXPY] cpu:freq:%d\r\n", sysctl_clock_get_freq(SYSCTL_CLOCK_CPU));
  printk("[MAIXPY] kpu:freq:%d\r\n", sysctl_clock_get_freq(SYSCTL_CLOCK_AI));
  sysctl_clock_enable(SYSCTL_CLOCK_AI);
  sysctl_set_power_mode(SYSCTL_POWER_BANK6, SYSCTL_POWER_V18);
  sysctl_set_power_mode(SYSCTL_POWER_BANK7, SYSCTL_POWER_V18);
  sysctl_enable_irq();
  rtc_init();
  rtc_timer_set(2000, 1, 1, 0, 0, 0);
  flash_init(&manuf_id, &device_id);
  printk("[MAIXPY] Flash:0x%02x:0x%02x\r\n", manuf_id, device_id);
  /* Init SPI IO map and function settings */
  sysctl_set_spi0_dvp_data(1);
  /* open core 1 */
  // printk("open second core...\r\n");
  register_core1(core1_function, 0);

#if MICROPY_PY_THREAD
  xTaskCreateAtProcessor(0,                     // processor
                         mp_task,               // function entry
                         "mp_task",             //task name
                         MP_TASK_STACK_LEN,     //stack_deepth
                         &config,               //function arg
                         MP_TASK_PRIORITY,      //task priority
                         &mp_main_task_handle); //task handl
  // xTaskCreateAtProcessor(1, core2_task, "core2_task", 256, NULL, tskIDLE_PRIORITY+1, NULL );
  vTaskStartScheduler();
  for (;;)
    ;
#else
  mp_task(&config);
#endif
  return 0;
}
void do_str(const char *src, mp_parse_input_kind_t input_kind)
{
  nlr_buf_t nlr;
  if (nlr_push(&nlr) == 0)
  {
    mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
    qstr source_name = lex->source_name;
    mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
    mp_obj_t module_fun = mp_compile(&parse_tree, source_name, MP_EMIT_OPT_NONE, true);
    mp_call_function_0(module_fun);
    nlr_pop();
  }
  else
  {
    // uncaught exception
    mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
  }
}

void nlr_jump_fail(void *val)
{
  printk("nlr_jump_fail\r\n");
  while (1)
    ;
}

#if !MICROPY_DEBUG_PRINTERS
// With MICROPY_DEBUG_PRINTERS disabled DEBUG_printf is not defined but it
// is still needed by esp-open-lwip for debugging output, so define it here.
#include <stdarg.h>
int mp_vprintf(const mp_print_t *print, const char *fmt, va_list args);
int DEBUG_printf(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  int ret = mp_vprintf(MICROPY_DEBUG_PRINTER, fmt, ap);
  va_end(ap);
  return ret;
}
#endif
