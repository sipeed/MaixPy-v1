#ifndef __MAIX_SPEECH_RECOGNITION_H
#define __MAIX_SPEECH_RECOGNITION_H

#include <stdint.h>

#include "VAD.h"
#include "MFCC.h"
#include "DTW.h"
#include "ADC.h"

#include "i2s.h"

typedef enum __iw_state {
  Init,
  Idle,
  Ready,
  MaybeNoise,
  Restrain,
  Speek,
  Done,
} IwState;

extern v_ftr_tag ftr_curr; // current result

void iw_load(i2s_device_number_t device_num, dmac_channel_number_t channel_num, uint32_t priority);

void iw_free();

void iw_atap_tag(uint16_t n_thl, uint16_t z_thl, uint32_t s_thl);

void iw_set_state(IwState state);

IwState iw_get_state();

v_ftr_tag *iw_get_ftr();

#endif