#include "speech_recognizer.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "syslog.h"
#include "sysctl.h"

#include "dmac.h"
#include "plic.h"
#include "uarths.h"
#include "i2s.h"
#include "Maix_i2s.h"
#include "fpioa.h"

#include "VAD.h"
#include "MFCC.h"
#include "DTW.h"
#include "flash.h"
#include "ADC.h"
#include "ide_dbg.h"

#define SR_THREAD 1
#if SR_THREAD
/*****freeRTOS****/
#if MICROPY_PY_THREAD
#include "FreeRTOS.h"
#include "task.h"
#endif

#if MICROPY_PY_THREAD
#define SR_TASK_PRIORITY 4
#define SR_TASK_STACK_SIZE (10 * 1024)
#define SR_TASK_STACK_LEN (SR_TASK_STACK_SIZE / sizeof(StackType_t))
TaskHandle_t sr_task_handle;
#endif

#endif

extern v_ftr_tag *ftr_save;

extern volatile bool ide_get_script_status();
uint16_t VcBuf[atap_len];
atap_tag atap_arg;
valid_tag valid_voice[max_vc_con];
v_ftr_tag ftr;
v_ftr_tag ftr_temp;
v_ftr_tag ftr_mdl_temp[10];
v_ftr_tag *pftr_mdl_temp[10];

#define save_ok 0
#define VAD_fail 1
#define MFCC_fail 2
#define Flash_fail 3

#define FFT_N 512

uint16_t rx_buf[FRAME_LEN];
uint32_t g_rx_dma_buf[FRAME_LEN * 2];
uint64_t fft_out_data[FFT_N / 2];

volatile uint32_t g_index;
volatile uint8_t uart_rec_flag;
volatile uint32_t receive_char;
volatile uint8_t i2s_rec_flag;
volatile uint8_t i2s_start_flag = 0;
atap_tag user_atap_arg = {0, 0, 0, 10000};

sr_status_t sr_status;
int sr_record_result = -1;
int sr_recognizer_result = -1;

/**
 * sr_action: 0 nothing
 * 1 need record
 * 2 need recognizer
 */
uint8_t sr_action = 0;
uint32_t sr_record_addr = 0;
uint8_t comm;
static const char *TAG = "SpeechRecognizer";

uint8_t speech_recognizer_save_mdl(uint16_t *v_dat, uint32_t addr);
uint8_t speech_recognizer_spch_recg(uint16_t *v_dat, uint32_t *mtch_dis);

sr_status_t speech_recognizer_get_status(void)
{
    return sr_status;
}

uint8_t speech_recognizer_set_Threshold(uint16_t n_thl, uint16_t z_thl, uint32_t s_thl)
{
    // user_atap_arg.n_thl = n_thl;
    // user_atap_arg.z_thl = z_thl;
    user_atap_arg.s_thl = s_thl;
    return 0;
}

int i2s_dma_irq(void *ctx)
{
    i2s_device_number_t *i2s_num = (i2s_device_number_t *)ctx;
    uint32_t i;
    if (i2s_start_flag)
    {
        int16_t s_tmp;
        if (g_index)
        {
            i2s_receive_data_dma(*i2s_num, &g_rx_dma_buf[g_index], frame_mov * 2, DMAC_CHANNEL3);
            g_index = 0;
            for (i = 0; i < frame_mov; i++)
            {
                s_tmp = (int16_t)(g_rx_dma_buf[2 * i] & 0xffff); //g_rx_dma_buf[2 * i + 1] Low left
                rx_buf[i] = s_tmp + 32768;
            }
            i2s_rec_flag = 1;
        }
        else
        {
            i2s_receive_data_dma(*i2s_num, &g_rx_dma_buf[0], frame_mov * 2, DMAC_CHANNEL3);
            g_index = frame_mov * 2;
            for (i = frame_mov; i < frame_mov * 2; i++)
            {
                s_tmp = (int16_t)(g_rx_dma_buf[2 * i] & 0xffff); //g_rx_dma_buf[2 * i + 1] Low left
                rx_buf[i] = s_tmp + 32768;
            }
            i2s_rec_flag = 2;
        }
    }
    else
    {
        i2s_receive_data_dma(*i2s_num, &g_rx_dma_buf[0], frame_mov * 2, DMAC_CHANNEL3);
        g_index = frame_mov * 2;
    }
    return 0;
}

#if SR_THREAD
#if MICROPY_PY_THREAD

void sr_task(void *arg)
{
    uint8_t res;
    uint32_t dis;
    mp_printf(&mp_plat_print, "[MaixPy] sr_task start\n");
    sr_status = SR_NONE;
    while (1)
    {
        if (sr_action == 0)
        {
            vTaskDelay(50 / portTICK_PERIOD_MS);
            sr_status = SR_NONE;
            // mp_printf(&mp_plat_print, "[MaixPy] sr_task runing...\n");
            if (ide_get_script_status() == false)
            {
                if (sr_task_handle != NULL)
                {
                    vTaskDelete(sr_task_handle);
                }
            }
        }
        else if (sr_action == 1)
        {
            sr_status = SR_RECORD_WAIT_SPEACKING;
            // mp_printf(&mp_plat_print, "[MaixPy] SR_RECORD_WAIT_SPEACKING...\n");
            if (speech_recognizer_save_mdl(VcBuf, sr_record_addr) == save_ok)
            {
                sr_record_result = 0;
            }
            else
            {
                sr_record_result = -3;
                sr_action = 0;
            }
            // mp_printf(&mp_plat_print, "[MaixPy] SR_RECORD_SUCCESSFUL\n");
            sr_status = SR_RECORD_SUCCESSFUL;
            vTaskSuspend(sr_task_handle);
        }
        else if (sr_action == 2)
        {
            // mp_printf(&mp_plat_print, "[MaixPy] SR_RECOGNIZER_WAIT_SPEACKING...\n");
            sr_status = SR_RECOGNIZER_WAIT_SPEACKING;
            res = speech_recognizer_spch_recg(VcBuf, &dis);
            if (dis != dis_err)
                sr_recognizer_result = res;
            else
                sr_recognizer_result = -1;
            sr_action = 0;
            sr_status = SR_RECOGNIZER_SUCCESSFULL;
            vTaskSuspend(sr_task_handle);
            // mp_printf(&mp_plat_print, "[MaixPy] SR_RECOGNIZER_SUCCESSFULL\n");
        }
    }
}

int speech_recognizer_finish(void)
{
    if (sr_task_handle != NULL)
    {
        vTaskDelete(sr_task_handle);
    }
    return 0;
}

#endif
#endif
int speech_recognizer_get_result(void)
{
    int ret = sr_recognizer_result;
    sr_recognizer_result = -1;
    return ret;
}

int speech_recognizer_init(Maix_i2s_obj_t *dev)
{

    dmac_init();
    dmac_set_irq(DMAC_CHANNEL3, i2s_dma_irq, (void *)dev->i2s_num, 3);
    i2s_receive_data_dma(dev->i2s_num, &g_rx_dma_buf[0], frame_mov * 2, DMAC_CHANNEL3);

    /* Enable the machine interrupt */
    sysctl_enable_irq();
#if SR_THREAD
#if MICROPY_PY_THREAD
    xTaskCreateAtProcessor(0,                 // processor
                           sr_task,           // function entry
                           "sr_task",         //task name
                           SR_TASK_STACK_LEN, //stack_deepth
                           NULL,              //function arg
                           SR_TASK_PRIORITY,  //task priority
                           &sr_task_handle);  //task handl
    vTaskSuspend(sr_task_handle);
#endif
#endif

    return 0;
}

int speech_recognizer_record(uint8_t keyword_num, uint8_t model_num)
{
    if (keyword_num > 10)
        return -1;
    if (model_num > 4)
        return -2;
    sr_action = 1;

    comm = keyword_num;
    uint8_t prc_count = model_num;
    uint32_t addr = 0;

    g_index = 0;
    i2s_rec_flag = 0;
    i2s_start_flag = 1;

    addr = ftr_start_addr + comm * size_per_comm + prc_count * size_per_ftr;
#if MICROPY_PY_THREAD
    sr_record_addr = addr;
    vTaskResume(sr_task_handle);
#else
    if (speech_recognizer_save_mdl(VcBuf, addr) == save_ok)
    {
        return 0;
    }
    else
    {
        return -3;
    }
#endif
    return 0;
}


int speech_recognizer_recognize(void)
{
#if !MICROPY_PY_THREAD
    uint8_t res;
    uint32_t dis;
#endif

    g_index = 0;
    i2s_rec_flag = 0;
    i2s_start_flag = 1;
    sr_action = 2;
#if MICROPY_PY_THREAD
    vTaskResume(sr_task_handle);
    return 0;
#else
    res = speech_recognizer_spch_recg(VcBuf, &dis);
    if (dis != dis_err)
        return res;
    else
        return -1;
#endif
}

int speech_recognizer_add_voice_model(uint8_t keyword_num, uint8_t model_num, const int16_t *voice_model, uint16_t frame_num)
{
    ftr_save[keyword_num * 4 + model_num].save_sign = save_mask;
    ftr_save[keyword_num * 4 + model_num].frm_num = frame_num;
    save_ftr_mdl_mem_init();
    for (int i = 0; i < (vv_frm_max * mfcc_num); i++)
        ftr_save[keyword_num * 4 + model_num].mfcc_dat[i] = voice_model[i];
    return 0;
}

int speech_recognizer_get_data(uint8_t keyword_num, uint8_t model_num, uint16_t *frm_num, int16_t **voice_model, uint32_t *voice_model_len)
{
    *frm_num = ftr_save[keyword_num * 4 + model_num].frm_num;
    *voice_model = ftr_save[keyword_num * 4 + model_num].mfcc_dat;
    *voice_model_len = vv_frm_max * mfcc_num;
    return 0;
}

uint8_t speech_recognizer_save_mdl(uint16_t *v_dat, uint32_t addr)
{
    uint16_t i, num;
    uint16_t frame_index;
get_noise1:
    frame_index = 0;
    num = atap_len / frame_mov;
    //wait for finish
    while (1)
    {
        while (i2s_rec_flag == 0)
        {
            if (ide_get_script_status() == false)
                return 0;
        }
        if (i2s_rec_flag == 1)
        {
            for (i = 0; i < frame_mov; i++)
                v_dat[frame_mov * frame_index + i] = rx_buf[i];
        }
        else
        {
            for (i = 0; i < frame_mov; i++)
                v_dat[frame_mov * frame_index + i] = rx_buf[i + frame_mov];
        }
        i2s_rec_flag = 0;
        frame_index++;
        if (frame_index >= num)
            break;
    }

    noise_atap(v_dat, atap_len, &atap_arg);
    if (atap_arg.s_thl > user_atap_arg.s_thl)
    {
        sr_status = SR_GET_NOISEING;
        mp_printf(&mp_plat_print, "[MaixPy] get noise again...\n");
        goto get_noise1;
    }
    sr_status = SR_RECORD_WAIT_SPEACKING;
    mp_printf(&mp_plat_print, "[MaixPy] Please speaking...\n");
    //wait for finish
    while (i2s_rec_flag == 0)
    {
        if (ide_get_script_status() == false)
            return 0;
    }
    if (i2s_rec_flag == 1)
    {
        for (i = 0; i < frame_mov; i++)
            v_dat[i + frame_mov] = rx_buf[i];
    }
    else
    {
        for (i = 0; i < frame_mov; i++)
            v_dat[i + frame_mov] = rx_buf[i + frame_mov];
    }
    i2s_rec_flag = 0;
    while (1)
    {
        while (i2s_rec_flag == 0)
        {
            if (ide_get_script_status() == false)
                return 0;
        }
        if (i2s_rec_flag == 1)
        {
            for (i = 0; i < frame_mov; i++)
            {
                v_dat[i] = v_dat[i + frame_mov];
                v_dat[i + frame_mov] = rx_buf[i];
            }
        }
        else
        {
            for (i = 0; i < frame_mov; i++)
            {
                v_dat[i] = v_dat[i + frame_mov];
                v_dat[i + frame_mov] = rx_buf[i + frame_mov];
            }
        }
        i2s_rec_flag = 0;
        if (VAD2(v_dat, valid_voice, &atap_arg) == 1)
            break;
        if (receive_char == 's')
            return MFCC_fail;
    }
    //  if (valid_voice[0].end == ((void *)0)) {
    //      LOGI(TAG, "VAD_fail\n");
    //      return VAD_fail;
    //  }

    get_mfcc(&(valid_voice[0]), &ftr, &atap_arg);
    if (ftr.frm_num == 0)
    {
        //LOGI(TAG, "MFCC_fail\n");
        return MFCC_fail;
    }
    //  ftr.word_num = valid_voice[0].word_num;
    return save_ftr_mdl(&ftr, addr);
    //  ftr_mdl_temp[addr] = ftr;
    //  return save_ok;
}

uint8_t speech_recognizer_spch_recg(uint16_t *v_dat, uint32_t *mtch_dis)
{
    uint16_t i;
    uint32_t ftr_addr;
    uint32_t min_dis;
    uint16_t min_comm;
    uint32_t cur_dis;
    v_ftr_tag *ftr_mdl;
    uint16_t num;
    uint16_t frame_index;
    uint32_t cycle0, cycle1;

get_noise2:
    frame_index = 0;
    num = atap_len / frame_mov;
    //wait for finish
    i2s_rec_flag = 0;
    while (1)
    {
        while (i2s_rec_flag == 0)
        {
            if (ide_get_script_status() == false)
                return 0;
        }
        if (i2s_rec_flag == 1)
        {
            for (i = 0; i < frame_mov; i++)
                v_dat[frame_mov * frame_index + i] = rx_buf[i];
        }
        else
        {
            for (i = 0; i < frame_mov; i++)
                v_dat[frame_mov * frame_index + i] = rx_buf[i + frame_mov];
        }
        i2s_rec_flag = 0;
        frame_index++;
        if (frame_index >= num)
            break;
    }
    noise_atap(v_dat, atap_len, &atap_arg);
    if (atap_arg.s_thl > user_atap_arg.s_thl)
    {
        sr_status = SR_GET_NOISEING;
        mp_printf(&mp_plat_print, "[MaixPy] get noise again...\n");
        goto get_noise2;
    }
    sr_status = SR_RECOGNIZER_WAIT_SPEACKING;
    mp_printf(&mp_plat_print, "[MaixPy] Please speaking...\n");

    //wait for finish
    while (i2s_rec_flag == 0)
    {
        if (ide_get_script_status() == false)
            return 0;
    }
    if (i2s_rec_flag == 1)
    {
        for (i = 0; i < frame_mov; i++)
            v_dat[i + frame_mov] = rx_buf[i];
    }
    else
    {
        for (i = 0; i < frame_mov; i++)
            v_dat[i + frame_mov] = rx_buf[i + frame_mov];
    }
    i2s_rec_flag = 0;
    while (1)
    {
        while (i2s_rec_flag == 0)
        {
            if (ide_get_script_status() == false)
                return 0;
        }
        if (i2s_rec_flag == 1)
        {
            for (i = 0; i < frame_mov; i++)
            {
                v_dat[i] = v_dat[i + frame_mov];
                v_dat[i + frame_mov] = rx_buf[i];
            }
        }
        else
        {
            for (i = 0; i < frame_mov; i++)
            {
                v_dat[i] = v_dat[i + frame_mov];
                v_dat[i + frame_mov] = rx_buf[i + frame_mov];
            }
        }
        i2s_rec_flag = 0;
        if (VAD2(v_dat, valid_voice, &atap_arg) == 1)
            break;
        if (receive_char == 's')
        {
            *mtch_dis = dis_err;
            LOGI(TAG, "send 'c' to start\n");
            return 0;
        }
    }
    // LOGI(TAG, "vad ok\n");
    //  if (valid_voice[0].end == ((void *)0)) {
    //      *mtch_dis=dis_err;
    //      USART1_LOGI(TAG, "VAD fail ");
    //      return (void *)0;
    //  }

    get_mfcc(&(valid_voice[0]), &ftr, &atap_arg);
    if (ftr.frm_num == 0)
    {
        *mtch_dis = dis_err;
        LOGI(TAG, "MFCC fail ");
        return 0;
    }
    //  for (i = 0; i < ftr.frm_num * mfcc_num; i++) {
    //      if (i % 12 == 0)
    //          LOGI(TAG, "\n");
    //      LOGI(TAG, "%d ", ftr.mfcc_dat[i]);
    //  }
    //  ftr.word_num = valid_voice[0].word_num;
    LOGI(TAG, "MFCC ok\n");
    i = 0;
    min_comm = 0;
    min_dis = dis_max;
    cycle0 = read_csr(mcycle);
    for (ftr_addr = ftr_start_addr; ftr_addr < ftr_end_addr; ftr_addr += size_per_ftr)
    {
        //  ftr_mdl=(v_ftr_tag*)ftr_addr;
        ftr_mdl = (v_ftr_tag *)(&ftr_save[ftr_addr / size_per_ftr]);
        cur_dis = ((ftr_mdl->save_sign) == save_mask) ? dtw(ftr_mdl, &ftr) : dis_err;
        if ((ftr_mdl->save_sign) == save_mask)
        {
            mp_printf(&mp_plat_print, "[MaixPy] no. %d, frm_num = %d, save_mask=%d\r\n", i + 1, ftr_mdl->frm_num, ftr_mdl->save_sign);
            // LOGI(TAG, "cur_dis=%d\n", cur_dis);
        }
        if (cur_dis < min_dis)
        {
            min_dis = cur_dis;
            min_comm = i + 1;
        }
        i++;
    }
    cycle1 = read_csr(mcycle) - cycle0;
    // mp_printf(&mp_plat_print, "[MaixPy] recg cycle = 0x%08x\r\n", cycle1);
    if (min_comm % 4)
        min_comm = min_comm / ftr_per_comm + 1;
    else
        min_comm = min_comm / ftr_per_comm;

    *mtch_dis = min_dis;
    return (int)min_comm; //(commstr[min_comm].intst_tr);
}