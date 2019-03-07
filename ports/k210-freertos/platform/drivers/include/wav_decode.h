#ifndef _WAV_DECODE_H
#define _WAV_DECODE_H

/* .WAV file format :

 Endian      Offset      Length      Contents
  big         0           4 bytes     'RIFF'             // 0x52494646
  little      4           4 bytes     <file length - 8>
  big         8           4 bytes     'WAVE'             // 0x57415645

Next, the fmt chunk describes the sample format:

  big         12          4 bytes     'fmt '          // 0x666D7420
  little      16          4 bytes     0x00000010      // Length of the fmt data (16 bytes)
  little      20          2 bytes     0x0001          // Format tag: 1 = PCM
  little      22          2 bytes     <channels>      // Channels: 1 = mono, 2 = stereo
  little      24          4 bytes     <sample rate>   // Samples per second: e.g., 22050
  little      28          4 bytes     <bytes/second>  // sample rate * block align
  little      32          2 bytes     <block align>   // channels * bits/sample / 8
  little      34          2 bytes     <bits/sample>   // 8 or 16
(option)
Finally, the data chunk contains the sample data:

  big         36          4 bytes     'data'        // 0x64617461
  little      40          4 bytes     <length of the data block>
  little      44          *           <sample data>

*/

#include "stdint.h"

//---------------------------------decode-----------------------------

/* Audio file information structure */
typedef struct _wav_t
{
	uint16_t numchannels;
	uint32_t samplerate;
	uint32_t byterate;
	uint16_t blockalign;
	uint16_t bitspersample;
	uint32_t datasize;
}wav_decode_t;

/* Error Identification structure */
typedef enum _wav_err_t
{
	OK = 0,						   //0
	FILE_END,					   //1
	FILE_FAIL,					   //2
	UNVALID_RIFF_ID,			   //3
	UNVALID_RIFF_SIZE,			   //4
	UNVALID_WAVE_ID,			   //5
	UNVALID_FMT_ID,				   //6
	UNVALID_FMT_SIZE,			   //7
	UNSUPPORETD_FORMATTAG,		   //8
	UNSUPPORETD_NUMBER_OF_CHANNEL, //9
	UNSUPPORETD_SAMPLE_RATE,	   //10
	UNSUPPORETD_BITS_PER_SAMPLE,   //11
	UNVALID_LIST_SIZE,			   //12
	UNVALID_DATA_ID,			   //13
}wav_err_t;

wav_err_t wav_init(wav_decode_t *wav_obj,void* head, uint32_t file_size, uint32_t* head_len);

//---------------------------------encode-----------------------------

typedef struct
{
	uint32_t  riff_id;  /* {'r', 'i', 'f', 'f'}  */
	uint32_t  file_size;
	uint32_t  wave_id;
} file_chunk_t;

typedef struct
{
  uint32_t fmt_ID;  /* {'f', 'm', 't', ' '} */
  uint32_t chunk_size;
  uint16_t format_tag;
  uint16_t numchannels;
  uint32_t samplerate;
  uint32_t byterate;
  uint16_t blockalign;
  uint16_t bitspersample;
  /* Note: there may be additional fields here, 
     depending upon wFormatTag. */
} format_chunk_t;

typedef struct
{
  uint32_t data_ID;  /* {'d', 'a', 't', 'a'}  */
  uint32_t chunk_size;
  uint32_t* wave_data;
} data_chunk_t;

typedef struct _wav_encode_t
{
	file_chunk_t file;
	format_chunk_t format;
	data_chunk_t data;
}wav_encode_t;

#endif /* _WAV_DECODE_H */
