#ifndef _HTPA_H
#define _HTPA_H

#include "stdint.h"
#include "sipeed_i2c.h"


#include "htpa_32x32d.h"

#define HTPA_WIDTH  HTPA_32X32D_WIDTH
#define HTPA_HEIGHT HTPA_32X32D_HEIGHT
#define HTPA_ADDR   HTPA_32X32D_ADDR
#define HTPA_ADDR_EEPROM   HTPA_32X32D_ADDR_EEPROM
#define HTPA_EEPROM_I2C_FREQ 400000

#define HTPA_CMD_ADDR_CONF       0x01
#define HTPA_CMD_ADDR_STATUS     0x02
#define HTPA_CMD_ADDR_TRIM1      0x03
#define HTPA_CMD_ADDR_TRIM2      0x04
#define HTPA_CMD_ADDR_TRIM3      0x05
#define HTPA_CMD_ADDR_TRIM4      0x06
#define HTPA_CMD_ADDR_TRIM5      0x07
#define HTPA_CMD_ADDR_TRIM6      0x08
#define HTPA_CMD_ADDR_TRIM7      0x09
#define HTPA_CMD_ADDR_DATA1      0x0A
#define HTPA_CMD_ADDR_DATA2      0x0B

#define HTPA_TABLE_NUMBER        118
#define HTPA_PCSCALEVAL          100000000 // 10^8
#define HTPA_NROF_TA_ELEMENTS    7
#define HTPA_NROF_AD_ELEMENTS    1595  //130 possible due to Program memory, higher values possible if NROFTAELEMENTS is decreased
#define HTPA_TAEQUIDISTANCE      100   //dK
#define HTPA_ADEQUIDISTANCE      64    //dig
#define HTPA_ADEXPBITS           6     //2^HTPA_ADEXPBITS=HTPA_ADEQUIDISTANCE
#define HTPA_TABLEOFFSET         640

extern const unsigned int table_temp[HTPA_NROF_AD_ELEMENTS][HTPA_NROF_TA_ELEMENTS];
extern const unsigned int table_x_ta_temps[HTPA_NROF_TA_ELEMENTS];
extern const unsigned int table_y_ad_values[HTPA_NROF_AD_ELEMENTS];

typedef struct{
    float pix_c_min;
    float pix_c_max;
    uint8_t grad_scale;
    uint16_t table_number;
    uint8_t  epsilon;
    uint8_t calib_mbit;
    uint8_t calib_bias;
    uint8_t calib_clk;
    uint8_t calib_bpa;
    uint8_t calib_pu;
    uint8_t array_type;
    uint16_t vdd_th1;
    uint16_t vdd_th2;
    float    ptat_grad;
    float    ptat_off;
    uint16_t ptat_th1;
    uint16_t ptat_th2;
    uint8_t vdd_sc_grad;
    uint8_t vdd_sc_off;
    int8_t  global_off;
    uint16_t global_gain;
    uint32_t device_id;
    uint8_t  nr_of_def_pix;

    uint32_t vdd_sc_grad_div;
    uint32_t vdd_sc_off_div;
    uint32_t grad_scale_div;

    // uint8_t  dead_pix_mask[12];
    // uint16_t dead_pix_addr[24];
    int16_t vdd_comp_grad[256];
    int16_t vdd_comp_off[256];
    int16_t th_grad[1024];
    int16_t th_off[1024];
    uint16_t p[1024];
}htpa_eeprom_param_t;

typedef struct{
    i2c_device_number_t   i2c_num;
    uint8_t               scl_pin;
    uint8_t               sda_pin;
    uint32_t              i2c_freq;
    uint16_t              width;
    uint16_t              height;
    uint32_t              mclk;
    float                 ta;
    float                 ptat_mean;
    float                 vdd_mean;
    uint8_t               adc_resolution;
    uint16_t             elec_off[258]; // 256
    int32_t              v[1024];
    uint8_t              temp[258];
    uint8_t              temp2[258];
    float                pix_c[1024];
    uint16_t              ptats[8];
    uint16_t              vdd[8];

    htpa_eeprom_param_t   eeprom;

    bool                 is_init;
    bool                 is_get_elec_off;
}htpa_t;




int htpa_init(htpa_t* obj, i2c_device_number_t i2c_num, uint8_t scl_pin, uint8_t sda_pin, uint32_t i2c_freq);
void htpa_destroy(htpa_t* obj);
int htpa_snapshot(htpa_t* obj, int32_t** pixels);

int htpa_read_reg(htpa_t* obj, uint8_t addr);
int htpa_write_reg(htpa_t* obj, uint8_t addr, uint8_t data);

int htpa_get_eeprom_param(htpa_t* obj);
#endif
