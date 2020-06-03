/*
* Copyright 2020 Sipeed Co.,Ltd.

* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef MAIX_SPEECH_RECOGNIZER_H
#define MAIX_SPEECH_RECOGNIZER_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "syslog.h"

#include "flash.h"

#include "VAD.h"
#include "MFCC.h"
#include "DTW.h"
#include "flash.h"
#include "ADC.h"
#include "i2s.h"
#include "Maix_i2s.h"

typedef enum _sr_status_t
{
    SR_NONE = 0,
    SR_RECORD_WAIT_SPEACKING,
    SR_RECORD_SUCCESSFUL,
    SR_RECOGNIZER_WAIT_SPEACKING,
    SR_RECOGNIZER_SUCCESSFULL,
    SR_GET_NOISEING,
}
sr_status_t;

int speech_recognizer_init(Maix_i2s_obj_t *dev);
int speech_recognizer_record(uint8_t keyword_num, uint8_t model_num);
sr_status_t speech_recognizer_get_status(void);
int speech_recognizer_get_result(void);
int speech_recognizer_finish(void);
// int speech_recognizer_print_model(uint8_t keyword_num, uint8_t model_num);
int speech_recognizer_get_data(uint8_t keyword_num, uint8_t model_num, uint16_t *frm_num, int16_t **voice_model, uint32_t *voice_model_len);
int speech_recognizer_add_voice_model(uint8_t keyword_num, uint8_t model_num, const int16_t *voice_model, uint16_t frame_num);
uint8_t speech_recognizer_set_Threshold(uint16_t n_thl, uint16_t z_thl, uint32_t s_thl);
int speech_recognizer_recognize(void);

#endif
