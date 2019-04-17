/**
 * @file ESP8266.h
 * @brief The definition of class ESP8266. 
 * @author Wu Pengfei<pengfei.wu@itead.cc> 
 * @date 2015.02
 * 
 * @par Copyright:
 * Copyright (c) 2015 ITEAD Intelligent Systems Co., Ltd. \n\n
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version. \n\n
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __ESP8285_H__
#define __ESP8285_H__


#include "py/stream.h"
#include "py/runtime.h"
#include "py/misc.h"
#include "py/mphal.h"
#include "py/objstr.h"
#include "extmod/misc.h"
#include "lib/netutils/netutils.h"

#include "utils.h"
#include "modmachine.h"
#include "mphalport.h"
#include "mpconfigboard.h"
#include "modnetwork.h"


////////////////////////// config /////////////////////////

#define ESP8285_MAX_ONCE_SEND 2048

//////////////////////////////////////////////////////////

typedef struct _ipconfig_obj
{
	mp_obj_t ip;
	mp_obj_t gateway;
	mp_obj_t netmask;
	mp_obj_t ssid;
	mp_obj_t MAC;
}ipconfig_obj;

typedef struct _esp8285_obj
{
	mp_obj_t uart_obj;
	uint8_t buffer[ESP8285_BUF_SIZE];
	
}esp8285_obj;

/*
 * Provide an easy-to-use way to manipulate ESP8266. 
 */

bool kick(esp8285_obj* nic);

/**
 * Restart ESP8266 by "AT+RST". 
 *
 * This method will take 3 seconds or more. 
 *
 * @retval true - success.
 * @retval false - failure.
 */
bool reset(esp8285_obj* nic);

/**
 * Get the version of AT Command Set. 
 * 
 * @return the char* of version. 
 */
 
uint8_t* getVersion(esp8285_obj* nic);

/**
 * Set operation mode to staion. 
 * 
 * @retval true - success.
 * @retval false - failure.
 */
 
bool setOprToStation(esp8285_obj* nic);

/**
 * Join in AP. 
 *
 * @param ssid - SSID of AP to join in. 
 * @param pwd - Password of AP to join in. 
 * @retval true - success.
 * @retval false - failure.
 * @note This method will take a couple of seconds. 
 */
bool joinAP(esp8285_obj* nic,uint8_t* ssid, uint8_t* pwd);


/**
 * Enable DHCP for client mode. 
 *
 * @param mode - server mode (0=soft AP, 1=station, 2=both
 * @param enabled - true if dhcp should be enabled, otherwise false
 * 
 * @note This method will enable DHCP but only for client mode!
 */
bool enableClientDHCP(esp8285_obj* nic,uint8_t mode, bool enabled);

/**
 * Leave AP joined before. 
 *
 * @retval true - success.
 * @retval false - failure.
 */
bool leaveAP(esp8285_obj* nic);

/**
 * Get the current status of connection(UDP and TCP). 
 * 
 * @return the status. 
 */
uint8_t* getIPStatus(esp8285_obj* nic);

/**
 * Get the IP address of ESP8266. 
 *
 * @return the IP list. 
 */
uint8_t* getLocalIP(esp8285_obj* nic);

/**
 * Enable IP MUX(multiple connection mode). 
 *
 * In multiple connection mode, a couple of TCP and UDP communication can be builded. 
 * They can be distinguished by the identifier of TCP or UDP named mux_id. 
 * 
 * @retval true - success.
 * @retval false - failure.
 */
bool enableMUX(esp8285_obj* nic);

/**
 * Disable IP MUX(single connection mode). 
 *
 * In single connection mode, only one TCP or UDP communication can be builded. 
 * 
 * @retval true - success.
 * @retval false - failure.
 */
bool disableMUX(esp8285_obj* nic);


/**
 * Create TCP connection in single mode. 
 * 
 * @param addr - the IP or domain name of the target host. 
 * @param port - the port number of the target host. 
 * @retval true - success.
 * @retval false - failure.
 */
bool createTCP(esp8285_obj* nic,uint8_t* addr, uint32_t port);
/**
 * Release TCP connection in single mode. 
 * 
 * @retval true - success.
 * @retval false - failure.
 */
bool releaseTCP(esp8285_obj* nic);

/**
 * Register UDP port number in single mode.
 * 
 * @param addr - the IP or domain name of the target host. 
 * @param port - the port number of the target host. 
 * @retval true - success.
 * @retval false - failure.
 */
bool registerUDP(esp8285_obj* nic,uint8_t* addr, uint32_t port);

/**
 * Unregister UDP port number in single mode. 
 * 
 * @retval true - success.
 * @retval false - failure.
 */
bool unregisterUDP(esp8285_obj* nic);

/**
 * Create TCP connection in multiple mode. 
 * 
 * @param mux_id - the identifier of this TCP(available value: 0 - 4). 
 * @param addr - the IP or domain name of the target host. 
 * @param port - the port number of the target host. 
 * @retval true - success.
 * @retval false - failure.
 */
bool createTCP_mul(esp8285_obj* nic,uint8_t mux_id, uint8_t* addr, uint32_t port);

/**
 * Release TCP connection in multiple mode. 
 * 
 * @param mux_id - the identifier of this TCP(available value: 0 - 4). 
 * @retval true - success.
 * @retval false - failure.
 */
bool releaseTCP_mul(esp8285_obj* nic,uint8_t mux_id);

/**
 * Register UDP port number in multiple mode.
 * 
 * @param mux_id - the identifier of this TCP(available value: 0 - 4). 
 * @param addr - the IP or domain name of the target host. 
 * @param port - the port number of the target host. 
 * @retval true - success.
 * @retval false - failure.
 */
bool registerUDP_mul(esp8285_obj* nic,uint8_t mux_id, uint8_t* addr, uint32_t port);

/**
 * Unregister UDP port number in multiple mode. 
 * 
 * @param mux_id - the identifier of this TCP(available value: 0 - 4). 
 * @retval true - success.
 * @retval false - failure.
 */
bool unregisterUDP_mul(esp8285_obj* nic,uint8_t mux_id);

/**
 * Set the timeout of TCP Server. 
 * 
 * @param timeout - the duration for timeout by second(0 ~ 28800, default:180). 
 * @retval true - success.
 * @retval false - failure.
 */
bool setTCPServerTimeout(esp8285_obj* nic,uint32_t timeout);

/**
 * Start TCP Server(Only in multiple mode). 
 * 
 * After started, user should call method: getIPStatus to know the status of TCP connections. 
 * The methods of receiving data can be called for user's any purpose. After communication, 
 * release the TCP connection is needed by calling method: releaseTCP with mux_id. 
 *
 * @param port - the port number to listen(default: 333).
 * @retval true - success.
 * @retval false - failure.
 *
 * @see char* getIPStatus(void);
 * @see uint32_t recv(uint8_t *coming_mux_id, uint8_t *buffer, uint32_t len, uint32_t timeout);
 * @see bool releaseTCP(uint8_t mux_id);
 */
bool startTCPServer(esp8285_obj* nic,uint32_t port);

/**
 * Stop TCP Server(Only in multiple mode). 
 * 
 * @retval true - success.
 * @retval false - failure.
 */
bool stopTCPServer(esp8285_obj* nic);

/**
 * Start Server(Only in multiple mode). 
 * 
 * @param port - the port number to listen(default: 333).
 * @retval true - success.
 * @retval false - failure.
 *
 * @see char* getIPStatus(void);
 * @see uint32_t recv(uint8_t *coming_mux_id, uint8_t *buffer, uint32_t len, uint32_t timeout);
 */
bool startServer(esp8285_obj* nic,uint32_t port);

/**
 * Stop Server(Only in multiple mode). 
 * 
 * @retval true - success.
 * @retval false - failure.
 */
bool stopServer(esp8285_obj* nic);

bool get_host_byname(esp8285_obj* nic,uint8_t* host,uint32_t len,uint8_t* out_ip);

/**
 * Send data based on TCP or UDP builded already in single mode. 
 * 
 * @param buffer - the buffer of data to send. 
 * @param len - the length of data to send. 
 * @retval true - success.
 * @retval false - failure.
 */
bool esp_send(esp8285_obj* nic,const uint8_t *buffer, uint32_t len, uint32_t timeout);
        
/**
 * Send data based on one of TCP or UDP builded already in multiple mode. 
 * 
 * @param mux_id - the identifier of this TCP(available value: 0 - 4). 
 * @param buffer - the buffer of data to send. 
 * @param len - the length of data to send. 
 * @retval true - success.
 * @retval false - failure.
 */
bool esp_send_mul(esp8285_obj* nic,uint8_t mux_id, const uint8_t *buffer, uint32_t len);

/**
 * Receive data from TCP or UDP builded already in single mode. 
 *
 * @param buffer - the buffer for storing data. 
 * @param buffer_size - the length of the buffer. 
 * @param timeout - the time waiting data. 
 * @return the length of data received actually. 
 */
uint32_t esp_recv(esp8285_obj* nic,uint8_t *buffer, uint32_t buffer_size, uint32_t timeout);

/**
 * Receive data from one of TCP or UDP builded already in multiple mode. 
 *
 * @param mux_id - the identifier of this TCP(available value: 0 - 4). 
 * @param buffer - the buffer for storing data. 
 * @param buffer_size - the length of the buffer. 
 * @param timeout - the time waiting data. 
 * @return the length of data received actually. 
 */
uint32_t esp_recv_mul(esp8285_obj* nic,uint8_t mux_id, uint8_t *buffer, uint32_t buffer_size, uint32_t timeout);

/**
 * Receive data from all of TCP or UDP builded already in multiple mode. 
 *
 * After return, coming_mux_id store the id of TCP or UDP from which data coming. 
 * User should read the value of coming_mux_id and decide what next to do. 
 * 
 * @param coming_mux_id - the identifier of TCP or UDP. 
 * @param buffer - the buffer for storing data. 
 * @param buffer_size - the length of the buffer. 
 * @param timeout - the time waiting data. 
 * @return the length of data received actually. 
 */
uint32_t esp_recv_mul_id(esp8285_obj* nic,uint8_t *coming_mux_id, uint8_t *buffer, uint32_t buffer_size, uint32_t timeout);

/* 
 * Empty the buffer or UART RX.
 */
void rx_empty(esp8285_obj* nic);

/* 
 * Recvive data from uart and search first target. Return true if target found, false for timeout.
 */
bool recvFind(esp8285_obj* nic,uint8_t* target, uint32_t timeout);

/* 
 * Recvive data from uart and search first target and cut out the subchar* between begin and end(excluding begin and end self). 
 * Return true if target found, false for timeout.
 */
bool recvFindAndFilter(esp8285_obj* nic,uint8_t* target, uint8_t* begin, uint8_t* end, uint8_t* data, uint32_t timeout);

/*
 * Receive a package from uart. 
 *
 * @param buffer - the buffer storing data. 
 * @param buffer_size - guess what!
 * @param data_len - the length of data actually received(maybe more than buffer_size, the remained data will be abandoned).
 * @param timeout - the duration waitting data comming.
 * @param coming_mux_id - in single connection mode, should be NULL and not NULL in multiple. 
 */
uint32_t recvPkg(esp8285_obj*nic,uint8_t *buffer, uint32_t buffer_size, uint32_t *data_len, uint32_t timeout, uint8_t *coming_mux_id);


bool eAT(esp8285_obj* nic);
bool eATE(esp8285_obj* nic,bool enable);
bool eATRST(esp8285_obj* nic);
bool eATGMR(esp8285_obj* nic,uint8_t* version);
bool qATCWMODE(esp8285_obj* nic,uint8_t *mode);
bool sATCWMODE(esp8285_obj* nic,uint8_t mode);
bool sATCWJAP(esp8285_obj* nic,uint8_t* ssid, uint8_t* pwd);
bool sATCWDHCP(esp8285_obj* nic,uint8_t mode, bool enabled);
bool eATCWQAP(esp8285_obj* nic);

bool eATCIPSTATUS(esp8285_obj* nic,uint8_t* list);
bool sATCIPSTARTSingle(esp8285_obj* nic,uint8_t* type, uint8_t* addr, uint32_t port);
bool sATCIPSTARTMultiple(esp8285_obj*nic,uint8_t mux_id, uint8_t* type, uint8_t* addr, uint32_t port);
bool sATCIPSENDSingle(esp8285_obj*nic,const uint8_t *buffer, uint32_t len, uint32_t timeout);
bool sATCIPSENDMultiple(esp8285_obj* nic,uint8_t mux_id, const uint8_t *buffer, uint32_t len);
bool sATCIPCLOSEMulitple(esp8285_obj* nic,uint8_t mux_id);
bool eATCIPCLOSESingle(esp8285_obj* nic);
bool eATCIFSR(esp8285_obj* nic,uint8_t* list);
bool sATCIPMUX(esp8285_obj* nic,uint8_t mode);
bool sATCIPSERVER(esp8285_obj* nic,uint8_t mode, uint32_t port);
bool sATCIPSTO(esp8285_obj* nic,uint32_t timeout);
bool sATCIPMODE(esp8285_obj* nic,uint8_t mode);
bool sATCIPDOMAIN(esp8285_obj* nic,uint8_t* domain_name);
bool qATCWJAP_CUR(esp8285_obj* nic);
bool sATCIPSTA_CUR(esp8285_obj* nic,uint8_t* ip,uint8_t* gateway,uint8_t* netmask);
bool qATCIPSTA_CUR(esp8285_obj* nic);
bool eINIT(esp8285_obj* nic);
bool get_ipconfig(esp8285_obj* nic, ipconfig_obj* ipconfig);

/*
 * +IPD,len:data
 * +IPD,id,len:data
 */
 
/*
* bool sATCWSAP(char* ssid, char* pwd, uint8_t chl, uint8_t ecn);
* bool eATCWLIF(char* &list);

*/

/**
 * Set operation mode to softap. 
 * 
 * @retval true - success.
 * @retval false - failure.
 * bool setOprToSoftAP(void);
 */

/**
 * Set operation mode to station + softap. 
 * 
 * @retval true - success.
 * @retval false - failure.
 * bool setOprToStationSoftAP(void);
 */


/**
 * Search available AP list and return it.
 * 
 * @return the list of available APs. 
 * @note This method will occupy a lot of memeory(hundreds of Bytes to a couple of KBytes). 
 * Do not call this method unless you must and ensure that your board has enough memery left.
 * char* getAPList(void);
 */

/**
 * Set SoftAP parameters. 
 * 
 * @param ssid - SSID of SoftAP. 
 * @param pwd - PASSWORD of SoftAP. 
 * @param chl - the channel (1 - 13, default: 7). 
 * @param ecn - the way of encrypstion (0 - OPEN, 1 - WEP, 
 *  2 - WPA_PSK, 3 - WPA2_PSK, 4 - WPA_WPA2_PSK, default: 4). 
 * @note This method should not be called when station mode.
 * bool setSoftAPParam(char* ssid, char* pwd, uint8_t chl = 7, uint8_t ecn = 4);
*/

/**
 * Get the IP list of devices connected to SoftAP. 
 * 
 * @return the list of IP.
 * @note This method should not be called when station mode. 
 * char* getJoinedDeviceIP(void);
 */
//bool eATCWLAP(char* &list);
#endif
