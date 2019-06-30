
#include "avi.h"
#include "stdio.h"
#include "video.h"


uint8_t* const AVI_VIDS_FLAG_TBL[2]={(uint8_t*)"00dc",(uint8_t*)"01dc"};
uint8_t* const AVI_AUDS_FLAG_TBL[2]={(uint8_t*)"00wb",(uint8_t*)"01wb"};

int avi_init(uint8_t* buf, uint32_t size, avi_t* avi)
{
	
    uint32_t offset;
    uint8_t* buf_start = buf;
	avi_status_t res=AVI_STATUS_OK;
	avi_header_t* header;
	list_header_t* list_header;
	avih_header_t* avih_header; 
	strh_header_t* strh_header; 
	
	strf_bmp_header_t* bmp_header; 
	strf_wav_header_t* wav_header; 
	
	header=(avi_header_t*)buf; 
	if(header->riff_id!=AVI_RIFF_ID)return AVI_STATUS_ERR_RIFF;
	if(header->avi_id!=AVI_AVI_ID)return AVI_STATUS_ERR_AVI;
	buf+=sizeof(avi_header_t);

	list_header=(list_header_t*)(buf);						
	if(list_header->list_id!=AVI_LIST_ID)return AVI_STATUS_ERR_LIST;
	if(list_header->list_type!=AVI_HDRL_ID)return AVI_STATUS_ERR_HDRL;
	buf+=sizeof(list_header_t);	

	avih_header=(avih_header_t*)(buf);
	if(avih_header->block_id!=AVI_AVIH_ID)return AVI_STATUS_ERR_AVIH;
	avi->usec_per_frame=avih_header->usec_per_frame;
	avi->max_byte_sec=avih_header->max_byte_sec;
	avi->total_frame=avih_header->total_frame;
	buf+=avih_header->block_size+8;

	list_header=(list_header_t*)(buf);
	if(list_header->list_id!=AVI_LIST_ID)return AVI_STATUS_ERR_LIST;
	if(list_header->list_type!=AVI_STRL_ID)return AVI_STATUS_ERR_STRL;

	strh_header=(strh_header_t*)(buf+12);
	if(strh_header->block_id!=AVI_STRH_ID)return AVI_STATUS_ERR_STRH;
 	if(strh_header->stream_type==AVI_VIDS_STREAM)
	{
		if(strh_header->handler!=AVI_FORMAT_MJPG)return AVI_STATUS_ERR_FORMAT;
		avi->video_flag=(uint8_t*)AVI_VIDS_FLAG_TBL[0];
		avi->audio_flag=(uint8_t*)AVI_AUDS_FLAG_TBL[1];
		bmp_header=(strf_bmp_header_t*)(buf+12+strh_header->block_size+8);
		if(bmp_header->block_id!=AVI_STRF_ID)return AVI_STATUS_ERR_STRF;
		avi->width=bmp_header->bmi_header.width;
		avi->height=bmp_header->bmi_header.height; 
		buf+=list_header->block_size+8;
		list_header=(list_header_t*)(buf);
		if(list_header->list_id!=AVI_LIST_ID)
		{
			avi->audio_sample_rate=0;
			avi->audio_channels=0;
			avi->audio_format=0;
			
		}else
		{			
			if(list_header->list_type!=AVI_STRL_ID)return AVI_STATUS_ERR_STRL;
			strh_header=(strh_header_t*)(buf+12);
			if(strh_header->block_id!=AVI_STRH_ID)return AVI_STATUS_ERR_STRH;
			if(strh_header->stream_type!=AVI_AUDS_STREAM)return AVI_STATUS_ERR_FORMAT;
			wav_header=(strf_wav_header_t*)(buf+12+strh_header->block_size+8);
			if(wav_header->block_id!=AVI_STRF_ID)return AVI_STATUS_ERR_STRF;
			avi->audio_sample_rate=wav_header->sample_rate;
			avi->audio_channels=wav_header->channels;
			avi->audio_format=wav_header->format_tag;
		}
	}else if(strh_header->stream_type==AVI_AUDS_STREAM)
	{ 
		avi->video_flag=(uint8_t*)AVI_VIDS_FLAG_TBL[1];
		avi->audio_flag=(uint8_t*)AVI_AUDS_FLAG_TBL[0];
		wav_header=(strf_wav_header_t*)(buf+12+strh_header->block_size+8);
		if(wav_header->block_id!=AVI_STRF_ID)return AVI_STATUS_ERR_STRF;
		avi->audio_sample_rate=wav_header->sample_rate;
		avi->audio_channels=wav_header->channels;
		avi->audio_format=wav_header->format_tag;
		buf+=list_header->block_size+8;
		list_header=(list_header_t*)(buf);
		if(list_header->list_id!=AVI_LIST_ID)return AVI_STATUS_ERR_LIST;
		if(list_header->list_type!=AVI_STRL_ID)return AVI_STATUS_ERR_STRL;
		strh_header=(strh_header_t*)(buf+12);
		if(strh_header->block_id!=AVI_STRH_ID)return AVI_STATUS_ERR_STRH;
		if(strh_header->stream_type!=AVI_VIDS_STREAM)return AVI_STATUS_ERR_FORMAT;
		bmp_header=(strf_bmp_header_t*)(buf+12+strh_header->block_size+8);
		if(bmp_header->block_id!=AVI_STRF_ID)return AVI_STATUS_ERR_STRF;
		if(bmp_header->bmi_header.compression!=AVI_FORMAT_MJPG)return AVI_STATUS_ERR_FORMAT;
		avi->width=bmp_header->bmi_header.width;
		avi->height=bmp_header->bmi_header.height; 	
	}
	offset=avi_srarch_id(buf_start,size,(uint8_t*)"movi");
	if(offset==0)
        return AVI_STATUS_ERR_MOVI;
	avi->offset_movi = offset;
	if(avi->audio_sample_rate)
	{
		buf = buf_start + offset;
		offset=avi_srarch_id(buf+4, size - offset-4, avi->audio_flag);
		if(offset==0)
			return AVI_STATUS_ERR_STREAM;
		buf+=offset+4;
		avi->audio_buf_size = buf[0]|buf[1]<<8|buf[2]<<16|buf[3]<<24;
	}		
	return res;
}

#ifdef VIDEO_DEBUG
void avi_debug_info(avi_t* avi)
{
    mp_printf(&mp_plat_print, "avi->SecPerFrame:%d\r\n",avi->usec_per_frame);
	mp_printf(&mp_plat_print, "avi->TotalFrame:%d\r\n",avi->total_frame);
	mp_printf(&mp_plat_print, "avi->Width:%d\r\n",avi->width);
	mp_printf(&mp_plat_print, "avi->Height:%d\r\n",avi->height);
	mp_printf(&mp_plat_print, "avi->AudioType:%d\r\n",avi->audio_format);
	mp_printf(&mp_plat_print, "avi->SampleRate:%d\r\n",avi->audio_sample_rate);
	mp_printf(&mp_plat_print, "avi->Channels:%d\r\n",avi->audio_channels);
	mp_printf(&mp_plat_print, "avi->AudioBufSize:%d\r\n",avi->audio_buf_size);
	mp_printf(&mp_plat_print, "avi->VideoFLAG:%s\r\n",avi->video_flag); 
	mp_printf(&mp_plat_print, "avi->AudioFLAG:%s\r\n",avi->audio_flag); 

    mp_printf(&mp_plat_print, "\nfps:%.2f\n", 1000.0/(avi->usec_per_frame/1000.0));
    mp_printf(&mp_plat_print, "audio channels:%d\n", avi->audio_channels);
    mp_printf(&mp_plat_print, "audio sample rate:%d\n", avi->audio_sample_rate*10);
}
#endif

uint32_t avi_srarch_id(uint8_t* buf, uint32_t size, uint8_t* id)
{
    uint32_t i;
	size-=4;
	for(i=0;i<size;i++)
	{
	   	if(buf[i]==id[0])
			if(buf[i+1]==id[1])
				if(buf[i+2]==id[2])	
					if(buf[i+3]==id[3])
                        return i;
	}
	return 0;
}

#define	 MAKEWORD(ptr)	(uint16_t)(((uint16_t)*((uint8_t*)(ptr))<<8)|(uint16_t)*(uint8_t*)((ptr)+1))
#define  MAKEDWORD(ptr)	(uint32_t)(((uint16_t)*(uint8_t*)(ptr)|(((uint16_t)*(uint8_t*)(ptr+1))<<8)|\
						(((uint16_t)*(uint8_t*)(ptr+2))<<16)|(((uint16_t)*(uint8_t*)(ptr+3))<<24))) 

int avi_get_streaminfo(uint8_t* buf, avi_t* avi)
{
	avi->stream_id=MAKEWORD(buf+2);
	avi->stream_size=MAKEDWORD(buf+4);
	if(avi->stream_size%2)avi->stream_size++;// must be even
	if(avi->stream_id==AVI_VIDS_FLAG||avi->stream_id==AVI_AUDS_FLAG)
        return AVI_STATUS_OK;
	return AVI_STATUS_ERR_STREAM;	
}

//////////////////////////////////////////////////////////////////////


/**
 * 
 * @avi_config: config: usec_per_frame, max_byte_sec, width, height,
 *                      audio_sample_rate, audio_channels, audio_format
 * @return return 0 if success, or returen error code(>0 from errno.h)
 */
int avi_record_header_init(const char* path, avi_t* avi_config)
{
	uint8_t* buf = video_hal_malloc(2048);// actually < 2048
	uint8_t* buf_start = buf;
	avi_header_t* header;
	list_header_t* list_header0;
	list_header_t* list_header;
	list_header_t* list_header_video;
	avih_header_t* avih_header; 
	strh_header_t* strh_header; 

	strf_bmp_header_t* bmp_header; 
	strf_wav_header_t* wav_header; 


	//TODO: check parameters
	if(!avi_config->record_audio)
	{
		avi_config->audio_sample_rate = 0;
		// avi_config->audio_format = 0;
		// avi_config->audio_channels = 0;
	}

	header=(avi_header_t*)buf;
	memset(header, 0, sizeof(avi_header_t));
	header->riff_id = AVI_RIFF_ID;                        //"RIFF"
	header->avi_id = AVI_AVI_ID;                          //"AVI "
	buf += sizeof(avi_header_t);

	list_header0=(list_header_t*)(buf);
	memset(list_header0, 0, sizeof(list_header_t));
	list_header0->list_id = AVI_LIST_ID;                   //"LIST"
	list_header0->list_type = AVI_HDRL_ID;                 //"hdrl"
	buf += sizeof(list_header_t);

	avih_header=(avih_header_t*)(buf);
	memset(avih_header, 0, sizeof(avih_header_t));
	avih_header->block_id = AVI_AVIH_ID;                  //"avih"
	avih_header->block_size = sizeof(avih_header_t)-8;    //0x38(56)
	avih_header->usec_per_frame = avi_config->usec_per_frame;
	avih_header->max_byte_sec = avi_config->max_byte_sec;
	avih_header->streams = 2;                             //2 streams, video and audio
	avih_header->ref_buf_size = 0x00100000;
	avih_header->width = avi_config->width;
	avih_header->height = avi_config->height;
	buf += sizeof(avih_header_t);

	// LIST for video header
	list_header_video=(list_header_t*)(buf);
	list_header_video->list_id = AVI_LIST_ID;                  //"LIST"
	list_header_video->list_type = AVI_STRL_ID;                //"strl"
	buf += sizeof(list_header_t);

	strh_header=(strh_header_t*)(buf);
	strh_header->block_id = AVI_STRH_ID;                 //"strh"
	strh_header->block_size = sizeof(strh_header_t)-8;   //0x38(56)
	strh_header->stream_type = AVI_VIDS_STREAM;          //"vids"  //video befor audio info
	strh_header->handler = AVI_FORMAT_MJPG;              //"MJPG"
	strh_header->init_frames = 0x00;                     //first frame
	strh_header->scale = 0x01;
	strh_header->rate = (uint32_t)(1000/(avi_config->usec_per_frame/1000.0));
	strh_header->start = 0x000000;
	strh_header->length = 0x00;
	strh_header->ref_buf_size = 5128;
	strh_header->quality = 0xFFFFFFFF;
	strh_header->sample_size = 0x00;
	strh_header->frame.right = avi_config->width;
	strh_header->frame.bottom = avi_config->height;
	buf += sizeof(strh_header_t);

	bmp_header = (strf_bmp_header_t*)(buf);
	bmp_header->block_id = AVI_STRF_ID;                     //"strf"
	bmp_header->block_size = sizeof(strf_bmp_header_t) -8;  //0x28(40)
	bmp_header->bmi_header.bmp_size = sizeof(bmp_header_t);
	bmp_header->bmi_header.width = avi_config->width;
	bmp_header->bmi_header.height = avi_config->height;
	bmp_header->bmi_header.planes = 1;
	bmp_header->bmi_header.bit_count = 24;
	bmp_header->bmi_header.compression = AVI_FORMAT_MJPG;


	list_header_video->block_size =  sizeof(list_header_t)-8 + 
	                           sizeof(strh_header_t) +  
							   sizeof(strf_bmp_header_t);// 29*4
	buf += sizeof(strf_bmp_header_t);                       // video stream header ok

	// LIST for audio header
	list_header=(list_header_t*)(buf);
	list_header->list_id = AVI_LIST_ID;                  //"LIST"
	list_header->list_type = AVI_STRL_ID;                //"strl"
	buf += sizeof(list_header_t);

	strh_header=(strh_header_t*)(buf);
	strh_header->block_id = AVI_STRH_ID;                 //"strh"
	strh_header->block_size = sizeof(strh_header_t)-8;   //0x38(56)
	strh_header->stream_type = AVI_AUDS_STREAM;          //"auds"  //audio info
	strh_header->handler = avi_config->audio_format;         //PCM=0x01
	strh_header->scale = 1;
	strh_header->rate  = avi_config->audio_sample_rate;
	strh_header->ref_buf_size = 4096;//TODO:
	strh_header->quality = 0xFFFFFFFF;
	buf += sizeof(strh_header_t);

	wav_header = (strf_wav_header_t*)(buf);
	wav_header->block_id = AVI_STRF_ID;                     //"strf"
	wav_header->block_size = sizeof(strf_wav_header_t) -8;  //0x28(40)
	wav_header->format_tag = avi_config->audio_format;
	wav_header->channels = avi_config->audio_channels;
	wav_header->sample_rate = avi_config->audio_sample_rate;
	wav_header->block_align = 4;
	wav_header->bits_depth  = 16;

	list_header->block_size =  sizeof(list_header_t)-8 +    // 1*4
	                           sizeof(strh_header_t) +      //16*4
							   sizeof(strf_wav_header_t);   // 6*4
	buf += sizeof(strf_wav_header_t);                       // video stream header ok

	list_header0->block_size = sizeof(list_header_t)-8 +  //sizeof(hdrl)
	                           sizeof(avih_header_t) +    
							   list_header_video->block_size+8 + //list video size
							   list_header->block_size+8;        //list audio size

	//TODO: fill JUNK data to align
	list_header=(list_header_t*)(buf);
	list_header->list_id = AVI_LIST_ID;                  //"LIST"
	list_header->list_type = AVI_MOVI_ID;                //"movi"
	buf += sizeof(list_header_t);
	
	avi_config->offset_movi = (uint8_t*)(&list_header->list_type) - buf_start;
	
	int ret;
	ret = video_hal_file_open(avi_config, path, true);
	if(ret < 0)
	{
		video_hal_free(buf_start);
		return -ret;	
	}
	ret = video_hal_file_write(avi_config, buf_start, buf - buf_start);
	video_hal_free(buf_start);
	if( ret <= 0)
	{
		video_hal_file_close(avi_config);
		return -ret;
	}
	avi_config->total_frame = 0;
	return 0;
}

/**
 * 
 * @return <0 if error occurred, or return length of append data
 */
int avi_record_append_video(avi_t* avi, image_t* img)
{
	int ret;
	uint8_t* buf = NULL;
	avi_data_t data = {
		.id = *((uint32_t*)AVI_VIDS_FLAG_TBL[0]),
		.len = 0,
		.data = buf
	};
	ret = video_hal_file_write(avi, (uint8_t*)&data, 8);
	if( ret <= 0 )
		return ret;
	ret = video_hal_image_encode_mjpeg(avi, img);
	if (ret < 0 )
		return ret;
	data.len = ret;
	ret = video_hal_file_seek(avi, -(data.len+4), VIDEO_HAL_FILE_SEEK_CUR);
	if( ret != 0)
		return -ret;
	ret = video_hal_file_write(avi, (uint8_t*)&data.len, 4);
	if( ret <= 0 )
		return ret;
	ret = video_hal_file_seek(avi, data.len, VIDEO_HAL_FILE_SEEK_CUR);
	if( ret != 0)
		return -ret;
	avi->content_size += 8 + data.len; //header length + data length
	++avi->total_frame;
	return data.len;
}

int avi_record_append_audio(avi_t* avi, uint8_t* buf, uint32_t len)
{
	avi_data_t data = {
		.id = (uint32_t)AVI_AUDS_FLAG_TBL[1],
		.len = len,
		.data = buf
	};
	int ret = video_hal_file_write(avi, (uint8_t*)&data, 8);
	if( ret <= 0 )
		return -ret;	
	ret = video_hal_file_write(avi, buf, len);
	if( ret <= 0 )
		return -ret;
	return 0;
}

int avi_record_fail(avi_t* avi)
{
	video_hal_file_close(avi);
	return 0;
}

int avi_record_finish(avi_t* avi)
{
	
	int ret;
	uint32_t size;

	//write content length
	  //move cursor after to LIST(LIST....movi00dc)
	ret = video_hal_file_seek(avi, -(avi->content_size+8), VIDEO_HAL_FILE_SEEK_CUR);
	if( ret != 0)
		return -ret;
	size = avi->content_size+4; // len(content) + len(movi)
	ret = video_hal_file_write(avi, (uint8_t*)&size, 4);
	if( ret <= 0)
		return -ret;
	//write file length
	size = video_hal_file_size(avi);
	if(size <= 0)
		return -ret;
	size -= 8;  // not include AVI Header(RIFF)(4B) and size(4B)
	ret = video_hal_file_seek(avi, 4, VIDEO_HAL_FILE_SEEK_SET);
	if(ret!=0)
		return -ret;
	ret = video_hal_file_write(avi, (uint8_t*)&size, 4);
	if( ret <= 0)
		return -ret;
	//write total_frame in avih
	ret = video_hal_file_seek(avi, 40, VIDEO_HAL_FILE_SEEK_CUR); //4+ sizeof(list_header_t) +4*6
	if(ret!=0)
		return -ret;
	ret = video_hal_file_write(avi, (uint8_t*)(&avi->total_frame), 4);
	if( ret <= 0)
		return -ret;
	video_hal_file_close(avi);
	return 0;
}

