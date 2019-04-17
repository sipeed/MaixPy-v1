

#include "buffer.h"
#include "string.h"

//TODO: add mutex if multiple thread supported

void Buffer_Init(Buffer_t* buffer, uint8_t* dataBuffer, uint32_t maxSize)
{
	buffer->buffer  = dataBuffer;
	buffer->maxSize = maxSize;
	buffer->front   = 0;
	buffer->rear    = 0;
	memset(dataBuffer,0,maxSize);
	dataBuffer[0]=' ';
}

//////////////////////////////
///@breif put multiple node data to queue 
///@param data: the first node data adress of node data array will put into queue
///@param length: the length of node data that will put into queue
/////////////////////////////
bool Buffer_Puts(Buffer_t* buffer, uint8_t* data, uint16_t length)
{
	if (buffer->maxSize - Buffer_Size(buffer) <= length)//队满
		return false;
	for (uint16_t i = 0; i<length; ++i)
	{
		buffer->rear = (buffer->rear + 1) % buffer->maxSize;
		buffer->buffer[buffer->rear] = data[i];
	}
    
	return true;
}


//////////////////////////////
///@breif get multiple node data from queue
///@param data: the first node data adress of node data array will get from the queue
///@param length: the length of node data that will get from the queue
/////////////////////////////
bool Buffer_Gets(Buffer_t* buffer, uint8_t *data, uint16_t length)
{
	if (Buffer_Size(buffer)<length)
		return false;

	for (uint16_t i = 0; i<length; ++i)
	{
		buffer->front = (buffer->front + 1) % buffer->maxSize;
		data[i] = buffer->buffer[buffer->front];
	}
	return true;
}


/**
 * query some data from the queue, and the data remain be a part of the queue
 * @param data the address of the data save to
 * @param startPosition Index of where to start query
 * #param num The number of data will be query
 * @retval Is query succeed
 */
int32_t Buffer_Query(Buffer_t* buffer, uint8_t* data, uint16_t length, uint16_t startPosition)
{
	uint32_t size = (buffer->rear - startPosition + 1 + buffer->maxSize) % buffer->maxSize;
	uint32_t index = startPosition;
	uint16_t indexData = 0;
	int32_t indexReturn = -1;

	while (size)
	{
		if (buffer->buffer[index] == data[indexData])
		{
			if (indexData == 0)
				indexReturn = index;
			++indexData;
			if (indexData == length)//find success
			{
				return indexReturn;
			}
		}
		else
		{
			indexData = 0;
			indexReturn = -1;
			if (buffer->buffer[index] == data[indexData])
			{
				if (indexData == 0)
					indexReturn = index;
				++indexData;
				if (indexData == length)//find success
				{
					return indexReturn;
				}
			}
		}
		index = (index + 1) % buffer->maxSize;
		--size;
	}

	return -1;
}

////////////////////////////
///@brief get the size of queue
///@retval the size of queue
///////////////////////////
uint32_t Buffer_Size(Buffer_t* buffer)
{
	return (buffer->rear - buffer->front + buffer->maxSize) % buffer->maxSize;
}

////////////////////////////
///@brief get size sigh specific end position
///
////////////////////////////

uint32_t Buffer_Size2(Buffer_t* buffer,uint32_t index)
{
	return (index - buffer->front + buffer->maxSize) % buffer->maxSize;
}


////////////////////////////////
///@breif clear the queue
////////////////////////////////
void Buffer_Clear(Buffer_t* buffer)
{
	buffer->front = 0;
	buffer->rear = 0;
}


int32_t Buffer_StartPostion(Buffer_t* buffer)
{
	return (buffer->front + 1)%buffer->maxSize;
}


