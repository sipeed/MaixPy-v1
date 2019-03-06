#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "py/objarray.h"
#include "py/binary.h"
#include "py_audio.h"
#include "mphalport.h"

#include "vfs_internal.h"
#include "Maix_i2s.h"
#include "modMaix.h"

#include "wav.h"
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
		printk("[MAIXPY]: chun1 size = %d\r\n",LG_READ_WORD(index));
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

mp_obj_t wav_play_process(audio_t* audio)
{
	uint32_t head_len = 0;
	uint32_t err_code = 0;
	uint32_t close_code = 0;
	mp_obj_list_t* ret_list = (mp_obj_list_t*)m_new(mp_obj_list_t,sizeof(mp_obj_list_t));//m_new
	audio->decode_obj = m_new(wav_decode_t,1);//new format obj
    mp_obj_list_init(ret_list, 0);
	if(NULL == audio->decode_obj)
	{
		printf("[MAIXPY]: Can not create decode object\n");
		m_del(mp_obj_list_t,ret_list,1);
		m_del(wav_decode_t,audio->decode_obj,1);
		vfs_internal_close(audio->fp,&close_code);
	}
	uint32_t read_num = vfs_internal_read(audio->fp,audio->buf,500,&err_code);//read head
	if(err_code != 0)
	{
		printf("[MAIXPY]: read head error close file\n");
		m_del(mp_obj_list_t,ret_list,1);
		m_del(wav_decode_t,audio->decode_obj,1);
		vfs_internal_close(audio->fp,&close_code);
		mp_raise_OSError(err_code);
	}
	wav_err_t status = wav_init(audio->decode_obj,audio->buf,file_size,&head_len);//wav init
	//debug
	if(status != OK)
	{
		printf("[MAIXPY]: wav error code : %d\n",status);
		m_del(mp_obj_list_t,ret_list,1);
		m_del(wav_decode_t,audio->decode_obj,1);
		vfs_internal_close(audio->fp,&close_code);
		mp_raise_msg(&mp_type_OSError,"wav init error");
	}
	wav_decode_t* wav_fmt = audio->decode_obj;
	printf("[MAIXPY]: result = %d\n", status);
	printf("[MAIXPY]: numchannels = %d\n", wav_fmt->numchannels);
	printf("[MAIXPY]: samplerate = %d\n", wav_fmt->samplerate);
	printf("[MAIXPY]: byterate = %d\n", wav_fmt->byterate);
	printf("[MAIXPY]: blockalign = %d\n", wav_fmt->blockalign);
	printf("[MAIXPY]: bitspersample = %d\n", wav_fmt->bitspersample);
	printf("[MAIXPY]: datasize = %d\n", wav_fmt->datasize);
	// printf("[MAIXPY]: head_len = %d\n", head_len);
	mp_obj_list_append(ret_list, mp_obj_new_int(wav_fmt->numchannels));
	mp_obj_list_append(ret_list, mp_obj_new_int(wav_fmt->samplerate));
	mp_obj_list_append(ret_list, mp_obj_new_int(wav_fmt->byterate));
	mp_obj_list_append(ret_list, mp_obj_new_int(wav_fmt->blockalign));
	mp_obj_list_append(ret_list, mp_obj_new_int(wav_fmt->bitspersample));
	mp_obj_list_append(ret_list, mp_obj_new_int(wav_fmt->datasize));
	vfs_internal_seek(audio->fp,head_len,VFS_SEEK_SET,err_code);
	if(err_code != 0)
	{
		printf("[MAIXPY]: seek error  close file\n");
		m_del(mp_obj_list_t,ret_list,1);
		m_del(wav_decode_t,audio->decode_obj,1);
		vfs_internal_close(audio->fp,&close_code);
		mp_raise_OSError(err_code);
	}
	memset(audio->buf, audio->points * sizeof(uint32_t), 0);//clear buffer
	return MP_OBJ_FROM_PTR(ret_list);
}

mp_obj_t wav_play(audio_t* audio)
{
	wav_decode_t* decode_obj = audio->decode_obj; //get format
	Maix_i2s_obj_t* i2s_dev = audio->dev;//get device
	uint32_t read_num = 0;
	uint32_t play_points = 0;//play points number
	uint32_t err_code = 0;
	read_num = vfs_internal_read(audio->fp,audio->buf,audio->points * sizeof(uint32_t), &err_code);//read data
	if(read_num % 4 != 0) // read_num must be multiple of 4
		read_num = read_num - read_num%4;
	if(read_num == 0)
		return mp_const_none;
	play_points = read_num / sizeof(uint32_t); 
	i2s_play(i2s_dev->i2s_num,
				DMAC_CHANNEL5,
				audio->buf,
				read_num,
				play_points,
				decode_obj->bitspersample,
				decode_obj->numchannels);//play readed data  
	return mp_const_true;
}

mp_obj_t wav_record_process(audio_t* audio,uint32_t channels)
{
	uint32_t head_len = 0;
    uint32_t err_code = 0;
    uint32_t close_code = 0;
	Maix_i2s_obj_t* i2s_dev = audio->dev;
    mp_obj_list_t* ret_list = (mp_obj_list_t*)m_new(mp_obj_list_t,sizeof(mp_obj_list_t));//m_new
    mp_obj_list_init(ret_list, 0);
	//创建编码实例
	audio->encode_obj = m_new(wav_encode_t,1);//new format obj
	if(NULL == audio->encode_obj)
	{
		printf("[MAIXPY]: Can not create encode object\n");
		m_del(mp_obj_list_t,ret_list,1);
		m_del(wav_encode_t,audio->encode_obj,1);
		vfs_internal_close(audio->fp,&close_code);
	}
	//file chunk
	wav_encode_t* wav_encode = audio->encode_obj;
	wav_encode->file.riff_id = 0x46464952;//'riff'
	wav_encode->file.file_size = 0;
	wav_encode->file.wave_id = 0x45564157;//'wave'
	//formate chunk
	format_chunk_t* wav_fmt = &wav_encode->format;
	wav_fmt->fmt_ID = 0x20746D66;//'fmt '
	wav_fmt->chunk_size = 16;
	wav_fmt->format_tag = 1;
	wav_fmt->numchannels = channels;
	wav_fmt->samplerate = i2s_dev->sample_rate;
	wav_fmt->bitspersample = 0;
	for(int i = 0; i < 4; i++)
	{
		if(I2S_RECEIVER == i2s_dev->channel[i].mode)//find the received channel
		{
			if(0 != i2s_dev->channel[i].resolution)//set resolution
				wav_fmt->bitspersample = (i2s_dev->channel[i].resolution + 2) * 4;
			if(RESOLUTION_32_BIT == i2s_dev->channel[i].resolution)
				wav_fmt->bitspersample = 32;
			break;
		}
	}
	wav_fmt->blockalign = wav_fmt->numchannels * (wav_fmt->bitspersample/8);
	wav_fmt->byterate = wav_encode->format.samplerate * wav_encode->format.blockalign; // samplerate * blockalign
	//data chunk
	wav_encode->data.data_ID = 0x61746164;//'data'
	wav_encode->data.chunk_size = 0; 
	wav_encode->data.wave_data = audio->buf;
	printf("[MAIXPY]: numchannels = %d\n", wav_fmt->numchannels);
	printf("[MAIXPY]: samplerate = %d\n", wav_fmt->samplerate);
	printf("[MAIXPY]: byterate = %d\n", wav_fmt->byterate);
	printf("[MAIXPY]: blockalign = %d\n", wav_fmt->blockalign);
	printf("[MAIXPY]: bitspersample = %d\n", wav_fmt->bitspersample);
	mp_obj_list_append(ret_list, mp_obj_new_int(wav_fmt->numchannels));
	mp_obj_list_append(ret_list, mp_obj_new_int(wav_fmt->samplerate));
	mp_obj_list_append(ret_list, mp_obj_new_int(wav_fmt->byterate));
	mp_obj_list_append(ret_list, mp_obj_new_int(wav_fmt->blockalign));
	mp_obj_list_append(ret_list, mp_obj_new_int(wav_fmt->bitspersample));
	vfs_internal_seek(audio->fp,44,VFS_SEEK_SET,&err_code);//head length 44
	if(err_code != 0)
	{
		printf("[MAIXPY]: seek error  close file\n");
		m_del(mp_obj_list_t,ret_list,1);
		m_del(wav_encode_t,audio->encode_obj,1);
		vfs_internal_close(audio->fp,&close_code);
		mp_raise_OSError(err_code);
	}
	memset(audio->buf, audio->points * sizeof(uint32_t), 0);//clear buffer
	return MP_OBJ_FROM_PTR(ret_list);
}
mp_obj_t wav_record(audio_t* audio,dmac_channel_number_t DMA_channel)
{
	Maix_i2s_obj_t* i2s_dev = audio->dev;//get device
	i2s_receive_data_dma(i2s_dev->i2s_num, audio->buf, audio->points , DMAC_CHANNEL5);
    i2s_work_mode_t align_mode = 0;
    for(int i = 0; i < 4; i++){
        if(I2S_RECEIVER == i2s_dev->channel[i].mode){//find the received channel
            align_mode = i2s_dev->channel[i].align_mode;//get align mode
            break;
        }
    }
	wav_encode_t* wav_encode = audio->encode_obj;//get format
	format_chunk_t* wav_fmt = &wav_encode->format;
	dmac_wait_idle(DMA_channel);
	wav_encode->data.chunk_size +=  audio->points * sizeof(uint32_t);
	if(align_mode == RIGHT_JUSTIFYING_MODE)//only support right justifying
		wav_right_justifying_record(audio->buf,wav_fmt->numchannels)
	return mp_const_true;
}
int wav_right_justifying_record(audio_t* audio)
{
	if(1 == numchannels){//mono audio
		for(int i = 0; i < audio->points; i++){
			printf("LSB = %x\n",buf[i] & 0xff);
			printf("MSB = %x\n",buf[i] >> 16 & 0xff);
		}
	}
	else if(2 == wav_fmt->numchannels){
		//TODO
	}//stereo audio

	return 0;
}