/*******   MFCC.C    *******/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <math.h>
#include "ADC.h"
#include "VAD.h"
#include "MFCC.h"
#include "MFCC_Arg.h"
#include <float.h>
#include "sysctl.h"
#include "dmac.h"
#include "hal_fft.h"
#include "FIR.h"

void cr4_fft_1024_stm32(void *pssOUT, void *pssIN, uint16_t Nbin);
void normalize(int16_t *mfcc_p, uint16_t frm_num);

uint32_t fft_out[mfcc_fft_point];
int16_t fft_in[mfcc_fft_point];

#define FFT_N 512
uint64_t fft_out_data[FFT_N / 2];
extern void fft_dma_init(void);
extern volatile fft_t *const fft;

void fft_input_intdata(int16_t *data, uint8_t point)
{
    uint16_t point_num = 0;
    uint16_t i;
    fft_data_t input_data;

    if (point == 0)
        point_num = 512;
    else if (point == 1)
        point_num = 256;
    else if (point == 2)
        point_num = 128;
    else if (point == 3)
        point_num = 64;
    point_num = point_num / 2;  // one time send two data

    for (i = 0; i < point_num; i++) {
        input_data.R1 = data[2 * i];
        input_data.I1 = 0;
        input_data.R2 = data[2 * i + 1];
        input_data.I2 = 0;

        fft->fft_input_fifo.fft_input_fifo = *(uint64_t *)&input_data;
    }
}

void fft_sync_data(int16_t *data, uint8_t point, fft_data_t *fft_data)
{
    uint16_t point_num = 0;
    uint16_t i;

    if (point == 0)
        point_num = 512;
    else if (point == 1)
        point_num = 256;
    else if (point == 2)
        point_num = 128;
    else if (point == 3)
        point_num = 64;
    point_num = point_num / 2;  // one time send two data

    for (i = 0; i < point_num; i++) {
        (fft_data + i)->R1 = data[2 * i];
        (fft_data + i)->I1 = 0;
        (fft_data + i)->R2 = data[2 * i + 1];
        (fft_data + i)->I2 = 0;
    }
}


/*
 *  cr4_fft_1024_stm32输入参数是有符号数
 *
 *  cr4_fft_1024_stm32输入参数包括实数和虚数
    但语音数据只包括实数部分 虚数用0填充
    fft点数超出输入数据长度时 超过部分用0填充

    cr4_fft_1024_stm32输出数据包括实数和虚数
    应该取其绝对值 即平方和的根
 */

uint32_t *mfcc_fft(int16_t *dat_buf, uint16_t buf_len)
{
    uint16_t i;
    int32_t real, imag;
    fft_data_t output_data;

    if (buf_len > mfcc_fft_point)
        return (void *)0;

    for (i = 0; i < buf_len; i++) {
        fft_in[i] =  *(dat_buf+i);//虚部高位 实部低位
    }
    for (; i < mfcc_fft_point; i++)
        fft_in[i] = 0;//超出部分用0填充

    fft_data_t fft_in_buf[512];
    memset(fft_in_buf, 0 , sizeof(fft_in_buf));
    fft_sync_data(fft_in, FFT_512, fft_in_buf);
    fft_complex_uint16_dma(DMAC_CHANNEL3, DMAC_CHANNEL4, 0x1ff, FFT_DIR_FORWARD, (uint64_t *)fft_in_buf, 512, fft_out_data);

    for (i = 0; i < frq_max / 2; i++) {
        output_data = *(fft_data_t *)&fft_out_data[i];
        imag = (int16_t)output_data.I1;
        real = (int16_t)output_data.R1;
        real = real*real+imag*imag;
        fft_out[2 * i] = sqrtf((float)real)*10;

        imag = (int16_t)output_data.I2;
        real = (int16_t)output_data.R2;

        real = real*real+imag*imag;
        fft_out[2 * i + 1] = sqrtf((float)real)*10;
    }
    return fft_out;
}

/*  MFCC：Mel频率倒谱系数
 *
    参数：
    valid   有效语音段起点终点

    返回值：
    v_ftr   MFCC值，帧数

    Mel=2595*lg(1+f/700)
    1000Hz以下按线性刻度 1000Hz以上按对数刻度
    三角型滤波器中心频率 在Mel频率刻度上等间距排列
    预加重:6dB/倍频程 一阶高通滤波器  H(z)=1-uz^(-1) y(n)=x(n)-ux(n-1) u=0.94~0.97

    MFCC 步骤：
    1.对语音信号预加重、分帧、加汉明窗处理，然后进行短时傅里叶变换，得出频谱
    2.取频谱平方，得能量谱。并用24个Mel带通滤波器进行滤波，输出Mel功率谱
    3.对每个滤波器的输出值取对数，得到相应频带的对数功率谱。然后对24个对数功率进行
      反离散余弦变换得到12个MFCC系数
*/

void get_mfcc(valid_tag *valid, v_ftr_tag *v_ftr, atap_tag *atap_arg)
{
    uint16_t *vc_dat;
    uint16_t h, i;
    uint32_t *frq_spct;          //频谱
    int16_t vc_temp[FRAME_LEN]; //语音暂存区
    int32_t temp;

    uint32_t pow_spct[tri_num];  //三角滤波器输出对数功率谱
    uint16_t frm_con;
    int16_t *mfcc_p;
    int8_t  *dct_p;
    int32_t mid;
    uint16_t v_frm_num;

    //USART1_printf("start=%d end=%d",(uint32_t)(valid->start),(uint32_t)(valid->end));
    v_frm_num = (((uint32_t)(valid->end)-(uint32_t)(valid->start))/2-FRAME_LEN)/(FRAME_LEN-frame_mov)+1;
    if (v_frm_num > vv_frm_max) {
        // printf("frm_num=%d ", v_frm_num);
        v_ftr->frm_num = 0;
    } else {
        mid = (int32_t)atap_arg->mid_val;
        mfcc_p = v_ftr->mfcc_dat;
        frm_con = 0;
        //low pass filter
    //  for (vc_dat = (uint16_t *)(valid->start); vc_dat <= ((uint16_t *)(valid->end-FRAME_LEN)); vc_dat += 1) {
    //      *vc_dat = (uint16_t)(Fir(*vc_dat));

    //  }
        for (vc_dat = (uint16_t *)(valid->start); vc_dat <= ((uint16_t *)(valid->end-FRAME_LEN)); vc_dat += (FRAME_LEN-frame_mov)) {
            for (i = 0; i < FRAME_LEN; i++) {
                //预加重
            //  printf("vc_dat[%d]=%d ",i,((int32_t)(*(vc_dat+i))-mid));
                temp = ((int32_t)(*(vc_dat+i))-mid) - hp_ratio(((int32_t)(*(vc_dat+i-1))-mid));
            //  printf("vc_hp[%d]=%d ",i,temp);
                //加汉明窗 并放大10倍
                vc_temp[i] = (int16_t)(temp*hamm[i]/(hamm_top/10));
            //  printf("vc_hm[%d]=%d\r\n",i,vc_temp[i]);
            }

            frq_spct = mfcc_fft(vc_temp, FRAME_LEN);

            for (i = 0; i < frq_max; i++) {
                //printf("frq_spct[%d]=%d ",i,frq_spct[i]);
                frq_spct[i] *= frq_spct[i];//能量谱
                //printf("E_spct[%d]=%d\r\n",i,frq_spct[i]);
            }

            //加三角滤波器
            pow_spct[0] = 0;
            for (i = 0; i < tri_cen[1]; i++)
                pow_spct[0] += (frq_spct[i]*tri_even[i]/(tri_top/10));
            for (h = 2; h < tri_num; h += 2) {
                pow_spct[h] = 0;
                for (i = tri_cen[h-1]; i < tri_cen[h+1]; i++)
                    pow_spct[h] += (frq_spct[i]*tri_even[i]/(tri_top/10));
            }

            for (h = 1; h < (tri_num-2); h += 2) {
                pow_spct[h] = 0;
                for (i = tri_cen[h-1]; i < tri_cen[h+1]; i++)
                    pow_spct[h] += (frq_spct[i]*tri_odd[i]/(tri_top/10));
            }
            pow_spct[tri_num-1] = 0;
            for (i = tri_cen[tri_num-2]; i < (mfcc_fft_point/2); i++)
                pow_spct[tri_num-1] += (frq_spct[i]*tri_odd[i]/(tri_top/10));

            //三角滤波器输出取对数
            for (h = 0; h < tri_num; h++) {
                //USART1_printf("pow_spct[%d]=%d ",h,pow_spct[h]);
                pow_spct[h] = (uint32_t)(log(pow_spct[h])*100);//取对数后 乘100 提升数据有效位数
                //USART1_printf("%d\r\n",pow_spct[h]);
            }

            //反离散余弦变换
            dct_p = (int8_t *)dct_arg;
            for (h = 0; h < mfcc_num; h++) {
                mfcc_p[h] = 0;
                for (i = 0; i < tri_num; i++)
                    mfcc_p[h] += (((int32_t)pow_spct[i])*((int32_t)dct_p[i])/100);
                //printf("%d,",mfcc_p[h]);
                dct_p += tri_num;
            }
            //USART1_printf("\r\n");
            mfcc_p += mfcc_num;
            frm_con++;
        }
        mfcc_p = v_ftr->mfcc_dat;
        normalize(mfcc_p, frm_con);
        v_ftr->frm_num = frm_con;
    }
}

int16_t avg(int16_t *mfcc_p, uint16_t frm_num)
{
    int i, j;
    double sum = 0.0f;

    // printf("frm_num = %d, mfcc_num = %d\n", frm_num, mfcc_num);
    for (i = 0; i < frm_num; i++)
        for (j = 0; j < mfcc_num; j++) {
            sum += mfcc_p[i * mfcc_num + j];
//          printf("[%d, %d]%f %f ", i, j, sum, mfcc_p[i * mfcc_num + j]);
        }
    return (int16_t)(sum / (frm_num * mfcc_num));
}

int16_t stdev(int16_t *mfcc_p, int16_t avg1, uint16_t frm_num)
{
    int i, j;
    double sum = 0.0f;

    for (i = 0; i < frm_num; i++)
        for (j = 0; j < mfcc_num; j++)
            sum += (mfcc_p[i * mfcc_num + j] - avg1) * (mfcc_p[i * mfcc_num + j] - avg1);
    return (int16_t)(sqrt(sum / (frm_num * mfcc_num)));
}

void normalize(int16_t *mfcc_p, uint16_t frm_num)
{
    int i, j;
    int16_t avg1 = avg(mfcc_p, frm_num);
    int16_t stdev1 = stdev(mfcc_p, avg1, frm_num);

    // printf("avg1 = %d, stdev1 = %d\n", avg1, stdev1);
    for (i = 0; i < frm_num; i++)
        for (j = 0; j < mfcc_num; j++)
            mfcc_p[i * mfcc_num + j] = (mfcc_p[i * mfcc_num + j] - avg1) * 100 / stdev1;
}
