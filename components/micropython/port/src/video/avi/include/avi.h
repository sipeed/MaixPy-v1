#ifndef __AVI_H
#define __AVI_H 
  
#include "stdbool.h"
#include "stdint.h"

#include "py/mpconfig.h"


#include "imlib.h" // need image_t related

#define AVI_AUDIO_BUF_MAX_NUM 4

//Little endian
#define AVI_RIFF_ID			0X46464952  
#define AVI_AVI_ID			0X20495641
#define AVI_LIST_ID			0X5453494C  
#define AVI_HDRL_ID			0X6C726468
#define AVI_MOVI_ID			0X69766F6D          //"movi"
#define AVI_STRL_ID			0X6C727473

#define AVI_AVIH_ID			0X68697661
#define AVI_STRH_ID			0X68727473
#define AVI_STRF_ID			0X66727473
#define AVI_STRD_ID			0X64727473

#define AVI_VIDS_STREAM		0X73646976
#define AVI_AUDS_STREAM		0X73647561

#define AVI_VIDS_FLAG		0X6463
#define AVI_AUDS_FLAG		0X7762

#define AVI_FORMAT_MJPG		0X47504A4D          //"MJPG"

#define AVI_AUDIO_FORMAT_PCM  0x00000001        // PCM
#define AVI_AUDIO_FORMAT_MP2  0x00000050        // MP2
#define AVI_AUDIO_FORMAT_MP3  0x00000055        // MP3
#define AVI_AUDIO_FORMAT_AC3  0x00002000        // AC3


typedef enum {
	AVI_STATUS_OK = 0     ,
	AVI_STATUS_ERR_RIFF   ,
	AVI_STATUS_ERR_AVI    ,
	AVI_STATUS_ERR_LIST   ,
	AVI_STATUS_ERR_HDRL   ,
	AVI_STATUS_ERR_AVIH   ,
	AVI_STATUS_ERR_STRL   ,
	AVI_STATUS_ERR_STRH   ,
	AVI_STATUS_ERR_STRF   ,
	AVI_STATUS_ERR_MOVI   ,
	AVI_STATUS_ERR_FORMAT ,
	AVI_STATUS_ERR_STREAM ,
    AVI_STATUS_MAX
}avi_status_t;

typedef struct
{	
	uint32_t riff_id;				//fixed 'RIFF' 0X61766968
	uint32_t file_size;			    //not include fiff_id and file_size
	uint32_t avi_id;				//fixed 'AVI '==0X41564920 
}avi_header_t;


typedef struct
{	
	uint32_t list_id;               // fiexed 'LIST'==0X4c495354
	uint32_t block_size;            // not include list_id and block_size
	uint32_t list_type;             // hdrl(info)/movi(data)/idxl(index,optionally)
}list_header_t;

typedef struct
{	
	uint32_t block_id;
	uint32_t block_size;
	uint32_t usec_per_frame;
	uint32_t max_byte_sec;
	uint32_t padding_franularity;
	uint32_t flags;
	uint32_t total_frame;
	uint32_t init_frames;
	uint32_t streams;
	uint32_t ref_buf_size;
	uint32_t width;
	uint32_t height;
	uint32_t reserved[4];
}avih_header_t;


typedef struct
{	
	uint32_t block_id;
	uint32_t block_size;
	uint32_t stream_type;
	uint32_t handler;
	uint32_t flags;
	uint16_t priority;
	uint16_t language;
	uint32_t init_frames;
	uint32_t scale;
	uint32_t rate;
	uint32_t start;
	uint32_t length;
 	uint32_t ref_buf_size;
    uint32_t quality;
	uint32_t sample_size;
	struct
	{				
	   	short left;
		short top;
		short right;
		short bottom;
	}frame;				
}strh_header_t;//uint32_t * 16


typedef struct
{
	uint32_t	 bmp_size;
 	int32_t     width;
	int32_t     height;
	uint16_t     planes;
	uint16_t     bit_count;
	uint32_t     compression;
	uint32_t     size_image;
	int32_t     x_pix_per_meter;
	int32_t     y_pix_per_meter;
	uint32_t     color_used;
	uint32_t     color_important;
}bmp_header_t;

typedef struct 
{
	uint8_t  blue;
	uint8_t  green;
	uint8_t  red;
	uint8_t  reserved;
}avi_rgb_quad_t;

typedef struct 
{
	uint32_t block_id;         //strf==0X73747266
	uint32_t block_size;
	bmp_header_t bmi_header;
	// avi_rgb_quad_t bm_colors[1];
}strf_bmp_header_t;



typedef struct 
{
	uint32_t block_id;         //"strf"==0X73747266
	uint32_t block_size;       //not include block_size
   	uint16_t format_tag;       //AVI_AUDIO_FORMAT_***
	uint16_t channels;         //
	uint32_t sample_rate;      //
	uint32_t baud_rate;        //avg bytes per second
	uint16_t block_align;      //
	uint16_t bits_depth;       //bits
}strf_wav_header_t ;

typedef struct{
	uint32_t id;               // video: "00dc" / "01dc", audio: "00wb" / "01wb"
	uint32_t len;
	uint8_t* data;
} avi_data_t;

typedef struct{
	uint8_t* buf;
	uint32_t len;
	volatile bool     empty;
} audio_buf_info_t __attribute__((aligned(8)));

typedef struct
{
	uint32_t usec_per_frame;
	uint32_t max_byte_sec;
	uint32_t total_frame;
	uint32_t width;
	uint32_t height;
	uint32_t audio_sample_rate;
	uint16_t audio_channels;
	uint16_t audio_buf_size;
	uint16_t audio_format;          //AVI_AUDIO_FORMAT_***
	uint16_t stream_id;           //'dc'==0X6463(video) 'wb'==0X7762(audio)
	uint32_t stream_size;         // must be even, or +1 to be even
	uint8_t* video_flag;          //"00dc"/"01dc"
	uint8_t* audio_flag;          //"00wb"/"01wb"

	uint32_t frame_count;         //frame palyed
	int      status;
	uint64_t time_us_fps_ctrl;

    void*    file;
	uint8_t* video_buf;
	uint8_t* img_buf;
	uint32_t offset_movi;         //start index of movi flag

	audio_buf_info_t audio_buf[AVI_AUDIO_BUF_MAX_NUM];
	volatile uint8_t  index_buf_save;
	volatile uint8_t  index_buf_play;
	uint32_t audio_count;
	uint8_t  volume;

	bool     record;
	bool     record_audio;
	uint8_t  mjpeg_quality;
	uint64_t content_size;       //video and audio data size(include audio and video header size)
} avi_t __attribute__((aligned(8)));


int avi_init(uint8_t* buff, uint32_t size, avi_t* avi);
uint32_t avi_srarch_id(uint8_t* buf, uint32_t size, uint8_t* id);
int avi_get_streaminfo(uint8_t* buf, avi_t* avi);
void avi_debug_info(avi_t* avi);

/**
 * 
 * @avi_config: config: usec_per_frame, max_byte_sec, width, height,
 *                      audio_sample_rate, audio_channels, audio_format
 * @return return 0 if success, or returen error code(>0 from errno.h)
 */
int avi_record_header_init(const char* path, avi_t* avi_config);
/**
 * 
 * @return <0 if error occurred, or return length of append data
 */
int avi_record_append_video(avi_t* avi, image_t* img);
int avi_record_append_audio(avi_t* avi, uint8_t* buf, uint32_t len);
int avi_record_fail(avi_t* avi);
int avi_record_finish(avi_t* avi);


#endif

