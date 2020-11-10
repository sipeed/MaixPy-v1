#ifndef ASR_PREPROCESS_H_
#define ASR_PREPROCESS_H_

int pp_init(int mic_i2s_dev, int dma_i2s_ch, uint8_t lr_shift);
void pp_deinit(void);
void pp_start(void);
void pp_stop(void);
uint8_t* pp_loop(void);

#endif

