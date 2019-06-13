#ifndef __ESP32_SPI_IO_H
#define __ESP32_SPI_IO_H

#include <stdint.h>

extern uint8_t cs_num, rst_num, rdy_num;

void esp32_spi_config_io(uint8_t cs, uint8_t rst, uint8_t rdy, 
                            uint8_t mosi, uint8_t miso, uint8_t sclk);
                            
uint8_t soft_spi_rw(uint8_t data);
void soft_spi_rw_len(uint8_t *send, uint8_t *recv, uint32_t len);

uint64_t get_millis(void);

#endif
