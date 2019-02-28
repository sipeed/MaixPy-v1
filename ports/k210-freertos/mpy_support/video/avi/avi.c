
#include "avi.h"
#include "stdio.h"

uint8_t* const AVI_VIDS_FLAG_TBL[2]={"00dc","01dc"};
uint8_t* const AVI_AUDS_FLAG_TBL[2]={"00wb","01wb"};
const uint8_t* AVI_FLAG_MOVI = "movi";

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
	avi->sec_per_frame=avih_header->sec_per_frame;
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
	offset=avi_srarch_id(buf_start,size,(uint8_t*)AVI_FLAG_MOVI);
	if(offset==0)
        return AVI_STATUS_ERR_MOVI;
	avi->offset_movi = offset;
	if(avi->audio_sample_rate)
	{
		buf = buf_start + offset;
		offset=avi_srarch_id(buf, size - offset, avi->audio_flag);
		if(offset==0)
			return AVI_STATUS_ERR_STREAM;
		buf+=offset+4;
		avi->audio_buf_size=*((uint32_t*)buf);
	}		
	return res;
}

#ifdef VIDEO_DEBUG
void avi_debug_info(avi_t* avi)
{
    printf("avi->SecPerFrame:%d\r\n",avi->sec_per_frame);
	printf("avi->TotalFrame:%d\r\n",avi->total_frame);
	printf("avi->Width:%d\r\n",avi->width);
	printf("avi->Height:%d\r\n",avi->height);
	printf("avi->AudioType:%d\r\n",avi->audio_format);
	printf("avi->SampleRate:%d\r\n",avi->audio_sample_rate);
	printf("avi->Channels:%d\r\n",avi->audio_channels);
	printf("avi->AudioBufSize:%d\r\n",avi->audio_buf_size);
	printf("avi->VideoFLAG:%s\r\n",avi->video_flag); 
	printf("avi->AudioFLAG:%s\r\n",avi->audio_flag); 

    printf("\nfps:%.2f\n", 1000.0/(avi->sec_per_frame/1000.0));
    printf("audio channels:%d\n", avi->audio_channels);
    printf("audio sample rate:%d\n", avi->audio_sample_rate*10);
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
 * @avi_config: config: sec_per_frame, max_byte_sec, width, height,
 *                      audio_sample_rate, audio_channels, audio_format
 */
int avi_record_header_init(uint8_t* buf, uint32_t buf_size, avi_t* avi_config)
{
	uint8_t* buf_start = buf;
	avi_header_t* header;
	list_header_t* list_header;
	avih_header_t* avih_header; 
	strh_header_t* strh_header; 

	strf_bmp_header_t* bmp_header; 
	strf_wav_header_t* wav_header; 

	//TODO: check buf_size

	header=(avi_header_t*)buf;
	memset(header, 0, sizeof(avi_header_t));
	header->riff_id = AVI_RIFF_ID;                        //"RIFF"
	header->avi_id = AVI_AVI_ID;                          //"AVI "
	buf += sizeof(avi_header_t);

	list_header=(list_header_t*)(buf);
	memset(list_header, 0, sizeof(list_header_t));
	list_header->list_id = AVI_LIST_ID;                   //"LIST"
	list_header->list_type = AVI_HDRL_ID;                 //"hdrl"
	buf += sizeof(list_header_t);

	avih_header=(avih_header_t*)(buf);
	memset(avih_header, 0, sizeof(avih_header_t));
	avih_header->block_id = AVI_AVIH_ID;                  //"avih"
	avih_header->block_size = sizeof(avih_header_t)-8;    //0x38(56)
	avih_header->sec_per_frame = avi_config->sec_per_frame;
	avih_header->max_byte_sec = avi_config->max_byte_sec;
	avih_header->streams = 2;                             //2 streams, video and audio
	avih_header->width = avi_config->width;
	avih_header->height = avi_config->height;
	buf += sizeof(avih_header_t);

	// LIST for video header
	list_header=(list_header_t*)(buf);
	list_header->list_id = AVI_LIST_ID;                  //"LIST"
	list_header->list_type = AVI_STRL_ID;                //"strl"
	buf += sizeof(list_header_t);

	strh_header=(strh_header_t*)(buf);
	strh_header->block_id = AVI_STRH_ID;                 //"strh"
	strh_header->block_size = sizeof(strh_header_t)-8;   //0x38(56)
	strh_header->stream_type = AVI_VIDS_STREAM;          //"vids"  //video befor audio info
	strh_header->handler = AVI_FORMAT_MJPG;              //"MJPG"
	strh_header->init_frames = 0x01;                     //first frame
	strh_header->scale = 0x00;//TODO: 0x0f
	strh_header->rate = 0x00;
	strh_header->start = 0x000000; //TODO: 0x00000cd8
	strh_header->length = 0x00;//TODO:
	strh_header->ref_buf_size = 0xffffffff; //TODO:
	strh_header->quality = 0x00;
	strh_header->sample_size = 0x00;
	//TODO: frame...
	buf += sizeof(strh_header_t);

	bmp_header = (strf_bmp_header_t*)(buf);
	bmp_header->block_id = AVI_STRF_ID;                     //"strf"
	bmp_header->block_size = sizeof(strf_bmp_header_t) -8;  //0x28(40)
	bmp_header->bmi_header.width = avi_config->width;
	bmp_header->bmi_header.height = avi_config->height;

	list_header->block_size =  sizeof(list_header_t)-8 + 
	                           sizeof(strh_header_t) +  
							   sizeof(strf_bmp_header_t);
	buf += list_header->block_size+8;                       // video stream header ok

	// LIST for audio header
	list_header=(list_header_t*)(buf);
	list_header->list_id = AVI_LIST_ID;                  //"LIST"
	list_header->list_type = AVI_STRL_ID;                //"strl"
	buf += sizeof(list_header_t);

	strh_header=(strh_header_t*)(buf);
	strh_header->block_id = AVI_STRH_ID;                 //"strh"
	strh_header->block_size = sizeof(strh_header_t)-8;   //0x38(56)
	strh_header->stream_type = AVI_AUDS_STREAM;          //"vids"  //video befor audio info
	strh_header->handler = avi_config->audio_format;         //PCM=0x01

	//TODO: frame...
	buf += sizeof(strh_header_t);

	wav_header = (strf_wav_header_t*)(buf);
	wav_header->block_id = AVI_STRF_ID;                     //"strf"
	wav_header->block_size = sizeof(strf_wav_header_t) -8;  //0x28(40)
	wav_header->sample_rate = avi_config->audio_sample_rate;
	wav_header->channels = avi_config->audio_channels;
	wav_header->format_tag = avi_config->audio_format;

	list_header->block_size =  sizeof(list_header_t)-8 + 
	                           sizeof(strh_header_t) +  
							   sizeof(strf_wav_header_t);
	buf += list_header->block_size+8;                       // video stream header ok

	//TODO: fill JUNK data to align

	memcpy(buf,AVI_FLAG_MOVI, 4);
	
	avi_config->offset_movi = buf - buf_start;
	
	return 0;
}

