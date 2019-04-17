#include <stdlib.h>
#include "wav_decode.h"
#include <string.h>
#include <stdio.h>


/* Audio Parsing Constants */
#define RIFF_ID 0x52494646 /* correspond to the letters 'RIFF' */
#define WAVE_ID 0x57415645 /* correspond to the letters 'WAVE' */
#define FMT_ID 0x666D7420  /* correspond to the letters 'fmt ' */
#define LIST_ID 0x4C495354 /* correspond to the letters 'LIST' */
#define DATA_ID 0x64617461 /* correspond to the letters 'data' */

#define BG_READ_WORD(x) ((((uint32_t)wav_head_buff[x + 0]) << 24) | (((uint32_t)wav_head_buff[x + 1]) << 16) | \
						 (((uint32_t)wav_head_buff[x + 2]) << 8) | (((uint32_t)wav_head_buff[x + 3]) << 0))

#define LG_READ_WORD(x) ((((uint32_t)wav_head_buff[x + 3]) << 24) | (((uint32_t)wav_head_buff[x + 2]) << 16) | \
						 (((uint32_t)wav_head_buff[x + 1]) << 8) | (((uint32_t)wav_head_buff[x + 0]) << 0))

#define LG_READ_HALF(x) ((((uint16_t)wav_head_buff[x + 1]) << 8) | (((uint16_t)wav_head_buff[x + 0]) << 0))

wav_err_t wav_init(wav_decode_t *wav_obj,void* head, uint32_t file_size, uint32_t* head_len)
{
	uint8_t* wav_head_buff = (uint8_t*)head;
	uint32_t index;

	index = 0;
	if (BG_READ_WORD(index) != RIFF_ID) //Chunk ID 
		return UNVALID_RIFF_ID;

	index += 4;

	if ((LG_READ_WORD(index) + 8) != file_size)//Chunk Size modify
		return UNVALID_RIFF_SIZE;

	index += 4;
	if (BG_READ_WORD(index) != WAVE_ID)//Formate
		return UNVALID_WAVE_ID;

	index += 4;
	if (BG_READ_WORD(index) != FMT_ID)//sub chunk1 id
		return UNVALID_FMT_ID;

	index += 4;
	if (LG_READ_WORD(index) != 0x10)//sub chunk1 size maybe can pass 
	{
		return UNVALID_FMT_SIZE;
	}

	index += 4;
	if (LG_READ_HALF(index) != 0x01)//audio Format only support PCM
		return UNSUPPORETD_FORMATTAG;

	index += 2;
	wav_obj->numchannels = LG_READ_HALF(index);//Num Channel
	if (wav_obj->numchannels != 1 && wav_obj->numchannels != 2)
		return UNSUPPORETD_NUMBER_OF_CHANNEL;

	index += 2;
	wav_obj->samplerate = LG_READ_WORD(index);//sample rate

	index += 4;
	wav_obj->byterate = LG_READ_WORD(index);//bytearte
	index += 4;
	wav_obj->blockalign = LG_READ_HALF(index);//block align
	index += 2;
	wav_obj->bitspersample = LG_READ_HALF(index);//bits_per_sample
	if (wav_obj->bitspersample != 8 && wav_obj->bitspersample != 16 && wav_obj->bitspersample != 24)
		return UNSUPPORETD_BITS_PER_SAMPLE;

	index += 2;
	if (BG_READ_WORD(index) == LIST_ID)//extend format block
	{
		index += 4;
		index += LG_READ_WORD(index);
		index += 4;
		if (index >= 500)
			return UNVALID_LIST_SIZE;
	}

	if (BG_READ_WORD(index) != DATA_ID)// "data"
		return UNVALID_DATA_ID;

	index += 4;
	wav_obj->datasize = LG_READ_WORD(index);//data size
	index += 4;

	// wav_obj->wave_file_curpos = index;//curpos
	*head_len = index;
	return OK;
}
































// enum errorcode_e wav_decode_init(struct wav_obj_t *wav_obj)
// {
// 	uint32_t tmp;

// 	wav_obj->buff_end = 0;
// 	wav_obj->buff0_len = 256 * 1024;
// 	wav_obj->buff1_len = 256 * 1024;
// 	wav_obj->buff0 = (uint8_t *)malloc(wav_obj->buff0_len);
// 	wav_obj->buff1 = (uint8_t *)malloc(wav_obj->buff1_len);

// 	tmp = wav_obj->wave_file_len - wav_obj->wave_file_curpos;
// 	wav_obj->buff0_read_len = wav_obj->buff0_len > tmp ? tmp : wav_obj->buff0_len;
// 	memcpy(wav_obj->buff0, (wav_obj->wave_file + wav_obj->wave_file_curpos), wav_obj->buff0_read_len);
// 	wav_obj->wave_file_curpos += wav_obj->buff0_read_len;

// 	if (wav_obj->buff0_len > wav_obj->buff0_read_len)
// 	{
// 		wav_obj->buff_end = 1;
// 		mp_printf(&mp_plat_print, "%s buf_end=1\r\n", __func__);
// 	}

// 	wav_obj->buff_current = wav_obj->buff0;
// 	wav_obj->buff_current_len = wav_obj->buff0_len;
// 	wav_obj->buff0_used = 1;
// 	wav_obj->buff1_used = 0;
// 	wav_obj->buff_index = 0;
// 	return OK;
// }

// enum errorcode_e wav_decode(struct wav_obj_t *wav_obj)
// {
// 	uint32_t tmp;

// 	if (wav_obj->buff0_used == 0)
// 	{
// 		tmp = wav_obj->wave_file_len - wav_obj->wave_file_curpos;
// 		wav_obj->buff0_read_len = (wav_obj->buff0_len > tmp) ? tmp : wav_obj->buff0_len;
// 		memcpy(wav_obj->buff0, wav_obj->wave_file + wav_obj->wave_file_curpos, wav_obj->buff0_read_len);
// 		wav_obj->wave_file_curpos += wav_obj->buff0_read_len;
// 		wav_obj->buff0_used = 1;
// 		if (wav_obj->buff0_len > wav_obj->buff0_read_len)
// 		{
// 			if (wav_obj->wave_file_curpos == wav_obj->wave_file_len)
// 				return FILE_END;
// 			return FILE_FAIL;
// 		}
// 	}
// 	else if (wav_obj->buff1_used == 0)
// 	{
// 		tmp = wav_obj->wave_file_len - wav_obj->wave_file_curpos;
// 		wav_obj->buff1_read_len = (wav_obj->buff1_len > tmp) ? tmp : wav_obj->buff1_len;
// 		memcpy(wav_obj->buff1, wav_obj->wave_file + wav_obj->wave_file_curpos, wav_obj->buff1_read_len);
// 		wav_obj->wave_file_curpos += wav_obj->buff0_read_len;
// 		wav_obj->buff1_used = 1;
// 		if (wav_obj->buff1_len > wav_obj->buff1_read_len)
// 		{
// 			if (wav_obj->wave_file_curpos == wav_obj->wave_file_len)
// 				return FILE_END;
// 			return FILE_FAIL;
// 		}
// 	}
// 	return OK;
// }

// enum errorcode_e wav_decode_finish(struct wav_obj_t *wav_obj)
// {
// 	free(wav_obj->buff0);
// 	free(wav_obj->buff1);
// 	return OK;
// }
