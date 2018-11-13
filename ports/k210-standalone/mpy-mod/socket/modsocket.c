/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 * and Mnemote Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016, 2017 Nick Moore @mnemote
 *
 * Based on extmod/modlwip.c
 * Copyright (c) 2013, 2014 Damien P. George
 * Copyright (c) 2015 Galen Hazelwood
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
//#include <stdlib.h>
#include <string.h>

#include "py/runtime0.h"
#include "py/nlr.h"
#include "py/objlist.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/stream.h"
#include "py/mperrno.h"
#include "py/gc.h"
#include "lib/netutils/netutils.h"

#include "esp8285.h"
#include "sleep.h"

//#define SOCKET_POLL_US (100000)
#define MOD_NETWORK_IPADDR_BUF_SIZE (4)

#define MOD_NETWORK_AF_INET (2)
#define MOD_NETWORK_AF_INET6 (10)

#define MOD_NETWORK_SOCK_STREAM (1)
#define MOD_NETWORK_SOCK_DGRAM (2)
#define MOD_NETWORK_SOCK_RAW (3)

#define K210_DEBUG 1
#if K210_DEBUG==1
#define debug_print(x,arg...) printf("[Sipeed]"x,##arg)
#else 
#define debug_print(x,arg...) 
#endif

STATIC const mp_obj_type_t socket_type;

typedef struct _socket_obj_t {
    mp_obj_base_t base;
    uint8_t domain;
    uint8_t type;
    uint8_t proto;
    bool peer_closed;
	unsigned char stream[1024];
	int stream_cur;
	int stream_len;
    unsigned int retries;
} socket_obj_t;

int get_line(unsigned char* src,int num,unsigned char* buf)
{
	char pattern[30] = {0};
	char* tmp = pattern;
	while(--num)
	{
		sprintf(tmp,"%s","%*s");
		tmp = tmp + strlen("%*s");
	}
	sprintf(tmp,"%s","%s");
	if(0 == sscanf(src,pattern,buf))
	{
		printf("sscanf is failed\n");
		return -1;
	}
	return 1;
}

int parse_ipv4_addr(mp_obj_t addr_in, uint8_t *out_ip, netutils_endian_t endian) {
    size_t addr_len;
    const char *addr_str = mp_obj_str_get_data(addr_in, &addr_len);
    if (addr_len == 0) {
        // special case of no address given
        memset(out_ip, 0, NETUTILS_IPV4ADDR_BUFSIZE);
        return 0;
    }
    const char *s = addr_str;
    const char *s_top = addr_str + addr_len;
    for (mp_uint_t i = 3 ; ; i--) {
        mp_uint_t val = 0;
        for (; s < s_top && *s != '.'; s++) {
            val = val * 10 + *s - '0';
        }
        if (endian == NETUTILS_LITTLE) {
            out_ip[i] = val;
        } else {
            out_ip[NETUTILS_IPV4ADDR_BUFSIZE - 1 - i] = val;
        }
        if (i == 0 && s == s_top) {
            return 0;
        } else if (i > 0 && s < s_top && *s == '.') {
            s++;
        } else {
            printf("[MAIXPY]SOCKET:It is not string IP form\n");
			return 0;
        }
    }
	return 1;
}
int add_escapte(char* str,char* dest)
{
	unsigned char* tmp = dest;
	int len = strlen(str);
	int i = 0;
	if(str == NULL)
	{
		printf("[MAIXPY]SOCKET:str == NULL\n");
		return 0;
	}
	for(;i < len+1;i++)
	{
		if(i == 0)
		{
			tmp[i] = '\"';
			continue;
		}
		tmp[i]=str[i-1];
	}
	tmp[i] = '\"';
	return 1;
}

STATIC mp_uint_t socket_stream_read(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode) {

	socket_obj_t *sock = self_in;
	int read_len = 0;
	printf("[MAIXPY]SOCKET:begin stream_len = %d\n",sock->stream_len);
	printf("[MAIXPY]SOCKET:begin stream_cur = %d\n",sock->stream_cur);
	/*judge socket buf wether have data*/
	if(0 != sock->stream_len)//socket buf have data
	{
		printf("[MAIXPY]SOCKET:enter if branch\n");
		if(size > sock->stream_len)
			read_len = sock->stream_len;
		else
			read_len = size;
		
		memcpy(buf,&sock->stream[sock->stream_cur],read_len);
		sock->stream_len = sock->stream_len - read_len;
		sock->stream_cur = sock->stream_cur + read_len;
		if(sock->stream_len <= 0)//all data have been read
		{	
			sock->stream_len = 0;
			sock->stream_cur = 0;
		}
		return read_len;
	}
	read_len = size;
	memset(sock->stream,0, 1024);
	sock->stream_len = read_data(NULL,sock->stream);
	if(0 == sock->stream_len)//read data failed
	{
		printf("[MAIXPY]SOCKET:read data failed!\n");
		return 0;
	}
	if(read_len > sock->stream_len)//read size too big
	{
		printf("[MAIXPY]SOCKET:size too big\n");
		read_len = sock->stream_len;
	}
	memcpy(buf,&sock->stream[sock->stream_cur],read_len);
	sock->stream_cur += read_len;
	sock->stream_len -= read_len;
	debug_print("[MAIXPY]SOCKET:end stream_len = %d\n",sock->stream_len);
	debug_print("[MAIXPY]SOCKET:end stream_cur = %d\n",sock->stream_cur);
	if(sock->stream_len <= 0)
	{
		sock->stream_len = 0;
		sock->stream_cur = 0;
	}
	return read_len;
}

STATIC mp_obj_t socket_readall(mp_obj_t self_in) {

	socket_obj_t *sock = self_in;
	int size = 1023 - sock->stream_cur;
	
	mp_obj_t buf = mp_obj_new_bytearray(1,NULL);
	sock->stream_cur = 0;
	return 	MP_OBJ_FROM_PTR(buf);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(socket_readall_obj, socket_readall);


STATIC mp_uint_t socket_stream_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode) {
    socket_obj_t *sock = self_in;
	unsigned char tmp_data[100] = {0};
	int flag = 0;
	/*send data*/
	sprintf(tmp_data,"AT+CIPSEND=%d",size);
	flag = send_cmd(tmp_data,"OK",2);
	if(flag == 0)
	{
		printf("[MAIXPY]SOCKET:write data length failed/n");
		return 0;
	}
	flag =send_data(buf,size,2);
	if(flag == 0)
	{
		printf("[MAIXPY]SOCKET:write data failed/n");
		return 0;
	}
	//sock->stream_cur = 0;
    return size;
}
//STATIC MP_DEFINE_CONST_FUN_OBJ_3(socket_write_obj, socket_stream_write);

STATIC const mp_stream_p_t socket_stream_p = {
	.write = socket_stream_write,
	.read = socket_stream_read,
};
STATIC mp_obj_t socket_connect(const mp_obj_t arg0, const mp_obj_t arg1) {
	socket_obj_t *self = MP_OBJ_TO_PTR(arg0);	
	mp_uint_t len = 0;
    mp_obj_t *elem;
	mp_obj_tuple_t* tuple_in;
	assert(MP_OBJ_IS_TYPE(arg1, &mp_type_tuple));
    tuple_in = MP_OBJ_TO_PTR(arg1);
    len = tuple_in->len;
    elem = tuple_in->items;
    if (len != 2) 
    {
    	mp_raise_OSError("[MAIXPY]SOCKET:connect addr info in not right\n");
    }
    char *host = mp_obj_str_get_str(elem[0]);
	int port = mp_obj_get_int(elem[1]);
	debug_print("host = %s\n",host);
	debug_print("port = %d\n",port);
	unsigned char cmd[200];
	memset(cmd, 0, 200);
	sprintf(cmd,"AT+CIPSTART=\"TCP\",\"%s\",%d",host,port);
	debug_print("cmd = %s\n",cmd);
	if(0 == send_cmd(cmd,"OK",2))
	{
		printf("[MAIXPY]SOCKET:connect failed!\n");
		return mp_const_false;
	}
	printf("[MAIXPY]SOCKET:connect!\n");
    return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_connect_obj, socket_connect);

STATIC mp_obj_t socket_disconnect(const mp_obj_t arg0) {
	socket_obj_t *self = MP_OBJ_TO_PTR(arg0);	
	esp8285_quit_trans();
	send_cmd("AT+CIPCLOSE","OK",0);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(socket_disconnect_obj, socket_disconnect);




STATIC const mp_rom_map_elem_t socket_locals_dict_table[] = {
	/*
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_bind), MP_ROM_PTR(&socket_bind_obj) },
    { MP_ROM_QSTR(MP_QSTR_listen), MP_ROM_PTR(&socket_listen_obj) },
    { MP_ROM_QSTR(MP_QSTR_accept), MP_ROM_PTR(&socket_accept_obj) },
    */
    

    { MP_ROM_QSTR(MP_QSTR_disconnect), MP_ROM_PTR(&socket_disconnect_obj) },
	{ MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_connect), MP_ROM_PTR(&socket_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    { MP_ROM_QSTR(MP_QSTR_readall), MP_ROM_PTR(&socket_readall_obj) },
    /*
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&socket_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_sendall), MP_ROM_PTR(&socket_sendall_obj) },
    { MP_ROM_QSTR(MP_QSTR_sendto), MP_ROM_PTR(&socket_sendto_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&socket_recv_obj) },
    { MP_ROM_QSTR(MP_QSTR_recvfrom), MP_ROM_PTR(&socket_recvfrom_obj) },
	*/

};
STATIC MP_DEFINE_CONST_DICT(socket_locals_dict, socket_locals_dict_table);

STATIC mp_obj_t socket_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
	
	//mp_arg_check_num(pos_args, n_args, 0, 4, false);
    // create socket object (not bound to any NIC yet)
    socket_obj_t *sock = m_new_obj_with_finaliser(socket_obj_t);
    sock->base.type = &socket_type;
    sock->domain = MOD_NETWORK_AF_INET6;
    sock->type = MOD_NETWORK_SOCK_STREAM;
    sock->proto = 0;
    sock->peer_closed = false;
	sock->stream_cur = 0;
	/*
	for(int i = 0;i < 1024;i++)
	{
		sock->stream[i] = i;
	}
	*/
	memset(sock->stream,0, 1024);
    if (n_args > 0) {
        sock->domain = mp_obj_get_int(args[0]);
        if (n_args > 1) {
            sock->type = mp_obj_get_int(args[1]);
            if (n_args > 2) {
                sock->proto = mp_obj_get_int(args[2]);
            }
        }
    }
	
    return MP_OBJ_FROM_PTR(sock);
    
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(socket_make_new_obj, 0,4, socket_make_new);


STATIC const mp_obj_type_t socket_type = {
    { &mp_type_type },
    .name = MP_QSTR_socket,
    .make_new = socket_make_new,
    .protocol = &socket_stream_p,
    .locals_dict = (mp_obj_t)&socket_locals_dict,
};


/******************************************************************************/
// usocket module

STATIC mp_obj_t socket_setblocking(const mp_obj_t arg0,const mp_obj_t arg1) {
	socket_obj_t *self = MP_OBJ_TO_PTR(arg0);	
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_setblocking_obj, socket_setblocking);

STATIC mp_obj_t socket_settimeout(const mp_obj_t arg0,const mp_obj_t arg1) {
	socket_obj_t *self = MP_OBJ_TO_PTR(arg0);	
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_settimeout_obj, socket_settimeout);


// function usocket.getaddrinfo(host, port)
STATIC mp_obj_t mod_usocket_getaddrinfo(mp_obj_t host_in, mp_obj_t port_in) {

    size_t hlen;
    const char *host = mp_obj_str_get_data(host_in, &hlen);
    mp_int_t port = mp_obj_get_int(port_in);
    uint8_t out_ip[MOD_NETWORK_IPADDR_BUF_SIZE];
    bool have_ip = false;
	int res = 1;
	int parse_ret = 0;
	unsigned char* ret_ptr ;
	parse_ret = parse_ipv4_addr(host_in,out_ip,NETUTILS_BIG);
	unsigned char host_str[hlen+2];
	add_escapte(host, host_str);
	/*need dns*/
	if(parse_ret == 0){
		/*Escape host TODO*/
		unsigned char cmd_buf[strlen("AT+CIPDOMAIN=%s")+strlen(host_str)+1];
		sprintf(cmd_buf,"AT+CIPDOMAIN=\"%s\"",host);
		res = send_cmd(cmd_buf,"OK",3);//after sending command,all ack will be recieved to esp_buf
		unsigned char IP_buf[16]={0};
		ret_ptr = buf_addr();	
		get_line(ret_ptr,1,IP_buf);
		debug_print("IP_buf %s\n",IP_buf);
		sscanf(IP_buf,"+CIPDOMAIN:%s",IP_buf);
		debug_print("IP_buf %s\n",IP_buf);
		mp_obj_t IP = mp_obj_new_str(IP_buf, strlen(IP_buf));
		netutils_parse_ipv4_addr(IP,out_ip,NETUTILS_BIG);
	} 
	else{
		netutils_parse_ipv4_addr(host_str,out_ip,NETUTILS_BIG);
	}	
	mp_obj_tuple_t *tuple = MP_OBJ_TO_PTR(mp_obj_new_tuple(5, NULL));
	tuple->items[0] = MP_OBJ_NEW_SMALL_INT(MOD_NETWORK_AF_INET);
	tuple->items[1] = MP_OBJ_NEW_SMALL_INT(MOD_NETWORK_SOCK_STREAM);
	tuple->items[2] = MP_OBJ_NEW_SMALL_INT(0);
	tuple->items[3] = MP_OBJ_NEW_QSTR(MP_QSTR_);
	tuple->items[4] = netutils_format_inet_addr(out_ip, port, NETUTILS_BIG);

	return mp_obj_new_list(1, (mp_obj_t*)&tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_usocket_getaddrinfo_obj, mod_usocket_getaddrinfo);

STATIC const mp_rom_map_elem_t mp_module_socket_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_usocket) },
    { MP_ROM_QSTR(MP_QSTR_socket), MP_ROM_PTR(&socket_type) },
    { MP_ROM_QSTR(MP_QSTR_getaddrinfo), MP_ROM_PTR(&mod_usocket_getaddrinfo_obj) },
	{ MP_ROM_QSTR(MP_QSTR_settimeout), MP_ROM_PTR(&socket_settimeout_obj) },
    { MP_ROM_QSTR(MP_QSTR_setblocking), MP_ROM_PTR(&socket_setblocking_obj) },
    
    { MP_ROM_QSTR(MP_QSTR_AF_INET), MP_ROM_INT(MOD_NETWORK_AF_INET) },
    { MP_ROM_QSTR(MP_QSTR_AF_INET6), MP_ROM_INT(MOD_NETWORK_AF_INET6) },
    { MP_ROM_QSTR(MP_QSTR_SOCK_STREAM), MP_ROM_INT(MOD_NETWORK_SOCK_STREAM) },
    { MP_ROM_QSTR(MP_QSTR_SOCK_DGRAM),  MP_ROM_INT(MOD_NETWORK_SOCK_DGRAM) },
    { MP_ROM_QSTR(MP_QSTR_SOCK_RAW),    MP_ROM_INT(MOD_NETWORK_SOCK_RAW) },   
};

STATIC MP_DEFINE_CONST_DICT(mp_module_socket_globals, mp_module_socket_globals_table);

const mp_obj_module_t socket_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_socket_globals,
};
