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

#include <stdio.h>
#include <string.h>

#include "py/objtuple.h"
#include "py/objlist.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/mperrno.h"
#include "lib/netutils/netutils.h"
#include "modnetwork.h"

#if MICROPY_PY_USOCKET && !MICROPY_PY_LWIP

/******************************************************************************/
// socket class

STATIC const mp_obj_type_t socket_type;

/*


// method socket.bind(address)
STATIC mp_obj_t socket_bind(mp_obj_t self_in, mp_obj_t addr_in) {
    mod_network_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // get address
    uint8_t ip[MOD_NETWORK_IPADDR_BUF_SIZE];
    mp_uint_t port = netutils_parse_inet_addr(addr_in, ip, NETUTILS_BIG);

    // check if we need to select a NIC
    socket_select_nic(self, ip);

    // call the NIC to bind the socket
    int _errno;
    if (self->nic_type->bind(self, ip, port, &_errno) != 0) {
        mp_raise_OSError(_errno);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_bind_obj, socket_bind);

// method socket.listen(backlog)
STATIC mp_obj_t socket_listen(mp_obj_t self_in, mp_obj_t backlog) {
    mod_network_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (self->nic == MP_OBJ_NULL) {
        // not connected
        // TODO I think we can listen even if not bound...
        mp_raise_OSError(MP_ENOTCONN);
    }

    int _errno;
    if (self->nic_type->listen(self, mp_obj_get_int(backlog), &_errno) != 0) {
        mp_raise_OSError(_errno);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_listen_obj, socket_listen);

// method socket.accept()
STATIC mp_obj_t socket_accept(mp_obj_t self_in) {
    mod_network_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // create new socket object
    // starts with empty NIC so that finaliser doesn't run close() method if accept() fails
    mod_network_socket_obj_t *socket2 = m_new_obj_with_finaliser(mod_network_socket_obj_t);
    socket2->base.type = &socket_type;
    socket2->nic = MP_OBJ_NULL;
    socket2->nic_type = NULL;

    // accept incoming connection
    uint8_t ip[MOD_NETWORK_IPADDR_BUF_SIZE];
    mp_uint_t port;
    int _errno;
    if (self->nic_type->accept(self, socket2, ip, &port, &_errno) != 0) {
        mp_raise_OSError(_errno);
    }

    // new socket has valid state, so set the NIC to the same as parent
    socket2->nic = self->nic;
    socket2->nic_type = self->nic_type;

    // make the return value
    mp_obj_tuple_t *client = MP_OBJ_TO_PTR(mp_obj_new_tuple(2, NULL));
    client->items[0] = MP_OBJ_FROM_PTR(socket2);
    client->items[1] = netutils_format_inet_addr(ip, port, NETUTILS_BIG);

    return MP_OBJ_FROM_PTR(client);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(socket_accept_obj, socket_accept);



// method socket.setsockopt(level, optname, value)
STATIC mp_obj_t socket_setsockopt(size_t n_args, const mp_obj_t *args) {
    mod_network_socket_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    mp_int_t level = mp_obj_get_int(args[1]);
    mp_int_t opt = mp_obj_get_int(args[2]);

    const void *optval;
    mp_uint_t optlen;
    mp_int_t val;
    if (mp_obj_is_integer(args[3])) {
        val = mp_obj_get_int_truncated(args[3]);
        optval = &val;
        optlen = sizeof(val);
    } else {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args[3], &bufinfo, MP_BUFFER_READ);
        optval = bufinfo.buf;
        optlen = bufinfo.len;
    }

    int _errno;
    if (self->nic_type->setsockopt(self, level, opt, optval, optlen, &_errno) != 0) {
        mp_raise_OSError(_errno);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(socket_setsockopt_obj, 4, 4, socket_setsockopt);

mp_uint_t socket_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    mod_network_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (request == MP_STREAM_CLOSE) {
        if (self->nic != MP_OBJ_NULL) {
            self->nic_type->close(self);
            self->nic = MP_OBJ_NULL;
        }
        return 0;
    }
    return self->nic_type->ioctl(self, request, arg, errcode);
}

*/



int8_t g_fds[20] = {0,0,0,0,0,0,0,0}; // max fd: 8*8 = 64
int8_t require_new_fd(){
    int8_t i=0, j;
    for(; i<sizeof(g_fds); ++i){
        for(j=0; j<8; ++j){
            if( ((g_fds[i]>>j) & 0x01) == 0){
		g_fds[i] = (0x01<<j) | g_fds[i];
                return 8*i + j;
            }    
        }
    }
    return -1;
}

void del_fd(int8_t fd){
    if(fd < 0)
        return;
    g_fds[fd/8] = g_fds[fd/8] & (~( 0x01<<(fd%8) ));
}


STATIC void socket_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind){
    mod_network_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "<usocket.socket fd=%d, family=%d, type=%d>", self->fd, self->u_param.domain, self->u_param.type);
}

STATIC void socket_select_nic(mod_network_socket_obj_t *self, const byte *ip);

// method socket.sendto(bytes, address)
STATIC mp_obj_t socket_sendto(mp_obj_t self_in, mp_obj_t data_in, mp_obj_t addr_in) {
    mod_network_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // get the data
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(data_in, &bufinfo, MP_BUFFER_READ);

    // get address
    uint8_t ip[MOD_NETWORK_IPADDR_BUF_SIZE];
    mp_uint_t port = netutils_parse_inet_addr(addr_in, ip, NETUTILS_BIG);

    // check if we need to select a NIC
    socket_select_nic(self, ( const byte*)ip);

    // call the NIC to sendto
    int _errno;
    MP_THREAD_GIL_EXIT();
    mp_int_t ret = self->nic_type->sendto(self, (byte*)bufinfo.buf, bufinfo.len, ip, port, &_errno);
    MP_THREAD_GIL_ENTER();
    if (ret == -1) {
        mp_raise_OSError(_errno);
    }

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(socket_sendto_obj, socket_sendto);

// method socket.recvfrom(bufsize)
STATIC mp_obj_t socket_recvfrom(mp_obj_t self_in, mp_obj_t len_in) {
    mod_network_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->nic == MP_OBJ_NULL) {
        // not connected
        mp_raise_OSError(MP_ENOTCONN);
    }
    vstr_t vstr;
    vstr_init_len(&vstr, mp_obj_get_int(len_in));
    byte ip[4];
    mp_uint_t port;
    int _errno;
    MP_THREAD_GIL_EXIT();
    mp_int_t ret = self->nic_type->recvfrom(self, (byte*)vstr.buf, vstr.len, ip, &port, &_errno);
    MP_THREAD_GIL_ENTER();
    if (ret == -1) {
        mp_raise_OSError(_errno);
    }
    mp_obj_t tuple[2];
    if (ret == 0) {
        tuple[0] = mp_const_empty_bytes;
    } else {
        vstr.len = ret;
        tuple[0] = mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
    }
    tuple[1] = netutils_format_inet_addr(ip, port, NETUTILS_BIG);
    return mp_obj_new_tuple(2, tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_recvfrom_obj, socket_recvfrom);

// method socket.recv(bufsize)
STATIC mp_obj_t socket_recv(mp_obj_t self_in, mp_obj_t len_in) {
    mod_network_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->nic == MP_OBJ_NULL) {
        // not connected
        mp_raise_OSError(MP_ENOTCONN);
    }
    mp_int_t len = mp_obj_get_int(len_in);
    vstr_t vstr;
    vstr_init_len(&vstr, len);
    int _errno;
    MP_THREAD_GIL_EXIT();
    mp_uint_t ret = self->nic_type->recv(self, (byte*)vstr.buf, len, &_errno);
    MP_THREAD_GIL_ENTER();
    if (ret == MP_STREAM_ERROR) {
        if(!mp_is_nonblocking_error(_errno))
        {
            mp_raise_OSError(_errno);
        }
        ret = 0;
    }
    if (ret == 0) {
        return mp_const_empty_bytes;
    }
    vstr.len = ret;
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_recv_obj, socket_recv);

// method socket.send(bytes)
STATIC mp_obj_t socket_send(mp_obj_t self_in, mp_obj_t buf_in) {
    mod_network_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->nic == MP_OBJ_NULL) {
        // not connected
        mp_raise_OSError(MP_EPIPE);
    }
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);
    int _errno;
    MP_THREAD_GIL_EXIT();
    mp_uint_t ret = self->nic_type->send(self, bufinfo.buf, bufinfo.len, &_errno);
    MP_THREAD_GIL_ENTER();
    if (ret == MP_STREAM_ERROR) {
        mp_raise_OSError(_errno);
    }
    return mp_obj_new_int_from_uint(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_send_obj, socket_send);



// method socket.settimeout(value)
// timeout=0 means non-blocking
// timeout=None means blocking
// otherwise, timeout is in seconds
STATIC mp_obj_t socket_settimeout(mp_obj_t self_in, mp_obj_t timeout_in) {
	mod_network_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
	float timeout = mp_obj_get_float(timeout_in);
    if(timeout < 0)
        mp_raise_ValueError("[MaixPy] timeout parameter error");
    self->timeout = timeout;
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_settimeout_obj, socket_settimeout);

// method socket.setblocking(flag)
STATIC mp_obj_t socket_setblocking(mp_obj_t self_in, mp_obj_t blocking) {
	mod_network_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if(mp_obj_is_true(blocking))
    {
        self->timeout = UINT64_MAX;
    }
    else
    {
        self->timeout = 0;
    }
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_setblocking_obj, socket_setblocking);

// method socket.connect(address)

STATIC void socket_select_nic(mod_network_socket_obj_t *self, const byte *ip) {
    if (self->nic == MP_OBJ_NULL) {
        // select NIC based on IP
        self->nic = mod_network_find_nic(ip);
        self->nic_type = (mod_network_nic_type_t*)mp_obj_get_type(self->nic);		
        // call the NIC to open the socket
        int _errno;
        MP_THREAD_GIL_EXIT();
        mp_int_t ret = self->nic_type->socket(self, &_errno);
        MP_THREAD_GIL_ENTER();
        if (ret != 0) {
            mp_raise_OSError(_errno);
        }
    }
}
STATIC mp_obj_t socket_connect(mp_obj_t self_in, mp_obj_t addr_in) {
    mod_network_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // get address
    uint8_t ip[MOD_NETWORK_IPADDR_BUF_SIZE];
    mp_uint_t port = netutils_parse_inet_addr(addr_in, ip, NETUTILS_BIG);
    // check if we need to select a NIC
    socket_select_nic(self, ip);
    // call the NIC to connect the socket
    int _errno;
    MP_THREAD_GIL_EXIT();
    int ret = self->nic_type->connect(self, ip, port, &_errno);
    MP_THREAD_GIL_ENTER();
    if (ret != 0) {
        mp_raise_OSError(_errno);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_connect_obj, socket_connect);


STATIC const mp_rom_map_elem_t socket_locals_dict_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&mp_stream_close_obj) },
	{ MP_ROM_QSTR(MP_QSTR_settimeout), MP_ROM_PTR(&socket_settimeout_obj) },
    { MP_ROM_QSTR(MP_QSTR_setblocking), MP_ROM_PTR(&socket_setblocking_obj) },
    { MP_ROM_QSTR(MP_QSTR_connect), MP_ROM_PTR(&socket_connect_obj) },
	{ MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&socket_send_obj) },
	{ MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&socket_recv_obj) },
	{ MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_sendto), MP_ROM_PTR(&socket_sendto_obj) },
    { MP_ROM_QSTR(MP_QSTR_recvfrom), MP_ROM_PTR(&socket_recvfrom_obj) },
/*    
    { MP_ROM_QSTR(MP_QSTR_bind), MP_ROM_PTR(&socket_bind_obj) },
    { MP_ROM_QSTR(MP_QSTR_listen), MP_ROM_PTR(&socket_listen_obj) },
    { MP_ROM_QSTR(MP_QSTR_accept), MP_ROM_PTR(&socket_accept_obj) },  
    { MP_ROM_QSTR(MP_QSTR_setsockopt), MP_ROM_PTR(&socket_setsockopt_obj) },
*/
};
	
STATIC MP_DEFINE_CONST_DICT(socket_locals_dict, socket_locals_dict_table);

// constructor socket(family=AF_INET, type=SOCK_STREAM, proto=0, fileno=None)
STATIC mp_obj_t socket_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 4, false);

    // create socket object (not bound to any NIC yet)
    mod_network_socket_obj_t *s = m_new_obj_with_finaliser(mod_network_socket_obj_t);
    s->base.type = &socket_type;
    s->nic = MP_OBJ_NULL;
    s->nic_type = NULL;
    s->u_param.domain = MOD_NETWORK_AF_INET;
    s->u_param.type = MOD_NETWORK_SOCK_STREAM;
    s->u_param.fileno = 0;
    s->fd = require_new_fd();
    if(s->fd < 0)
    {
        mp_raise_OSError(MP_ENOMEM);
    }
    s->timeout = 10; // default timeout: 10s
    s->peer_closed = false;
	if (n_args >= 1) {
        s->u_param.domain = mp_obj_get_int(args[0]);
        if (n_args >= 2) {
            s->u_param.type = mp_obj_get_int(args[1]);
            if (n_args >= 4) {
                s->u_param.fileno = mp_obj_get_int(args[3]);
            }
        }
    }
    return MP_OBJ_FROM_PTR(s);
}

STATIC mp_uint_t socket_stream_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode) {
    mod_network_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->nic == MP_OBJ_NULL) {
        // not connected
        *errcode = MP_EPIPE;
        return MP_STREAM_ERROR;
    }
    MP_THREAD_GIL_EXIT();
    mp_uint_t ret = self->nic_type->recv(self, (byte*)buf, size, errcode);
    MP_THREAD_GIL_ENTER();
    return ret;
}

STATIC mp_uint_t socket_stream_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode) {
    mod_network_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->nic == MP_OBJ_NULL) {
        // not connected
        *errcode = MP_EPIPE;
        return MP_STREAM_ERROR;
    }
    MP_THREAD_GIL_EXIT();
    mp_uint_t ret = self->nic_type->send(self, buf, size, errcode);
    MP_THREAD_GIL_ENTER();
    return ret;
}
mp_uint_t socket_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    mod_network_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (request == MP_STREAM_CLOSE) {
        if (self->nic != MP_OBJ_NULL) {
            MP_THREAD_GIL_EXIT();
            self->nic_type->close(self);
            MP_THREAD_GIL_ENTER();
            self->nic = MP_OBJ_NULL;
            del_fd(self->fd);
            self->fd = -1;
        }
        return 0;
    }
	if(self->nic_type->ioctl) {
        MP_THREAD_GIL_EXIT();
    	int ret = self->nic_type->ioctl(self, request, arg, errcode);
        MP_THREAD_GIL_ENTER();
        return ret;
    }
    return EPERM;
}


STATIC const mp_stream_p_t socket_stream_p = {
    .ioctl = socket_ioctl,
    .read = socket_stream_read,
    .write = socket_stream_write,
    .is_text = false,
};


STATIC const mp_obj_type_t socket_type = {
	{ &mp_type_type },
	.name = MP_QSTR_socket,
	.make_new = socket_make_new,
    .print = socket_print,
	.protocol = &socket_stream_p,
	.locals_dict = (mp_obj_dict_t*)&socket_locals_dict,
};


// usocket module
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
			// mp_printf(&mp_plat_print, "[MaixPy] %s | It is not string IP format:%s\n",__func__, addr_str);
			return 0;
        }
    }
	return 1;
}

// function usocket.getaddrinfo(host, port)
STATIC mp_obj_t mod_usocket_getaddrinfo(size_t n_args, const mp_obj_t *pos_args) {
    // enum {
    //         ARG_timeout
    //     };
    // STATIC const mp_arg_t allowed_args[] = {
    //     { MP_QSTR_timeout, MP_ARG_INT,                   {.u_int = 3000} },
    // };
    // mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    // mp_arg_parse_all(n_args - 2, pos_args + 2, kw_args,
    //     MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    size_t hlen;
    const char *host = mp_obj_str_get_data(pos_args[0], &hlen);
    mp_int_t port = mp_obj_get_int(pos_args[1]);
    uint8_t out_ip[MOD_NETWORK_IPADDR_BUF_SIZE];
	bool parse_ret = false;
	parse_ret = parse_ipv4_addr(pos_args[0], out_ip, NETUTILS_BIG);
	mp_obj_t nic = MP_STATE_PORT(modnetwork_nic);
	mod_network_nic_type_t *nic_type = (mod_network_nic_type_t*)mp_obj_get_type(nic);
	if(parse_ret == 0)
	{
		if (nic_type->gethostbyname != NULL)
        {
            int ret = nic_type->gethostbyname(nic ,host,strlen(host),out_ip);
			if( ret != 0)
			{
				mp_raise_OSError(ret);
			}
        }
	}
	else
	{
		nlr_buf_t nlr;
		if (nlr_push(&nlr) == 0)
		{
			netutils_parse_ipv4_addr(pos_args[0], out_ip, NETUTILS_BIG);
			nlr_pop();
		}
	}
    mp_obj_tuple_t *tuple = MP_OBJ_TO_PTR(mp_obj_new_tuple(5, NULL));
    tuple->items[0] = MP_OBJ_NEW_SMALL_INT(MOD_NETWORK_AF_INET);
    tuple->items[1] = MP_OBJ_NEW_SMALL_INT(MOD_NETWORK_SOCK_STREAM);
    tuple->items[2] = MP_OBJ_NEW_SMALL_INT(0);
    tuple->items[3] = MP_OBJ_NEW_QSTR(MP_QSTR_);
    tuple->items[4] = netutils_format_inet_addr(out_ip, port, NETUTILS_BIG);
    return mp_obj_new_list(1, (mp_obj_t*)&tuple);  
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_usocket_getaddrinfo_obj, 2, 6, mod_usocket_getaddrinfo);

STATIC const mp_rom_map_elem_t mp_module_usocket_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_usocket) },

    { MP_ROM_QSTR(MP_QSTR_socket), MP_ROM_PTR(&socket_type) },
    { MP_ROM_QSTR(MP_QSTR_getaddrinfo), MP_ROM_PTR(&mod_usocket_getaddrinfo_obj) },    

    // class constants
    { MP_ROM_QSTR(MP_QSTR_AF_INET), MP_ROM_INT(MOD_NETWORK_AF_INET) },
    { MP_ROM_QSTR(MP_QSTR_AF_INET6), MP_ROM_INT(MOD_NETWORK_AF_INET6) },
    { MP_ROM_QSTR(MP_QSTR_SOCK_STREAM), MP_ROM_INT(MOD_NETWORK_SOCK_STREAM) },
    { MP_ROM_QSTR(MP_QSTR_SOCK_DGRAM), MP_ROM_INT(MOD_NETWORK_SOCK_DGRAM) },
    { MP_ROM_QSTR(MP_QSTR_SOCK_RAW), MP_ROM_INT(MOD_NETWORK_SOCK_RAW) },

};

STATIC MP_DEFINE_CONST_DICT(mp_module_usocket_globals, mp_module_usocket_globals_table);

const mp_obj_module_t socket_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_usocket_globals,
};
	
#endif // MICROPY_PY_USOCKET && !MICROPY_PY_LWIP
