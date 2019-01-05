#ifndef _SIPEEED_SPI_H
#define _SIPEEED_SPI_H

#include <spi.h>
#include "sipeed_type.h"

#define sipeed_spi_device_num_t spi_device_num_t
#define sipeed_spi_work_mode_t spi_work_mode_t
#define sipeed_spi_frame_format_t spi_work_mode_t
#define sipeed_dmac_channel_number_t dmac_channel_number_t
#define sipeed_spi_chip_select_t spi_chip_select_t
#define sipeed_spi_instruction_address_trans_mode_t spi_instruction_address_trans_mode_t
#define sipeed_spi_transfer_width_t spi_transfer_width_t

void sipeed_spi_deinit(spi_device_num_t spi_num);
void sipeed_spi_transfer_data_standard(spi_device_num_t spi_num, spi_chip_select_t chip_select, const uint8_t *tx_buff,uint8_t *rx_buff,  size_t len);
#endif /* _SIPEEED_SPI_H */
