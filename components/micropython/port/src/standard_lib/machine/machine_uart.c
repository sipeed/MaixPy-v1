/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "py/nlr.h"
#include "py/obj.h"
#include "py/binary.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "py/parsenum.h"
#include "py/formatfloat.h"
#include "py/runtime.h"
#include "lib/utils/interrupt_char.h"
#include "sleep.h"

#include "mpconfigboard.h"
#include "modmachine.h"
#include "machine_uart.h"
#include "uart.h"
#include "uarths.h"
#include "syslog.h"
#include "plic.h"
#include "machine_uart.h"
#include "ide_dbg.h"
#include "buffer.h"
#include "imlib_config.h"
#include "mphalport.h"

extern int uart_channel_getc(uart_device_number_t channel);

#define Maix_DEBUG 0
#if Maix_DEBUG==1
#define debug_print(x,arg...) mp_printf(&mp_plat_print, "[MaixPy]"x,##arg)
#else 
#define debug_print(x,arg...) 
#endif

#define Maix_KDEBUG 0
#if Maix_KDEBUG==1
#define debug_printk(x,arg...) mp_printf(&mp_plat_print, "[MaixPy]"x,##arg)
#else 
#define debug_printk(x,arg...) 
#endif


machine_uart_obj_t* g_repl_uart_obj = NULL;

STATIC const char *_parity_name[] = {"None", "odd", "even"};
STATIC const char *_stop_name[] = {"1", "1.5", "2"};
Buffer_t g_uart_send_buf_ide;

//QueueHandle_t UART_QUEUE[UART_DEVICE_MAX] = {};

/******************************************************************************/
// MicroPython bindings for UART

void DISABLE_RX_INT(machine_uart_obj_t *self)
{
	self->rx_int_flag = 0;
	uart_irq_unregister(self->uart_num, UART_RECEIVE);
}
void DISABLE_HSRX_INT(machine_uart_obj_t *self)
{
	self->rx_int_flag = 0;
	plic_irq_disable(IRQN_UARTHS_INTERRUPT);
	plic_irq_unregister(IRQN_UARTHS_INTERRUPT);

}
mp_uint_t uart_rx_any(machine_uart_obj_t *self) 
{
	int buffer_bytes = self->read_buf_head - self->read_buf_tail;
	if (buffer_bytes < 0)
	{
		return buffer_bytes + self->read_buf_len ;
	} 
	else if (buffer_bytes > 0)
	{
		return buffer_bytes ;
	} 
	else 
	{
		//__HAL_UART_GET_FLAG(&self->uart, UART_FLAG_RXNE) != RESET
		return 0;
	}
}

mp_obj_t uart_any(mp_obj_t self_)
{
	machine_uart_obj_t* self = (machine_uart_obj_t*)self_;
	return mp_obj_new_int(uart_rx_any(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_uart_any_obj, uart_any);

int uart_rx_irq(void *ctx)
{
	uint8_t read_tmp;
	machine_uart_obj_t* ctx_self= (machine_uart_obj_t*)ctx;
	if (ctx_self == NULL) {
		return 0;
	}
	if (ctx_self->read_buf_len != 0) {
		if(ctx_self->attached_to_repl)
		{
#if  !defined(OMV_MINIMUM)|| CONFIG_MAIXPY_IDE_SUPPORT
			if(ctx_self->ide_debug_mode)
			{
				int read_ret = 0; 
				do{
					if(MICROPY_UARTHS_DEVICE == ctx_self->uart_num)
						read_ret = uarths_getchar();
					else if(UART_DEVICE_MAX > ctx_self->uart_num)
						read_ret = uart_channel_getc(ctx_self->uart_num);
					if(read_ret == EOF)
						break;
					read_tmp = (uint8_t)read_ret;
					ide_dbg_dispatch_cmd(ctx_self, &read_tmp);
				}while(1);
			}
			else
#endif // #if  defined(OMV_MINIMUM)|| CONFIG_MAIXPY_IDE_SUPPORT
			{
				int read_ret = 0;
				do{
					uint16_t next_head = (ctx_self->read_buf_head + 1) % ctx_self->read_buf_len;
					// only read data if room in buf
					if (next_head != ctx_self->read_buf_tail) {
						if(MICROPY_UARTHS_DEVICE == ctx_self->uart_num)
							read_ret = uarths_getchar();
						else if(UART_DEVICE_MAX > ctx_self->uart_num)
							read_ret = uart_channel_getc(ctx_self->uart_num);
						if(read_ret == EOF)
							break;
						read_tmp = (uint8_t)read_ret;
						ctx_self->read_buf[ctx_self->read_buf_head] = read_tmp;
						ctx_self->read_buf_head = next_head;
						ctx_self->data_len++;
						// Handle interrupt coming in on a UART REPL
						if (read_tmp == mp_interrupt_char) {
							if (MP_STATE_VM(mp_pending_exception) == MP_OBJ_NULL) {
								mp_keyboard_interrupt();
							} else {
								MP_STATE_VM(mp_pending_exception) = MP_OBJ_NULL;
								//pendsv_object = &MP_STATE_VM(mp_kbd_exception);
							}
						}
					}
					else {
						do{
							// No room: leave char in buf, disable interrupt,open it util rx char
							if(MICROPY_UARTHS_DEVICE == ctx_self->uart_num)
								read_ret = uarths_getchar();
							else if(UART_DEVICE_MAX > ctx_self->uart_num)
								read_ret = uart_channel_getc(ctx_self->uart_num);
						}while(read_ret!=EOF);
						break;
					}
				}while(1);
			}
		}
		else
		{
			int read_ret = 0;
			do{
				uint16_t next_head = (ctx_self->read_buf_head + 1) % ctx_self->read_buf_len;
				while (next_head != ctx_self->read_buf_tail)
				{
					if(MICROPY_UARTHS_DEVICE == ctx_self->uart_num)
						read_ret = uarths_getchar();
					else if(UART_DEVICE_MAX > ctx_self->uart_num)
						read_ret = uart_channel_getc(ctx_self->uart_num);
					if(read_ret == EOF)
						break;
					ctx_self->read_buf[ctx_self->read_buf_head] = (uint8_t)read_ret;
					ctx_self->read_buf_head = next_head;
					ctx_self->data_len++;
					next_head = (ctx_self->read_buf_head + 1) % ctx_self->read_buf_len;
				}
				if(next_head == ctx_self->read_buf_tail)
				{
					do{
						if(MICROPY_UARTHS_DEVICE == ctx_self->uart_num)
							read_ret = uarths_getchar();
						else if(UART_DEVICE_MAX > ctx_self->uart_num)
							read_ret = uart_channel_getc(ctx_self->uart_num);
					}while(read_ret!=EOF);
					break;
				}
			}while(read_ret!=EOF);
		}
	}
#if MICROPY_PY_THREAD
	mp_hal_wake_main_task_from_isr();
#endif
	return 0;
}



void ENABLE_RX_INT(machine_uart_obj_t *self)
{
	uart_irq_register(self->uart_num, UART_RECEIVE, uart_rx_irq, self, 1);
	self->rx_int_flag = 1;
}

void ENABLE_HSRX_INT(machine_uart_obj_t *self)
{
	self->rx_int_flag = 1;
	uarths_set_irq(UARTHS_RECEIVE,uart_rx_irq,self, 1);
}


bool uart_rx_wait(machine_uart_obj_t *self, uint32_t timeout) 
{
    uint32_t start = mp_hal_ticks_ms();
	debug_print("uart_rx_wait | read_buf_head = %d\n",self->read_buf_head);
	debug_print("uart_rx_wait | read_buf_tail = %d\n",self->read_buf_tail);
    for (;;) {
        if (self->read_buf_tail != self->read_buf_head) {
            return true; // have at least 1 char ready for reading
        }
        if (mp_hal_ticks_ms() - start >= timeout) {
            return false; // timeout
        }
    }
}

int uart_rx_char(mp_obj_t self_) 
{
	machine_uart_obj_t* self = (machine_uart_obj_t*)self_;
    if (self->read_buf_tail != self->read_buf_head) {
        uint8_t data;
        data = self->read_buf[self->read_buf_tail];
        self->read_buf_tail = (self->read_buf_tail + 1) % self->read_buf_len;
		self->data_len--;
        if (self->rx_int_flag == 0) {
            //re-enable IRQ now we have room in buffer
      		if(MICROPY_UARTHS_DEVICE == self->uart_num)
				ENABLE_HSRX_INT(self);	
			else if(UART_DEVICE_MAX > self->uart_num)
				ENABLE_RX_INT(self);
        }
        return data;
    }
	return -1;
}


// assumes there is a character available
mp_obj_t uart_rx_char_py(void *self_) 
{
	return mp_obj_new_int(uart_rx_char(self_));
}

mp_obj_t uart_readchar(machine_uart_obj_t *self) 
{
	int data = uart_rx_char(self);

	if(data != -1)
	{
		return mp_obj_new_bytes((byte*)&data,1);
	}
	return MP_OBJ_NULL;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_uart_rx_char_obj, uart_rx_char_py);

int uart_rx_data(machine_uart_obj_t *self,uint8_t* buf_in,uint32_t size) 
{
	uint16_t data_num = 0;
	uint8_t* buf = buf_in;
    while(self->read_buf_tail != self->read_buf_head && size > 0) 
	{
        *buf = self->read_buf[self->read_buf_tail];
        self->read_buf_tail = (self->read_buf_tail + 1) % self->read_buf_len;
		self->data_len--;
		buf++;
		data_num++;
		size--;

    }
	return data_num;
}

STATIC bool uart_tx_wait(machine_uart_obj_t *self, uint32_t timeout) 
{
	//TODO add time out function for tx
	//uint32_t start = mp_hal_ticks_ms();
	return true;
}

STATIC size_t uart_tx_data(machine_uart_obj_t *self, const void *src_data, size_t size, int *errcode) 
{
	*errcode = 0;
    if (size == 0)
        return 0;

    //uint32_t timeout;
	//K210 does not have cts function API at present
	//TODO:
	/*
    if (Determine whether to use CTS) {
        // CTS can hold off transmission for an arbitrarily long time. Apply
        // the overall timeout rather than the character timeout.
        timeout = self->timeout;
    } 
    */
    //timeout = 2 * self->timeout_char;
    uint8_t *src = (uint8_t*)src_data;
    size_t num_tx = 0;
	size_t cal = 0;
	if(self->attached_to_repl)
	{
		if( !self->ide_debug_mode)
		{
			while (num_tx < size) {
				/*
				if (Determine whether to send data(timeout)) {
					*errcode = MP_ETIMEDOUT;
					return num_tx;
				}
				*/
				if(MICROPY_UARTHS_DEVICE == self->uart_num)
					cal = uarths_send_data(src+num_tx, size - num_tx);
				else if(UART_DEVICE_MAX > self->uart_num)
					cal= uart_send_data(self->uart_num, (const char*)(src+num_tx), size - num_tx);	
				num_tx += cal;
			}
		}
		else
		{
			Buffer_Puts(&g_uart_send_buf_ide, src, size);
		}
	}
	else
	{
		while (num_tx < size) {
			if(MICROPY_UARTHS_DEVICE == self->uart_num)
				cal = uarths_send_data(src+num_tx, size - num_tx);
			else if(UART_DEVICE_MAX > self->uart_num)
				cal= uart_send_data(self->uart_num, (const char*)(src+num_tx), size - num_tx);
 	        num_tx = num_tx + cal;
	    }
	}
    // wait for the UART frame to complete
    /*
    if (Determine whether the transmission is completed(timeout)) {
        *errcode = MP_ETIMEDOUT;
        return num_tx;
    }
	*/
    return num_tx;
}

void uart_tx_strn(machine_uart_obj_t *uart_obj, const char *str, uint len) {
    int errcode;
    uart_tx_data(uart_obj, str, len, &errcode);
}


void uart_attach_to_repl(machine_uart_obj_t *self, bool attached) {
	g_repl_uart_obj = self;
    self->attached_to_repl = attached;
}

STATIC void machine_uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    //uart_get_baudrate(self->uart_num, &baudrate);
    mp_printf(print, "[MAIXPY]UART%d:( baudrate=%u, bits=%u, parity=%s, stop=%s)",
        self->uart_num,self->baudrate, self->bitwidth, _parity_name[self->parity],
        _stop_name[self->stop]);
}

STATIC void machine_uart_init_helper(machine_uart_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_baudrate,
	       ARG_bitwidth,
		   ARG_parity,
		   ARG_stop,
		   ARG_timeout,
		   ARG_timeout_char,
		   ARG_read_buf_len,
		   ARG_ide,
		   ARG_from_ide}; // uart communicate with IDE
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 115200} },
        { MP_QSTR_bits, MP_ARG_INT, {.u_int = UART_BITWIDTH_8BIT} },
        { MP_QSTR_parity, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_stop, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1000} },
        { MP_QSTR_timeout_char, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 10} },
        { MP_QSTR_read_buf_len, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MAIX_UART_BUF} },
		{ MP_QSTR_ide, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
		{ MP_QSTR_from_ide, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = true} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    // set baudrate
    if (args[ARG_baudrate].u_int < 0 || args[ARG_baudrate].u_int > 0x500000 ) {
        mp_raise_ValueError("[MAIXPY]UART:invalid baudrate");
    }else{	
    	self->baudrate =args[ARG_baudrate].u_int;
    }
	
    // set data bits
    if (args[ARG_bitwidth].u_int >=UART_BITWIDTH_5BIT && args[ARG_bitwidth].u_int <=UART_BITWIDTH_8BIT) {
            self->bitwidth=args[ARG_bitwidth].u_int;
    }else{
            mp_raise_ValueError("[MAIXPY]UART:invalid data bits");
    }

    // set parity
	if(args[ARG_parity].u_obj == mp_const_none)
	{
		self->parity = UART_PARITY_NONE;
	}
	else
	{
		self->parity = mp_obj_get_int(args[ARG_parity].u_obj);
		if (UART_PARITY_NONE > self->parity || self->parity > UART_PARITY_EVEN)
			mp_raise_ValueError("[MAIXPY]UART:invalid parity");
	}

    // set stop bits  
	mp_float_t stop_bits;
	if( args[ARG_stop].u_obj == mp_const_none)
		stop_bits = 1;
	else
	{
		stop_bits = mp_obj_get_float(args[ARG_stop].u_obj);
		if(stop_bits == 0)
			stop_bits = 1;
	}
	if(stop_bits!=1 && stop_bits!=1.5 && stop_bits!=2)
		mp_raise_ValueError("[MAIXPY]UART:invalid stop bits");
	if(stop_bits == 1)
		self->stop = UART_STOP_1;
	else if(stop_bits == 1.5)
		self->stop = UART_STOP_1_5;
	else if(stop_bits == 2)
		self->stop = UART_STOP_2;

	// set timeout 
	if(args[ARG_timeout].u_int >= 0)
		self->timeout = args[ARG_timeout].u_int;
	if(args[ARG_timeout_char].u_int >= 0)
		self->timeout_char = args[ARG_timeout_char].u_int;
	self->active = true;
	m_del(byte, self->read_buf, self->read_buf_len);
	if(args[ARG_read_buf_len].u_int <= 0)
	{
		self->read_buf = NULL;
		self->read_buf_len = 0;
	}
	else
	{
        self->read_buf_len = args[ARG_read_buf_len].u_int + 1;
        self->read_buf = m_new(byte, self->read_buf_len );
	}
	self->read_buf_head = 0;
    self->read_buf_tail = 0;
	if(MICROPY_UARTHS_DEVICE == self->uart_num){
		if(self->bitwidth != 8 || self->parity != 0)
			mp_raise_ValueError("[MAIXPY]UART:invalid param");
		uarths_init();
		uarths_config(self->baudrate,self->stop);
		uarths_set_interrupt_cnt(UARTHS_RECEIVE,0);
		ENABLE_HSRX_INT(self);

	}
	else if(UART_DEVICE_MAX > self->uart_num){
	    uart_init(self->uart_num);
		msleep(1);
	    uart_config(self->uart_num, (size_t)self->baudrate, (size_t)self->bitwidth, self->stop,  self->parity);
		uart_set_receive_trigger(self->uart_num, UART_RECEIVE_FIFO_1);
		ENABLE_RX_INT(self);
	}
	if(args[ARG_ide].u_bool)
	{
		self->ide_debug_mode = true;
		ide_dbg_init();
		if(args[ARG_from_ide].u_bool)
		{
			ide_dbg_init2();
		}
		else
		{
			ide_dbg_init3();
		}
		
		// init ringbuffer, use read buffer, for we do not use it to read data in IDE mode
		Buffer_Init(&g_uart_send_buf_ide, self->read_buf, self->read_buf_len);
	}
	else
	{
		self->ide_debug_mode = false;
	}
}

STATIC mp_obj_t machine_uart_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    // get uart id
    mp_int_t uart_num = mp_obj_get_int(args[0]);
    if (uart_num < 0 || UART_DEVICE_MAX == uart_num) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "[MAIXPY]UART%b:does not exist", uart_num));
    }else if (uart_num > MICROPY_UARTHS_DEVICE) {
    	nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "[MAIXPY]UART%b:does not exist", uart_num));
    }
    // create instance
    machine_uart_obj_t *self = m_new_obj(machine_uart_obj_t);
    self->base.type = &machine_uart_type;
    self->uart_num = uart_num;
    self->baudrate = 0;
    self->bitwidth = 8;
    self->parity = 0;
    self->stop = 1;
	self->read_buf_len = 0;
	self->data_len = 0;
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    machine_uart_init_helper(self, n_args - 1, args + 1, &kw_args);
    return MP_OBJ_FROM_PTR(self);
}

/*******************************************
name:uart.init(xxx,xxx,xxx)
arg: xxxxxxx
     xxxxx
return:xxx
*******************************************/
STATIC mp_obj_t machine_uart_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    machine_uart_init_helper(args[0], n_args -1 , args + 1, kw_args);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_uart_init_obj, 0, machine_uart_init);


STATIC mp_obj_t machine_uart_repl_uart() {
	if(g_repl_uart_obj)
    	return g_repl_uart_obj;
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_uart_repl_uart_obj, machine_uart_repl_uart);

STATIC mp_obj_t machine_uart_deinit(mp_obj_t self_in) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
	self->active = false;
	if(MICROPY_UARTHS_DEVICE == self->uart_num)
		DISABLE_HSRX_INT(self); 
	else if(UART_DEVICE_MAX > self->uart_num)
		DISABLE_RX_INT(self);
	m_del_obj(machine_uart_obj_t, self);
	m_del(byte, self->read_buf, self->read_buf_len);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_uart_deinit_obj, machine_uart_deinit);


STATIC mp_obj_t machine_set_uart_repl_uart(mp_obj_t arg) {
	if(g_repl_uart_obj)
    {
		machine_uart_deinit((mp_obj_t)g_repl_uart_obj);
	}
	g_repl_uart_obj = (machine_uart_obj_t*)arg;;
	g_repl_uart_obj->attached_to_repl = true;
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_set_uart_repl_uart_obj, machine_set_uart_repl_uart);


STATIC const mp_rom_map_elem_t machine_uart_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_uart_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_uart_deinit_obj) },

	{ MP_ROM_QSTR(MP_QSTR_readchar), MP_ROM_PTR(&machine_uart_rx_char_obj)},
	{ MP_ROM_QSTR(MP_QSTR_any), MP_ROM_PTR(&machine_uart_any_obj)},
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj)},
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
	
	{ MP_ROM_QSTR(MP_QSTR_UART1), MP_ROM_INT(UART_DEVICE_1) },
	{ MP_ROM_QSTR(MP_QSTR_UART2), MP_ROM_INT(UART_DEVICE_2) },
	{ MP_ROM_QSTR(MP_QSTR_UART3), MP_ROM_INT(UART_DEVICE_3) },
	{ MP_ROM_QSTR(MP_QSTR_UARTHS), MP_ROM_INT(MICROPY_UARTHS_DEVICE) },

	{ MP_ROM_QSTR(MP_QSTR_PARITY_ODD), MP_ROM_INT(UART_PARITY_ODD) },
	{ MP_ROM_QSTR(MP_QSTR_PARITY_EVEN), MP_ROM_INT(UART_PARITY_EVEN) },

	{ MP_ROM_QSTR(MP_QSTR_repl_uart), MP_ROM_PTR(&machine_uart_repl_uart_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_repl_uart), MP_ROM_PTR(&machine_set_uart_repl_uart_obj) },
};

STATIC MP_DEFINE_CONST_DICT(machine_uart_locals_dict, machine_uart_locals_dict_table);

STATIC mp_uint_t machine_uart_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t* buf = buf_in;
	*errcode = 0;
	if(self->active == 0)
		return 0;
    // make sure we want at least 1 char
    if (size == 0) {
        return 0;
    }
    // read the data
	int data_num = 0;
	if(uart_rx_wait(self, self->timeout_char))
	{
		if(self->attached_to_repl)
		{
		    while(size) 
			{
		        int data = uart_rx_char(self);
				if(-1 != data)
				{
		        	*buf++ = (uint8_t)data;
					data_num++;
					size--;
					debug_print("[machine_uart_read] data is valid,size = %d,data = %c\n",size,data);
				}
		        else if (-1 == data || !uart_rx_any(self)) 
				{
		            break;
		        }	
		    }
		}
		else
		{
			int ret_num = 0;
			while(data_num<size)
			{
				ret_num = uart_rx_data(self, buf+data_num, size-data_num);
				if(0 != ret_num)
				{
					data_num += ret_num;
				}
				else if(0 == ret_num || !uart_rx_any(self)) 
				{
					break;
				}
			}
		}
	}
	if(data_num != 0)
	{
		return data_num;
	}
	else
	{
		debug_print("[machine_uart_read] retrun error\n");
		*errcode = MP_EAGAIN;
		return MP_STREAM_ERROR;//don't return MP_STREAM_ERROR.It will lead error which can't get reading buf
		//return 0;
	}
}

STATIC mp_uint_t machine_uart_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    const byte *buf = buf_in;

	if(self->active == 0)
		return 0;
	
    // wait to be able to write the first character. EAGAIN causes write to return None
    if (!uart_tx_wait(self, self->timeout)) {
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }

    // write the data
    size_t num_tx = uart_tx_data(self, buf, size, errcode);

    if (*errcode == 0 || *errcode == MP_ETIMEDOUT) {
        // return number of bytes written, even if there was a timeout
        return num_tx;
    } else {
        return MP_STREAM_ERROR;
    }
}


STATIC mp_uint_t machine_uart_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_uint_t ret;
	if(self->active == 0)
		return 0;
    if (request == MP_STREAM_POLL) {
        uintptr_t flags = arg;
        ret = 0;
        if ((flags & MP_STREAM_POLL_RD) && uart_rx_any(self)) {
            ret |= MP_STREAM_POLL_RD;
        }
		//TODO:add Judging transmission enable
        if ((flags & MP_STREAM_POLL_WR) ) {
            ret |= MP_STREAM_POLL_WR;
        }
    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}


STATIC const mp_stream_p_t uart_stream_p = {
    .read = machine_uart_read,
    .write = machine_uart_write,
    .ioctl = machine_uart_ioctl,
    .is_text = false,
};

const mp_obj_type_t machine_uart_type = {
    { &mp_type_type },
    .name = MP_QSTR_UART,
    .print = machine_uart_print,
    .make_new = machine_uart_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &uart_stream_p,
    .locals_dict = (mp_obj_dict_t*)&machine_uart_locals_dict,
};
