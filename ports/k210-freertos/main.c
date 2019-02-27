/*
* Copyright 2019 Sipeed Co.,Ltd.

* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

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
#include "FreeRTOS.h"
#include "task.h"
/*******storage********/
#include "vfs_spiffs.h"
#include "spiffs_configport.h"
#include "spiffs-port.h"
#include "machine_sdcard.h"
#include "machine_uart.h"
/**********omv**********/
#include "omv_boardconfig.h"
#include "framebuffer.h"
#include "sensor.h"
#include "omv.h"
#include "sipeed_conv.h"

#define UART_BUF_LENGTH_MAX 269

// #define MPY_HEAP_SIZE  2* 1024 * 1024

#define MPY_HEAP_SIZE  512* 1024 

uint8_t CPU_freq = 0;
uint8_t PLL0_freq = 0;
uint8_t PLL1_freq = 0;
uint8_t PLL2_freq = 0;

uint8_t* _fb_base;
uint8_t* _jpeg_buf;

#if MICROPY_ENABLE_GC
static char heap[MPY_HEAP_SIZE] __attribute__((aligned(8))); 
#endif

#if MICROPY_PY_THREAD 
#define MP_TASK_PRIORITY        4
#define MP_TASK_STACK_SIZE      (32 * 1024)
#define MP_TASK_STACK_LEN       (MP_TASK_STACK_SIZE / sizeof(StackType_t))
TaskHandle_t mp_main_task_handle;
#endif

#define FORMAT_FS_FORCE 0
// u8_t spiffs_work_buf[SPIFFS_CFG_LOG_PAGE_SZ(fs)*2];
// u8_t spiffs_fds[32*4];
// u8_t spiffs_cache_buf[(SPIFFS_CFG_LOG_PAGE_SZ(fs)+32)*4];
spiffs_user_mount_t spiffs_user_mount_handle;

void do_str(const char *src, mp_parse_input_kind_t input_kind);

const char Banner[] = {"\r\n __  __              _____  __   __  _____   __     __ \r\n\
|  \\/  |     /\\     |_   _| \\ \\ / / |  __ \\  \\ \\   / /\r\n\
| \\  / |    /  \\      | |    \\ V /  | |__) |  \\ \\_/ / \r\n\
| |\\/| |   / /\\ \\     | |     > <   |  ___/    \\   /  \r\n\
| |  | |  / ____ \\   _| |_   / . \\  | |         | |   \r\n\
|_|  |_| /_/    \\_\\ |_____| /_/ \\_\\ |_|         |_|\r\n\
Official Site:https://www.sipeed.com/\r\n\
Wiki:https://maixpy.sipeed.com/\r\n"};

STATIC bool init_sdcard_fs(void) {
    bool first_part = true;
    for (int part_num = 1; part_num <= 4; ++part_num) {
        // create vfs object
        fs_user_mount_t *vfs_fat = m_new_obj_maybe(fs_user_mount_t);
        mp_vfs_mount_t *vfs = m_new_obj_maybe(mp_vfs_mount_t);
        if (vfs == NULL || vfs_fat == NULL) {
            break;
        }
        vfs_fat->flags = FSUSER_FREE_OBJ;
        sdcard_init_vfs(vfs_fat, part_num);

        // try to mount the partition
        FRESULT res = f_mount(&vfs_fat->fatfs);
        if (res != FR_OK) {
            // couldn't mount
            m_del_obj(fs_user_mount_t, vfs_fat);
            m_del_obj(mp_vfs_mount_t, vfs);
        } 
		else 
		{
            // mounted via FatFs, now mount the SD partition in the VFS
            if (first_part) {
                // the first available partition is traditionally called "sd" for simplicity
                vfs->str = "/sd";
                vfs->len = 3;
            } else {
                // subsequent partitions are numbered by their index in the partition table
                if (part_num == 2) {
                    vfs->str = "/sd2";
                } else if (part_num == 2) {
                    vfs->str = "/sd3";
                } else {
                    vfs->str = "/sd4";
                }
                vfs->len = 4;
            }
            vfs->obj = MP_OBJ_FROM_PTR(vfs_fat);
            vfs->next = NULL;
            for (mp_vfs_mount_t **m = &MP_STATE_VM(vfs_mount_table);; m = &(*m)->next) {
                if (*m == NULL) {
                    *m = vfs;
                    break;
                }
            }
            if (first_part) {
                // use SD card as current directory
                MP_STATE_PORT(vfs_cur) = vfs;
				first_part = false;
            }
        }
    }
	
    if (first_part) {
        printf("PYB: can't mount SD card\n");
        return false;
    } else {
        return true;
    }
}



MP_NOINLINE STATIC bool init_flash_spiffs()
{

	spiffs_user_mount_t* vfs_spiffs = &spiffs_user_mount_handle;
	vfs_spiffs->flags = SYS_SPIFFS;
	vfs_spiffs->base.type = &mp_spiffs_vfs_type;
	vfs_spiffs->fs.user_data = vfs_spiffs;
	vfs_spiffs->cfg.hal_read_f = spiffs_read_method;
	vfs_spiffs->cfg.hal_write_f = spiffs_write_method;
	vfs_spiffs->cfg.hal_erase_f = spiffs_erase_method;
	
	vfs_spiffs->cfg.phys_size = SPIFFS_CFG_PHYS_SZ(); // use all spi flash
	vfs_spiffs->cfg.phys_addr = SPIFFS_CFG_PHYS_ADDR(); // start spiffs at start of spi flash
	vfs_spiffs->cfg.phys_erase_block = SPIFFS_CFG_PHYS_ERASE_SZ(); // according to datasheet
	vfs_spiffs->cfg.log_block_size = SPIFFS_CFG_LOG_BLOCK_SZ(); // let us not complicate things
	vfs_spiffs->cfg.log_page_size = SPIFFS_CFG_LOG_PAGE_SZ(); // as we said
	int res = SPIFFS_mount(&vfs_spiffs->fs,
					   &vfs_spiffs->cfg,
					   spiffs_work_buf,
					   spiffs_fds,
					   sizeof(spiffs_fds),
				       spiffs_cache_buf,
					   sizeof(spiffs_cache_buf),
					   0);
	if(FORMAT_FS_FORCE || res != SPIFFS_OK || res==SPIFFS_ERR_NOT_A_FS)
	{
		SPIFFS_unmount(&vfs_spiffs->fs);
		printf("[MAIXPY]:Spiffs Unmount.\n");
		printf("[MAIXPY]:Spiffs Formating...\n");
		s32_t format_res=SPIFFS_format(&vfs_spiffs->fs);
		printf("[MAIXPY]:Spiffs Format %s \n",format_res?"failed":"successful");
		if(0 != format_res)
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
		printf("[MAIXPY]:Spiffs Mount %s \n", res?"failed":"successful");
		if(!res)
		{

		}
	}
	
	mp_vfs_mount_t *vfs = m_new_obj(mp_vfs_mount_t);
    if (vfs == NULL) {
        printf("[MaixPy]:can't mount flash\n");
		return false;
    }
    vfs->str = "/flash";
    vfs->len = 6;
    vfs->obj = MP_OBJ_FROM_PTR(vfs_spiffs);
    vfs->next = NULL;
	for (mp_vfs_mount_t **m = &MP_STATE_VM(vfs_mount_table);; m = &(*m)->next) {
		if (*m == NULL) {
			*m = vfs;
			break;
		}
	}
	return true;
}


void mp_task(
	#if MICROPY_PY_THREAD 
	void *pvParameter
	#endif
	) {
#if MICROPY_PY_THREAD
		volatile void *stack_p = 0;
        volatile void *mp_main_stack_top = &stack_p;
		mp_thread_init(mp_main_stack_top, MP_TASK_STACK_LEN);
#else
		volatile* mp_main_stack_top = (uint32_t)get_sp();
#endif
soft_reset:
		// initialise the stack pointer for the main thread
		mp_stack_set_top((void *)(uint64_t)mp_main_stack_top);
		//mp_stack_set_limit(MP_TASK_STACK_SIZE - 1024);//Not open MICROPY_STACK_CHECK
#if MICROPY_ENABLE_GC
		gc_init(heap, heap + sizeof(heap));
		printk("heap0=%x\r\n",heap);
#endif
		mp_init();
		mp_obj_list_init(mp_sys_path, 0);
		mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_));
		mp_obj_list_init(mp_sys_argv, 0);//append agrv here
    	readline_init0();
		// module init
		omv_init();
		// initialise peripherals
		bool mounted_sdcard = false;
		bool mounted_flash= false;
		mounted_flash = init_flash_spiffs();//init spiffs of flash
		sd_init();
		if (sdcard_is_present()) {
			spiffs_stat  fno;
        // if there is a file in the flash called "SKIPSD", then we don't mount the SD card
	        if (!mounted_flash || SPIFFS_stat(&spiffs_user_mount_handle.fs,"SKIPSD",&fno) != SPIFFS_OK){
	            mounted_sdcard = init_sdcard_fs();
	        }
    	}
		if (mounted_sdcard) {
		}
#if MICROPY_HW_UART_REPL
		{
			mp_obj_t args[3] = {
				MP_OBJ_NEW_SMALL_INT(MICROPY_UARTHS_DEVICE),
				MP_OBJ_NEW_SMALL_INT(115200),
				MP_OBJ_NEW_SMALL_INT(8),
			};
			MP_STATE_PORT(Maix_stdio_uart) = machine_uart_type.make_new((mp_obj_t)&machine_uart_type, MP_ARRAY_SIZE(args), 0, args);
			uart_attach_to_repl(MP_STATE_PORT(Maix_stdio_uart), true);
		}
#else
		MP_STATE_PORT(Maix_stdio_uart) = NULL;
#endif
		// run boot-up scripts
		mp_hal_set_interrupt_char(CHAR_CTRL_C);
		pyexec_frozen_module("_boot.py");
		mp_hal_stdout_tx_strn(Banner, strlen(Banner));

		for (;;) {
			if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
				if (pyexec_raw_repl() != 0) {
					break;
				}
			} else {
				if (pyexec_friendly_repl() != 0) {
					break;
				}
			}
		}

#if MICROPY_PY_THREAD
		mp_thread_deinit();
#endif
#if MICROPY_ENABLE_GC
		gc_sweep_all();
#endif
		mp_hal_stdout_tx_strn("[MaixPy]: soft reboot\r\n", 23);
		mp_deinit();
		msleep(10);	    
		goto soft_reset;
		// sysctl->soft_reset.soft_reset = 1;
}


#define CPU 0
#define KPU 1
#define I2S 2
int main()
{	
	uint8_t manuf_id, device_id;
	sysctl_pll_set_freq(SYSCTL_PLL0, PLL0_MAX_OUTPUT_FREQ);
	sysctl_pll_set_freq(SYSCTL_PLL1, PLL1_MAX_OUTPUT_FREQ);
	sysctl_pll_set_freq(SYSCTL_PLL2, PLL2_MAX_OUTPUT_FREQ);
	uarths_init();
	uint32_t store_freq[3] = {0};
	uint32_t max_freq[3] = {0};
	store_freq[0] = CPU_MAX_FREQ;//0 -> PLL0
	store_freq[1] = KPU_MAX_FREQ;//1 -> PLL1
	store_freq[2] = I2S_MAX_FREQ;//2 -> PLL2
	max_freq[0] = CPU_MAX_FREQ;//0 -> PLL0
	max_freq[1] = KPU_MAX_FREQ;//1 -> PLL1
	max_freq[2] = I2S_MAX_FREQ;//2 -> PLL2
	w25qxx_init_dma(3, 0);
	w25qxx_enable_quad_mode_dma();
	w25qxx_read_id_dma(&manuf_id, &device_id);
	uint32_t res = 0;
	for(int i = 0; i < FREQ_READ_NUM; i++)
	{
		res = sys_spiffs_read(FREQ_STORE_ADDR + i * 4 ,4,(uint8_t* )(&store_freq[i]));
		store_freq[i] = store_freq[i] > max_freq[i] ? max_freq[i] : store_freq[i];
		store_freq[i] = store_freq[i] < 26000000 ? max_freq[i] : store_freq[i];
	}
	sysctl_cpu_set_freq(store_freq[CPU]);
	uarths_init();
	sysctl_clock_set_threshold(SYSCTL_THRESHOLD_AI, (PLL1_MAX_OUTPUT_FREQ / store_freq[KPU]) / 2);
	printk("[MAIXPY]Pll0:freq:%d\r\n",sysctl_clock_get_freq(SYSCTL_CLOCK_PLL0));
	printk("[MAIXPY]Pll1:freq:%d\r\n",sysctl_clock_get_freq(SYSCTL_CLOCK_PLL1));
	printk("[MAIXPY]Pll2:freq:%d\r\n",sysctl_clock_get_freq(SYSCTL_CLOCK_PLL2));
	printk("[MAIXPY]cpu:freq:%d\r\n",sysctl_clock_get_freq(SYSCTL_CLOCK_CPU));
	printk("[MAIXPY]kpu:freq:%d\r\n",sysctl_clock_get_freq(SYSCTL_CLOCK_AI));
	sysctl_clock_enable(SYSCTL_CLOCK_AI);
	sysctl_set_power_mode(SYSCTL_POWER_BANK6,SYSCTL_POWER_V18);
	sysctl_set_power_mode(SYSCTL_POWER_BANK7,SYSCTL_POWER_V18);
	dmac_init();
	plic_init();
    sysctl_enable_irq();
	rtc_init();
	rtc_timer_set(2019,1, 1,0, 0, 0);
	w25qxx_init_dma(3, 0);
	w25qxx_enable_quad_mode_dma();
	w25qxx_read_id_dma(&manuf_id, &device_id);
	printk("[MAIXPY]Flash:0x%02x:0x%02x\r\n", manuf_id, device_id);
    /* Init SPI IO map and function settings */
    sysctl_set_spi0_dvp_data(1);
#if MICROPY_PY_THREAD 
	xTaskCreateAtProcessor(0, // processor
						 mp_task, // function entry
						 "mp_task", //task name
						 MP_TASK_STACK_LEN, //stack_deepth
						 NULL, //function arg
						 MP_TASK_PRIORITY, //task priority
						 &mp_main_task_handle);//task handl
	vTaskStartScheduler();
	for(;;);
#else
	mp_task();
#endif

}
void do_str(const char *src, mp_parse_input_kind_t input_kind) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, MP_EMIT_OPT_NONE, true);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
}

void nlr_jump_fail(void *val) {
    while (1);
}

#if !MICROPY_DEBUG_PRINTERS
// With MICROPY_DEBUG_PRINTERS disabled DEBUG_printf is not defined but it
// is still needed by esp-open-lwip for debugging output, so define it here.
#include <stdarg.h>
int mp_vprintf(const mp_print_t *print, const char *fmt, va_list args);
int DEBUG_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = mp_vprintf(MICROPY_DEBUG_PRINTER, fmt, ap);
    va_end(ap);
    return ret;
}
#endif


