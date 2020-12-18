#ifndef _ADC_H
#define _ADC_H


#define fs			8000					//ADC采样率 Hz 8000
#define	voice_len	3000					//录音时间长度 单位ms
#define	VcBuf_Len	((fs/1000)*voice_len)	//语音缓存区长度 单位点数 每个采样点16位
#define atap_len_t	300						//背景噪音自适应时间长度 ms
#define atap_len	((fs/1000)*atap_len_t)	//背景噪音自适应长度

#ifdef __cplusplus
extern "C" {
#endif

void ADC_DMA_Init(void);

#ifdef __cplusplus
}
#endif

#endif

