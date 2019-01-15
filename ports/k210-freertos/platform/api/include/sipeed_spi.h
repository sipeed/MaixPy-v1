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

void sipeed_spi_init(sipeed_spi_device_num_t spi_num,
                         sipeed_spi_device_num_t work_mode,
            			 sipeed_spi_device_num_t frame_format,
           				 sipeed_size_t data_bit_length,
            			 sipeed_uint32_t endian);
void sipeed_spi_init_non_standard(sipeed_spi_device_num_t spi_num, 
										  sipeed_uint32_t instruction_length, 
										  sipeed_uint32_t address_length,
										  sipeed_uint32_t wait_cycles, 
										  sipeed_spi_instruction_address_trans_mode_t instruction_address_trans_mode);

void sipeed_spi_send_data_standard(sipeed_spi_device_num_t spi_num, 
	                                       sipeed_spi_chip_select_t chip_select, 
	                                       const sipeed_uint8_t *cmd_buff,
	                                       sipeed_size_t cmd_len, 
	                                       const sipeed_uint8_t *tx_buff, 
	                                       sipeed_size_t tx_len);

void sipeed_spi_receive_data_standard(sipeed_spi_device_num_t spi_num, 
											   sipeed_spi_chip_select_t chip_select, 
											   const sipeed_uint8_t *cmd_buff,
											   sipeed_size_t cmd_len, 
											   sipeed_uint8_t *rx_buff, 
											   sipeed_size_t rx_len);

void sipeed_spi_receive_data_multiple(sipeed_spi_device_num_t spi_num, 
											   sipeed_spi_chip_select_t chip_select, 
											   const sipeed_uint32_t *cmd_buff,
											   sipeed_size_t cmd_len, 
											   sipeed_uint8_t *rx_buff, 
											   sipeed_size_t rx_len);

void sipeed_spi_send_data_multiple(sipeed_spi_device_num_t spi_num, 
										   sipeed_spi_chip_select_t chip_select, 
										   const sipeed_uint32_t *cmd_buff,
										   sipeed_size_t cmd_len, 
										   const sipeed_uint8_t *tx_buff, 
										   sipeed_size_t tx_len);

void sipeed_spi_send_data_standard_dma(sipeed_dmac_channel_number_t channel_num, 
												sipeed_spi_device_num_t spi_num,
												sipeed_spi_chip_select_t chip_select,
												const sipeed_uint8_t *cmd_buff, 
												sipeed_size_t cmd_len, 
												const sipeed_uint8_t *tx_buff, 
												sipeed_size_t tx_len);

														   

void sipeed_spi_receive_data_standard_dma(sipeed_dmac_channel_number_t dma_send_channel_num,
										            sipeed_dmac_channel_number_t dma_receive_channel_num,
										   		    sipeed_spi_device_num_t spi_num, 
										   		    sipeed_spi_chip_select_t chip_select, 
										   		    const sipeed_uint8_t *cmd_buff,
										   		    sipeed_size_t cmd_len, 
										   		    sipeed_uint8_t *rx_buff, 
										   		    sipeed_size_t rx_len);

void sipeed_spi_send_data_multiple_dma(sipeed_dmac_channel_number_t channel_num, 
 											    sipeed_spi_device_num_t spi_num,
 										        sipeed_spi_chip_select_t chip_select,
 										        const sipeed_uint32_t *cmd_buff, 
 										        sipeed_size_t cmd_len, 
 										        const sipeed_uint8_t *tx_buff, 
 										        sipeed_size_t tx_len);

void sipeed_spi_receive_data_multiple_dma(sipeed_dmac_channel_number_t dma_send_channel_num,
										  		    sipeed_dmac_channel_number_t dma_receive_channel_num,
										  		    sipeed_spi_device_num_t spi_num, 
										  		    sipeed_spi_chip_select_t chip_select, 
										  		    const sipeed_uint32_t *cmd_buff,
										  		    sipeed_size_t cmd_len, 
										  		    sipeed_uint8_t *rx_buff, 
										  		    sipeed_size_t rx_len);

void sipeed_spi_fill_data_dma(sipeed_dmac_channel_number_t channel_num, 
									 sipeed_spi_device_num_t spi_num, 
									 sipeed_spi_chip_select_t chip_select,
		                        	 const sipeed_uint32_t *tx_buff, 
		                       		 sipeed_size_t tx_len);

void sipeed_spi_send_data_normal_dma(sipeed_dmac_channel_number_t channel_num, 
											  sipeed_spi_device_num_t spi_num,
											  sipeed_spi_chip_select_t chip_select,
											  const void *tx_buff, 
											  sipeed_size_t tx_len, 
											  sipeed_spi_transfer_width_t spi_transfer_width);

sipeed_uint32_t sipeed_spi_set_clk_rate(sipeed_spi_device_num_t spi_num, sipeed_uint32_t spi_clk);



#endif /* _SIPEEED_SPI_H */
