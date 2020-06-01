/* 
 *  全部falsh 512KB 256页 每页2KB
 *  每个语音特征模板占用4KB 采用冗余模板 每个语音指令4个特征模板
 *  初步设计设定20个语音指令 共占用320KB
 *  flash最后320KB用于存储语音特征模板
 *  编译器需设置 以免存储区被代码占用
 *  烧写程序时也不能擦除存储区 选擦除需要的页
*/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "flash.h"
#include "uarths.h"
#include "MFCC.h"

// v_ftr_tag ftr_save[20 * 4];
v_ftr_tag *ftr_save = NULL;

#include <stdlib.h>
int *save_ftr_mdl_mem_init(void)
{
    if (ftr_save != NULL)
    {
        return ftr_save;
    }
    else
    {
        ftr_save = (v_ftr_tag *)malloc(20*4 * sizeof(v_ftr_tag));
        return ftr_save;
    }
    
}

uint8_t save_ftr_mdl(v_ftr_tag *ftr, uint32_t addr)
{
    //  uint32_t ftr_size;
    save_ftr_mdl_mem_init();
    addr = addr / size_per_ftr;

    if (addr > 40)
    {
        printf("flash addr error");
        return Flash_Fail;
    }
    ftr->save_sign = save_mask;
    ftr_save[addr] = *ftr;

    //  ftr_size=2*mfcc_num*ftr->frm_num;

    return Flash_Success;
}
#if 0
void init_voice_mdl(void)
{
    uint16_t i, j, comm;

    for (comm = 0; comm < 4; comm++) {
        for (j = 0; j < 4; j++) {
            ftr_save[comm*4+j].save_sign = save_mask;
            ftr_save[comm*4+j].frm_num = mdl_fram_num[comm*4+j];
//          ftr_save[comm*4+j].word_num = 2;
        }
    }
//
    for (i = 0; i < (vv_frm_max * mfcc_num); i++)
        ftr_save[0].mfcc_dat[i] = start1[i];

    for (i = 0; i < (vv_frm_max * mfcc_num); i++)
        ftr_save[1].mfcc_dat[i] = start2[i];

    for (i = 0; i < (vv_frm_max * mfcc_num); i++)
        ftr_save[2].mfcc_dat[i] = start3[i];

    for (i = 0; i < (vv_frm_max * mfcc_num); i++)
        ftr_save[3].mfcc_dat[i] = start4[i];


//
    for (i = 0; i < (vv_frm_max * mfcc_num); i++)
        ftr_save[4].mfcc_dat[i] = pause1[i];

    for (i = 0; i < (vv_frm_max * mfcc_num); i++)
        ftr_save[5].mfcc_dat[i] = pause2[i];

    for (i = 0; i < (vv_frm_max * mfcc_num); i++)
        ftr_save[6].mfcc_dat[i] = pause3[i];

    for (i = 0; i < (vv_frm_max * mfcc_num); i++)
        ftr_save[7].mfcc_dat[i] = pause4[i];

//
    for (i = 0; i < (vv_frm_max * mfcc_num); i++)
        ftr_save[8].mfcc_dat[i] = cancle1[i];

    for (i = 0; i < (vv_frm_max * mfcc_num); i++)
        ftr_save[9].mfcc_dat[i] = cancle2[i];

    for (i = 0; i < (vv_frm_max * mfcc_num); i++)
        ftr_save[10].mfcc_dat[i] = cancle3[i];

    for (i = 0; i < (vv_frm_max * mfcc_num); i++)
        ftr_save[11].mfcc_dat[i] = cancle4[i];

//
    for (i = 0; i < (vv_frm_max * mfcc_num); i++)
        ftr_save[12].mfcc_dat[i] = confirm1[i];

    for (i = 0; i < (vv_frm_max * mfcc_num); i++)
        ftr_save[13].mfcc_dat[i] = confirm2[i];

    for (i = 0; i < (vv_frm_max * mfcc_num); i++)
        ftr_save[14].mfcc_dat[i] = confirm3[i];

    for (i = 0; i < (vv_frm_max * mfcc_num); i++)
        ftr_save[15].mfcc_dat[i] = confirm4[i];
}
#endif