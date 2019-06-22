/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include "syscalls.h"
#include "sysctl.h"
#include "apu.h"

#define BEAFORMING_BASE_ADDR    (0x50250200)
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

volatile apu_reg_t *const apu = (volatile apu_reg_t *)BEAFORMING_BASE_ADDR;

/*
Voice strength average value right shift factor.  When performing sound direction detect,
the average value of samples from different channels is required, this right shift factor
is used to perform division.
0x0: no right shift;               0x1: right shift by 1-bit;
 . . . . . .
0xF: right shift by 15-bit.
*/
void apu_set_audio_gain(uint16_t gain)
{
    apu_ch_cfg_t ch_cfg = apu->bf_ch_cfg_reg;

    ch_cfg.we_bf_target_dir = 0;
    ch_cfg.we_bf_sound_ch_en = 0;
    ch_cfg.we_data_src_mode = 0;
    ch_cfg.we_audio_gain = 1;
    ch_cfg.audio_gain = gain;
    apu->bf_ch_cfg_reg = ch_cfg;
}

/*set sampling shift*/
void apu_set_smpl_shift(uint8_t smpl_shift)
{
    apu_dwsz_cfg_t tmp = apu->bf_dwsz_cfg_reg;

    tmp.smpl_shift_bits = smpl_shift;
    apu->bf_dwsz_cfg_reg = tmp;
}

/*get sampling shift*/
uint8_t apu_get_smpl_shift(void)
{
    apu_dwsz_cfg_t tmp = apu->bf_dwsz_cfg_reg;

    return tmp.smpl_shift_bits;
}

/*
 * APU unit sound channel enable control bits.  Bit 'x' corresponds to enable bit for sound
 * channel 'x' (x = 0, 1, 2, . . ., 7).  APU sound channels are related with I2S host RX channels.
 * APU sound channel 0/1 correspond to the left/right channel of I2S RX0; APU channel 2/3 correspond
 * to left/right channels of I2S RX1; and things like that.  Software write '1' to enable a sound
 * channel and hardware automatically clear the bit after the sample buffers used for direction
 * searching is filled full.
 * 0x1: writing '1' to enable the corresponding APU sound channel.
 */
void apu_set_channel_enabled(uint8_t channel_bit)
{
    apu_ch_cfg_t ch_cfg;

    ch_cfg.we_audio_gain = 0;
    ch_cfg.we_bf_target_dir = 0;
    ch_cfg.we_bf_sound_ch_en = 1;
    ch_cfg.bf_sound_ch_en = channel_bit;
    apu->bf_ch_cfg_reg = ch_cfg;
}

/*
 * APU unit sound channel enable control bits.  Bit 'x' corresponds to enable bit for sound
 * channel 'x' (x = 0, 1, 2, . . ., 7).  APU sound channels are related with I2S host RX channels.
 * APU sound channel 0/1 correspond to the left/right channel of I2S RX0; APU channel 2/3 correspond
 * to left/right channels of I2S RX1; and things like that.  Software write '1' to enable a sound
 * channel and hardware automatically clear the bit after the sample buffers used for direction
 * searching is filled full.
 * 0x1: writing '1' to enable the corresponding APU sound channel.
 */
void apu_channel_enable(uint8_t channel_bit)
{
    apu_ch_cfg_t ch_cfg = apu->bf_ch_cfg_reg;

    ch_cfg.we_audio_gain = 0;
    ch_cfg.we_bf_target_dir = 0;
    ch_cfg.we_data_src_mode = 0;
    ch_cfg.we_bf_sound_ch_en = 1;
    ch_cfg.bf_sound_ch_en = channel_bit;
    apu->bf_ch_cfg_reg = ch_cfg;
}

/*
 *  audio data source configure parameter.  This parameter controls where the audio data source comes from.
 *  0x0: audio data directly sourcing from apu internal buffer;
 *  0x1: audio data sourcing from FFT result buffer.
 */
void apu_set_src_mode(uint8_t src_mode)
{
    apu_ch_cfg_t ch_cfg = apu->bf_ch_cfg_reg;

    ch_cfg.we_audio_gain = 0;
    ch_cfg.we_bf_target_dir = 0;
    ch_cfg.we_bf_sound_ch_en = 0;
    ch_cfg.we_data_src_mode = 1;
    ch_cfg.data_src_mode = src_mode;
    apu->bf_ch_cfg_reg = ch_cfg;
}

/*
 * I2S host beam-forming direction sample ibuffer read index configure register
 */
void apu_set_direction_delay(uint8_t dir_num, uint8_t *dir_bidx)
{
    apu->bf_dir_bidx[dir_num][0] =(apu_dir_bidx_t)
    {
        .dir_rd_idx0 = dir_bidx[0],
        .dir_rd_idx1 = dir_bidx[1],
        .dir_rd_idx2 = dir_bidx[2],
        .dir_rd_idx3 = dir_bidx[3]
    };
    apu->bf_dir_bidx[dir_num][1] =(apu_dir_bidx_t)
    {
        .dir_rd_idx0 = dir_bidx[4],
        .dir_rd_idx1 = dir_bidx[5],
        .dir_rd_idx2 = dir_bidx[6],
        .dir_rd_idx3 = dir_bidx[7]
    };
}

/*
 * radius mic_num_a_circle: the num of mic per circle; center: 0: no center mic, 1:have center mic
 */
void apu_set_delay(float radius, uint8_t mic_num_a_circle, uint8_t center)
{
    uint8_t offsets[16][8];
    int i,j;
    float seta[8], delay[8], hudu_jiao;
    float cm_tick = (float)SOUND_SPEED * 100 / I2S_FS;/*distance per tick (cm)*/
    float min;

    for (i = 0; i < mic_num_a_circle; ++i)
    {
        seta[i] = 360 * i / mic_num_a_circle;
        hudu_jiao = 2 * M_PI * seta[i] / 360;
        delay[i] = radius * (1 - cos(hudu_jiao)) / cm_tick;
    }
    if(center)
        delay[mic_num_a_circle] = radius / cm_tick;

    for (i = 0; i < mic_num_a_circle + center; ++i)
    {
        offsets[0][i] = (int)(delay[i] + 0.5);
    }
    for(; i < 8; i++)
        offsets[0][i] = 0;


    for (j = 1; j < DIRECTION_RES; ++j)
    {
        for (i = 0; i < mic_num_a_circle; ++i)
        {
            seta[i] -= 360 / DIRECTION_RES;
            hudu_jiao = 2 * M_PI * seta[i] / 360;
            delay[i] = radius * (1 - cos(hudu_jiao)) / cm_tick;
        }
        if(center)
            delay[mic_num_a_circle] = radius / cm_tick;

        min = 2 * radius;
        for (i = 0; i < mic_num_a_circle; ++i)
        {
            if(delay[i] < min)
                min = delay[i];
        }
        if(min)
        {
            for (i = 0; i < mic_num_a_circle + center; ++i)
            {
                delay[i] = delay[i] - min;
            }
        }

        for (i = 0; i < mic_num_a_circle + center; ++i)
        {
            offsets[j][i] = (int)(delay[i] + 0.5);
        }
        for(; i < 8; i++)
            offsets[0][i] = 0;
    }
    for (size_t i = 0; i < DIRECTION_RES; i++)
    {
        apu_set_direction_delay(i, offsets[i]);
    }
}

/*
 Sound direction searching enable bit.  Software writes '1' to start sound direction searching function.
 When all the sound sample buffers are filled full, this bit is cleared by hardware (this sample buffers
 are used for direction detect only).
 0x1: enable direction searching.
 */
void apu_dir_enable(void)
{
    apu_ctl_t bf_en_tmp = apu->bf_ctl_reg;

    bf_en_tmp.we_bf_dir_search_en = 1;
    bf_en_tmp.bf_dir_search_en = 1;
    apu->bf_ctl_reg = bf_en_tmp;
}

void apu_dir_reset(void)
{
    apu_ctl_t bf_en_tmp = apu->bf_ctl_reg;

    bf_en_tmp.we_search_path_rst = 1;
    bf_en_tmp.search_path_reset = 1;
    apu->bf_ctl_reg = bf_en_tmp;
}

/*
 Valid voice sample stream generation enable bit.  After sound direction searching is done, software can
 configure this bit to generate a stream of voice samples for voice recognition.
 0x1: enable output of voice sample stream.
 0x0: stop the voice samlpe stream output.
 */
void apu_voc_enable(uint8_t enable_flag)
{
    apu_ctl_t bf_en_tmp = apu->bf_ctl_reg;

    bf_en_tmp.we_bf_stream_gen = 1;
    bf_en_tmp.bf_stream_gen_en = enable_flag;
    apu->bf_ctl_reg = bf_en_tmp;
}

void apu_voc_reset(void)
{
    apu_ctl_t bf_en_tmp = apu->bf_ctl_reg;

    bf_en_tmp.we_voice_gen_path_rst = 1;
    bf_en_tmp.voice_gen_path_reset = 1;
    apu->bf_ctl_reg = bf_en_tmp;
}

/*
Target direction select for valid voice output.  When the source voice direaction searching
is done, software can use this field to select one from 16 sound directions for the following
voice recognition
0x0: select sound direction 0;   0x1: select sound direction 1;
 . . . . . .
0xF: select sound direction 15.
*/
void apu_voc_set_direction(en_bf_dir_t direction)
{
    apu_ch_cfg_t ch_cfg = apu->bf_ch_cfg_reg;

    ch_cfg.we_bf_sound_ch_en = 0;
    ch_cfg.we_audio_gain = 0;
    ch_cfg.we_data_src_mode = 0;
    ch_cfg.we_bf_target_dir = 1;
    ch_cfg.bf_target_dir = direction;
    apu->bf_ch_cfg_reg = ch_cfg;

    apu_ctl_t bf_en_tmp = apu->bf_ctl_reg;

    bf_en_tmp.we_update_voice_dir = 1;
    bf_en_tmp.update_voice_dir = 1;
    apu->bf_ctl_reg = bf_en_tmp;
}

/*
 *I2S host beam-forming Filter FIR16 Coefficient Register
 */
void apu_dir_set_prev_fir(uint16_t *fir_coef)
{
    uint8_t i = 0;

    for (i = 0; i < 9; i++) {
        apu->bf_pre_fir0_coef[i] =
        (apu_fir_coef_t){
            .fir_tap0 = fir_coef[i * 2],
            .fir_tap1 = i == 8 ? 0 : fir_coef[i * 2 + 1]
        };
    }
}

void apu_dir_set_post_fir(uint16_t *fir_coef)
{
    uint8_t i = 0;

    for (i = 0; i < 9; i++) {
        apu->bf_post_fir0_coef[i] =
        (apu_fir_coef_t){
            .fir_tap0 = fir_coef[i * 2],
            .fir_tap1 = i == 8 ? 0 : fir_coef[i * 2 + 1]
        };
    }
}

void apu_voc_set_prev_fir(uint16_t *fir_coef)
{
    uint8_t i = 0;

    for (i = 0; i < 9; i++) {
        apu->bf_pre_fir1_coef[i] =
        (apu_fir_coef_t){
            .fir_tap0 = fir_coef[i * 2],
            .fir_tap1 = i == 8 ? 0 : fir_coef[i * 2 + 1]
        };
    }
}

void apu_voc_set_post_fir(uint16_t *fir_coef)
{
    uint8_t i = 0;

    for (i = 0; i < 9; i++) {
        apu->bf_post_fir1_coef[i] =
        (apu_fir_coef_t){
            .fir_tap0 = fir_coef[i * 2],
            .fir_tap1 = i == 8 ? 0 : fir_coef[i * 2 + 1]
        };
    }
}

void apu_set_fft_shift_factor(uint8_t enable_flag, uint16_t shift_factor)
{
    apu->bf_fft_cfg_reg =
    (apu_fft_cfg_t){
        .fft_enable = enable_flag,
        .fft_shift_factor = shift_factor
    };

    apu_ch_cfg_t ch_cfg = apu->bf_ch_cfg_reg;

    ch_cfg.we_data_src_mode = 1;
    ch_cfg.data_src_mode = enable_flag;
    apu->bf_ch_cfg_reg = ch_cfg;
}

void apu_dir_set_down_size(uint8_t dir_dwn_size)
{
    apu_dwsz_cfg_t tmp = apu->bf_dwsz_cfg_reg;

    tmp.dir_dwn_siz_rate = dir_dwn_size;
    apu->bf_dwsz_cfg_reg = tmp;
}

void apu_dir_set_interrupt_mask(uint8_t dir_int_mask)
{
    apu_int_mask_t tmp = apu->bf_int_mask_reg;

    tmp.dir_data_rdy_msk = dir_int_mask;
    apu->bf_int_mask_reg = tmp;
}

void apu_voc_set_down_size(uint8_t voc_dwn_size)
{
    apu_dwsz_cfg_t tmp = apu->bf_dwsz_cfg_reg;

    tmp.voc_dwn_siz_rate = voc_dwn_size;
    apu->bf_dwsz_cfg_reg = tmp;
}

void apu_voc_set_interrupt_mask(uint8_t voc_int_mask)
{
    apu_int_mask_t tmp = apu->bf_int_mask_reg;

    tmp.voc_buf_rdy_msk = voc_int_mask;
    apu->bf_int_mask_reg = tmp;
}

void apu_set_down_size(uint8_t dir_dwn_size, uint8_t voc_dwn_size)
{
    apu_dwsz_cfg_t tmp = apu->bf_dwsz_cfg_reg;

    tmp.dir_dwn_siz_rate = dir_dwn_size;
    tmp.voc_dwn_siz_rate = voc_dwn_size;
    apu->bf_dwsz_cfg_reg = tmp;
}

void apu_set_interrupt_mask(uint8_t dir_int_mask, uint8_t voc_int_mask)
{
    apu->bf_int_mask_reg =
    (apu_int_mask_t){
        .dir_data_rdy_msk = dir_int_mask,
        .voc_buf_rdy_msk = voc_int_mask
    };
}

void apu_dir_clear_int_state(void)
{
    apu->bf_int_stat_reg =
    (apu_int_stat_t){
        .dir_search_data_rdy = 1
    };
}

void apu_voc_clear_int_state(void)
{
    apu->bf_int_stat_reg =
    (apu_int_stat_t){
        .voc_buf_data_rdy = 1
    };
}

/* reset saturation_counter */
void apu_voc_reset_saturation_counter(void)
{
    apu->saturation_counter = 1<<31;
}

/*get saturation counter*/
/*heigh 16 bit is counter, low 16 bit is total.*/
uint32_t apu_voc_get_saturation_counter(void)
{
    return apu->saturation_counter;
}

/*set saturation limit*/
void apu_voc_set_saturation_limit(uint16_t upper, uint16_t bottom)
{
    apu->saturation_limits = (uint32_t)bottom<<16 | upper;
}

/*get saturation limit*/
/*heigh 16 bit is counter, low 16 bit is total.*/
uint32_t apu_voc_get_saturation_limit(void)
{
    return apu->saturation_limits;
}

static void print_fir(const char *member_name, volatile apu_fir_coef_t *pfir)
{
    printf("  for(int i = 0; i < 9; i++){\n");
    for (int i = 0; i < 9; i++) {
        apu_fir_coef_t fir = pfir[i];

        printf("    apu->%s[%d] = (apu_fir_coef_t){\n", member_name, i);
        printf("      .fir_tap0 = 0x%x,\n", fir.fir_tap0);
        printf("      .fir_tap1 = 0x%x\n", fir.fir_tap1);
        printf("    };\n");
    }
    printf("  }\n");
}

void apu_print_setting(void)
{
    printf("void apu_setting(void) {\n");
    apu_ch_cfg_t bf_ch_cfg_reg = apu->bf_ch_cfg_reg;

    printf("  apu->bf_ch_cfg_reg = (apu_ch_cfg_t){\n");
    printf("    .we_audio_gain = 1, .we_bf_target_dir = 1, .we_bf_sound_ch_en = 1,\n");
    printf("    .audio_gain = 0x%x, .bf_target_dir = %d, .bf_sound_ch_en = %d, .data_src_mode = %d\n",
           bf_ch_cfg_reg.audio_gain, bf_ch_cfg_reg.bf_target_dir, bf_ch_cfg_reg.bf_sound_ch_en, bf_ch_cfg_reg.data_src_mode);
    printf("  };\n");

    apu_ctl_t bf_ctl_reg = apu->bf_ctl_reg;

    printf("  apu->bf_ctl_reg = (apu_ctl_t){\n");
    printf("    .we_bf_stream_gen = 1, .we_bf_dir_search_en = 1,\n");
    printf("    .bf_stream_gen_en = %d, .bf_dir_search_en = %d\n",
           bf_ctl_reg.bf_stream_gen_en, bf_ctl_reg.bf_dir_search_en);
    printf("  };\n");

    printf("  for(int i = 0; i < 16; i++){\n");
    for (int i = 0; i < 16; i++) {
        apu_dir_bidx_t bidx0 = apu->bf_dir_bidx[i][0];
        apu_dir_bidx_t bidx1 = apu->bf_dir_bidx[i][1];

        printf("    apu->bf_dir_bidx[%d][0] = (apu_dir_bidx_t){\n", i);
        printf("      .dir_rd_idx0 = 0x%x,\n", bidx0.dir_rd_idx0);
        printf("      .dir_rd_idx1 = 0x%x,\n", bidx0.dir_rd_idx1);
        printf("      .dir_rd_idx2 = 0x%x,\n", bidx0.dir_rd_idx2);
        printf("      .dir_rd_idx3 = 0x%x\n", bidx0.dir_rd_idx3);
        printf("    };\n");
        printf("    apu->bf_dir_bidx[%d][1] = (apu_dir_bidx_t){\n", i);
        printf("      .dir_rd_idx0 = 0x%x,\n", bidx1.dir_rd_idx0);
        printf("      .dir_rd_idx1 = 0x%x,\n", bidx1.dir_rd_idx1);
        printf("      .dir_rd_idx2 = 0x%x,\n", bidx1.dir_rd_idx2);
        printf("      .dir_rd_idx3 = 0x%x\n", bidx1.dir_rd_idx3);
        printf("    };\n");
    }
    printf("  }\n");

    print_fir("bf_pre_fir0_coef", apu->bf_pre_fir0_coef);
    print_fir("bf_post_fir0_coef", apu->bf_post_fir0_coef);
    print_fir("bf_pre_fir1_coef", apu->bf_pre_fir1_coef);
    print_fir("bf_post_fir1_coef", apu->bf_post_fir1_coef);


    apu_dwsz_cfg_t bf_dwsz_cfg_reg = apu->bf_dwsz_cfg_reg;

    printf("  apu->bf_dwsz_cfg_reg = (apu_dwsz_cfg_t){\n");
    printf("    .dir_dwn_siz_rate = %d, .voc_dwn_siz_rate = %d\n",
           bf_dwsz_cfg_reg.dir_dwn_siz_rate, bf_dwsz_cfg_reg.voc_dwn_siz_rate);
    printf("  };\n");

    apu_fft_cfg_t bf_fft_cfg_reg = apu->bf_fft_cfg_reg;

    printf("  apu->bf_fft_cfg_reg = (apu_fft_cfg_t){\n");
    printf("    .fft_enable = %d, .fft_shift_factor = 0x%x\n",
           bf_fft_cfg_reg.fft_enable, bf_fft_cfg_reg.fft_shift_factor);
    printf("  };\n");

    apu_int_mask_t bf_int_mask_reg = apu->bf_int_mask_reg;

    printf("  apu->bf_int_mask_reg = (apu_int_mask_t){\n");
    printf("    .dir_data_rdy_msk = %d, .voc_buf_rdy_msk = %d\n",
           bf_int_mask_reg.dir_data_rdy_msk, bf_int_mask_reg.voc_buf_rdy_msk);
    printf("  };\n");

    printf("}\n");
}

