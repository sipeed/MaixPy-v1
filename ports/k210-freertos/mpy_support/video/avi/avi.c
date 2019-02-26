
#include "avi.h"
#include "stdio.h"

uint8_t* const AVI_VIDS_FLAG_TBL[2]={"00dc","01dc"};
uint8_t* const AVI_AUDS_FLAG_TBL[2]={"00wb","01wb"};

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
			avi->audio_type=0;
			
		}else
		{			
			if(list_header->list_type!=AVI_STRL_ID)return AVI_STATUS_ERR_STRL;
			strh_header=(strh_header_t*)(buf+12);
			if(strh_header->block_id!=AVI_STRH_ID)return AVI_STATUS_ERR_STRH;	//STRH ID错误 
			if(strh_header->stream_type!=AVI_AUDS_STREAM)return AVI_STATUS_ERR_FORMAT;//格式错误
			wav_header=(strf_wav_header_t*)(buf+12+strh_header->block_size+8);//strf
			if(wav_header->block_id!=AVI_STRF_ID)return AVI_STATUS_ERR_STRF;		//STRF ID错误   
			avi->audio_sample_rate=wav_header->sample_rate;						//音频采样率
			avi->audio_channels=wav_header->channels;							//音频通道数
			avi->audio_type=wav_header->format_tag;						//音频格式
		}
	}else if(strh_header->stream_type==AVI_AUDS_STREAM)		 		//音频帧在前
	{ 
		avi->video_flag=(uint8_t*)AVI_VIDS_FLAG_TBL[1];					//视频流标记  "01dc"
		avi->audio_flag=(uint8_t*)AVI_AUDS_FLAG_TBL[0];					//音频流标记  "00wb"
		wav_header=(strf_wav_header_t*)(buf+12+strh_header->block_size+8);//strf
		if(wav_header->block_id!=AVI_STRF_ID)return AVI_STATUS_ERR_STRF;		//STRF ID错误 
		avi->audio_sample_rate=wav_header->sample_rate;						//音频采样率
		avi->audio_channels=wav_header->channels;							//音频通道数
		avi->audio_type=wav_header->format_tag;						//音频格式
		buf+=list_header->block_size+8;								//偏移
		list_header=(list_header_t*)(buf);
		if(list_header->list_id!=AVI_LIST_ID)return AVI_STATUS_ERR_LIST;		//LIST ID错误 
		if(list_header->list_type!=AVI_STRL_ID)return AVI_STATUS_ERR_STRL;	//STRL ID错误   
		strh_header=(strh_header_t*)(buf+12);
		if(strh_header->block_id!=AVI_STRH_ID)return AVI_STATUS_ERR_STRH;	//STRH ID错误 
		if(strh_header->stream_type!=AVI_VIDS_STREAM)return AVI_STATUS_ERR_FORMAT;//格式错误  
		bmp_header=(strf_bmp_header_t*)(buf+12+strh_header->block_size+8);//strf
		if(bmp_header->block_id!=AVI_STRF_ID)return AVI_STATUS_ERR_STRF;		//STRF ID错误  
		if(bmp_header->bmi_header.compression!=AVI_FORMAT_MJPG)return AVI_STATUS_ERR_FORMAT;//格式错误  
		avi->width=bmp_header->bmi_header.width;
		avi->height=bmp_header->bmi_header.height; 	
	}

	offset=avi_srarch_id(buf_start,size,"movi");					//查找movi ID
	if(offset==0)
        return AVI_STATUS_ERR_MOVI;						//MOVI ID错误
	if(avi->audio_sample_rate)//有音频流,才查找
	{
		buf_start+=offset;
		offset=avi_srarch_id(buf_start,size,avi->audio_flag);			//查找音频流标记
		if(offset==0)return AVI_STATUS_ERR_STREAM;						//流错误
		buf_start+=offset+4;
		avi->audio_buf_size=*((uint32_t*)buf_start);						//得到音频流buf大小.
	}		
	return res;
}

void avi_debug_info(avi_t* avi)
{
    printf("avi->SecPerFrame:%d\r\n",avi->sec_per_frame);
	printf("avi->TotalFrame:%d\r\n",avi->total_frame);
	printf("avi->Width:%d\r\n",avi->width);
	printf("avi->Height:%d\r\n",avi->height);
	printf("avi->AudioType:%d\r\n",avi->audio_type);
	printf("avi->SampleRate:%d\r\n",avi->audio_sample_rate);
	printf("avi->Channels:%d\r\n",avi->audio_channels);
	printf("avi->AudioBufSize:%d\r\n",avi->audio_buf_size);
	printf("avi->VideoFLAG:%s\r\n",avi->video_flag); 
	printf("avi->AudioFLAG:%s\r\n",avi->audio_flag); 

    printf("\nfps:%.2f\n", 1000.0/(avi->sec_per_frame/1000));
    printf("audio channels:%d\n", avi->audio_channels);
    printf("audio sample rate:%d\n", avi->audio_sample_rate*10);
}


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
	avi->stream_id=MAKEWORD(buf+2);			//得到流类型
	avi->stream_size=MAKEDWORD(buf+4);		//得到流大小 
	if(avi->stream_size%2)avi->stream_size++;	//奇数加1(avi->StreamSize,必须是偶数)
	if(avi->stream_id==AVI_VIDS_FLAG||avi->stream_id==AVI_AUDS_FLAG)
        return AVI_STATUS_OK;
	return AVI_STATUS_ERR_STREAM;	
}


