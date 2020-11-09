
#include "isolated_word.h"

#include "printf.h"

// 音频算法部分，单例模式，主要解决音频的背景录入与识别处理。
v_ftr_tag ftr_curr; // 当前识别结果

static uint32_t atap_tag_mid_val = 0;       //语音段中值 相当于有符号的0值 用于短时过零率计算
static uint16_t atap_tag_n_thl = 0; //噪声阈值，用于短时过零率计算
static uint16_t atap_tag_z_thl = 0;     //短时过零率阈值，超过此阈值，视为进入过渡段。
static uint32_t atap_tag_s_thl = 10000;     //短时累加和阈值，超过此阈值，视为进入过渡段。

static atap_tag atap_arg;
void iw_atap_tag(uint16_t n_thl, uint16_t z_thl, uint32_t s_thl)
{
    // printk("iw_atap_tag n_thl: %d z_thl: %d s_thl: %d > ", n_thl, z_thl, s_thl);
    atap_tag_n_thl = n_thl, atap_tag_z_thl = z_thl, atap_tag_s_thl = s_thl;
}

static IwState iw_state = Init; // 识别状态 default 0
static volatile uint32_t g_index = 0;
static volatile uint8_t i2s_start_flag = 0;
static uint16_t rx_buf[FRAME_LEN];
static uint32_t g_rx_dma_buf[FRAME_LEN * 2];
static volatile enum I2sFlag {
    NONE,
    FIRST,
    SECOND
} i2s_recv_flag = NONE;
static uint8_t i2s_device = 0, dma_channel = 2, __lr_shift = 0;

void iw_set_state(IwState state)
{
    iw_state = state;
}

IwState iw_get_state()
{
    return iw_state;
}

v_ftr_tag *iw_get_ftr()
{
    return &ftr_curr;
}

int iw_i2s_dma_irq(void *ctx)
{
    uint32_t i;
    if (i2s_start_flag)
    {
        int16_t s_tmp;
        if (g_index)
        {
            i2s_receive_data_dma(i2s_device, &g_rx_dma_buf[g_index], frame_mov * 2, dma_channel);
            g_index = 0;
            for (i = 0; i < frame_mov; i++)
            {
                s_tmp = (int16_t)(g_rx_dma_buf[2 * i + __lr_shift] & 0xffff); //g_rx_dma_buf[2 * i + 1] Low left
                rx_buf[i] = s_tmp + 32768;
            }
            i2s_recv_flag = FIRST;
        }
        else
        {
            i2s_receive_data_dma(i2s_device, &g_rx_dma_buf[0], frame_mov * 2, dma_channel);
            g_index = frame_mov * 2;
            for (i = frame_mov; i < frame_mov * 2; i++)
            {
                s_tmp = (int16_t)(g_rx_dma_buf[2 * i + __lr_shift] & 0xffff); //g_rx_dma_buf[2 * i + 1] Low left
                rx_buf[i] = s_tmp + 32768;
            }
            i2s_recv_flag = SECOND;
        }
    }
    else
    {
        i2s_receive_data_dma(i2s_device, &g_rx_dma_buf[0], frame_mov * 2, dma_channel);
        g_index = frame_mov * 2;
    }

    static uint16_t v_dat[atap_len];
    static valid_tag valid_voice[max_vc_con];
    static uint16_t frame_index;
    const uint16_t num = atap_len / frame_mov;
    switch (iw_state)
    {
    case Init:
        break;
    case Idle:
        frame_index = 0;
        iw_state = Ready;
        break;
    case Ready: // 准备中
    {
        // get record data

        // memcpy(v_dat + frame_mov * frame_index,  (i2s_recv_flag == FIRST) ? rx_buf : rx_buf + frame_mov, frame_mov);

        if (i2s_recv_flag == FIRST)
        {
            for (i = 0; i < frame_mov; i++)
                v_dat[frame_mov * frame_index + i] = rx_buf[i];
        }
        else
        {
            for (i = 0; i < frame_mov; i++)
                v_dat[frame_mov * frame_index + i] = rx_buf[i + frame_mov];
        }

        frame_index++;
        if (frame_index >= num)
        {
            iw_state = MaybeNoise;
            break;
        }
        break;
    }
    case MaybeNoise: // 噪音判断
    {
        noise_atap(v_dat, atap_len, &atap_arg);
        // printk("s_thl: %d > ", atap_arg.s_thl);
        if (atap_arg.s_thl > atap_tag_s_thl)
        {
            // printk("get noise again...\n");
            iw_state = Idle;
        }
        else
        {
            iw_state = Restrain; // into Recognizer
        }
        break;
    }
    case Restrain: // 背景噪音
    {
        if (i2s_recv_flag == FIRST)
        {
            for (i = 0; i < frame_mov; i++)
                v_dat[i + frame_mov] = rx_buf[i];
        }
        else
        {
            for (i = 0; i < frame_mov; i++)
                v_dat[i + frame_mov] = rx_buf[i + frame_mov];
        }
        iw_state = Speak;
    }
    case Speak: // 录音识别
    {
        if (i2s_recv_flag == FIRST)
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
        if (VAD2(v_dat, valid_voice, &atap_arg) == 1)
        {
            // printk("vad ok\n");
            get_mfcc(&(valid_voice[0]), &ftr_curr, &atap_arg);
            if (ftr_curr.frm_num == 0)
            {
                // printk("MFCC fail ");
                // return 0;
                iw_state = Idle;
                break;
            }
            else
            {
                // printk("MFCC ok\n");
                iw_state = Done;
                break;
            }
            break;
        }
        break;
    }
    case Done: // 完成
    {
        // iw_state = Idle; // debug
        break; // other
    }
    }
    // printk("s: %d >\r\n", iw_state);

    return 0;
}

void iw_run(i2s_device_number_t device_num, dmac_channel_number_t channel_num, uint8_t lr_shift, uint32_t priority)
{
    iw_stop();
    i2s_device = device_num;
    dma_channel = channel_num;
    __lr_shift = lr_shift;
    dmac_irq_register(channel_num, iw_i2s_dma_irq, NULL, priority);
    i2s_receive_data_dma(device_num, &g_rx_dma_buf[0], frame_mov * 2, channel_num);
    if (iw_state != Idle)
    {
        g_index = 0;
        i2s_recv_flag = NONE;
        i2s_start_flag = 1;
        iw_state = Idle;
    }
}

void iw_stop()
{
    if (iw_state != Init)
    {
        g_index = 0;
        i2s_recv_flag = NONE;
        i2s_start_flag = 0;
        iw_state = Init;
        dmac_wait_done(dma_channel);
        //sysctl_disable_irq();
        dmac_channel_disable(dma_channel);
        dmac_free_irq(dma_channel);
    }
}

#ifdef UNIT_TEST

void iw_begin()
{
    //io_mux_init
    fpioa_set_function(20, FUNC_I2S0_IN_D0);
    fpioa_set_function(18, FUNC_I2S0_SCLK);
    fpioa_set_function(19, FUNC_I2S0_WS);

    //i2s init
    i2s_init(i2s_device, I2S_RECEIVER, 0x3);

    i2s_rx_channel_config(i2s_device, I2S_CHANNEL_0,
                          RESOLUTION_16_BIT, SCLK_CYCLES_32,
                          TRIGGER_LEVEL_4, STANDARD_MODE);

    i2s_set_sample_rate(i2s_device, 8000);

    dmac_init();

    iw_run(i2s_device, dma_channel, 3);

    /* Enable the machine interrupt */
    sysctl_enable_irq();
}

#define ftr_size 10 * 4
#define save_mask 12345
v_ftr_tag ftr_save[ftr_size];

#define iw_memset() for(uint16_t i = 0; i < ftr_size; i++) memset(&ftr_save[0], 0, sizeof(ftr_save[0]))

int iw_record(uint8_t model_num)
{
    if (iw_state == Done)
    {
        if (model_num < ftr_size) {
            ftr->save_sign = save_mask;
            ftr_save[model_num] = ftr_curr;
            iw_state = Idle;
            return Done;
        }
    }
    return iw_state;
}

void iw_set_model(uint8_t model_num, const int16_t *voice_model, uint16_t frame_num)
{
    ftr_save[model_num].save_sign = save_mask;
    ftr_save[model_num].frm_num = frame_num;
    memcpy(ftr_save[model_num].mfcc_dat, voice_model, sizeof(ftr_save[model_num].mfcc_dat));
}

void iw_print_model(uint8_t model_num)
{
  printk("frm_num=%d\n", ftr_save[model_num].frm_num);
  for (int i = 0; i < (vv_frm_max * mfcc_num); i++)
  {
    if (((i + 1) % 49) == 0) // next line
      printk("%d,\n", ftr_save[model_num].mfcc_dat[i]);
    else
      printk("%d, ", ftr_save[model_num].mfcc_dat[i]);
  }
  printk("\nprint model ok!\n");
}

int iw_recognize()
{
    if (iw_state == Done)
    {
        int16_t min_comm = -1;
        uint32_t min_dis = dis_max;
        // uint32_t cycle0 = read_csr(mcycle);
        for (uint32_t ftr_num = 0; ftr_num < ftr_size; ftr_num += 1)
        {
            //  ftr_mdl=(v_ftr_tag*)ftr_num;
            v_ftr_tag *ftr_mdl = (v_ftr_tag *)(&ftr_save[ftr_num]);
            if ((ftr_mdl->save_sign) == save_mask)
            {
                printk("no. %d, ftr_mdl->frm_num %d, ", ftr_num, ftr_mdl->frm_num);
                
                uint32_t cur_dis = dtw(ftr_mdl, &ftr_curr);
                printk("cur_dis %d, ftr_curr.frm_num %d\n", cur_dis, ftr_curr.frm_num);
            
                if (cur_dis < min_dis)
                {
                    min_dis = cur_dis;
                    min_comm = ftr_num;
                    printk("min_comm: %d >\r\n", min_comm);
                }
            }
        }
        // uint32_t cycle1 = read_csr(mcycle) - cycle0;
        // printk("[INFO] recg cycle = 0x%08x\n", cycle1);
        //printk("recg end ");
        printk("min_comm: %d >\r\n", min_comm);

        iw_state = Idle;
        return Done;
    }
    return iw_state;
}

#endif
