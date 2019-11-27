#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "py/objtuple.h"
#include "py/objlist.h"
#include "py/stream.h"
#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "lib/netutils/netutils.h"
#include "modnetwork.h"
#include "modmachine.h"
#include "mpconfigboard.h"
#include "sleep.h"

#include "esp32_spi.h"
#include "esp32_spi_io.h"
#include "fpioa.h"

typedef struct _esp32_nic_obj_t
{
    mp_obj_base_t base;

    int8_t sock_id;    // sock id generate by esp32, user can't see this id
    int8_t socket_fd;  // sock id generate by micropython, set these two var for gc collect to avoid close using connection
    bool to_be_closed;
    char* firmware_ver;
} esp32_nic_obj_t;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STATIC mp_obj_t esp32_nic_ifconfig(mp_obj_t self_in) {
    esp32_spi_net_t* inet = esp32_spi_get_network_data();
	if(inet == NULL)
	{
		return mp_const_none;
	}

	mp_obj_t tuple[3] = {   
        tuple[0] = netutils_format_ipv4_addr( inet->localIp, NETUTILS_BIG ),
        tuple[1] = netutils_format_ipv4_addr( inet->subnetMask, NETUTILS_BIG ),
        tuple[2] = netutils_format_ipv4_addr( inet->gatewayIp, NETUTILS_BIG )
						};

	return mp_obj_new_tuple(3, tuple);
}

STATIC mp_obj_t esp32_nic_isconnected(mp_obj_t self_in) {
    uint8_t is_connected = esp32_spi_is_connected();
    return mp_obj_new_bool(is_connected == 0);
}

STATIC mp_obj_t esp32_nic_disconnect(mp_obj_t self_in) {
    esp32_spi_disconnect_from_AP();
    return mp_const_none;
}

STATIC mp_obj_t esp32_nic_ping(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	enum { ARG_host, ARG_host_type};
	esp32_nic_obj_t* self = NULL;
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_host, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_host_type, MP_ARG_INT, {.u_int = -1} },
        // { MP_QSTR_host_type, MP_ARG_INT, {.u_obj = mp_const_none} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	//get nic
	if((mp_obj_type_t*)&mod_network_nic_type_esp32 == mp_obj_get_type(pos_args[0]))
	{
		self = pos_args[0];
	}
    // get host
    size_t host_len =0;
    const char* host = NULL;
	if (args[ARG_host].u_obj != MP_OBJ_NULL) {
        if(mp_obj_get_type(args[ARG_host].u_obj) == &mp_type_str)
            host = mp_obj_str_get_data(args[ARG_host].u_obj, &host_len);
        else if(mp_obj_get_type(args[ARG_host].u_obj) == &mp_type_set){
            host = MP_OBJ_TO_PTR(args[ARG_host].u_obj);
        }
    }
        
    
    // get host type (host name or address)
    int host_type = 1;
	if (args[ARG_host_type].u_int != -1) {
        host_type = args[ARG_host_type].u_int;
    }
    int32_t time = esp32_spi_ping((uint8_t*)host, host_type, 100);
    if(time == -2)
    {
        mp_raise_msg(&mp_type_OSError, "get host name fail");
    }
    else if(time < 0)
    {
        mp_raise_msg(&mp_type_OSError, "get response fail");
    }
    return mp_obj_new_int(time);
}

STATIC mp_obj_t esp32_nic_connect( size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args )
{
	enum { ARG_ssid, ARG_key};
	esp32_nic_obj_t* self = NULL;
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_ssid, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_key, MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	//get nic
	if((mp_obj_type_t*)&mod_network_nic_type_esp32 == mp_obj_get_type(pos_args[0]))
	{
		self = pos_args[0];
	}
    // get ssid
    size_t ssid_len =0;
    const char *ssid = NULL;
	if (args[ARG_ssid].u_obj != mp_const_none) {
        ssid = mp_obj_str_get_data(args[ARG_ssid].u_obj, &ssid_len);
    }		
    // get key
    size_t key_len = 0;
    const char *key = NULL;
    if (args[ARG_key].u_obj != mp_const_none) {
        key = mp_obj_str_get_data(args[1].u_obj, &key_len);
    }
    // connect to AP
    
    int8_t err = esp32_spi_connect_AP((uint8_t*)ssid, (uint8_t*)key, 20);
    if(err != 0)
    {
        char* msg = m_new(char, 30);
        snprintf(msg, 20, "Connect fail:");
        if(err == -2)
            snprintf(msg+strlen(msg), 30-strlen(msg), "FAILED");
        else
            snprintf(msg+strlen(msg), 30-strlen(msg), "TIMEOUT %d", err);
    	mp_raise_msg(&mp_type_OSError, msg);
    }

    return mp_const_none;
}

STATIC mp_obj_t esp32_scan_wifi( mp_obj_t self_in )
{
    mp_obj_t list = mp_obj_new_list(0, NULL);
    char* fail_str = m_new(char, 30);
    
    esp32_spi_aps_list_t* aps_list = esp32_spi_scan_networks();
    if(aps_list == NULL)
        goto err;
    uint32_t count = aps_list->aps_num;
    for(int i=0; i<count; i++)
    {
        esp32_spi_ap_t* ap = aps_list->aps[i];
        mp_obj_t info[3];
        info[0] = mp_obj_new_str((char*)ap->ssid, strlen((const char*)ap->ssid));
        info[1] = mp_obj_new_int(ap->encr);
        info[2] = mp_obj_new_int(ap->rssi);
        mp_obj_t t = mp_obj_new_tuple(3, info);
        mp_obj_list_append(list, t);
    }
    aps_list->del(aps_list);
    return list;
err:
    snprintf(fail_str, 30, "wifi scan fail");
    mp_raise_msg(&mp_type_OSError, fail_str);
}

STATIC void esp32_make_new_helper(esp32_nic_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    int cs, rst, rdy, mosi, miso, sclk;

    enum
    {
        ARG_cs,
        ARG_rst,
        ARG_rdy,
        ARG_mosi,
        ARG_miso,
        ARG_sclk,
    };

    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_cs, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1}},
        {MP_QSTR_rst, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1}},
        {MP_QSTR_rdy, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1}},
        {MP_QSTR_mosi, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1}},
        {MP_QSTR_miso, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1}},
        {MP_QSTR_sclk, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1}},
    };

    mp_arg_val_t args_parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args_parsed);

    //cs
    cs = args_parsed[ARG_cs].u_int;
    if (cs == -1 || cs > FUNC_GPIOHS31 || cs < FUNC_GPIOHS0)
    {
        mp_raise_ValueError("gpiohs cs value error!");
    }

    //rst
    rst = args_parsed[ARG_rst].u_int;
    if (rst != -1)//no rst we will use soft reset
    {
        if (rst > FUNC_GPIOHS31 || rst < FUNC_GPIOHS0)
        {
            mp_raise_ValueError("gpiohs rst value error!");
        }
    }

    //rdy
    rdy = args_parsed[ARG_rdy].u_int;
    if (rdy == -1 || rdy > FUNC_GPIOHS31 || rdy < FUNC_GPIOHS0)
    {
        mp_raise_ValueError("gpiohs rdy value error!");
    }

    //mosi
    mosi = args_parsed[ARG_mosi].u_int;
    if (mosi == -1 || mosi > FUNC_GPIOHS31 || mosi < FUNC_GPIOHS0)
    {
        mp_raise_ValueError("gpiohs mosi value error!");
    }

    //miso
    miso = args_parsed[ARG_miso].u_int;
    if (miso == -1 || miso > FUNC_GPIOHS31 || miso < FUNC_GPIOHS0)
    {
        mp_raise_ValueError("gpiohs miso value error!");
    }

    //sclk
    sclk = args_parsed[ARG_sclk].u_int;
    if (sclk == -1 || sclk > FUNC_GPIOHS31 || sclk < FUNC_GPIOHS0)
    {
        mp_raise_ValueError("gpiohs sclk value error!");
    }

    esp32_spi_config_io(cs - FUNC_GPIOHS0, rst, rdy - FUNC_GPIOHS0,
                        mosi - FUNC_GPIOHS0, miso - FUNC_GPIOHS0, sclk - FUNC_GPIOHS0);
    esp32_spi_init();
    char* version = m_new(char, 32);
    char* ret = esp32_spi_firmware_version(version);
    if(ret == NULL)
    {
        m_del(char, version, 32);
        mp_raise_msg(&mp_type_OSError, "Get version fail");
    }
    char* version2 = m_new(char, strlen(version)); //set format to major.minor.dev e.g. 1.4.0
    strcpy(version2, version);
    m_del(char, version, 32);
    self->firmware_ver = version2;
}

//network.ESP32_SPI(cs=1,rst=2,rdy=3,mosi=4,miso=5,sclk=6)
STATIC mp_obj_t esp32_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    if (n_args != 0 || n_kw != 6)
    {
        mp_raise_ValueError("error argument");
        return mp_const_none;
    }
    esp32_nic_obj_t *self = m_new_obj(esp32_nic_obj_t);
    self->base.type = (mp_obj_type_t *)&mod_network_nic_type_esp32;
    self->sock_id = -1;
    self->firmware_ver = NULL;
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    esp32_make_new_helper(self, n_args, args, &kw_args);
    mod_network_register_nic((mp_obj_t)self);
    return MP_OBJ_FROM_PTR(self);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

STATIC mp_obj_t esp32_firmware_version(mp_obj_t self_in)
{
    esp32_nic_obj_t* self = (esp32_nic_obj_t*)self_in;
    if(self->firmware_ver)
        return mp_obj_new_str(self->firmware_ver, strlen(self->firmware_ver));
    else
        return mp_const_empty_bytes;
}


STATIC mp_obj_t esp32_adc(size_t n_args, const mp_obj_t *pos_args) {
    //A0 -> ch5, A1->ch4, A2->ch7, A3->ch6, A4->ch3, A5->ch0
    uint16_t adc[ESP32_ADC_CH_NUM] = {0};
    static const uint8_t channels_remap[ESP32_ADC_CH_NUM] = {5, 4, 7, 6, 3, 0};
    uint8_t channels[ESP32_ADC_CH_NUM] = {5, 4, 7, 6, 3, 0};
    uint8_t len = sizeof(channels);
    mp_int_t temp;
    if(n_args > 1){
        size_t t_len = 0;
        mp_obj_t* t = NULL;
        if(mp_obj_is_type(pos_args[1], &mp_type_tuple)){
            mp_obj_tuple_get(pos_args[1], &t_len, &t);   
        }else if(mp_obj_is_type(pos_args[1], &mp_type_list)){
            mp_obj_list_get(pos_args[1], &t_len, &t);
        }
        len = (uint8_t)t_len;
        for(uint8_t i=0; i< len; ++i){
            temp = mp_obj_get_int(t[i]);
            if(temp >= ESP32_ADC_CH_NUM || temp < 0){
                mp_raise_OSError(MP_EINVAL);
            }
            channels[i] =channels_remap[temp];
        }
    }

    if (esp32_spi_get_adc_val(channels, len, adc) == 0)
    {
        mp_obj_t *tuple, *tmp;

        tmp = m_new(mp_obj_t, len);

        for (uint8_t index = 0; index < len; index++)
            tmp[index] = mp_obj_new_int(adc[index]);

        tuple = mp_obj_new_tuple(len, tmp);

        m_del(mp_obj_t, tmp, len);
        return tuple;
    }
    else
    {
        mp_raise_ValueError("[MaixPy]: esp32 read adc failed!\r\n");
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(esp32_adc_obj, 1, 2, esp32_adc);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp32_version_obj, esp32_firmware_version);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp32_scan_wifi_obj, esp32_scan_wifi);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(esp32_nic_connect_obj, 1, esp32_nic_connect);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp32_nic_disconnect_obj, esp32_nic_disconnect);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp32_nic_isconnected_obj, esp32_nic_isconnected);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(esp32_nic_ping_obj, 1, esp32_nic_ping);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp32_nic_ifconfig_obj, esp32_nic_ifconfig);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

STATIC const mp_rom_map_elem_t esp32_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR_version), MP_ROM_PTR(&esp32_version_obj)},
    {MP_ROM_QSTR(MP_QSTR_adc), MP_ROM_PTR(&esp32_adc_obj)},
    {MP_ROM_QSTR(MP_QSTR_scan), MP_ROM_PTR(&esp32_scan_wifi_obj)},
    { MP_ROM_QSTR(MP_QSTR_connect), MP_ROM_PTR(&esp32_nic_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_disconnect), MP_ROM_PTR(&esp32_nic_disconnect_obj) },  
    { MP_ROM_QSTR(MP_QSTR_isconnected), MP_ROM_PTR(&esp32_nic_isconnected_obj) },
    { MP_ROM_QSTR(MP_QSTR_ifconfig), MP_ROM_PTR(&esp32_nic_ifconfig_obj) },
    { MP_ROM_QSTR(MP_QSTR_ping), MP_ROM_PTR(&esp32_nic_ping_obj) },
    { MP_ROM_QSTR(MP_QSTR_OPEN), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_WPA_PSK), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_WPA2_PSK), MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_WPA_WPA2_PSK), MP_ROM_INT(4) },
};

STATIC MP_DEFINE_CONST_DICT(esp32_locals_dict, esp32_locals_dict_table);


STATIC int esp32_socket_socket(mod_network_socket_obj_t *socket, int *_errno) {
    esp32_nic_obj_t* nic = (esp32_nic_obj_t*)socket->nic;
    nic->socket_fd = socket->fd;
    return 0;
}

STATIC int esp32_socket_connect(mod_network_socket_obj_t *socket, byte *ip, mp_uint_t port, int *_errno) {
	if((mp_obj_type_t*)&mod_network_nic_type_esp32 != mp_obj_get_type(MP_OBJ_TO_PTR(socket->nic)))
	{
		*_errno = -1;
		return -1;
	}
    esp32_nic_obj_t* nic = (esp32_nic_obj_t*)socket->nic;

	switch(socket->u_param.type)
	{
		case MOD_NETWORK_SOCK_STREAM:
		{
            uint8_t ret = esp32_spi_get_socket();
            if(ret == 0xff)
            {
                *_errno = MP_EIO;
                return -1;
            }
            nic->sock_id = (int8_t)ret;
            nic->to_be_closed = false;
            int8_t conn = esp32_spi_socket_connect((uint8_t)nic->sock_id, ip, 0, port, TCP_MODE);
			if(-2 == conn)
			{
				*_errno = MP_EIO;
				return -1;
			}
            else if(conn == -1)
            {
                *_errno = MP_ECONNREFUSED;
				return -1;
            }
            else if(conn == -3)
            {
                *_errno = MP_ETIMEDOUT;
				return -1;
            }
			break;
		}
		case MOD_NETWORK_SOCK_DGRAM:
		{
            *_errno = MP_EPERM;
            return -1;
			// break;
		}
		default:
		{
            *_errno = MP_EPERM;
            return -1;
		}
	}
    return 0;
}

STATIC mp_uint_t esp32_socket_recv(mod_network_socket_obj_t *socket, byte *buf, mp_uint_t len, int *_errno) {
	if((mp_obj_type_t*)&mod_network_nic_type_esp32 != mp_obj_get_type(MP_OBJ_TO_PTR(socket->nic)))
	{
		*_errno = MP_EPIPE;
		return MP_STREAM_ERROR;
	}
    esp32_nic_obj_t* self = (esp32_nic_obj_t*)socket->nic;
    int read_len = 0;
    uint16_t once_read_len = 0;
    int ret = -1;
    int len_avail = 0;
    mp_uint_t start_time = mp_hal_ticks_ms();
    do{
        if(self->sock_id >= 0)
        {
            len_avail = esp32_spi_socket_available(self->sock_id);
            if(len_avail == -1)
            {
                *_errno = MP_EIO;
                return MP_STREAM_ERROR;
            }
            if(len_avail > 0)
            {
                once_read_len = len_avail>(len-read_len) ? (len-read_len) : len_avail;
                once_read_len = once_read_len>SPI_MAX_DMA_LEN ? SPI_MAX_DMA_LEN : once_read_len;
                ret = esp32_spi_socket_read(self->sock_id, (uint8_t*)buf+read_len, once_read_len);
                if(ret == -1)
                {
                    *_errno = MP_EIO;
                    return MP_STREAM_ERROR;
                }
                read_len += ret;
            }
            if(read_len >= len)
                break;
            if(socket->timeout == 0)
                break;
            if( mp_hal_ticks_ms() - start_time > ((uint32_t)socket->timeout*1000) )
            {
                *_errno = MP_ETIMEDOUT;
                return MP_STREAM_ERROR;
            }
        }
        if(socket->u_param.type == MOD_NETWORK_SOCK_STREAM)
        {
            if((self->sock_id < 0) || (esp32_spi_socket_status(self->sock_id) == SOCKET_CLOSED))
            {
                self->sock_id = -1;
                if(!self->to_be_closed)
                {
                    self->to_be_closed = true;
                    break;
                }
                *_errno = MP_EAGAIN; // MP_EAGAIN or MP_EWOULDBLOCK according to `mp_is_nonblocking_error()`
                return MP_STREAM_ERROR;
            }
        }else{
            if(read_len > 0)
            {
                if(!self->to_be_closed)
                {
                    self->to_be_closed = true;
                    break;
                }
                self->to_be_closed = false;
                *_errno = MP_EAGAIN; // MP_EAGAIN or MP_EWOULDBLOCK according to `mp_is_nonblocking_error()`
                return MP_STREAM_ERROR;
            }
        }
    }while(1);
	return read_len;
}

STATIC mp_uint_t esp32_socket_send(mod_network_socket_obj_t *socket, const byte *buf, mp_uint_t len, int *_errno) {

	if((mp_obj_type_t*)&mod_network_nic_type_esp32 != mp_obj_get_type(MP_OBJ_TO_PTR(socket->nic)))
	{
		*_errno = MP_EPIPE;
		return -1;
	}
    int status = SOCKET_CLOSED;
    esp32_nic_obj_t* self = (esp32_nic_obj_t*)socket->nic;
    if(self->sock_id >= 0)
        status = esp32_spi_socket_status(self->sock_id);
    if(status == SOCKET_CLOSED)
    {
        self->sock_id = -1;   
        // return 0;//TODO: should return 0 here? In CPython return len
        *_errno = MP_ENOTCONN;
        return -1;
    }
    mp_uint_t sent_len = 0;
    uint16_t len_send;
    while(1)
    {
        len_send = len > SPI_MAX_DMA_LEN ? SPI_MAX_DMA_LEN : len;
        //TODO: esp32_spi_socket_write is nonblock in esp32 firmware
        if(esp32_spi_socket_write(self->sock_id, (uint8_t*)buf, len_send ) == 0)
        {
            *_errno = MP_EIO;
            return -1;
        }
        sent_len += len_send;
        if(sent_len >= len)
            break;
    }
	
    return len;
}

STATIC mp_uint_t esp32_socket_sendto(mod_network_socket_obj_t *socket, const byte *buf, mp_uint_t len,  uint8_t* ip, mp_uint_t port, int *_errno) 
{
    int8_t ret;
	if((mp_obj_type_t*)&mod_network_nic_type_esp32 != mp_obj_get_type(MP_OBJ_TO_PTR(socket->nic)))
	{
		*_errno = MP_EPIPE;
		return -1;
	}
    if(socket->u_param.type != MOD_NETWORK_SOCK_DGRAM){
        *_errno = MP_EPERM;
        return -1;
    }
    esp32_nic_obj_t* self = (esp32_nic_obj_t*)socket->nic;
    if(self->sock_id < 0)
    {
        uint8_t ret = esp32_spi_get_socket();
        if(ret == 0xff)
        {
            *_errno = MP_EIO;
            return -1;
        }
        self->sock_id = (int8_t)ret;
    }
    self->to_be_closed = false;
    ret = esp32_spi_socket_connect((uint8_t)self->sock_id, ip, 0, port, UDP_MODE);
    if(-2 == ret)
    {
        *_errno = MP_EIO;
        return -1;
    }
    else if(ret == -1)
    {
        *_errno = MP_ECONNREFUSED;
        return -1;
    }
    else if(ret == -3)
    {
        *_errno = MP_ETIMEDOUT;
        return -1;
    }
    ret = esp32_spi_add_udp_data((uint8_t)self->sock_id, (uint8_t*)buf, (uint16_t)len);
    if(ret != 0)
    {
        *_errno = MP_EIO;
        return -1;
    }
    ret = esp32_spi_send_udp_data((uint8_t)self->sock_id);
    if(ret !=0 )
    {
        *_errno = MP_EIO;
        return -1;
    }
    return len;
}

STATIC mp_uint_t esp32_socket_recvfrom(mod_network_socket_obj_t *socket, byte *buf, mp_uint_t len,  byte* ip, mp_uint_t* port, int *_errno)
{
   if((mp_obj_type_t*)&mod_network_nic_type_esp32 != mp_obj_get_type(MP_OBJ_TO_PTR(socket->nic)))
	{
		*_errno = MP_EPIPE;
		return MP_STREAM_ERROR;
	}
    esp32_nic_obj_t* self = (esp32_nic_obj_t*)socket->nic;
    int read_len = 0;
    uint16_t once_read_len = 0;
    int ret = -1;
    int len_avail = 0;
    mp_uint_t start_time = mp_hal_ticks_ms();
    do{
        if(self->sock_id >= 0)
        {
            len_avail = esp32_spi_socket_available(self->sock_id);
            if(len_avail == -1)
            {
                *_errno = MP_EIO;
                return MP_STREAM_ERROR;
            }
            if(len_avail > 0)
            {
                once_read_len = len_avail>(len-read_len) ? (len-read_len) : len_avail;
                once_read_len = once_read_len>SPI_MAX_DMA_LEN ? SPI_MAX_DMA_LEN : once_read_len;
                ret = esp32_spi_socket_read(self->sock_id, (uint8_t*)buf+read_len, once_read_len);
                if(ret == -1)
                {
                    *_errno = MP_EIO;
                    return MP_STREAM_ERROR;
                }
                read_len += ret;
            }
            if(read_len >= len)
                break;
            if(socket->timeout == 0)
                break;
            if( mp_hal_ticks_ms() - start_time > ((uint32_t)socket->timeout*1000) )
            {
                *_errno = MP_ETIMEDOUT;
                return MP_STREAM_ERROR;
            }
        }
        if(socket->u_param.type == MOD_NETWORK_SOCK_STREAM)
        {
            if((self->sock_id < 0 ) || (esp32_spi_socket_status(self->sock_id) == SOCKET_CLOSED))
            {
                self->sock_id = -1;
                if(!self->to_be_closed)
                {
                    self->to_be_closed = true;
                    break;
                }
                *_errno = MP_EAGAIN; // MP_EAGAIN or MP_EWOULDBLOCK according to `mp_is_nonblocking_error()`
                return MP_STREAM_ERROR;
            }
        }else{
            if(read_len > 0)
            {
                if(!self->to_be_closed)
                {
                    self->to_be_closed = true;
                    break;
                }
                self->to_be_closed = false;
                *_errno = MP_EAGAIN; // MP_EAGAIN or MP_EWOULDBLOCK according to `mp_is_nonblocking_error()`
                return MP_STREAM_ERROR;
            }
        }
    }while(1);
    uint16_t port_16;
    int8_t res = -1;
    if( self->sock_id >= 0)
        res = esp32_spi_get_remote_info(self->sock_id, (uint8_t*)ip, &port_16);
    if(res !=0)
    {
        *_errno = MP_EIO;
        return MP_STREAM_ERROR;
    }
    *port = port_16;
	return read_len;
}

STATIC void esp32_socket_close(mod_network_socket_obj_t *socket) {
	if((mp_obj_type_t*)&mod_network_nic_type_esp32 != mp_obj_get_type(MP_OBJ_TO_PTR(socket->nic)))
	{
		return ;
	}
    esp32_nic_obj_t* self = (esp32_nic_obj_t*)socket->nic;
    if(self->socket_fd == socket->fd)
    {
        if(self->sock_id >= 0)
        {
            /*int8_t status = */esp32_spi_socket_close((uint8_t)self->sock_id);
            self->sock_id = -1;
        }
    }
}

STATIC int esp32_socket_gethostbyname(mp_obj_t nic, const char *name, mp_uint_t len, uint8_t* out_ip) {
	if((mp_obj_type_t*)&mod_network_nic_type_esp32 != mp_obj_get_type(nic))
	{
        return MP_EPERM;
    }

    uint8_t tmp_ip[6];
    int ret = esp32_spi_get_host_by_name((uint8_t*)name, tmp_ip);
    if( ret != 0)
        return ret;
    memcpy(out_ip, tmp_ip, 4);
    return 0;
}

const mod_network_nic_type_t mod_network_nic_type_esp32 = {
    .base = {
        {&mp_type_type},
        .name = MP_QSTR_ESP32_SPI,
        .make_new = esp32_make_new,
        .locals_dict = (mp_obj_dict_t *)&esp32_locals_dict,
    },
    .gethostbyname = esp32_socket_gethostbyname,
    .connect = esp32_socket_connect,
    .socket = esp32_socket_socket,
    .send = esp32_socket_send,
    .recv = esp32_socket_recv,
    .close = esp32_socket_close,
    .sendto = esp32_socket_sendto,
    .recvfrom = esp32_socket_recvfrom,
    /*
    .bind = cc3k_socket_bind,
    .listen = cc3k_socket_listen,
    .accept = cc3k_socket_accept,
    .setsockopt = cc3k_socket_setsockopt,
    .settimeout = cc3k_socket_settimeout,
    .ioctl = cc3k_socket_ioctl,
*/
};
