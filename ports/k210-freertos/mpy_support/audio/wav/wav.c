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
#include "dmac.h"

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

static int on_irq_audio_trans(void *ctx)
{
	audio_t* audio_obj =  (audio_t*)ctx;
	Maix_i2s_obj_t* i2s_dev = audio_obj->dev;
	wav_decode_t* wav_play_obj = audio_obj->play_obj;
    // printk("[MAIXPY]: wav_play_obj->play_order %d ok\r\n",wav_play_obj->play_order);
    wav_play_obj->audio_buf[wav_play_obj->play_order].empty = true;
	wav_play_obj->play_order++;
    if(wav_play_obj->play_order > MAX_BUF_NUM - 1)
        wav_play_obj->play_order = 0;
    return 0;
}

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

mp_obj_t wav_play_process(audio_t* audio,uint32_t file_size)
{
	uint32_t head_len = 0;
	uint32_t err_code = 0;
	uint32_t close_code = 0;
	mp_obj_list_t* ret_list = (mp_obj_list_t*)m_new(mp_obj_list_t,sizeof(mp_obj_list_t));//m_new
	audio->play_obj = m_new(wav_decode_t,1);//new format obj
    mp_obj_list_init(ret_list, 0);
	if(NULL == audio->play_obj)
	{
		printf("[MAIXPY]: Can not create decode object\n");
		m_del(mp_obj_list_t,ret_list,1);
		m_del(wav_decode_t,audio->play_obj,1);
		vfs_internal_close(audio->fp,&close_code);
	}
	uint32_t read_num = vfs_internal_read(audio->fp,audio->buf,500,&err_code);//read head
	if(err_code != 0)
	{
		printf("[MAIXPY]: read head error close file\n");
		m_del(mp_obj_list_t,ret_list,1);
		m_del(wav_decode_t,audio->play_obj,1);
		vfs_internal_close(audio->fp,&close_code);
		mp_raise_OSError(err_code);
	}
	wav_err_t status = wav_init(audio->play_obj,audio->buf,file_size,&head_len);//wav init
	//debug
	if(status != OK)
	{
		printf("[MAIXPY]: wav error code : %d\n",status);
		m_del(mp_obj_list_t,ret_list,1);
		m_del(wav_decode_t,audio->play_obj,1);
		vfs_internal_close(audio->fp,&close_code);
		mp_raise_msg(&mp_type_OSError,"wav init error");
	}
	wav_decode_t* wav_fmt = audio->play_obj;
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
		m_del(wav_decode_t,audio->play_obj,1);
		vfs_internal_close(audio->fp,&close_code);
		mp_raise_OSError(err_code);
	}
	memset(audio->buf, audio->points * sizeof(uint32_t), 0);//clear buffer
#define WAV_BUF_SIZE (4*1024)
	wav_decode_t* wav_play_obj = audio->play_obj;
	for(int i = 0; i < MAX_BUF_NUM; i++)//init wav buf
	{
		wav_play_obj->audio_buf[i].buf = malloc(WAV_BUF_SIZE);
		wav_play_obj->audio_buf[i].len = WAV_BUF_SIZE;
		wav_play_obj->audio_buf[i].empty = true;
	}
	wav_play_obj->play_order = 0;
	wav_play_obj->read_order = 0;
	dmac_set_irq(WAV_DMA_CHANNEL, on_irq_audio_trans, (void*)audio, 1);
	return MP_OBJ_FROM_PTR(ret_list);
}

mp_obj_t wav_play(audio_t* audio)
{
	wav_decode_t* play_obj = audio->play_obj; //get format
	wav_decode_t* wav_play_obj = play_obj;
	Maix_i2s_obj_t* i2s_dev = audio->dev;//get device
	uint32_t read_num = 0;
	uint32_t err_code = 0;
	if(play_obj->audio_buf[play_obj->read_order].empty)//empty ,altread to read
	{
		// printf("[MAIXPY]: read_order = %d\n",play_obj->read_order);
		read_num = vfs_internal_read(audio->fp, 
									 play_obj->audio_buf[play_obj->read_order].buf, 
									 audio->points * sizeof(uint32_t), 
									 &err_code);//read data
		play_obj->audio_buf[play_obj->read_order].len = read_num;
		play_obj->audio_buf[play_obj->read_order].empty = false;
		int* audio_buf = play_obj->audio_buf[play_obj->read_order].buf;
		short MSB_audio = 0;
		short LSB_audio = 0;
		uint32_t vol_coe = 2;
		for(int i = 0; i < read_num / sizeof(uint32_t); i++)//Currently only supports two-channel wav files
		{
			LSB_audio = audio_buf[i];
			LSB_audio = LSB_audio / vol_coe;//The volume can be adjusted by adjusting vol_coe, TODO: package the interface for adjusting the volume
			MSB_audio = audio_buf[i] >> 16;
			MSB_audio = MSB_audio / vol_coe;
			audio_buf[i] = ( MSB_audio << 16 ) | LSB_audio;
		}
		play_obj->read_order++;
		if(play_obj->read_order > MAX_BUF_NUM - 1)
        	play_obj->read_order = 0;
	}
	if(!wav_play_obj->audio_buf[wav_play_obj->play_order].empty)//not empty ,already to play
	{
		// printf("[MAIXPY]: play_order = %d\n",wav_play_obj->play_order);
		i2s_play(i2s_dev->i2s_num,
					WAV_DMA_CHANNEL,
					wav_play_obj->audio_buf[wav_play_obj->play_order].buf,
					wav_play_obj->audio_buf[wav_play_obj->play_order].len,
					wav_play_obj->audio_buf[wav_play_obj->play_order].len / sizeof(uint32_t),
					wav_play_obj->bitspersample,
					wav_play_obj->numchannels);//play readed data
	}
	return mp_const_true;
}

mp_obj_t wav_record_process(audio_t* audio,uint32_t channels)
{
	uint32_t head_len = 0;
    uint32_t err_code = 0;
    uint32_t close_code = 0;
	Maix_i2s_obj_t* i2s_dev = audio->dev;
    for(int i = 0; i < 4; i++){
        if(I2S_RECEIVER == i2s_dev->channel[i].mode){//find the received channel
            audio->align_mode = i2s_dev->channel[i].align_mode;//get align mode
            break;
        }
    }
	//create encode object
	audio->record_obj = m_new(wav_encode_t,1);//new format obj
	if(NULL == audio->record_obj)
	{
		printf("[MAIXPY]: Can not create encode object\n");
		m_del(wav_encode_t,audio->record_obj,1);
		vfs_internal_close(audio->fp,&close_code);
	}
	//file chunk
	wav_encode_t* wav_encode = audio->record_obj;
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
	wav_fmt->bitspersample = 16;//only support 16bit resolution
	// for(int i = 0; i < 4; i++)//find the received channel
	// {
	// 	if(I2S_RECEIVER == i2s_dev->channel[i].mode)//find the first received channel
	// 	{
	// 		if(0 != i2s_dev->channel[i].resolution)//set resolution
	// 			wav_fmt->bitspersample = (i2s_dev->channel[i].resolution + 2) * 4;
	// 		if(RESOLUTION_32_BIT == i2s_dev->channel[i].resolution)
	// 			wav_fmt->bitspersample = 32;
	// 		break;
	// 	}
	// }
	//data chunk
	wav_encode->data.data_ID = 0x61746164;//'data'
	wav_encode->data.chunk_size = 0; 
    wav_encode->data.wave_data = audio->buf;
	vfs_internal_seek(audio->fp,44,VFS_SEEK_SET,&err_code);//head length 44
	if(err_code != 0)
	{
		printf("[MAIXPY]: seek error  close file\n");
		m_del(wav_encode_t,audio->record_obj,1);
		vfs_internal_close(audio->fp,&close_code);
		mp_raise_OSError(err_code);
	}
	memset(audio->buf, audio->points * sizeof(uint32_t), 0);//clear buffer
	return mp_const_none;
}
mp_obj_t wav_record(audio_t* audio,dmac_channel_number_t DMA_channel)
{
	Maix_i2s_obj_t* i2s_dev = audio->dev;//get device
	i2s_receive_data_dma(i2s_dev->i2s_num, audio->buf, audio->points , DMA_channel);
	dmac_wait_idle(DMA_channel);	
	wav_process_data(audio);
	return mp_const_true;
}
int wav_process_data(audio_t* audio)//GO righit channel record, right chnanel play
{
	wav_encode_t* wav_encode = audio->record_obj;
	uint32_t err_code = 0;

	// for(int i = 0; i < audio->points; i++){
	// 	printf("data[%d] : LSB = %x | MSB = %x\n",i, audio->buf[i] & 0xffff, (audio->buf[i] >> 16) & 0xffff);
	// }
	if(1 == wav_encode->format.numchannels){//mono audio record | Go mic right 

		uint16_t* buf = (uint16_t*)malloc(audio->points * sizeof(uint32_t));//
		int j = 0;
		for(int i = 0; i < audio->points; i += 1){
			buf[i*2] = 0;//left channel
			buf[i*2+1] = audio->buf[i] & 0xffff;//right channle 16 bit resolution
			// buf[i*2+1] = (audio->buf[i] >> 8) & 0xffff;//24 bit resolution
		}
		vfs_internal_write(audio->fp,buf,audio->points * sizeof(uint32_t),&err_code);
		wav_encode->data.chunk_size +=  audio->points * sizeof(uint32_t);
		free(buf);
	}
	else if(2 == wav_encode->format.numchannels){//stereo audio record
		//TODO
	}

	return 0;
}
void wav_finish(audio_t* audio)
{
	uint32_t err_code = 0;
    uint32_t close_code = 0;
	if(audio->play_obj != NULL)
	{
		m_del(wav_decode_t,audio->play_obj,1);
		audio->play_obj = NULL;
	}
	if(audio->record_obj != NULL)
	{
		//write head data
		vfs_internal_seek(audio->fp,0,VFS_SEEK_SET,&err_code);//
		wav_encode_t* wav_encode = audio->record_obj;
		wav_encode->file.file_size = 44 - 8 + wav_encode->data.chunk_size;
		vfs_internal_write(audio->fp, &wav_encode->file, 12, &err_code);//write file chunk
		if(err_code != 0)
		{
			printf("[MAIXPY]: write file chunk error  close file\n");
			m_del(wav_encode_t,audio->record_obj,1);
			vfs_internal_close(audio->fp,&close_code);
			mp_raise_OSError(err_code);
		}
		format_chunk_t* wav_fmt = &wav_encode->format;
		wav_fmt->numchannels = 2;//always is 2,because i2s_play only play 2-channels audoi
		wav_fmt->blockalign = wav_fmt->numchannels * (wav_fmt->bitspersample/8);// channel * (bit_per_second / 8)
		wav_fmt->byterate = wav_fmt->samplerate * wav_fmt->blockalign; // samplerate * blockalign
		printf("[MAIXPY]: numchannels = %d\n", wav_fmt->numchannels);
		printf("[MAIXPY]: samplerate = %d\n", wav_fmt->samplerate);
		printf("[MAIXPY]: byterate = %d\n", wav_fmt->byterate);
		printf("[MAIXPY]: blockalign = %d\n", wav_fmt->blockalign);
		printf("[MAIXPY]: bitspersample = %d\n", wav_fmt->bitspersample);
		vfs_internal_write(audio->fp, &wav_encode->format, 24, &err_code);//write fromate chunk
		if(err_code != 0)
		{
			printf("[MAIXPY]: write formate chunk error  close file\n");
			m_del(wav_encode_t,audio->record_obj,1);
			vfs_internal_close(audio->fp,&close_code);
			mp_raise_OSError(err_code);
		}
		vfs_internal_write(audio->fp, &wav_encode->data, 8 ,&err_code);//write data chunk
		if(err_code != 0)
		{
			printf("[MAIXPY]: write data chunk error  close file\n");
			m_del(wav_encode_t,audio->record_obj,1);
			vfs_internal_close(audio->fp,&close_code);
			mp_raise_OSError(err_code);
		}
		m_del(wav_encode_t,audio->record_obj,1);
		audio->record_obj = NULL;
	}
	vfs_internal_close(audio->fp,&close_code);
}