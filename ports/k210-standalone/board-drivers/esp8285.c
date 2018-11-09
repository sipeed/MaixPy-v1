#include "esp8285.h"
#include "sleep.h"
#include "uart.h"
#include "stdlib.h"
#include "string.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "plic.h"
#include "sysctl.h"
#include "utils.h"
#include "atomic.h"

#define uartn 0
#define baudrate 115200
#define bits UART_BITWIDTH_8BIT
#define stop UART_STOP_1
#define parity UART_PARITY_NONE

//#define printf(...)

void _init()
{
	uart_init(uartn);
	uart_config(uartn, (size_t)baudrate, (size_t)bits, stop,  parity);
} 

void _send(uint8_t *buf_in,uint32_t size){
	uart_send_data(uartn,buf_in,size);
	//printf("uart%d_send:%s\n",uartn,buf_in);
}

uint32_t _rev(uint8_t *buf_in,uint32_t size){
	//int rev_len = uart_receive_data(uartn, buf_in, size);
	//printf("uart%d_rev szie %d\n",uartn,size);
	uint8_t *temp_buf_in = buf_in;
	uint32_t temp_size = size;
	//while(temp_size--){
		//*temp_buf_in = uartapb_getc(uartn);
		//printf("uart%d_rev_char:%c\n",uartn,*temp_buf_in);
	//	temp_buf_in++;
	//}
	int rev_len = uart_receive_data(uartn, buf_in, size);
	return rev_len;
	//if (rev_len >0 )
	//*(buf_in+size) = "\0";
	//printf("uart%d_rev:%s\n",uartn,buf_in);
}

const uint8_t *ATK_ESP8266_ECN_TBL[5]={"OPEN","WEP","WPA_PSK","WPA2_PSK","WPA_WAP2_PSK"};

void esp8285_init(void)
{
	_init();
}

uint8_t esp8285_wifista_config(uint8_t *ssid,uint8_t *password)
{
	uint8_t *p;
	p=malloc(50);
	//esp8285_quit_trans();
	esp8285_send_cmd("AT+RESTORE","ready",1);
	esp8285_send_cmd("ATE0","OK",1);
	esp8285_send_cmd("AT","OK",1);
	esp8285_send_cmd("ATE0","OK",1);
	printf("config esp8285 ...\n");
	esp8285_send_cmd("AT+CIPMODE=0","OK",1);
	printf("esp8285 config success!\n");
	esp8285_send_cmd("ATE0","OK",1);
//	msleep(1000);
	esp8285_send_cmd("AT+CWMODE=1","OK",1);
	//esp8285_send_cmd("AT+CWMODE=3","OK",20);
	//while(esp8285_send_cmd("AT+CWHOSTNAME=\"Sipeed\"","OK",200))
	esp8285_send_cmd("AT+CIPMUX=0","OK",1);
	esp8285_send_cmd("AT+CWQAP","OK",1);
	sprintf((char*)p,"AT+CWJAP_CUR=\"%s\",\"%s\"",ssid,password);
	//printf("AT+CWJAP_CUR=\"%s\",\"%s\"",ssid,password);
	esp8285_send_cmd(p,"OK",1);
	free(p);
	return 0;
}

void esp8285_at_response(uint8_t mode)
{
	// if(USART3_RX_STA&0X8000)
	// { 
	// 	USART3_RX_BUF[USART3_RX_STA&0X7FFF]=0;
	// 	printf("%s",USART3_RX_BUF);
	// 	if(mode)USART3_RX_STA=0;
	// } 
}
uint8_t temp_rev_str[1024];
uint8_t *esp8285_check_cmd(uint8_t *str)
{
	uint8_t *p;
	uint8_t *temp_p;
	uint8_t *temp_rev_str_p;
	temp_rev_str_p = temp_rev_str;
	temp_p = str;
	while(*temp_p !='\0')
	{
		while(_rev(temp_rev_str_p,1)<=0);
		//printf("temp_p = %c\n",temp_rev_str_p);
		if(*temp_rev_str_p == *temp_p)
		{
			temp_p++;
			temp_rev_str_p++;
			//printf("%c\n",*temp_rev_str_p);
		}else{
			//printf("temp_p init\n");
			temp_p = str;
			temp_rev_str_p = temp_rev_str;
		}
	}
	
	p = (uint8_t *)(strstr(temp_rev_str,str));
	//printf("esp8285_check_cmd p %d\n",p);
	if(p != NULL)
	{
		printf("uart%d_rev:%s\n",uartn,temp_rev_str);
	}
	
	return p;
}

uint16_t esp8285_rev_buflen(uint8_t *str)
{
	uint16_t len = 0;
	uint8_t *temp_p;
	uint8_t *temp_rev_str_p;
	temp_rev_str_p = temp_rev_str;
	temp_p = str;
	while(*temp_p !='\0')
	{
		while(_rev(temp_rev_str_p,1)<=0);
		//printf("temp_p = %c\n",temp_rev_str_p);
		if(*temp_rev_str_p == *temp_p)
		{
			temp_p++;
			//printf("%c\n",*temp_rev_str_p);
		}else{
			//printf("temp_p init\n");
			len = len * 10;
			len = len + *temp_rev_str_p - 0x30;
			temp_p = str;
		}
		temp_rev_str_p++;
	}
	
	return len;
}

uint8_t esp8285_send_cmd(uint8_t *cmd,uint8_t *ack,uint16_t waittime)
{
	uint8_t res=1;
	printf("esp8285_send_cmd %s \n",cmd);
	memset(temp_rev_str,0x00,1024);
	_send(cmd,strlen(cmd));
	_send("\r\n",2);
	if(esp8285_check_cmd(ack))
	{
		printf("%s ack:%s\r\n",cmd,(uint8_t*)ack);
		res = 0;
	}
	return res;
}

uint8_t esp8285_send_data(uint8_t *data,uint32_t size)
{
	//uint8_t res=0; 
	printf("uart%d_send_data:%s\n",uartn,data);
	_send(data,size);
	//return res;
}

uint32_t esp8285_rev_data(uint8_t *data,uint32_t size,uint32_t waittime)
{
	uint32_t rev_size = 0;
	if(waittime)
	{
		while(--waittime)
		{
			rev_size = _rev(data,size);
			if(rev_size>0)break;
		}
	}
	return rev_size;
}

uint8_t esp8285_quit_trans(void)
{
	_send('+',1);
	msleep(15);
	_send('+',1);
	msleep(15);
	_send('+',1);
	_send("\r\n",2);
	msleep(500);
	_send("+++\r\n",5);
	msleep(1500);
	while(esp8285_send_cmd("AT","OK",200)){
		msleep(10);
	}
	return esp8285_send_cmd("AT","OK",20);
}

uint8_t esp8285_consta_check(void)
{
	uint8_t *p;
	uint8_t res;
	uint8_t temp_rev_str[1];
	if(esp8285_quit_trans())return 0;		
	esp8285_send_cmd("AT+CIPSTATUS",":",50);
	p=esp8285_check_cmd("+CIPSTATUS:"); 
	_rev(temp_rev_str,1);
	res=*p;
	return res;
}

void esp8285_get_wanip(uint8_t* ipbuf)
{
	uint8_t *p,*p1;
	if(esp8285_send_cmd("AT+CIFSR","OK",50))
	{
		ipbuf[0]=0;
		return;
	}	
	p=esp8285_check_cmd("\"");
	p1=(uint8_t*)strstr((const char*)(p+1),"\"");
	*p1=0;
	sprintf((char*)ipbuf,"%s",p+1);
}


























