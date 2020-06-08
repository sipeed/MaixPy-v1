/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
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

#include <string.h>
#include "py/objtuple.h"
#include "py/objlist.h"
#include "py/stream.h"
#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "lib/netutils/netutils.h"
#include "modnetwork.h"
#include "modmachine.h"
#include "esp8285.h"
#include "mpconfigboard.h"
#include "buffer.h"

STATIC bool nic_connected = false;
typedef struct _nic_obj_t {
		mp_obj_base_t base;
#if MICROPY_UART_NIC
		mp_obj_t uart_obj;
		esp8285_obj esp8285;
#endif 
} nic_obj_t;

typedef struct _stream_obj_t {
	unsigned char stream[2048];
	int stream_cur;
	int stream_len;
}stream_obj_t;

stream_obj_t stream;


STATIC void esp8285_socket_close(mod_network_socket_obj_t *socket) {
	if((mp_obj_type_t*)&mod_network_nic_type_esp8285 != mp_obj_get_type(MP_OBJ_TO_PTR(socket->nic)))
	{
		return ;
	}
	nic_obj_t* self = MP_OBJ_TO_PTR(socket->nic);
	releaseTCP(&self->esp8285);
}

STATIC mp_uint_t esp8285_socket_recv(mod_network_socket_obj_t *socket, byte *buf, mp_uint_t len, int *_errno) {
	if((mp_obj_type_t*)&mod_network_nic_type_esp8285 != mp_obj_get_type(MP_OBJ_TO_PTR(socket->nic)))
	{
		*_errno = MP_EPIPE;
		return MP_STREAM_ERROR;
	}
	nic_obj_t* self = MP_OBJ_TO_PTR(socket->nic);
	int ret = 0;
    uint32_t read_len = 0;
	ret = esp_recv(&self->esp8285, (char*)buf, len, &read_len, (uint32_t)(socket->timeout*1000), &socket->peer_closed, socket->first_read_after_write);
    socket->first_read_after_write = false;
    if(ret == -1)
    {
        *_errno = MP_EPIPE;
        return MP_STREAM_ERROR;
    }
    else if(ret == -2) // EOF
    {
        *_errno = MP_EAGAIN; // MP_EAGAIN or MP_EWOULDBLOCK according to `mp_is_nonblocking_error()`
        return MP_STREAM_ERROR;
    }
    else if(ret == -3) // timeout
    {
        *_errno = MP_ETIMEDOUT;
        return MP_STREAM_ERROR;
    }
    else if(ret == -4)//peer closed
    {
        *_errno = MP_ENOTCONN;
        return MP_STREAM_ERROR;
    }
	return (mp_uint_t)read_len;
}

STATIC mp_uint_t esp8285_socket_send(mod_network_socket_obj_t *socket, const byte *buf, mp_uint_t len, int *_errno) {

	if((mp_obj_type_t*)&mod_network_nic_type_esp8285 != mp_obj_get_type(MP_OBJ_TO_PTR(socket->nic)))
	{
		*_errno = MP_EPIPE;
		return MP_STREAM_ERROR;
	}
	nic_obj_t* self = MP_OBJ_TO_PTR(socket->nic);
    if(socket->peer_closed)
    {
        *_errno = MP_ENOTCONN;
        return MP_STREAM_ERROR;
    }
    Buffer_Clear(&self->esp8285.buffer);//clear receive buffer
    socket->first_read_after_write = true;
	if(0 == esp_send(&self->esp8285,(const char*)buf,len, (uint32_t)(socket->timeout*1000) ) )
	{
		*_errno = MP_EPIPE;
		return MP_STREAM_ERROR;
	}
    // printk("%s len %d\r\n", __func__, len);
    mp_hal_delay_us(len * 50); // maybe 50 us time required to send per byte
    // vTaskDelay(len / portTICK_PERIOD_MS);
    return len;
}


STATIC int esp8285_socket_socket(mod_network_socket_obj_t *socket, int *_errno) {

    return 0;
}


STATIC int esp8285_socket_connect(mod_network_socket_obj_t *socket, byte *ip, mp_uint_t port, int *_errno) {
	if((mp_obj_type_t*)&mod_network_nic_type_esp8285 != mp_obj_get_type(MP_OBJ_TO_PTR(socket->nic)))
	{
		*_errno = -1;
		return -1;
	}
    socket->peer_closed = false;
	nic_obj_t* self = MP_OBJ_TO_PTR(socket->nic);
	switch(socket->u_param.type)
	{
		case MOD_NETWORK_SOCK_STREAM:
		{
			if(false == createTCP(&self->esp8285, (char*)ip,port))
			{
				*_errno = -1;
				return -1;
			}
			break;
		}
		case MOD_NETWORK_SOCK_DGRAM:
		{
			if(false == registerUDP(&self->esp8285, (char*)ip,port))
			{
				*_errno = -1;
				return -1;
			}
			break;
		}
		default:
		{
			if(false == createTCP(&self->esp8285, (char*)ip,port))
			{
				*_errno = -1;
				return -1;
			}
			break;
		}
	}
    return 0;
}


STATIC int esp8285_socket_gethostbyname(mp_obj_t nic, const char *name, mp_uint_t len, uint8_t* out_ip) {
	if((mp_obj_type_t*)&mod_network_nic_type_esp8285 == mp_obj_get_type(nic))
	{
		if( get_host_byname(&((nic_obj_t*)nic)->esp8285,name,len, (char*)out_ip, 3000) )
            return 0;
        else
            return MP_EINVAL;
	}
    return MP_EPERM;
}

STATIC mp_obj_t esp8285_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
	
#if MICROPY_UART_NIC

    // set uart to communicate
	if(&machine_uart_type != mp_obj_get_type(args[0])) 
	{
		mp_raise_ValueError("invalid uart stream");
	}
    mp_get_stream_raise(args[0], MP_STREAM_OP_READ | MP_STREAM_OP_WRITE | MP_STREAM_OP_IOCTL);
    nic_obj_t* nic_obj = m_new_obj(nic_obj_t);
    uint8_t* buff = m_new(uint8_t, ESP8285_BUF_SIZE);
    Buffer_Init(&nic_obj->esp8285.buffer, buff, ESP8285_BUF_SIZE);
    nic_obj->base.type = (mp_obj_type_t*)&mod_network_nic_type_esp8285;		
    nic_obj->esp8285.uart_obj = args[0];
    nic_obj->uart_obj = args[0];
    if (0 == eINIT(&nic_obj->esp8285)) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "couldn't init nic esp8285 ,try again please\n"));
    }
	mod_network_register_nic((mp_obj_t)nic_obj);

    return (mp_obj_t)nic_obj;
#endif
}

STATIC mp_obj_t esp8285_nic_connect(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	enum { ARG_ssid, ARG_key};
	nic_obj_t* self = NULL;
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_ssid, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_key, MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	//get nic
	if((mp_obj_type_t*)&mod_network_nic_type_esp8285 == mp_obj_get_type(pos_args[0]))
	{
		self = pos_args[0];
	}
    // get ssid
    size_t ssid_len =0;
    const char *ssid = NULL;
	if (args[ARG_key].u_obj != mp_const_none) {
        ssid = mp_obj_str_get_data(args[ARG_ssid].u_obj, &ssid_len);
    }		
    // get key
    size_t key_len = 0;
    const char *key = NULL;
    if (args[ARG_key].u_obj != mp_const_none) {
        key = mp_obj_str_get_data(args[1].u_obj, &key_len);
    }
    // connect to AP
    
    if (0 == joinAP(&self->esp8285, ssid, key)) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "could not connect to ssid=%s\n", ssid));
    }
	nic_connected = 1;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(esp8285_nic_connect_obj, 1, esp8285_nic_connect);

STATIC mp_obj_t esp8285_nic_disconnect(mp_obj_t self_in) {
    // should we check return value?
    nic_obj_t* self = self_in;
	if (false == leaveAP(&self->esp8285)) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "conldn't disconnect wifi,plase try again or reboot nic\n"));
    }
	nic_connected = 0;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp8285_nic_disconnect_obj, esp8285_nic_disconnect);


STATIC mp_obj_t esp8285_nic_isconnected(mp_obj_t self_in) {
    return mp_obj_new_bool(nic_connected);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp8285_nic_isconnected_obj, esp8285_nic_isconnected);

STATIC mp_obj_t esp8285_nic_ifconfig(mp_obj_t self_in) {
	nic_obj_t* self = self_in;
	ipconfig_obj esp_ipconfig;
	esp_ipconfig.gateway = mp_const_none;
	esp_ipconfig.ip = mp_const_none;
	esp_ipconfig.MAC = mp_const_none;
	esp_ipconfig.netmask = mp_const_none;
	esp_ipconfig.ssid = mp_const_none;
	if(false == get_ipconfig(&self->esp8285, &esp_ipconfig))
	{
		return mp_const_none;
	}
	mp_obj_t tuple[7] = { esp_ipconfig.ip,
						  esp_ipconfig.netmask,
						  esp_ipconfig.gateway,
						  mp_obj_new_str("0",strlen("0")),
						  mp_obj_new_str("0",strlen("0")),
						  esp_ipconfig.MAC,
						  esp_ipconfig.ssid
						};
	return mp_obj_new_tuple(MP_ARRAY_SIZE(tuple), tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp8285_nic_ifconfig_obj, esp8285_nic_ifconfig);

STATIC mp_obj_t esp8285_scan_wifi(mp_obj_t self_in)
{
    nic_obj_t* self = self_in;
    mp_obj_t list = mp_obj_new_list(0, NULL);
    char fail_str[30] ;
    char* buf = (char*)self->esp8285.buffer.buffer;
    bool end;
    int err_code = 0;

    if (!eATCWLAP_Start(&self->esp8285))
    {
        err_code = -1;
        goto err;
    }
    while(1)
    {
        if(!eATCWLAP_Get(&self->esp8285, &end) )
        {
            err_code = -2;
            goto err;
        }
        char* index1 = strstr(buf, "(");
        if(!index1)
        {
            err_code = -3;
            goto err;
        }
        char* index2 = strstr(index1, ")");
        if(!index2)
        {
            err_code = -4;
            goto err;
        }
        *index2 = '\0';
        index1 += 1;
        mp_obj_list_append(list, mp_obj_new_str(index1, strlen(index1)));
        if(end)
            break;
    }
    return list;
err:
    snprintf(fail_str, sizeof(fail_str), "wifi scan fail:%d", err_code);
    mp_raise_msg(&mp_type_OSError, fail_str);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp8285_scan_wifi_obj, esp8285_scan_wifi);

/* nic.enable_ap(ssid=None, key=None, chl=5, ecn=nic.WPA2_PSK) */
STATIC mp_obj_t esp8285_enable_ap(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum
    {
        ARG_WIFI_SSID,
        ARG_WIFI_KEY,
        ARG_WIFI_CH,
        ARG_WIFI_ECN,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_ssid, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_key, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_chl, MP_ARG_OBJ, {.u_obj = MP_ROM_INT(5)} },
        { MP_QSTR_ecn, MP_ARG_OBJ, {.u_obj = MP_ROM_INT(3)} },
    };
    nic_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_obj_t ssid = args[ARG_WIFI_SSID].u_obj;
    mp_obj_t key = args[ARG_WIFI_KEY].u_obj;
    mp_obj_t chl = args[ARG_WIFI_CH].u_obj;
    mp_obj_t ecn = args[ARG_WIFI_ECN].u_obj;

    if (eATCWSAP(&self->esp8285, mp_obj_str_get_str(ssid), mp_obj_str_get_str(key), 
                                 mp_obj_get_int(chl), mp_obj_get_int(ecn)) == false)
    {
        mp_raise_msg(&mp_type_OSError, "wifi enable fail");
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(esp8285_enable_ap_obj, 3, esp8285_enable_ap);

STATIC mp_obj_t esp8285_disable_ap(mp_obj_t self_in)
{
    nic_obj_t* self = self_in;

    if (sATCWMODE(&self->esp8285, 1) == false)
    {
        mp_raise_msg(&mp_type_OSError, "wifi disable fail");
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp8285_disable_ap_obj, esp8285_disable_ap);

STATIC const mp_rom_map_elem_t esp8285_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_connect), MP_ROM_PTR(&esp8285_nic_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_disconnect), MP_ROM_PTR(&esp8285_nic_disconnect_obj) },  
    { MP_ROM_QSTR(MP_QSTR_isconnected), MP_ROM_PTR(&esp8285_nic_isconnected_obj) },
    { MP_ROM_QSTR(MP_QSTR_ifconfig), MP_ROM_PTR(&esp8285_nic_ifconfig_obj) },
    { MP_ROM_QSTR(MP_QSTR_scan), MP_ROM_PTR(&esp8285_scan_wifi_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable_ap), MP_ROM_PTR(&esp8285_enable_ap_obj) },
    { MP_ROM_QSTR(MP_QSTR_disable_ap), MP_ROM_PTR(&esp8285_disable_ap_obj) },
    { MP_ROM_QSTR(MP_QSTR_OPEN), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_WPA_PSK), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_WPA2_PSK), MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_WPA_WPA2_PSK), MP_ROM_INT(4) },
};

STATIC MP_DEFINE_CONST_DICT(esp8285_locals_dict, esp8285_locals_dict_table);

const mod_network_nic_type_t mod_network_nic_type_esp8285 = {
    .base = {
        { &mp_type_type },
        .name = MP_QSTR_ESP8285,
        .make_new = esp8285_make_new,
        .locals_dict = (mp_obj_dict_t*)&esp8285_locals_dict,
    },
    .gethostbyname = esp8285_socket_gethostbyname,
    .connect = esp8285_socket_connect,
    .socket = esp8285_socket_socket,
    .send = esp8285_socket_send,
    .recv = esp8285_socket_recv,
    .close = esp8285_socket_close,
/*  
    .bind = cc3k_socket_bind,
    .listen = cc3k_socket_listen,
    .accept = cc3k_socket_accept,
    .sendto = cc3k_socket_sendto,
    .recvfrom = cc3k_socket_recvfrom,
    .setsockopt = cc3k_socket_setsockopt,
    .settimeout = cc3k_socket_settimeout,
    .ioctl = cc3k_socket_ioctl,
*/
};
