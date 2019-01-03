#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "py/stream.h"
#include "py/runtime.h"
#include "py/misc.h"
#include "py/mphal.h"
#include "extmod/misc.h"
#include "lib/netutils/netutils.h"

#include "utils.h"
#include "modmachine.h"
#include "mphalport.h"
#include "mpconfigboard.h"
#include "modnetwork.h"
#include "plic.h"
#include "sysctl.h"
#include "atomic.h"
#include "uart.h"
#include "uarths.h"

#include "esp8285.h"

#define Sipeed_DEBUG 0
#if Sipeed_DEBUG==1
#define debug_print(x,arg...) printf("[MaixPy]"x,##arg)
#else 
#define debug_print(x,arg...) 
#endif


/***************************************************************/
#define ESP_BUF_LEN 2048
unsigned char esp_buf[ESP_BUF_LEN] = {0};

uint32_t recv(mp_obj_t uart_obj,uint8_t *buf_in,uint32_t size){
	char* buf = NULL;
	int errcode  = 0;
	const mp_stream_p_t * uart_stream = mp_get_stream(uart_obj);
	if(buf_in == NULL || size == 0)
	{
		memset(esp_buf, 0, 1024);
		buf = esp_buf;
	}
	else
		buf = buf_in;
	if(0 == uart_rx_any(uart_obj))
	{
		debug_print("[MaixPy] %s | No data\n",__func__);
		return 0;
	}
	msleep(1);
	int rev_len = uart_stream->read(uart_obj,buf,size,&errcode); 
	if(0 == rev_len)
	{
		printf("[MaixPy] %s | receive data faild\n",__func__);
		return 0;
	}
	return rev_len;
}

uint32_t rev_all(mp_obj_t uart_obj,uint8_t *buf_in,int wait_times){

	uint8_t *buf = NULL;
	int remain_len = 0;
	int rev_len = 0;
	int total_rev_len = 0;
	struct _machine_uart_obj_t *uart = uart_obj;
	if(0 > wait_times)
		wait_times = 0;
	if(NULL == buf_in)
	{
		memset(esp_buf,0,1024);
		buf = esp_buf;
	}
	else
		buf = buf_in;
	if(uart_rx_wait(uart,(uart->timeout)*wait_times))
	{
		while(1)
		{
			rev_len = recv(uart_obj,buf+total_rev_len,10);
			if(0 == rev_len)
				break;
			total_rev_len = total_rev_len + rev_len;
		}
	}
	if(total_rev_len == 0)
	{
		debug_print(" %s | time out\n",__func__);
		return 0;
	}
	debug_print(" %s | total_rev_len = %d\n",__func__,total_rev_len);
	return total_rev_len;
}

int32_t read_data(mp_obj_t uart_obj,char* buf_in,char* buf_out,int wait_times)
{
	char* buf = NULL;
	char* buf_cur = NULL;
	int ret_len = 0;
	int data_len = 0;
	int errcode  = 0;
	struct _machine_uart_obj_t *uart = uart_obj;
	const mp_stream_p_t * uart_stream = mp_get_stream(uart_obj);
	int total_rev_len = 0;
	if(buf_out == NULL)
	{
		printf("[MaixPy] %s | buf of out is NULL\n",__func__);
		return -1;
	}
	if(NULL == buf_in)
		buf = esp_buf;
	else
		buf = buf_in;
	ret_len = uart_stream->read(uart_obj,buf,uart->read_buf_len,&errcode); //read all data
	debug_print(" %s | buf : %s\n",__func__,buf);
	if(0 == ret_len)
	{
		//printf("[MaixPy] %s | No data for reading\n",__func__);
		return 0;
	}

	if(-1 == check_ack(buf, ret_len, "IPD"))
	{
		debug_print("[MaixPy] %s | device don't return data\n",__func__);
		return -1;
	}
	buf_cur = buf;
	while(*buf_cur != '+')
		buf_cur++;
	sscanf(buf_cur,"+IPD,%d:",&data_len);
	debug_print("[MaixPy] %s | data_len = %d\n",__func__,data_len);
	while(*buf_cur++ != ':');
	memcpy(buf_out, buf_cur,data_len);
	debug_print(" %s | buf_out : %s\n",__func__,buf_out);
	return data_len;
}
uint8_t revc_string(mp_obj_t uart_obj,char* buf_in,uint8_t *str)
{
	uint8_t *str_p;
	uint8_t *buf;
	int ret_len = 0;
	struct _machine_uart_obj_t *uart = uart_obj;
	if(buf_in == NULL)
	{
		memset(esp_buf,0,1024);
		buf = esp_buf;
	}
	else
		buf = buf_in;
	str_p = str;
	while(*str_p !='\0' && uart_rx_wait(uart,uart->timeout))
	{
		if(recv(uart_obj,buf,1)<=0)
			break;
		if(*buf == *str_p)
		{
			str_p++;
		}else{
			str_p = str;
		}
		buf++;
		ret_len++;
	}
	return ret_len;	
}

uint32_t send(mp_obj_t uart_obj,uint8_t *buf_in,uint32_t size){
	const mp_stream_p_t * uart_stream = mp_get_stream(uart_obj);
	int errcode;
	int send_len = 0;
	if(buf_in == NULL || size == 0)
	{
		printf("[MaixPy] %s | send parameter error\n",__func__);
		return 0;
	}
	send_len = uart_stream->write(uart_obj,buf_in,size,&errcode); 
	if (MP_STREAM_ERROR == send_len)
	{
		printf("[MaixPy] %s | MP_STREAM_ERROR\n",__func__);
		return 0;
	}
	else if(0 == send_len)
	{
		printf("[MaixPy] %s | send data faild\n",__func__);
		return 0;
	}
	return send_len;
}


uint8_t send_data(mp_obj_t uart_obj,uint8_t *data,uint32_t size)
{
	uint8_t res=0; 
	uint32_t ret_len = 0;
	debug_print(" %s | send_data:%s\n",__func__,data);
	ret_len = send(uart_obj,data,size);
	if(ret_len == 0)
	{
		printf("[MaixPy] %s | send_data faild\n",__func__);
		return 0;	
	}
	res = revc_string(uart_obj,NULL,"OK");
	if(0 == res)
	{
		printf("[MaixPy] %s | 0 == res\n",__func__);
	}
	else
	{
		return ret_len;
	}	
}

uint8_t send_cmd(mp_obj_t uart_obj,uint8_t *cmd,uint8_t *ack,int wait_times)
{
	uint32_t ret=1;
	if(cmd == NULL)
	{
		printf("[MaixPy] %s | please input a command\n",__func__);
		return 0;
	}
	if(0 > wait_times)
		wait_times = 0;
SEND_CMD:
	debug_print(" %s | send_cmd %s \n",__func__,cmd);
	send(uart_obj,cmd,strlen(cmd));
	send(uart_obj,"\r\n",2);
	if(ack == NULL)
		return 1;// Received data but didn't know its ack string
	ret = rev_all(uart_obj,NULL,wait_times);
	if(ret == 0)
	{
		printf("[MaixPy] %s | can not get any about ack\n",__func__);
		return ret;
	}
	else
	{
		if(-1 != check_ack(NULL,ret,ack))
		{
			debug_print(" %s | Find ack\n",__func__);
			ret = 1;
		}
		else
		{
			if(-1 != check_ack(NULL,ret,"ERROR"))
			{
				printf("[MaixPy] %s | send command error\n",__func__);
				ret = 0;
			}
			else if(-1 != check_ack(NULL,ret,"busy"))
			{
				printf("[MaixPy] %s | send again\n",__func__);
				goto SEND_CMD;
			}
			printf("[MaixPy] %s | can not Find ack\n",__func__);
			ret = 0;
		}
	}

	return ret;
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
	int tagert_Length = strlen(tagert);
	int next[tagert_Length];
	get_next(tagert,next);
	index = Kmp_match(src_buf,src_len,tagert,next);
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
uint8_t esp8285_init(mp_obj_t uart_obj)
{
	//send_cmd(uart_obj,"AT+RESTORE","ready",5);
	bool config_flag = 1;
	config_flag = config_flag && send_cmd(uart_obj,"ATE0","OK",1);
	//printf("[MaixPy] %s | 1 \n",__func__);
	config_flag = config_flag && send_cmd(uart_obj,"AT","OK",1);
	//printf("[MaixPy] %s | 2 \n",__func__);
	config_flag = config_flag && send_cmd(uart_obj,"AT+CIPMODE=0","OK",1);
	//printf("[MaixPy] %s | 3 \n",__func__);
	config_flag = config_flag && send_cmd(uart_obj,"AT+CWMODE=1","OK",1);
	//printf("[MaixPy] %s | 4 \n",__func__);
	config_flag = config_flag && send_cmd(uart_obj,"AT+CIPMUX=0","OK",1);
	//printf("[MaixPy] %s | 5 \n",__func__);
	config_flag = config_flag && send_cmd(uart_obj,"AT+CWQAP","OK",1);
	//printf("[MaixPy] %s | 6 \n",__func__);
	if(1 == config_flag)
	{
		printf("[MaixPy] %s | esp8285 config success\n",__func__);
		return 1;
	}
	else
		return 0;
}

uint8_t esp8285_connect(mp_obj_t uart_obj,
							 uint8_t *ssid,uint32_t ssid_len,
							 uint8_t *password,uint32_t psw_len)
{
	uint8_t *command;
	uint32_t AT_len = strlen("AT+CWJAP_CUR=\"\",\"\"");
	uint32_t cmd_len = ssid_len+psw_len+AT_len;
	uint8_t ret = 0;
	command=m_new(uint8_t,cmd_len);
	sprintf((char*)command,"AT+CWJAP_CUR=\"%s\",\"%s\"",ssid,password);
	send_cmd(uart_obj,command,NULL,0);
	uint32_t rec_len = 0;
	memset(esp_buf, 0, 1024);
	while(1)
	{
		ret = rev_all(uart_obj,esp_buf+rec_len,5);
		if(0 != ret)
		{
			rec_len = rec_len + ret;
			if(-1 != check_ack(esp_buf,rec_len,"OK"))
			{
				ret = 1;
				printf("[MaixPy] %s | connect wifi\n",__func__);
				break;
			}
			else if(-1 != check_ack(esp_buf,rec_len,"ERROR"))
			{
				ret = 0;
				printf("[MaixPy] %s | connect ERROR\n",__func__);
				break;
			}
		}
		else
		{
			printf("[MaixPy] %s | connect failed\n",__func__);
			ret = 0;
		}
	}		
	m_del(uint8_t,command,cmd_len);
	return ret;
}

uint8_t esp8285_disconnect(mp_obj_t uart_obj)
{
	 uint8_t *command = "AT+CWQAP";
	 uint32_t cmd_len = strlen(command);
	 uint8_t ret = 0;
	 ret = send_cmd(uart_obj,command,"OK",5);
	 return ret;
}

ipconfig* esp8285_ipconfig(mp_obj_t uart_obj)
{	
	ipconfig* esp8285_ipconfig = m_new_obj_maybe(ipconfig);
	uint32_t rec_len = 0;
	uint8_t *CIPSTA_command = "AT+CIPSTA_CUR?";
	char* char_cur = NULL;
	//check ip_addr gateway netmask
	send_cmd(uart_obj,CIPSTA_command,NULL,0);
	rec_len = rev_all(uart_obj,NULL,5);
	if(0 == rec_len)
	{
		printf("[MaixPy] %s | esp8285_ipconfig could'n get CIPSTA return data\n",__func__);
		return NULL;
	}
	if(-1 == check_ack(NULL, rec_len,"OK"))
	{
		printf("[MaixPy] %s | esp8285_ipconfig  CIPSTA return ERROR\n",__func__);
		return NULL;
	}
	char_cur = strstr(esp_buf, "ip");
	if(char_cur == NULL)
	{
		printf("[MaixPy] %s | esp8285_ipconfig could'n get ip\n",__func__);
		return NULL;
	}
	char ip_buf[16] = {0};
	sscanf(char_cur,"ip:\"%[^\"]\"",ip_buf);
	esp8285_ipconfig->ip = mp_obj_new_str(ip_buf,strlen(ip_buf));
	char_cur = strstr(esp_buf, "gateway");
	if(char_cur == NULL)
	{
		printf("[MaixPy] %s | esp8285_ipconfig could'n get gateway\n",__func__);
		return NULL;
	}
	char gateway_buf[16] = {0};
	sscanf(char_cur,"gateway:\"%[^\"]\"",gateway_buf);
	esp8285_ipconfig->gateway = mp_obj_new_str(gateway_buf,strlen(gateway_buf));

	char_cur = strstr(esp_buf, "netmask");
	if(char_cur == NULL)
	{
		printf("[MaixPy] %s | esp8285_ipconfig could'n get netmask\n",__func__);
		return NULL;
	}
	char netmask_buf[16] = {0};
	sscanf(char_cur,"netmask:\"%[^\"]\"",netmask_buf);
	esp8285_ipconfig->netmask = mp_obj_new_str(netmask_buf,strlen(netmask_buf));
	//ssid & mac
	uint8_t *CWJAP_command = "AT+CWJAP_CUR?";
	send_cmd(uart_obj,CWJAP_command,NULL,0);
	rec_len = rev_all(uart_obj,NULL,5);	
	if(0 == rec_len)
	{
		printf("[MaixPy] %s | esp8285_ipconfig could'n get CWJAP data\n",__func__);
		return NULL;
	}	
	char ssid[50] = {0};
	char MAC[17] = {0};
	sscanf(esp_buf, "+CWJAP_CUR:\"%[^\"]\",\"%[^\"]\"", ssid, MAC);
	esp8285_ipconfig->ssid = mp_obj_new_str(ssid,strlen(ssid));
	esp8285_ipconfig->MAC = mp_obj_new_str(MAC,strlen(MAC));

	debug_print(" %s | ip_buf = %s\n",ip_buf);
	debug_print(" %s | gateway_buf = %s\n",gateway_buf);
	debug_print(" %s | netmask_buf = %s\n",netmask_buf);
	debug_print(" %s | ssid = %s\n",ssid);
	debug_print(" %s | MAC = %s\n",MAC);
	m_del_obj(ipconfig,esp8285_ipconfig);
	return esp8285_ipconfig;
}



int add_escapte(char* str,char* dest)
{
	unsigned char* tmp = dest;
	int len = strlen(str);
	int i = 0;
	if(str == NULL)
	{
		printf("[MaixPy] %s | str == NULL\n",__func__);
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


uint8_t esp8285_uart_gethostbyname(mp_obj_t uart_obj,uint8_t* host,uint32_t len,uint8_t* out_ip)
{
	unsigned char host_ip[len+2];//note!!
	add_escapte(host, host_ip);
	unsigned char cmd_buf[strlen("AT+CIPDOMAIN=%s")+len+1];
	sprintf(cmd_buf,"AT+CIPDOMAIN=\"%s\"",host);
	int ret = 0;
	ret = send_cmd(uart_obj,cmd_buf,"OK",3);
	if(-1 == ret)
	{
		printf("[MaixPy] %s | send command failed\n",__func__,host);
	}
	unsigned char IP_buf[16]={0};
	sscanf(esp_buf,"+CIPDOMAIN:%s",IP_buf);
	//printf("[MaixPy] %s | esp_buf = %s\n",__func__,esp_buf);
	//printf("[MaixPy] %s | IP_buf = %s\n",__func__,IP_buf);
	mp_obj_t IP = mp_obj_new_str(IP_buf, strlen(IP_buf));
	nlr_buf_t nlr;
	if (nlr_push(&nlr) == 0)
	{
		netutils_parse_ipv4_addr(IP,out_ip,NETUTILS_BIG);
		nlr_pop();
	}
	return 1;
}
int esp8285_uart_connect(mp_obj_t uart_obj,int type,const uint8_t* ip,mp_int_t port)
{
	mp_obj_t IP = netutils_format_ipv4_addr(ip,NETUTILS_BIG);
	char *host = mp_obj_str_get_str(IP);
	debug_print(" %s | host = %s\n",__func__,host);
	int cmd_len = strlen(host) + 
				  3 + //type
				  4 + //port char  
				  strlen("AT+CIPSTART=") + 
				  4;  //" char  
	uint8_t* command = m_new(uint8_t,cmd_len);
	switch(type)
	{
		case MOD_NETWORK_SOCK_STREAM:
			sprintf(command,"AT+CIPSTART=\"TCP\",\"%s\",%d",host,port);
			break;
		case MOD_NETWORK_SOCK_DGRAM:
			sprintf(command,"AT+CIPSTART=\"UDP\",\"%s\",%d",host,port);
			break;
		default:
			sprintf(command,"AT+CIPSTART=\"TCP\",\"%s\",%d",host,port);
			break;
	}
	debug_print(" %s | command %s\n",__func__,command);
	int ret = send_cmd(uart_obj,command,"OK",3);
	m_del(uint8_t, command, cmd_len);
	if(1 == ret)
	{
		if(-1 != check_ack(NULL, 1024, "CONNECT"))
			return 1; 
		else
		{
			printf("[MaixPy] %s | CONNECT ERROR\n",__func__);
			return 0;
		}
	}
	else if(-1 != check_ack(NULL, 1024, "ERROR"))
	{
		if(-1 != check_ack(NULL, 1024, "ALREADY CONNECTED"))
			return 1;
		else
		{
			printf("[MaixPy] %s | can not get \"OK\" ack\n",__func__);
			return 0;
		}
	}
}
