#ifndef __STFT_H
#define __STFT_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
// #include "fft.h"
#include "dmac.h"

// #define FFT_DMAC_IN_CHANNEL   DMAC_CHANNEL3
// #define FFT_DMAC_OUT_CHANNEL  DMAC_CHANNEL4

#define FLOAT_MODE 0
#if !FLOAT_MODE
	typedef uint8_t ftr_t;
#else
	typedef float ftr_t;
#endif
void mel_compute(int16_t *input_data, ftr_t *output_data, int16_t d0);

#endif