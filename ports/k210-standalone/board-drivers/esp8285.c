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

#define Sipeed_DEBUG 0
#if Sipeed_DEBUG==1
#define debug_print(x,arg...) printf("[Sipeed]"x,##arg)
#else 
#define debug_print(x,arg...) 
#endif


void _init()
{
	uart_init(uartn);
	uart_config(uartn, (size_t)baudrate, (size_t)bits, stop,  parity);
} 


/***************************************************************/

unsigned char esp_buf[1024] = {0};

uint32_t recv(uint8_t *buf_in,uint32_t size){
	char* buf = NULL;
	if(buf_in == NULL || size == 0)
	{
		memset(esp_buf, 0, 1024);
		buf = esp_buf;
	}
	else
		buf = buf_in;
	if(0 == uart_ringbuf_length(uartn))
	{
		printf("No data\r\n");
		return 0;
	}
	int rev_len = uart_receive_data(uartn, buf, size);
	if(0 == rev_len)
	{
		printf("receive data faild\r\n");
		return 0;
	}
	return rev_len;
}

uint32_t rev_all(uint8_t *buf_in,int waittime){

	uint8_t *tmp_buf = NULL;
	int remain_len = 0;
	int rev_len = 0;
	int time_out = waittime;
	int total_rev_len = 0;
	debug_print("rev_all \r\n");
	if(NULL == buf_in)
	{
		memset(esp_buf,0,1024);
		tmp_buf = esp_buf;
	}
	else
		tmp_buf = buf_in;
	while(time_out > 0)
	{
		remain_len = uart_ringbuf_length(uartn); 
		if(0 == remain_len)
		{
			time_out--;
			msleep(150);
			continue;
		}
		//time_out = waittime;
		rev_len = recv(tmp_buf+total_rev_len,remain_len);
		total_rev_len = total_rev_len + rev_len;
	}
	if(total_rev_len == 0)
	{
		printf("time out when got length\r\n");
		return 0;
	}
#if Sipeed_DEBUG==1
	tmp_buf = esp_buf;
	int a = 0;
	while(total_rev_len > a)
	{
		debug_print("%c",tmp_buf[a]);
		a++;
	}
#endif
	debug_print("total_rev_len = %d\r\n",total_rev_len);
	return total_rev_len;
}

uint32_t rev_data(uint8_t *buf,uint32_t size,uint32_t waittime)
{
	uint32_t rev_size = 0;
	char* buf_cur = NULL;
	if(buf == NULL)
	{
		memset(esp_buf,0,1024);
		buf_cur = esp_buf;
	}
	else
		buf_cur = buf;
	if(waittime)
	{
		while(--waittime)
		{
			rev_size = recv(buf,size);
			if(rev_size>0 && rev_size == size)
				break;
		}
	}
#if Sipeed_DEBUG==1
	char* tmp_buf = esp_buf;
	int a = 0;
	while(rev_size > a)
	{
		printf("%c",tmp_buf[a]);
		a++;
	}
#endif
	return rev_size;
}
uint32_t read_data(char* buf_in,char* buf_out)
{
	char* buf = NULL;
	char* buf_cur = NULL;
	int ret_len = 0;
	int data_len = 0;
	char temp[20] = {0};
	if(buf_out == NULL)
	{
		printf("buf of out is NULL\n");
		return 0;
	}
	ret_len = rev_all(NULL, 1);
	if(0 == ret_len)
	{
		printf("No data for reading\n");
		return 0;
	}
	if(buf_in == NULL)
		buf = esp_buf;
	else
		buf = buf_in;
	if(-1 == check_ack(buf, ret_len, "IPD"))
	{
		printf("device don't return data\n");
		return 0;
	}
	buf_cur = buf;
	while(*buf_cur != '+')
		buf_cur++;
	debug_print("cur = %s\n",buf_cur);
	
	sscanf(buf_cur,"+IPD,%d:",&data_len);
	debug_print("data len = %d\n",data_len);
	
	sprintf(temp,"+IPD,%d:",data_len);
	debug_print("temp = %s\n",temp);
	
	int data_cur = strlen(temp);
	memcpy(buf_out, buf_cur+data_cur,data_len);
	debug_print("buf_out = %s\n",buf_out);
	
	return data_len;
}
uint8_t revc_string(char* buf_in,uint8_t *str,int waittime)
{
	uint8_t *temp_p;
	uint8_t *temp_rev;
	int ret_len = 0;
	int wait = waittime;
	if(buf_in == NULL)
	{
		memset(esp_buf,0,1024);
		temp_rev = esp_buf;
	}
	else
		temp_rev = buf_in;
	temp_p = str;
	while(*temp_p !='\0' && wait > 0)
	{
		while(_rev(temp_rev,1)<=0);
		if(*temp_rev == *temp_p)
		{
			temp_p++;
			//wait = waittime;
		}else{
			temp_p = str;
			wait--;
		}
		temp_rev++;
		ret_len++;
	}
	debug_print("ret len = %d,wait = %d\n",ret_len,wait);
	return ret_len;	
}

uint32_t send(uint8_t *buf_in,uint32_t size){
	if(buf_in == NULL || size == 0)
	{
		printf("send parameter error\n");
		return 0;
	}
	int ret_len = 0;
	ret_len = uart_send_data(uartn,buf_in,size);
	if(0 == ret_len)
	{
		printf("send data faild\n");
		return 0;
	}
	return ret_len;
}

uint8_t send_cmd(uint8_t *cmd,uint8_t *ack,uint16_t waittime)
{
	if(cmd == NULL)
	{
		printf("please input a command\n");
		return 0;
	}
	uint32_t ret=1;
	printf("send_cmd %s \r\n",cmd);	
	send(cmd,strlen(cmd));
	send("\r\n",2);
	ret = rev_all(NULL,waittime);
	if(ret == 0)
	{
		printf("can not get any about ack\r\n");
		return ret;
	}
	if(ack == NULL && ret != 0)
		return 1;// rec the data but don't know it is wethea a ack;
	else if(ack != NULL)
	{
		if(-1 != check_ack(esp_buf,ret,ack))
		{
			debug_print("Find ack\n");
			ret = 1;
		}
		else
		{
			if(-1 != check_ack(esp_buf,ret,"ERROR"))
			{
				printf("send command error\n");
				ret = 0;
			}
			printf("can not Find ack\n");
			ret = 0;
		}
	}

	return ret;
}
uint8_t send_data(uint8_t *data,uint32_t size,int waittime)
{
	uint8_t res=0; 

	debug_print("uart%d_send_data:%s\n",uartn,data);
	res = send(data,size);
	if(res == 0)
	{
		printf("send_data faild\n");
		return 0;	
	}
	res = revc_string(NULL,"OK",300);
	int index =  check_ack(NULL,res,"SEND");
	if(-1 ==index)
		return 0;
	else
		return res;
	//return res;
}

void get_next(char* targe, int next[])  
{  
    int targe_Len = strlen(targe);  
    next[0] = -1;  
    int k = -1;  
    int j = 0;  
    while (j < targe_Len - 1)  
    {     
        if (k == -1 || targe[j] == targe[k])  
        {  
            ++j;  
            ++k;   
            if (targe[j] != targe[k])  
                next[j] = k;    
            else   
                next[j] = next[k];  
        }  
        else  
        {  
            k = next[k];  
        }  
    }  
}  
int Kmp_match(char* src,int src_len,char* targe, int* next)  
{  
    int i = 0;  
    int j = 0;  
    int sLen = src_len;  
    int pLen = strlen(targe);  
    while (i < sLen && j < pLen)  
    {     
        if (j == -1 || src[i] == targe[j])  
        {  
            i++;  
            j++;  
        }  
        else  
        {       
            j = next[j];  
        }  
    }  
    if (j == pLen)  
        return i - j;  
    else  
        return -1;  
} 
int check_ack(char* src,int src_len,char* tagert)
{
	int index = 0;
	char* src_buf = NULL;
	if(NULL == src)
		src_buf = esp_buf;
	else
		src_buf = src;
	debug_print("src_len = %d\n",src_len);
	int tagert_Length = strlen(tagert);
	int next[tagert_Length];
	get_next(tagert,next);
	index = Kmp_match(src_buf,src_len,tagert,next);
	debug_print("index = %d\n",index);
	return index;
}
uint8_t *buf_addr()
{
	return esp_buf;
}

uint16_t get_data_len(uint8_t* buf)
{
	uint16_t len = 0;
	char* buf_cur = NULL;
	if(buf = NULL)
		buf_cur = esp_buf;
	else
		buf_cur = buf;
	return len;
}
uint8_t wifista_config(uint8_t *ssid,uint8_t *password)
{
	uint8_t *p;
	p=malloc(50);
	//esp8285_quit_trans();
	//send_cmd("AT+RESTORE","ready",5);
	send_cmd("ATE0","OK",4);
	send_cmd("AT","OK",4);
	send_cmd("ATE0","OK",4);
	printf("config esp8285 ...\n");
	send_cmd("AT+CIPMODE=0","OK",4);
	printf("esp8285 config success!\n");
	send_cmd("ATE0","OK",4);
	send_cmd("AT+CWMODE=1","OK",4);
	send_cmd("AT+CIPMUX=0","OK",4);
	send_cmd("AT+CWQAP","OK",4);
	sprintf((char*)p,"AT+CWJAP_CUR=\"%s\",\"%s\"",ssid,password);
	send_cmd(p,"OK",50);
	free(p);
	return 0;
}



/**********************************************************************************/
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
uint8_t *rev_buf_addr()
{
	return temp_rev_str;
}
uint8_t esp8285_check_cmd(uint8_t *str)
{
	uint8_t *temp_p;
	uint8_t *temp_rev_str_p;
	temp_rev_str_p = temp_rev_str;
	temp_p = str;
	uint8_t res = 0;
	while(*temp_p !='\0')
	{
		while(_rev(temp_rev_str_p,1)<=0);
		//printf("temp_p = %c\n",temp_rev_str_p);
		if(*temp_rev_str_p == *temp_p)
		{
			temp_p++;
			res = 1;
			//printf("%c\n",*temp_rev_str_p);
		}else{
			//printf("temp_p init\n");
			temp_p = str;
		}
		temp_rev_str_p++;
	}
	return res;	
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
		res = 1;
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




























