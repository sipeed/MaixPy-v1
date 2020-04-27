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

#include "g_def.h"
#include "VAD.h"
#include "MFCC.h"
#include "DTW.h"
#include "flash.h"
#include "ADC.h"
#include "i2s.h"
#include "Maix_i2s.h"

int speech_recognizer_init(i2s_device_number_t device_num);
int speech_recognizer_record(uint8_t keyword_num, uint8_t model_num);
int speech_recognizer_print_model(uint8_t keyword_num, uint8_t model_num);
int speech_recognizer_add_voice_model(uint8_t keyword_num, uint8_t model_num,
                                       const int16_t *voice_model, uint16_t frame_num);
int speech_recognizer_recognize(void);

#endif
