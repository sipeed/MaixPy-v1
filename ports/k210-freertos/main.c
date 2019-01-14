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
/*****peripheral****/
#include "fpioa.h"
#include "gpio.h"
#include "timer.h"
#include "w25qxx.h"
#include "uarths.h"
#include "rtc.h"
#include "uart.h"
/*****freeRTOS****/
#include "FreeRTOS.h"
#include "task.h"
/*******spiffs********/
#include "vfs_spiffs.h"
#include "spiffs_configport.h"
#include "spiffs-port.h"

#define UART_BUF_LENGTH_MAX 269
#define MPY_HEAP_SIZE 1 * 1024 * 1024
extern int mp_hal_stdin_rx_chr(void);

// static char *stack_top;
#if MICROPY_ENABLE_GC
static char heap[MPY_HEAP_SIZE];
#endif

// #define MP_TASK_PRIORITY        4
// #define MP_TASK_STACK_SIZE      (16 * 1024)
// #define MP_TASK_STACK_LEN       (MP_TASK_STACK_SIZE / sizeof(StackType_t))

// STATIC StackType_t mp_task_stack[MP_TASK_STACK_LEN] __attribute__((aligned (8)));
// TaskHandle_t mp_main_task_handle;

#define FORMAT_FS_FORCE 0
static u8_t spiffs_work_buf[SPIFFS_CFG_LOG_PAGE_SZ(fs)*2];
static u8_t spiffs_fds[32*4];
static u8_t spiffs_cache_buf[(SPIFFS_CFG_LOG_PAGE_SZ(fs)+32)*4];
spiffs_user_mount_t spiffs_user_mount_handle;
uint8_t init_py_file[]={
0x69,0x6d,0x70,0x6f,0x72,0x74,0x20,0x75,0x6f,0x73,0x0a,0x69,0x6d,0x70,0x6f,0x72,
0x74,0x20,0x6f,0x73,0x0a,0x69,0x6d,0x70,0x6f,0x72,0x74,0x20,0x6d,0x61,0x63,0x68,
0x69,0x6e,0x65,0x0a,0x69,0x6d,0x70,0x6f,0x72,0x74,0x20,0x63,0x6f,0x6d,0x6d,0x6f,
0x6e,0x0a,0x70,0x69,0x6e,0x5f,0x69,0x6e,0x69,0x74,0x3d,0x63,0x6f,0x6d,0x6d,0x6f,
0x6e,0x2e,0x70,0x69,0x6e,0x5f,0x69,0x6e,0x69,0x74,0x28,0x29,0x0a,0x70,0x69,0x6e,
0x5f,0x69,0x6e,0x69,0x74,0x2e,0x69,0x6e,0x69,0x74,0x28,0x29,0x0a,0x74,0x65,0x73,
0x74,0x5f,0x67,0x70,0x69,0x6f,0x5f,0x70,0x69,0x6e,0x5f,0x6e,0x75,0x6d,0x3d,0x31,
0x35,0x0a,0x66,0x70,0x69,0x6f,0x61,0x3d,0x6d,0x61,0x63,0x68,0x69,0x6e,0x65,0x2e,
0x66,0x70,0x69,0x6f,0x61,0x28,0x29,0x0a,0x66,0x70,0x69,0x6f,0x61,0x2e,0x73,0x65,
0x74,0x5f,0x66,0x75,0x6e,0x63,0x74,0x69,0x6f,0x6e,0x28,0x74,0x65,0x73,0x74,0x5f,
0x67,0x70,0x69,0x6f,0x5f,0x70,0x69,0x6e,0x5f,0x6e,0x75,0x6d,0x2c,0x36,0x33,0x29,
0x0a,0x74,0x65,0x73,0x74,0x5f,0x70,0x69,0x6e,0x3d,0x6d,0x61,0x63,0x68,0x69,0x6e,
0x65,0x2e,0x70,0x69,0x6e,0x28,0x37,0x2c,0x32,0x2c,0x30,0x29,0x0a,0x6c,0x63,0x64,
0x3d,0x6d,0x61,0x63,0x68,0x69,0x6e,0x65,0x2e,0x73,0x74,0x37,0x37,0x38,0x39,0x28,
0x29,0x0a,0x6c,0x63,0x64,0x2e,0x69,0x6e,0x69,0x74,0x28,0x29,0x0a,0x6c,0x63,0x64,
0x2e,0x64,0x72,0x61,0x77,0x5f,0x73,0x74,0x72,0x69,0x6e,0x67,0x28,0x31,0x31,0x36,
0x2c,0x31,0x32,0x31,0x2c,0x22,0x57,0x65,0x6c,0x63,0x6f,0x6d,0x65,0x20,0x74,0x6f,
0x20,0x4d,0x61,0x69,0x78,0x50,0x79,0x22,0x29,0x0a,0x69,0x66,0x20,0x74,0x65,0x73,
0x74,0x5f,0x70,0x69,0x6e,0x2e,0x76,0x61,0x6c,0x75,0x65,0x28,0x29,0x20,0x3d,0x3d,
0x20,0x30,0x3a,0x0a,0x20,0x20,0x20,0x20,0x70,0x72,0x69,0x6e,0x74,0x28,0x27,0x74,
0x65,0x73,0x74,0x27,0x29,0x0a,0x20,0x20,0x20,0x20,0x6d,0x61,0x63,0x68,0x69,0x6e,
0x65,0x2e,0x74,0x65,0x73,0x74,0x28,0x29,0x0a};

void do_str(const char *src, mp_parse_input_kind_t input_kind);

const char* Banner = {"\n __  __              _____  __   __  _____   __     __ \n\
|  \\/  |     /\\     |_   _| \\ \\ / / |  __ \\  \\ \\   / /\n\
| \\  / |    /  \\      | |    \\ V /  | |__) |  \\ \\_/ / \n\
| |\\/| |   / /\\ \\     | |     > <   |  ___/    \\   /  \n\
| |  | |  / ____ \\   _| |_   / . \\  | |         | |   \n\
|_|  |_| /_/    \\_\\ |_____| /_/ \\_\\ |_|         |_|\n\
Official Site:http://www.sipeed.com/\n\
Wiki:http://maixpy.sipeed.com/\n"};

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
			return -1;
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
			printf("[MAIXPY]:Spiffs Write init file\n");
			spiffs_file fd;
			fd=SPIFFS_open(&vfs_spiffs->fs,"init.py", SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
			if(fd != -1){
				s32_t ls_res = SPIFFS_lseek(&vfs_spiffs->fs, fd,0,0);
				if(!ls_res){
					s32_t w_res = SPIFFS_write(&vfs_spiffs->fs, fd,init_py_file,sizeof(init_py_file));
					if(w_res <= 0){
					}else{
						SPIFFS_fflush(&vfs_spiffs->fs, fd);
					}
				}
			}
			SPIFFS_close (&vfs_spiffs->fs, fd);
		}
	}
	
	// mp_obj_t args[2] = {MP_OBJ_FROM_PTR(vfs_spiffs), mp_obj_new_str("/",1) };
	// mp_map_t kw_args;
    // mp_map_init_fixed_table(&kw_args, 0, NULL);
	// mp_vfs_mount(2, args, &kw_args);
	mp_vfs_mount_t *vfs = m_new_obj(mp_vfs_mount_t);
    if (vfs == NULL) {
        printf("[MaixPy]:can't mount flash\n");
    }
    vfs->str = "/flash";
    vfs->len = 6;
    vfs->obj = MP_OBJ_FROM_PTR(vfs_spiffs);
    vfs->next = NULL;
    MP_STATE_VM(vfs_mount_table) = vfs;
	return true;
}

void mp_task(
	#if MICROPY_PY_THREAD 
	void *pvParameter
	#endif
	) {
		volatile uint32_t sp = (uint32_t)get_sp();
#if MICROPY_PY_THREAD
		mp_thread_init(&mp_task_stack[0], MP_TASK_STACK_LEN);
#endif
// soft_reset:
		// initialise the stack pointer for the main thread
		mp_stack_set_top((void *)(uint64_t)sp);
		mp_stack_set_limit(MP_TASK_STACK_SIZE - 1024);
#if MICROPY_ENABLE_GC
		gc_init(heap, heap + sizeof(heap));
#endif
		mp_init();
		mp_obj_list_init(mp_sys_path, 0);
		mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_));
		mp_obj_list_init(mp_sys_argv, 0);//append agrv here
		init_flash_spiffs();//init spiffs of flash
    	readline_init0();
        readline_process_char(27);
		//pyexec_event_repl_init();
		pyexec_frozen_module("boot.py");
		MP_THREAD_GIL_EXIT();//given gil

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

		mp_hal_set_interrupt_char(CHAR_CTRL_C);
		printf(Banner);

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
		mp_deinit();
		printf("porwer reset\n");
		msleep(1);	    
		sysctl->soft_reset.soft_reset = 1;
}

int main()
{		
	printk("[MAIXPY]Pll0:freq:%d\r\n",sysctl_clock_get_freq(SYSCTL_CLOCK_PLL0));
	printk("[MAIXPY]Pll1:freq:%d\r\n",sysctl_clock_get_freq(SYSCTL_CLOCK_PLL1));
	sysctl_set_power_mode(SYSCTL_POWER_BANK6,SYSCTL_POWER_V33);
	sysctl_set_power_mode(SYSCTL_POWER_BANK7,SYSCTL_POWER_V33);
	dmac_init();
	plic_init();
    sysctl_enable_irq();
	rtc_init();
	rtc_timer_set(1970,1, 1,0, 0, 0);
	uint8_t manuf_id, device_id;
	w25qxx_init_dma(3, 0);
	w25qxx_enable_quad_mode_dma();
	w25qxx_read_id_dma(&manuf_id, &device_id);
	printk("[MAIXPY]Flash:0x%02x:0x%02x\n", manuf_id, device_id);
	/*
	xTaskCreateAtProcessor(0, // processor
					     mp_task, // function entry
					     "mp_task", //task name
					     MP_TASK_STACK_LEN, //stack_deepth
					     NULL, //function arg
					     MP_TASK_PRIORITY, //task priority
					     &mp_main_task_handle);//task handl
	*/
	mp_task();
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


