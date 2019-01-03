#ifndef __ESP8285_H
#define __ESP8285_H

#include <stdint.h>
#include "mpconfigboard.h"
#include "py/stream.h"
uint32_t recv(mp_obj_t uart_obj,uint8_t *buf_in,uint32_t size);
uint32_t send(mp_obj_t uart_obj,uint8_t *buf_in,uint32_t size);
int32_t read_data(mp_obj_t uart_obj,char* buf_in,char* buf_out,int wait_times);
uint8_t send_cmd(mp_obj_t uart_obj,uint8_t *cmd,uint8_t *ack,int wait_times);
uint8_t *buf_addr();
uint8_t send_data(mp_obj_t uart_obj,uint8_t *data,uint32_t size);
uint16_t get_data_len(uint8_t* buf);
void get_next(char* targe, int next[]);
int check_ack(char* src,int src_len,char* tagert);
int Kmp_match(char* src,int src_len,char* targe, int* next);
uint8_t esp8285_init(mp_obj_t uart_obj);
uint8_t esp8285_connect(mp_obj_t uart_obj, uint8_t *ssid,uint32_t ssid_len,uint8_t *password,uint32_t psw_len);
uint8_t esp8285_disconnect(mp_obj_t uart_obj);
ipconfig* esp8285_ipconfig(mp_obj_t uart_obj);
uint8_t esp8285_uart_gethostbyname(mp_obj_t uart_obj,uint8_t* host,uint32_t len,uint8_t* out_ip);
int esp8285_uart_connect(mp_obj_t uart_obj,int type,const uint8_t* ip,mp_int_t port);

#endif
