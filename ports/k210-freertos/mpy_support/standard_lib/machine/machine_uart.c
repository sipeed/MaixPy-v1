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

#include "mpconfigboard.h"
#include "modmachine.h"
#include "uart.h"
#include "uarths.h"
#include "syslog.h"
#include "plic.h"

typedef struct _machine_uart_obj_t {
    mp_obj_base_t base;
	uint8_t uart_num;
    uint32_t baudrate;
    uint8_t bitwidth;
    uart_parity_t parity;
    uart_stopbit_t stop;
	bool active : 1;
	bool attached_to_repl: 1;
	bool rx_int_flag: 1;
	uint16_t read_buf_len;             
    uint16_t read_buf_head;    
    uint16_t read_buf_tail;
	uint16_t timeout;
	uint16_t timeout_char;
    uint8_t *read_buf;
} machine_uart_obj_t;

#define Maix_DEBUG 0
#if Maix_DEBUG==1
#define debug_print(x,arg...) printf("[MAIXPY]"x,##arg)
#else 
#define debug_print(x,arg...) 
#endif

#define Maix_KDEBUG 0
#if Maix_KDEBUG==1
#define debug_prink(x,arg...) printk("[MAIXPY]"x,##arg)
#else 
#define debug_prink(x,arg...) 
#endif



STATIC const char *_parity_name[] = {"None", "1", "0"};


//QueueHandle_t UART_QUEUE[UART_DEVICE_MAX] = {};

/******************************************************************************/
// MicroPython bindings for UART

void DISABLE_RX_INT(machine_uart_obj_t *self)
{
	self->rx_int_flag = 0;
	uart_irq_unregister(self->uart_num, UART_RECEIVE);
}
//extern uarths_context_t g_uarths_context;
void DISABLE_HSRX_INT(machine_uart_obj_t *self)
{
	self->rx_int_flag = 0;
    //g_uarths_context.callback = NULL;
    //g_uarths_context.ctx = NULL;
	plic_irq_disable(IRQN_UARTHS_INTERRUPT);
    plic_irq_unregister(IRQN_UARTHS_INTERRUPT);

}
int uart_rx_irq(void *ctx) 
{
    machine_uart_obj_t *self = ctx;
	uint8_t data = 0;
	debug_prink("[uart_rx_irq]read_buf_len = %d\r\n",self->read_buf_len);
    if (self == NULL) {
        return 0;
    }
	if (self->read_buf_len != 0) {
		uint16_t next_head = (self->read_buf_head + 1) % self->read_buf_len;
		// only read data if room in buf
		if (next_head != self->read_buf_tail) {
			int ret =  0;
			if(MICROPY_UARTHS_DEVICE == self->uart_num)
				ret = uarths_receive_data(&data,1);
			else if(UART_DEVICE_MAX > self->uart_num)
				ret = uart_receive_data(self->uart_num,&data , 1);
			// can not receive any data ,return 
			if(0 == ret)
				return ;
			debug_prink("[uart_rx_irq]data = %c\r\n",data);
			self->read_buf[self->read_buf_head] = data;
			self->read_buf_head = next_head;
			debug_prink("[uart_rx_irq]read_buf_head = %d\r\n",self->read_buf_head);
			// Handle interrupt coming in on a UART REPL
			if (self->attached_to_repl && data == mp_interrupt_char) {
				if (MP_STATE_VM(mp_pending_exception) == MP_OBJ_NULL) {
					mp_keyboard_interrupt();
				} else {
					MP_STATE_VM(mp_pending_exception) = MP_OBJ_NULL;
					//pendsv_object = &MP_STATE_VM(mp_kbd_exception);
				}
				return;
			}

		}
		else {
			// No room: leave char in buf, disable interrupt,open it util rx char
			if(MICROPY_UARTHS_DEVICE == self->uart_num)
				DISABLE_HSRX_INT(self);	
			else if(UART_DEVICE_MAX > self->uart_num)
				DISABLE_RX_INT(self);
		}
	}

}

void ENABLE_RX_INT(machine_uart_obj_t *self)
{
	self->rx_int_flag = 1;
	uart_irq_register(self->uart_num, UART_RECEIVE, uart_rx_irq, self, 2);
}

void ENABLE_HSRX_INT(machine_uart_obj_t *self)
{
	self->rx_int_flag = 1;
	uarths_set_irq(UART_RECEIVE,uart_rx_irq,self,2);
}


STATIC bool uart_rx_wait(machine_uart_obj_t *self, uint32_t timeout) 
{
    uint32_t start = mp_hal_ticks_ms();
	debug_print("[uart_rx_wait]read_buf_head = %d\n",self->read_buf_head);
	debug_print("[uart_rx_wait]read_buf_tail = %d\n",self->read_buf_tail);
    for (;;) {
        if (self->read_buf_tail != self->read_buf_head ) {
            return true; // have at least 1 char ready for reading
        }
        if (mp_hal_ticks_ms() - start >= timeout) {
            return false; // timeout
        }
    }
}


// assumes there is a character available
int uart_rx_char(machine_uart_obj_t *self) 
{
    if (self->read_buf_tail != self->read_buf_head) {
        int data;
        data = self->read_buf[self->read_buf_tail];
        self->read_buf_tail = (self->read_buf_tail + 1) % self->read_buf_len;
        if (self->rx_int_flag == 0) {
            //re-enable IRQ now we have room in buffer
      		if(MICROPY_UARTHS_DEVICE == self->uart_num)
				ENABLE_HSRX_INT(self);	
			else if(UART_DEVICE_MAX > self->uart_num)
				ENABLE_RX_INT(self);
        }
        return data;
    }
	return 0;
}


mp_uint_t uart_rx_any(machine_uart_obj_t *self) 
{
	int buffer_bytes = self->read_buf_head - self->read_buf_tail;
	if (buffer_bytes < 0) 
	{
		return buffer_bytes + self->read_buf_len;
	} 
	else if (buffer_bytes > 0)
	{
		return buffer_bytes;
	} 
	else 
	{
		//__HAL_UART_GET_FLAG(&self->uart, UART_FLAG_RXNE) != RESET
		return 0;
	}
}

STATIC bool uart_tx_wait(machine_uart_obj_t *self, uint32_t timeout) 
{
	//TODO add time function out for tx
	//uint32_t start = mp_hal_ticks_ms();
	return true;
}

STATIC size_t uart_tx_data(machine_uart_obj_t *self, const void *src_data, size_t size, int *errcode) 
{
    if (size == 0) {
        *errcode = 0;
        return 0;
    }

    uint32_t timeout;
	//K210 does not have cts function API at present
	//TODO:
	/*
    if (Determine whether to use CTS) {
        // CTS can hold off transmission for an arbitrarily long time. Apply
        // the overall timeout rather than the character timeout.
        timeout = self->timeout;
    } 
    */
    timeout = 2 * self->timeout_char;

    const uint8_t *src = (uint8_t*)src_data;
    size_t num_tx = 0;
	size_t cal = 0;	
	
    while (num_tx < size) {
		/*
        if (Determine whether to send data(timeout)) {
            *errcode = MP_ETIMEDOUT;
            return num_tx;
        }
        */
        uint8_t data;
        data = *src++;
	
		if(MICROPY_UARTHS_DEVICE == self->uart_num)
			cal = uarths_send_data(&data,1);
		else if(UART_DEVICE_MAX > self->uart_num)
			cal= uart_send_data(self->uart_num, &data,1);
		
        num_tx = num_tx + cal;
    }
    // wait for the UART frame to complete
    /*
    if (Determine whether the transmission is completed(timeout)) {
        *errcode = MP_ETIMEDOUT;
        return num_tx;
    }
	*/
    *errcode = 0;
    return num_tx;
}

void uart_tx_strn(machine_uart_obj_t *uart_obj, const char *str, uint len) {
    int errcode;
    uart_tx_data(uart_obj, str, len, &errcode);
}


void uart_attach_to_repl(machine_uart_obj_t *self, bool attached) {
    self->attached_to_repl = attached;
}

STATIC void machine_uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    //uart_get_baudrate(self->uart_num, &baudrate);
    mp_printf(print, "[MAIXPY]UART%d:( baudrate=%u, bits=%u, parity=%s, stop=%u)",
        self->uart_num,self->baudrate, self->bitwidth, _parity_name[self->parity],
        self->stop);
}

STATIC void machine_uart_init_helper(machine_uart_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_baudrate, ARG_bitwidth, ARG_parity, ARG_stop ,ARG_timeout,ARG_timeout_char,ARG_read_buf_len};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 115200} },
        { MP_QSTR_bits, MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_parity, MP_ARG_INT, {.u_int = UART_PARITY_NONE} },
        { MP_QSTR_stop, MP_ARG_INT, {.u_int = UART_STOP_1} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1000} },
        { MP_QSTR_timeout_char, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 10} },
        { MP_QSTR_read_buf_len, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 256} },
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
    if (args[ARG_bitwidth].u_int >=5 && args[ARG_bitwidth].u_int <=8) {
            self->bitwidth=args[ARG_bitwidth].u_int;
    }else{
            mp_raise_ValueError("[MAIXPY]UART:invalid data bits");
    }

    // set parity
    if (UART_PARITY_NONE <= args[ARG_parity].u_int && args[ARG_parity].u_int <= UART_PARITY_EVEN) {
		self->parity = args[ARG_parity].u_int;
    }
	else{
		mp_raise_ValueError("[MAIXPY]UART:invalid parity");
	}

    // set stop bits  
    if( UART_STOP_1 <= args[ARG_stop].u_int && args[ARG_stop].u_int <= UART_STOP_2)
    {
	    switch (args[ARG_stop].u_int) {
	        case UART_STOP_1:
	            self->stop = UART_STOP_1;
	            break;
	        case UART_STOP_1_5:
	            self->stop = UART_STOP_1_5;
	            break;
	        case UART_STOP_2:
	            self->stop = UART_STOP_2;
	            break;
	        default:
	            mp_raise_ValueError("[MAIXPY]UART:invalid stop bits");
	            break;
	    }
    }
	// set timeout 
	if(args[ARG_timeout].u_int >= 0)
		self->timeout = args[ARG_timeout].u_int;
	if(args[ARG_timeout].u_int >= 0)
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
		self->bitwidth = 8;
		self->parity = 0;
		uarths_init();
		uarths_config(self->baudrate,self->stop);
		uarths_set_interrupt_cnt(UARTHS_RECEIVE,0);
		ENABLE_HSRX_INT(self);
	}
	else if(UART_DEVICE_MAX > self->uart_num){
	    uart_init(self->uart_num);
	    uart_config(self->uart_num, (size_t)self->baudrate, (size_t)self->bitwidth, self->stop,  self->parity);
		uart_set_receive_trigger(self->uart_num, UART_RECEIVE_FIFO_1);
		ENABLE_RX_INT(self);
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
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    machine_uart_init_helper(self, n_args - 1, args + 1, &kw_args);
	
    return MP_OBJ_FROM_PTR(self);
}


STATIC mp_obj_t machine_uart_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    machine_uart_init_helper(args[0], n_args -1 , args + 1, kw_args);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_uart_init_obj, 0, machine_uart_init);


STATIC mp_obj_t machine_uart_deinit(mp_obj_t self_in) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
	self->active = false;
	//TODO:add deinit function
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_uart_deinit_obj, machine_uart_deinit);



STATIC const mp_rom_map_elem_t machine_uart_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_uart_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_uart_deinit_obj) },
    
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj)},
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
	
	{ MP_ROM_QSTR(MP_QSTR_UART1), MP_ROM_INT(UART_DEVICE_1) },
	{ MP_ROM_QSTR(MP_QSTR_UART2), MP_ROM_INT(UART_DEVICE_2) },
	{ MP_ROM_QSTR(MP_QSTR_UART3), MP_ROM_INT(UART_DEVICE_3) },
	{ MP_ROM_QSTR(MP_QSTR_UARTHS), MP_ROM_INT(MICROPY_UARTHS_DEVICE) },
};

STATIC MP_DEFINE_CONST_DICT(machine_uart_locals_dict, machine_uart_locals_dict_table);

STATIC mp_uint_t machine_uart_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    byte *buf = buf_in;

	if(self->active == 0)
		return 0;
    // make sure we want at least 1 char
    if (size == 0) {
        return 0;
    }
    // wait for first char to become available
    debug_print("[machine_uart_read]buf = %s\n",self->read_buf);
	debug_print("[machine_uart_read]size = %d\n",size);
    if (!uart_rx_wait(self, self->timeout)) {
        // return EAGAIN error to indicate non-blocking (then read() method returns None)
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }

    // read the data
    byte *orig_buf = buf;
    for (;;) {
        int data = uart_rx_char(self);
        *buf++ = data;
		debug_print("[machine_uart_read]data = %c\n",data);
        if (--size == 0 || !uart_rx_wait(self, self->timeout_char)) {
            // return number of bytes read
            debug_print("[machine_uart_read]begin return \n");
            return buf - orig_buf;
        }
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
