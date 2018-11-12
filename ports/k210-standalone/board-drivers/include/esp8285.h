#ifndef __ESP8285_H
#define __ESP8285_H

#include <stdint.h>

uint32_t rev_all(uint8_t *buf_in,int waittime);
uint32_t send(uint8_t *buf_in,uint32_t size);
uint32_t recv(uint8_t *buf_in,uint32_t size);
uint32_t read_data(char* buf_in,char* buf_out);
uint8_t send_cmd(uint8_t *cmd,uint8_t *ack,uint16_t waittime);
uint8_t *buf_addr();
uint8_t send_data(uint8_t *data,uint32_t size,int waittime);
uint32_t rev_data(uint8_t *data,uint32_t size,uint32_t waittime);
uint8_t check_cmd(uint8_t *str,uint8_t* buf);
uint16_t get_data_len(uint8_t* buf);

void get_next(char* targe, int next[]) ;
int check_ack(char* src,int src_len,char* tagert);
int Kmp_match(char* src,int src_len,char* targe, int* next) ;
uint8_t wifista_config(uint8_t *ssid,uint8_t *password);




/******************************************************************/
uint32_t _rev(uint8_t *buf_in,uint32_t size);
uint16_t esp8285_rev_buflen(uint8_t *str);
uint8_t esp8285_wifista_config(uint8_t *ssid,uint8_t *password);
void esp8285_at_response(uint8_t mode);
uint8_t esp8285_check_cmd(uint8_t *str);
uint8_t esp8285_send_cmd(uint8_t *cmd,uint8_t *ack,uint16_t waittime);
uint8_t esp8285_send_data(uint8_t *data,uint32_t size);
uint8_t esp8285_quit_trans(void);
void esp8285_init(void);
uint32_t esp8285_rev_data(uint8_t *data,uint32_t size,uint32_t waittime);
uint8_t *rev_buf_addr();
#endif //__ESP8285_H



