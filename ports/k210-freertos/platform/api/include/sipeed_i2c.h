#ifndef __MAIX_I2C_H__
#define __MAIX_I2C_H__

#include "i2c.h"

#define I2C_CON_SPEED_STANDARD                        1 // <=100Kbit/s
#define I2C_CON_SPEED_FAST                            2 // <=400Kbit/s or <=1000Kbit/s
#define I2C_CON_SPEED_HIGH                            3 // <=3.4Mbit/s

void maix_i2c_init(i2c_device_number_t i2c_num, uint32_t address_width,
              uint32_t i2c_clk);
int maix_i2c_send_data(i2c_device_number_t i2c_num, uint32_t slave_address, const uint8_t *send_buf, size_t send_buf_len, uint16_t timeout_ms);
int maix_i2c_recv_data(i2c_device_number_t i2c_num, uint32_t slave_address, const uint8_t *send_buf, size_t send_buf_len, uint8_t *receive_buf,
                  size_t receive_buf_len, uint16_t timeout_ms);
void maix_i2c_deinit(i2c_device_number_t i2c_num);
#endif //__MAIX_I2C_H__
