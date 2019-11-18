

#include "htpa.h"
#include "sipeed_i2c.h"
#include "fpioa.h"
#include "errno.h"
#include "sleep.h" //TODO: optimize for multiple thread
#include "stdlib.h"

#define HTPA_DEBUG 0
#define HTPA_DEBUG_TIME 1

#if HTPA_DEBUG
    #include "printf.h"
    extern uint64_t mp_hal_ticks_ms(void);
#else
    #define HTPA_DEBUG_TIME 0
#endif // HTPA_DEBUG


int htpa_get_pixels(htpa_t* obj, bool is_get_vdd);

static inline float mean(uint16_t* data, int len)
{
    int sum = 0;
    for(int i=0; i<len; ++i)
    {
        sum += data[i];
    }
    return (float)sum/len;
} 

static inline uint32_t pow_2(uint32_t b)
{
    return 2<<(b-1);
}


static bool htpa_malloc(htpa_t* obj)
{
    // obj->eeprom.vdd_comp_grad = (int16_t*)malloc(256*2);
    // if(!obj->eeprom.vdd_comp_grad)
    //     return false;
    // obj->eeprom.vdd_comp_off  = (int16_t*)malloc(256*2);
    // if(!obj->eeprom.vdd_comp_off){
    //     free(obj->eeprom.vdd_comp_grad);
    //     return false;   
    // }
    // obj->eeprom.th_grad       = (int16_t*)malloc(1024*2);
    // if(!obj->eeprom.th_grad){
    //     free(obj->eeprom.vdd_comp_grad);
    //     free(obj->eeprom.vdd_comp_off);
    //     return false;   
    // }
    // obj->eeprom.th_off        = (int16_t*)malloc(1024*2);
    // if(!obj->eeprom.th_off){
    //     free(obj->eeprom.vdd_comp_grad);
    //     free(obj->eeprom.vdd_comp_off);
    //     free(obj->eeprom.th_grad);
    //     return false;   
    // }
    // obj->eeprom.p             = (uint16_t*)malloc(1024*2);
    // if(!obj->eeprom.p){
    //     free(obj->eeprom.vdd_comp_grad);
    //     free(obj->eeprom.vdd_comp_off);
    //     free(obj->eeprom.th_grad);
    //     free(obj->eeprom.th_off);
    //     return false;   
    // }
    // obj->elec_off = (uint16_t*)malloc(258*2);
    // if(!obj->elec_off){
    //     free(obj->eeprom.vdd_comp_grad);
    //     free(obj->eeprom.vdd_comp_off);
    //     free(obj->eeprom.th_grad);
    //     free(obj->eeprom.th_off);
    //     free(obj->eeprom.p);
    //     return false;   
    // }
    // obj->temp = (uint8_t*)malloc(258);
    // if(!obj->temp){
    //     free(obj->eeprom.vdd_comp_grad);
    //     free(obj->eeprom.vdd_comp_off);
    //     free(obj->eeprom.th_grad);
    //     free(obj->eeprom.th_off);
    //     free(obj->eeprom.p);
    //     free(obj->elec_off);
    //     return false;   
    // }
    // obj->pixels = (uint16_t*)malloc(1024*2);
    // if(!obj->pixels){
    //     free(obj->eeprom.vdd_comp_grad);
    //     free(obj->eeprom.vdd_comp_off);
    //     free(obj->eeprom.th_grad);
    //     free(obj->eeprom.th_off);
    //     free(obj->eeprom.p);
    //     free(obj->elec_off);
    //     free(obj->temp);
    //     return false;   
    // }
    // obj->pix_c = (float*)malloc(1024*sizeof(float));
    // if(!obj->pix_c){
    //     free(obj->eeprom.vdd_comp_grad);
    //     free(obj->eeprom.vdd_comp_off);
    //     free(obj->eeprom.th_grad);
    //     free(obj->eeprom.th_off);
    //     free(obj->eeprom.p);
    //     free(obj->elec_off);
    //     free(obj->temp);
    //     free(obj->pixels);
    //     return false;   
    // }
    // obj->v = (int32_t*)malloc(1024*sizeof(int32_t));
    // if(!obj->v){
    //     free(obj->eeprom.vdd_comp_grad);
    //     free(obj->eeprom.vdd_comp_off);
    //     free(obj->eeprom.th_grad);
    //     free(obj->eeprom.th_off);
    //     free(obj->eeprom.p);
    //     free(obj->elec_off);
    //     free(obj->temp);
    //     free(obj->pixels);
    //     free(obj->pix_c);
    //     return false;   
    // }
    return true;
}

#if HTPA_DEBUG
static void htpa_print_eeprom(htpa_t* obj)
{
    printk("PixCmin:%d, PixCmac:%d\r\n", (int)obj->eeprom.pix_c_min, (int)obj->eeprom.pix_c_max);
    printk("GradScale:%d\r\n", obj->eeprom.grad_scale);
    printk("TableNumber:%d\r\n", obj->eeprom.table_number);
    printk("epsilon:%d\r\n", obj->eeprom.epsilon);
    printk("calib MBIT:%d, BIAS:%d, CLK:%d, BPA:%d, PU:%d\r\n", obj->eeprom.calib_mbit, obj->eeprom.calib_bias, obj->eeprom.calib_clk, obj->eeprom.calib_bpa, obj->eeprom.calib_pu);
    printk("Array type:%d\r\n", obj->eeprom.array_type);
    printk("VddTH1:%d\r\n", obj->eeprom.vdd_th1);
    printk("VddTH2:%d\r\n", obj->eeprom.vdd_th2);
    printk("PTAT_grad:%d\r\n", (int)obj->eeprom.ptat_grad);
    printk("PTAT_off:%d\r\n", (int)obj->eeprom.ptat_off);
    printk("PTAT_TH1:%d\r\n", obj->eeprom.ptat_th1);
    printk("PTAT_TH2:%d\r\n", obj->eeprom.ptat_th2);
    printk("VddScGrad:%d\r\n", obj->eeprom.vdd_sc_grad);
    printk("VddScOff:%d\r\n", obj->eeprom.vdd_sc_off);
    printk("GlobalOff:%d\r\n", obj->eeprom.global_off);
    printk("obj->eeprom.global_gain:%d\r\n", obj->eeprom.global_gain);
    printk("DeviceID:%08X\r\n", obj->eeprom.device_id);
    printk("NrOfDefPix:%d\r\n", obj->eeprom.nr_of_def_pix);
}
#endif //HTPA_DEBUG

int htpa_read_reg(htpa_t* obj, uint8_t addr)
{
    if(!obj->is_init)
        return -EPERM;
    uint8_t temp[2] = {addr, 0};
    int ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, temp, 1, temp+1 , 1, 100);
    if(ret < 0)
        return ret;
    return (int)temp[1];    
}

int htpa_write_reg(htpa_t* obj, uint8_t addr, uint8_t data)
{
    if(!obj->is_init)
        return 0xFF;
    uint8_t temp[2] = {addr, data};
    int ret = maix_i2c_send_data(obj->i2c_num, HTPA_ADDR, temp, 2, 100);
    if(ret < 0){
        return ret;
    }
    return 0; 
}

int htpa_init(htpa_t* obj, i2c_device_number_t i2c_num, uint8_t scl_pin, uint8_t sda_pin, uint32_t i2c_freq)
{
    obj->width = HTPA_WIDTH;
    obj->height = HTPA_HEIGHT;
    obj->i2c_num = i2c_num;
    obj->scl_pin = scl_pin;
    obj->sda_pin = sda_pin;
    obj->i2c_freq = i2c_freq;
    if(!htpa_malloc(obj))
        return ENOMEM;
    fpioa_set_function(obj->scl_pin, FUNC_I2C0_SCLK + obj->i2c_num * 2);
    fpioa_set_function(obj->sda_pin, FUNC_I2C0_SDA + obj->i2c_num * 2);
    maix_i2c_init(obj->i2c_num, 7, HTPA_EEPROM_I2C_FREQ);
    uint8_t temp[2] = {HTPA_CMD_ADDR_CONF, 0x01}; // status register
    int ret = maix_i2c_send_data(obj->i2c_num, HTPA_ADDR, temp, 1, 100);
    if (ret != 0) {
        return ENOENT;
    }
    temp[0] = HTPA_CMD_ADDR_CONF;
    temp[1] = 0x01;
    ret = maix_i2c_send_data(obj->i2c_num, HTPA_ADDR, temp, 2, 100);
    if(ret < 0){
        return EIO;
    }
    msleep(5);
    ret = htpa_get_eeprom_param(obj);
    if(ret != 0){
        return EIO;
    }
    #if HTPA_DEBUG
        htpa_print_eeprom(obj);
    #endif //HTPA_DEBUG

    maix_i2c_init(obj->i2c_num, 7, obj->i2c_freq);

    temp[0] = HTPA_CMD_ADDR_TRIM1;
    temp[1] = obj->eeprom.calib_mbit;
    ret = maix_i2c_send_data(obj->i2c_num, HTPA_ADDR, temp, 2, 100);
    if(ret < 0){
        return EIO;
    }
    obj->adc_resolution = (obj->eeprom.calib_mbit & 0x0F) + 4;
    msleep(5);
    temp[0] = HTPA_CMD_ADDR_TRIM2;
    temp[1] = obj->eeprom.calib_bias;
    ret = maix_i2c_send_data(obj->i2c_num, HTPA_ADDR, temp, 2, 100);
    if(ret < 0){
        return EIO;
    }
    msleep(5);
    temp[0] = HTPA_CMD_ADDR_TRIM3;
    temp[1] = obj->eeprom.calib_bias;
    ret = maix_i2c_send_data(obj->i2c_num, HTPA_ADDR, temp, 2, 100);
    if(ret < 0){
        return EIO;
    }
    msleep(5);
    temp[0] = HTPA_CMD_ADDR_TRIM5;
    temp[1] = obj->eeprom.calib_bpa;
    ret = maix_i2c_send_data(obj->i2c_num, HTPA_ADDR, temp, 2, 100);
    if(ret < 0){
        return EIO;
    }
    msleep(5);
    temp[0] = HTPA_CMD_ADDR_TRIM6;
    temp[1] = obj->eeprom.calib_bpa;
    ret = maix_i2c_send_data(obj->i2c_num, HTPA_ADDR, temp, 2, 100);
    if(ret < 0){
        return EIO;
    }
    msleep(5);
    temp[0] = HTPA_CMD_ADDR_TRIM7;
    temp[1] = obj->eeprom.calib_pu;
    ret = maix_i2c_send_data(obj->i2c_num, HTPA_ADDR, temp, 2, 100);
    if(ret < 0){
        return EIO;
    }
    msleep(5);
    temp[0] = HTPA_CMD_ADDR_TRIM4;
    temp[1] = obj->eeprom.calib_clk;
    ret = maix_i2c_send_data(obj->i2c_num, HTPA_ADDR, temp, 2, 100);
    if(ret < 0){
        return EIO;
    }
    obj->mclk = 190476 * obj->eeprom.calib_clk + 1000000;
    msleep(5);

    // read data for first time usage
    ret = htpa_get_pixels(obj, false);
    if(ret != 0)
        return ret;
    ret = htpa_get_pixels(obj, true);
    if(ret != 0)
        return ret;
    obj->is_init = true;
    return 0;
}

int htpa_get_eeprom_param(htpa_t* obj)
{
    int ret;
    uint16_t addr;
    uint8_t send[2] = {0x03, 0x40};
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)obj->eeprom.vdd_comp_grad, 512, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x05;
    send[1] = 0x40;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)obj->eeprom.vdd_comp_off, 512, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x07;
    send[1] = 0x40;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)obj->eeprom.th_grad, 1024, 100);
    if(ret < 0)
        return ret;
    for(uint8_t i=0; i<obj->height/2; ++i)
    {
        addr = 0x0F40 - 64*(i+1);
        send[0] = ((uint8_t*)&addr)[1];
        send[1] = ((uint8_t*)&addr)[0];
        ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, ((uint8_t*)obj->eeprom.th_grad)+1024+64*i, 64, 100);
        if(ret < 0)
            return ret;
    }
    send[0] = 0x0F;
    send[1] = 0x40;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)obj->eeprom.th_off, 1024, 100);
    if(ret < 0)
        return ret;
    for(uint8_t i=0; i<obj->height/2; ++i)
    {
        addr = 0x1740 - 64*(i+1);
        send[0] = ((uint8_t*)&addr)[1];
        send[1] = ((uint8_t*)&addr)[0];
        ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, ((uint8_t*)obj->eeprom.th_off)+1024+64*i, 64, 100);
        if(ret < 0)
            return ret;
    }
    send[0] = 0x17;
    send[1] = 0x40;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)obj->eeprom.p, 1024, 100);
    if(ret < 0)
        return ret;
    for(uint8_t i=0; i<obj->height/2; ++i)
    {
        addr = 0x1F40 - 64*(i+1);
        send[0] = ((uint8_t*)&addr)[1];
        send[1] = ((uint8_t*)&addr)[0];
        ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, ((uint8_t*)obj->eeprom.p)+1024+64*i, 64, 100);
        if(ret < 0)
            return ret;
    }
    send[0] = 0x00;
    send[1] = 0x00;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.pix_c_min, 4, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x04;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.pix_c_max, 4, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x08;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.grad_scale, 1, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x0B;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.table_number, 2, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x0D;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.epsilon, 1, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x1A;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.calib_mbit, 1, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x1B;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.calib_bias, 1, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x1C;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.calib_clk, 1, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x1D;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.calib_bpa, 1, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x1E;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.calib_pu, 1, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x22;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.array_type, 1, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x26;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.vdd_th1, 2, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x28;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.vdd_th2, 2, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x34;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.ptat_grad, 4, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x38;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.ptat_off, 4, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x3C;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.ptat_th1, 2, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x3E;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.ptat_th2, 2, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x4E;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.vdd_sc_grad, 1, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x4F;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.vdd_sc_off, 1, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x54;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.global_off, 1, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x55;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.global_gain, 2, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x74;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.device_id, 4, 100);
    if(ret < 0)
        return ret;
    send[0] = 0x00;
    send[1] = 0x7F;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR_EEPROM, send, 2, (uint8_t*)&obj->eeprom.nr_of_def_pix, 1, 100);
    if(ret < 0)
        return ret;
    
    int i;
    for(i=0; i<1024; ++i)
    {
        obj->pix_c[i] = ((float)obj->eeprom.p[i] * (obj->eeprom.pix_c_max - obj->eeprom.pix_c_min)/65535.0 + obj->eeprom.pix_c_min) * 
                       (obj->eeprom.epsilon / 100.0) * (obj->eeprom.global_gain/10000.0) + 0.5;
    }
    obj->eeprom.vdd_sc_grad_div  = pow_2(obj->eeprom.vdd_sc_grad);
    obj->eeprom.vdd_sc_off_div   = pow_2(obj->eeprom.vdd_sc_off);
    obj->eeprom.grad_scale_div   = pow_2(obj->eeprom.grad_scale);
    return 0;
}

void htpa_destroy(htpa_t* obj)
{
    // free(obj->eeprom.vdd_comp_grad);
    // free(obj->eeprom.vdd_comp_off);
    // free(obj->eeprom.th_grad);
    // free(obj->eeprom.th_off);
    // free(obj->eeprom.p);
    // free(obj->elec_off);
    // free(obj->temp);
    // free(obj->pixels);
    // free(obj->pix_c);
    // free(obj->v);
    // obj->eeprom.vdd_comp_grad = NULL;
    // obj->eeprom.vdd_comp_off = NULL;
    // obj->eeprom.th_grad = NULL;
    // obj->eeprom.th_off = NULL;
    // obj->eeprom.p = NULL;
    // obj->elec_off = NULL;
    // obj->temp = NULL;
    // obj->pixels = NULL;
    // obj->pix_c = NULL;
    // obj->v = NULL;
    obj->is_init = false;
}


int htpa_get_electrical_offsets(htpa_t* obj)
{
    uint8_t temp[2];
    int ret;
    uint16_t i, j, k, index;
    temp[0] = HTPA_CMD_ADDR_CONF;
    temp[1] = 0x0B;
    ret = maix_i2c_send_data(obj->i2c_num, HTPA_ADDR, temp, 2, 100);
    if(ret < 0)
        return 1;
    temp[0] = HTPA_CMD_ADDR_STATUS;
    while(1)
    {
        ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR, temp, 1, temp+1, 1, 100);
        if(ret < 0)
            return 2;
        if( (temp[1] & 0x01) == 0x01)
            break;
    }
    temp[0] = HTPA_CMD_ADDR_DATA1;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR, temp, 1, (uint8_t*)obj->elec_off, 258, 200);
    if(ret < 0)
        return 3;
    temp[0] = HTPA_CMD_ADDR_DATA2;
    ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR, temp, 1, obj->temp, 258, 200);
    if(ret < 0)
        return 4;
    index = 0;
    for(i=1; i<129; ++i)//top 4 block, MSB to LSB, 128 uint16_t
    {
        obj->elec_off[index++] = ((uint16_t)((uint8_t*)obj->elec_off)[i*2])<<8 | ((uint8_t*)obj->elec_off)[i*2+1];
    }
    for(i=0; i<4; ++i)
    {
        k = 3 - i;
        for(j = 1; j < obj->width+1; ++j)
        {
            obj->elec_off[index++] = ((uint16_t)((uint8_t*)obj->temp)[k*64+j*2])<<8 | ((uint8_t*)obj->temp)[k*64+j*2+1];
        }
    }
    return 0;
}

int htpa_get_pixels(htpa_t* obj, bool is_get_vdd)
{
    uint16_t i, j, k, k2, col, index = 0;
    uint8_t temp[2];
    uint8_t* p;
    int ret = 0;
    uint8_t vdd_mask = is_get_vdd ? 0x04 : 0x00;

    temp[0] = HTPA_CMD_ADDR_CONF;
    temp[1] = 0x09 | vdd_mask; //0 << 4 | 0x09;
    ret = maix_i2c_send_data(obj->i2c_num, HTPA_ADDR, temp, 2, 100);
    if(ret < 0)
        return EIO;
    for(i=0; i<4; ++i)
    {
        k = 3 - i;
        temp[0] = HTPA_CMD_ADDR_STATUS;
        while(1)
        {
            ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR, temp, 1, temp+1, 1, 100);
            if(ret < 0)
                return EIO;
            if((temp[1] & 0x01) == 0x01)
                break;
        }
        temp[0] = HTPA_CMD_ADDR_DATA1;
        p = obj->temp2;
        ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR, temp, 1, p, 258, 200);
        if(ret < 0)
            return EIO;
        temp[0] = HTPA_CMD_ADDR_DATA2;
        ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR, temp, 1, obj->temp, 258, 200);
        if(ret < 0)
            return EIO;
        // start convet next block
        if(i<3)
        {
            temp[0] = HTPA_CMD_ADDR_CONF;
            temp[1] = (i+1) << 4 | 0x09 | vdd_mask;
            ret = maix_i2c_send_data(obj->i2c_num, HTPA_ADDR, temp, 2, 100);
            if(ret < 0)
                return EIO;
        }
        for(j=1; j<129; ++j)
        {
            obj->v[index++] = ((uint16_t)p[j*2])<<8 | p[j*2+1];
        }
        for(j=0; j<4; ++j)
        {
            k2 = 3 - j;
            for(col=1; col < 33; ++col)
            {
                obj->v[511+32*(k*4+k2)+col] = ((uint16_t)(obj->temp)[j*64 + 2*col])<<8 | (obj->temp)[j*64 + 2*col + 1];
            }
        }
        if(is_get_vdd)
        {
            obj->vdd[i] = ((uint16_t)p[0])<<8 | p[1];
            obj->vdd[7 - i] = ((uint16_t)((uint8_t*)obj->temp)[0])<<8 | ((uint8_t*)obj->temp)[1];
        }
        else
        {
            obj->ptats[i] = ((uint16_t)p[0])<<8 | p[1];
            obj->ptats[7 - i] = ((uint16_t)((uint8_t*)obj->temp)[0])<<8 | ((uint8_t*)obj->temp)[1];
        }
        

    }
    if(is_get_vdd)
    {
        obj->vdd_mean = mean(obj->vdd, 8);
        // for(int i=0; i<8; ++i)
        //     printk("vdd:%d ", obj->vdd[i]);
        // printk("\r\n");
        // printk("vdd_mean:%d\r\n", (int)obj->ptat_mean);
    }
    else
    {
        obj->ptat_mean = mean(obj->ptats, 8);
        obj->ta = obj->ptat_mean * obj->eeprom.ptat_grad + obj->eeprom.ptat_off  + 0.5;
        // for(int i=0; i<8; ++i)
        //     printk("ptats:%d ", obj->ptats[i]);
        // printk("\r\n");
        // printk("obj->ta:%d, obj->ptat_mean:%d\r\n", (int)obj->ta, (int)obj->ptat_mean);
    }
    return ret;
}

int htpa_get_vdd(htpa_t* obj)
{
    uint16_t i;
    uint8_t temp[2];
    int ret;
    
    for(i=0; i<4; ++i)
    {
        temp[0] = HTPA_CMD_ADDR_CONF;
        temp[1] = i << 4 | 0x0D;
        ret = maix_i2c_send_data(obj->i2c_num, HTPA_ADDR, temp, 2, 100);
        if(ret < 0)
            return EIO;
        temp[0] = HTPA_CMD_ADDR_STATUS;
        while(1)
        {
            ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR, temp, 1, temp+1, 1, 100);
            if(ret < 0)
                return EIO;
            if((temp[1] & 0x01) == 0x01)
                break;
        }
        temp[0] = HTPA_CMD_ADDR_DATA1;
        ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR, temp, 1, temp, 2, 200);
        if(ret < 0)
            return EIO;
        obj->vdd[i] = ((uint16_t)temp[0])<<8 | temp[1];
        temp[0] = HTPA_CMD_ADDR_DATA2;
        ret = maix_i2c_recv_data(obj->i2c_num, HTPA_ADDR, temp, 1, temp, 2, 200);
        if(ret < 0)
            return EIO;
        obj->vdd[7 - i] = ((uint16_t)temp[0])<<8 | temp[1];
    }
    return 0;
}

static inline void thermal_compensation(htpa_t* obj)
{
    for(int i=0; i<1024; ++i)
    {
        obj->v[i] = obj->v[i] - (obj->eeprom.th_grad[i] * obj->ptat_mean) / obj->eeprom.grad_scale_div - obj->eeprom.th_off[i];
    }
}

static inline void electrical_offset(htpa_t* obj)
{
    int i;
    for(i=0; i<512; ++i)
    {
        obj->v[i] = obj->v[i] - obj->elec_off[i%128];
    }
    for(i=512; i<1024; ++i)
    {
        obj->v[i] = obj->v[i] - obj->elec_off[i%128 + 128];
    }
}

static inline void vdd_compensation(htpa_t* obj)
{
    int i;
    for(i=0; i<512; ++i)
    {
        // printk("vdd comp: %d %d %d %d %d %d %d %d -- %d %d %d %d %d %d : %d\r\n", obj->v[i], obj->eeprom.vdd_comp_grad[i%128], (int)obj->ptat_mean, obj->eeprom.vdd_sc_grad, obj->eeprom.vdd_sc_grad_div,
        //                    obj->eeprom.vdd_comp_off[i%128], obj->eeprom.vdd_sc_off, obj->eeprom.vdd_sc_off_div, 
        //                    (int)obj->vdd_mean, obj->eeprom.vdd_th1, obj->eeprom.vdd_th2, obj->eeprom.vdd_th1, obj->eeprom.ptat_th2, obj->eeprom.ptat_th1, 
        //                    (int)(( obj->eeprom.vdd_comp_grad[i%128] * obj->ptat_mean /  obj->eeprom.vdd_sc_grad_div + obj->eeprom.vdd_comp_off[i%128] ) / 
        //                    obj->eeprom.vdd_sc_off_div * 
        //                   (obj->vdd_mean - obj->eeprom.vdd_th1 - 
        //                       (obj->eeprom.vdd_th2 - obj->eeprom.vdd_th1)/(obj->eeprom.ptat_th2 - obj->eeprom.ptat_th1) * 
        //                       (obj->ptat_mean - obj->eeprom.ptat_th1)
        //                    )));
        obj->v[i] -= ( obj->eeprom.vdd_comp_grad[i%128] * obj->ptat_mean /  obj->eeprom.vdd_sc_grad_div + obj->eeprom.vdd_comp_off[i%128] ) / 
                           obj->eeprom.vdd_sc_off_div * 
                          (obj->vdd_mean - obj->eeprom.vdd_th1 - 
                              (obj->eeprom.vdd_th2 - obj->eeprom.vdd_th1)/(obj->eeprom.ptat_th2 - obj->eeprom.ptat_th1) * 
                              (obj->ptat_mean - obj->eeprom.ptat_th1)
                           );
        // printk("%d\r\n", obj->v[i]);
    }
    for(i=512; i<1024; ++i)
    {
        obj->v[i] -= (obj->eeprom.vdd_comp_grad[i%128 + 128] * obj->ptat_mean /  obj->eeprom.vdd_sc_grad_div + obj->eeprom.vdd_comp_off[i%128 + 128] ) / 
                          obj->eeprom.vdd_sc_off_div * 
                          (obj->vdd_mean - obj->eeprom.vdd_th1 - 
                              (obj->eeprom.vdd_th2 - obj->eeprom.vdd_th1)/(obj->eeprom.ptat_th2 - obj->eeprom.ptat_th1) * 
                              (obj->ptat_mean - obj->eeprom.ptat_th1)
                           );
    }
}

static inline void sensitivity_compensation(htpa_t* obj)
{
    int i;
    // printk("pix_c:%d %d %d %d\r\n", (int)obj->pix_c[0], (int)obj->pix_c[1], obj->pixels[0], obj->pixels[1]);
    for(i=0; i<1024; ++i)
    {
        obj->v[i] = (int32_t)((int64_t)obj->v[i] * HTPA_PCSCALEVAL / obj->pix_c[i]);
    }
    // printk("pix_c:%d %d %d %d\r\n", (int)obj->pix_c[0], (int)obj->pix_c[1], obj->pixels[0], obj->pixels[1]);
}

/**
 * convert to ℃ by lookup table provided by the productor
 * temperature will be saved to `obj->v`, and unit is `℃ * 100`
 */
static inline void lookup_table(htpa_t* obj)
{
    int32_t vx, vy, ydist;
    int32_t dTA;
    uint16_t table_col = 0;
    uint16_t table_row;
    int i;

    // find column in table
    for (i = 0; i < HTPA_NROF_TA_ELEMENTS; i++)
    {
        // printk("ta: %d, %d\r\n", (int)obj->ta, table_x_ta_temps[i]);
        if (obj->ta >= table_x_ta_temps[i])
        {
            table_col = i;
        }
        else
        {
            break;
        }
        
    }

    dTA = obj->ta - table_x_ta_temps[table_col];
    ydist = (int32_t)HTPA_ADEQUIDISTANCE;
    // printk("col:%d, dTA:%d, ydist:%d, obj->eeprom.global_off:%d\r\n", table_col, dTA, ydist, obj->eeprom.global_off);

    for (int m = 0; m < 32; m++)
    {
        for (int n = 0; n < 32; n++)
        {

            // find row in table
            table_row = obj->v[m*32+n] + HTPA_TABLEOFFSET;
            table_row = table_row >> HTPA_ADEXPBITS;

            // bilinear interpolation
            vx = ((((int32_t)table_temp[table_row][table_col + 1] - (int32_t)table_temp[table_row][table_col]) * (int32_t)dTA) / (int32_t)HTPA_TAEQUIDISTANCE) + (int32_t)table_temp[table_row][table_col];
            vy = ((((int32_t)table_temp[table_row + 1][table_col + 1] - (int32_t)table_temp[table_row + 1][table_col]) * (int32_t)dTA) / (int32_t)HTPA_TAEQUIDISTANCE) + (int32_t)table_temp[table_row + 1][table_col];
            // obj->v[m*32+n] = (uint32_t)((vy - vx) * ((int32_t)(obj->v[m*32+n] + HTPA_TABLEOFFSET) - (int32_t)table_y_ad_values[table_row]) / ydist + (int32_t)vx);
            obj->v[m*32+n] = (uint32_t)((vy - vx) * ((int32_t)(obj->v[m*32+n] + HTPA_TABLEOFFSET) - (int32_t)table_y_ad_values[table_row]) / ydist + (int32_t)vx) + obj->eeprom.global_off;
            // if(m==0 &&(n==0 || n==1))
            //     printk("x:%d, y:%d, %d %d %d %d vx:%d, vy:%d Tobject:%d\r\n",table_row, table_col, table_temp[table_row][table_col], table_temp[table_row][table_col + 1], table_temp[table_row+1][table_col], table_temp[table_row + 1][table_col + 1], vx, vy, obj->v[m*32+n]);
            //convert to ℃*100
            obj->v[m*32+n] = (int32_t)((obj->v[m*32+n] - 2731.5)*10);
        }
    }
}

int htpa_snapshot(htpa_t* obj, int32_t** pixels)
{
    static bool is_get_vdd = false;
    int ret;
    if(!pixels)
        return EINVAL;
    
    #if HTPA_DEBUG_TIME
        uint64_t t;
        t = mp_hal_ticks_ms();
    #endif

    // 1. get electrical offsets
    obj->is_get_elec_off = !obj->is_get_elec_off; //...
    if(obj->is_get_elec_off)
    {
        ret = htpa_get_electrical_offsets(obj);  // 31ms
        if(ret != 0)
            return ret;
    }
    
    #if HTPA_DEBUG_TIME
        printk("elec_off t:%ld\r\n", mp_hal_ticks_ms() - t);
        t = mp_hal_ticks_ms();
    #endif // HTPA_DEBUG_TIME

    // 2. get pixels and ptats or vdds
    ret = htpa_get_pixels(obj, is_get_vdd); //  125ms
    if(ret != 0)
        return ret;
    is_get_vdd = !is_get_vdd;

    #if HTPA_DEBUG
        printk("pix: %d %d %d %d %d  %d\r\n", obj->elec_off[0], obj->elec_off[1], obj->v[0], obj->v[1], obj->v[511], obj->v[512]);
    #endif // HTPA_DEBUG

    #if HTPA_DEBUG_TIME
        printk("htpa_get_pixels t:%ld\r\n", mp_hal_ticks_ms() - t);
        t = mp_hal_ticks_ms();
    #endif // HTPA_DEBUG_TIME

    // 3. thermal compensation
    thermal_compensation(obj);
    #if HTPA_DEBUG
        printk("pix2: %d %d %d %d\r\n", obj->v[0], obj->v[1], obj->v[511], obj->v[512]);
    #endif // HTPA_DEBUG

    #if HTPA_DEBUG_TIME
        printk("thermal_compensation t:%ld\r\n", mp_hal_ticks_ms() - t);
        t = mp_hal_ticks_ms();
    #endif // HTPA_DEBUG_TIME

    // 4. electrical compensation
    electrical_offset(obj);
    #if HTPA_DEBUG
        printk("pix3: %d %d %d %d\r\n", obj->v[0], obj->v[1], obj->v[511], obj->v[512]);
    #endif // HTPA_DEBUG

    #if HTPA_DEBUG_TIME
        printk("electrical_offset t:%ld\r\n", mp_hal_ticks_ms() - t);
        t = mp_hal_ticks_ms();
    #endif // HTPA_DEBUG_TIME

    // 5. vdd compensation
    vdd_compensation(obj);
    #if HTPA_DEBUG
        printk("pix4: %d %d %d %d\r\n", obj->v[0], obj->v[1], obj->v[511], obj->v[512]);
    #endif // HTPA_DEBUG
    

    #if HTPA_DEBUG_TIME
        printk("vdd_compensation t:%ld\r\n", mp_hal_ticks_ms() - t);
        t = mp_hal_ticks_ms();
    #endif // HTPA_DEBUG_TIME

    // 6. sensitivity compensation
    sensitivity_compensation(obj);
    #if HTPA_DEBUG
        printk("pix5: %d %d %d %d\r\n", obj->v[0], obj->v[1], obj->v[511], obj->v[512]);
    #endif // HTPA_DEBUG
    

    #if HTPA_DEBUG_TIME
        printk("sensitivity_compensation t:%ld\r\n", mp_hal_ticks_ms() - t);
        t = mp_hal_ticks_ms();
    #endif // HTPA_DEBUG_TIME

    // 7. lookup table, convert to ℃
    lookup_table(obj);
    #if HTPA_DEBUG
        printk("pix6: %d %d %d %d\r\n", obj->v[0], obj->v[1], obj->v[511], obj->v[512]);
    #endif // HTPA_DEBUG
    
    #if HTPA_DEBUG_TIME
        printk("lookup_table t:%ld\r\n", mp_hal_ticks_ms() - t);
        t = mp_hal_ticks_ms();
    #endif // HTPA_DEBUG_TIME

    // 8. return temperature array, unit: ℃*100, type: uint8_t
    *pixels = obj->v;
    return 0;
}



