#ifndef __ESP32_SPI_IO_H
#define __ESP32_SPI_IO_H

#include <stdint.h>

void soft_spi_config_io(uint8_t mosi, uint8_t miso, uint8_t sclk);
uint8_t soft_spi_rw(uint8_t data);
void soft_spi_rw_len(uint8_t *send, uint8_t *recv, uint32_t len);

void hard_spi_config_io();
uint8_t hard_spi_rw(uint8_t data);
void hard_spi_rw_len(uint8_t *send, uint8_t *recv, uint32_t len);

uint64_t get_millis(void);

#endif
