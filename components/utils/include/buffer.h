/*
* @File  buffer.h
* @Brief fifo buffer
*
* @Author: Neucrack
* @Date: 2017-12-12 14:26:19
 * @Last Modified by: Neucrack
 * @Last Modified time: 2017-12-12 19:06:57
* @License MIT
*/

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
	uint16_t   front;
	uint16_t   rear;
	uint8_t*   buffer;
	uint32_t   maxSize;
}Buffer_t;

//注意，没有使用长度这个变量，是为了防止有中断打断时快速反应（用mfront和mrear计算）
//在put和get函数中体会，所以函数都是先操作头指针和尾指针，再进行存数
//可以添加锁操作


void Buffer_Init(Buffer_t* buffer, uint8_t* dataBuffer, uint32_t maxSize);


//////////////////////////////
///@breif put multiple node data to queue 
///@param data: the first node data adress of node data array will put into queue
///@param length: the length of node data that will put into queue
/////////////////////////////
bool Buffer_Puts(Buffer_t* buffer, uint8_t* data, uint16_t length);


//////////////////////////////
///@breif get multiple node data from queue
///@param data: the first node data adress of node data array will get from the queue
///@param length: the length of node data that will get from the queue
/////////////////////////////
bool Buffer_Gets(Buffer_t* buffer, uint8_t *data, uint16_t length);




int32_t Buffer_StartPostion(Buffer_t* buffer);


/**
* query some data from the queue, and the data remain be a part of the queue
* @param data the address of the data save to
* @param startPosition Index of where to start query
* #param num The number of data will be query
* @retval Is query succeed
*/
int32_t Buffer_Query(Buffer_t* buffer, uint8_t* data, uint16_t length, uint16_t startPosition);

////////////////////////////
///@breif get the size of queue
///@retval the size of queue
///////////////////////////
uint32_t Buffer_Size(Buffer_t* buffer);


uint32_t Buffer_Size2(Buffer_t* buffer,uint32_t index);


////////////////////////////////
///@breif clear the queue
////////////////////////////////
void Buffer_Clear(Buffer_t* buffer);


#ifdef __cplusplus
}
#endif

#endif

