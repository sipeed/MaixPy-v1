#ifndef _VAD_H
#define _VAD_H



#define	max_vc_con	3	//VAD最多检测的语音段数
#define frame_time	20						// 每帧时间长度 单位ms
#define frame_mov_t	10						// 帧移
#define FRAME_LEN	(frame_time*fs/1000)	// 帧长
#define frame_mov	(frame_mov_t *fs/1000)	// 帧移，相邻帧交叠部分

#ifdef __cplusplus
extern "C" {
#endif


#pragma pack(1)
typedef struct {
	uint32_t mid_val;	//语音段中值 相当于有符号的0值 用于短时过零率计算
	uint16_t n_thl;		//噪声阈值，用于短时过零率计算
	uint16_t z_thl;		//短时过零率阈值，超过此阈值，视为进入过渡段。
	uint32_t s_thl;		//短时累加和阈值，超过此阈值，视为进入过渡段。
} atap_tag;			//自适应参数
#pragma pack()

//#pragma pack(4)
typedef struct {
	uint16_t *start;	//起始点
	uint16_t *end;	//结束点
//	uint8_t word_num;
} valid_tag;	//有效语音段
//#pragma pack()

void noise_atap(const uint16_t *noise, uint16_t n_len, atap_tag *atap);
void VAD(const uint16_t *vc, uint16_t buf_len, valid_tag *valid_voice, atap_tag *atap_arg);
uint8_t VAD2(const uint16_t *vc, valid_tag *valid_voice, atap_tag *atap_arg);

#ifdef __cplusplus
}
#endif

#endif
