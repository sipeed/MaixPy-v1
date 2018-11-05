#ifndef __ESP8285_H
#define __ESP8285_H

#include <stdint.h>

uint8_t esp8285_wifista_config(uint8_t *ssid,uint8_t *password);
void esp8285_at_response(uint8_t mode);
uint8_t *esp8285_check_cmd(uint8_t *str);
uint8_t esp8285_send_cmd(uint8_t *cmd,uint8_t *ack,uint16_t waittime);
uint8_t esp8285_send_data(uint8_t *data,uint32_t size);
uint8_t esp8285_consta_check(void);
uint8_t esp8285_quit_trans(void);
void esp8285_get_wanip(uint8_t* ipbuf);
void esp8285_init(void);
uint32_t esp8285_rev_data(uint8_t *data,uint32_t size,uint32_t waittime);

#endif //__ESP8285_H



