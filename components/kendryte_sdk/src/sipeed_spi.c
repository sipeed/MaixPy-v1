#include <stdio.h>
#include <fpioa.h>
#include <spi.h>
#include <sipeed_spi.h>
#include <sysctl.h>
#include <utils.h>

void sipeed_spi_init(
            sipeed_spi_device_num_t spi_num,
            sipeed_spi_device_num_t work_mode,
            sipeed_spi_device_num_t frame_format,
            sipeed_size_t data_bit_length,
            sipeed_uint32_t endian)
{
    spi_init(spi_num, work_mode, frame_format, data_bit_length, endian);
}

void sipeed_spi_init_non_standard(sipeed_spi_device_num_t spi_num, sipeed_uint32_t instruction_length, sipeed_uint32_t address_length,
                           sipeed_uint32_t wait_cycles, sipeed_spi_instruction_address_trans_mode_t instruction_address_trans_mode)
{
    spi_init_non_standard(spi_num,instruction_length,address_length,wait_cycles, instruction_address_trans_mode);
}

void sipeed_spi_send_data_standard(sipeed_spi_device_num_t spi_num, sipeed_spi_chip_select_t chip_select, const sipeed_uint8_t *cmd_buff,
                            sipeed_size_t cmd_len, const sipeed_uint8_t *tx_buff, sipeed_size_t tx_len)
{
    spi_send_data_standard( spi_num,chip_select,cmd_buff,cmd_len,tx_buff,tx_len);
}
void sipeed_spi_receive_data_standard(sipeed_spi_device_num_t spi_num, sipeed_spi_chip_select_t chip_select, const sipeed_uint8_t *cmd_buff,
                               sipeed_size_t cmd_len, sipeed_uint8_t *rx_buff, sipeed_size_t rx_len)
{
    spi_receive_data_standard(spi_num,chip_select,cmd_buff,cmd_len,rx_buff,rx_len);

}
void sipeed_spi_receive_data_multiple(sipeed_spi_device_num_t spi_num, sipeed_spi_chip_select_t chip_select, const sipeed_uint32_t *cmd_buff,
                               sipeed_size_t cmd_len, sipeed_uint8_t *rx_buff, sipeed_size_t rx_len)
{
    spi_receive_data_multiple( spi_num,chip_select,cmd_buff,cmd_len,rx_buff,rx_len);
}

void sipeed_spi_send_data_multiple(sipeed_spi_device_num_t spi_num, sipeed_spi_chip_select_t chip_select, const sipeed_uint32_t *cmd_buff,
                            sipeed_size_t cmd_len, const sipeed_uint8_t *tx_buff, sipeed_size_t tx_len)
{
    spi_send_data_multiple( spi_num,  chip_select,cmd_buff,cmd_len, tx_buff, tx_len);
}

void sipeed_spi_send_data_standard_dma(sipeed_dmac_channel_number_t channel_num, sipeed_spi_device_num_t spi_num,
                                sipeed_spi_chip_select_t chip_select,
                                const sipeed_uint8_t *cmd_buff, sipeed_size_t cmd_len, const sipeed_uint8_t *tx_buff, sipeed_size_t tx_len)
{
    spi_send_data_standard_dma(channel_num, spi_num, chip_select, cmd_buff, cmd_len, tx_buff, tx_len);
}

void sipeed_spi_receive_data_standard_dma(sipeed_dmac_channel_number_t dma_send_channel_num,
                                   sipeed_dmac_channel_number_t dma_receive_channel_num,
                                   sipeed_spi_device_num_t spi_num, sipeed_spi_chip_select_t chip_select, const sipeed_uint8_t *cmd_buff,
                                   sipeed_size_t cmd_len, sipeed_uint8_t *rx_buff, sipeed_size_t rx_len)
{
    spi_receive_data_standard_dma(dma_send_channel_num, dma_receive_channel_num, spi_num, chip_select, cmd_buff, cmd_len, rx_buff, rx_len);
}

void sipeed_spi_send_data_multiple_dma(sipeed_dmac_channel_number_t channel_num, sipeed_spi_device_num_t spi_num,
                                sipeed_spi_chip_select_t chip_select,
                                const sipeed_uint32_t *cmd_buff, sipeed_size_t cmd_len, const sipeed_uint8_t *tx_buff, sipeed_size_t tx_len)
{
    spi_send_data_multiple_dma(channel_num, spi_num,chip_select,cmd_buff,cmd_len,tx_buff,tx_len);

}

void sipeed_spi_receive_data_multiple_dma(sipeed_dmac_channel_number_t dma_send_channel_num,
                                   sipeed_dmac_channel_number_t dma_receive_channel_num,
                                   sipeed_spi_device_num_t spi_num, sipeed_spi_chip_select_t chip_select, const sipeed_uint32_t *cmd_buff,
                                   sipeed_size_t cmd_len, sipeed_uint8_t *rx_buff, sipeed_size_t rx_len)
{
    spi_receive_data_multiple_dma( dma_send_channel_num, dma_receive_channel_num,
                                    spi_num,  chip_select, cmd_buff,
                                    cmd_len, rx_buff, rx_len);
}

void sipeed_spi_fill_data_dma(sipeed_dmac_channel_number_t channel_num, sipeed_spi_device_num_t spi_num, sipeed_spi_chip_select_t chip_select,
                       const sipeed_uint32_t *tx_buff, sipeed_size_t tx_len)
{
    spi_fill_data_dma( channel_num,  spi_num,  chip_select, tx_buff, tx_len);
}

void sipeed_spi_send_data_normal_dma(sipeed_dmac_channel_number_t channel_num, sipeed_spi_device_num_t spi_num,
                              sipeed_spi_chip_select_t chip_select,
                              const void *tx_buff, sipeed_size_t tx_len, sipeed_spi_transfer_width_t spi_transfer_width)
{
    spi_send_data_normal_dma( channel_num,  spi_num,
                               chip_select, tx_buff, tx_len, spi_transfer_width);

}

sipeed_uint32_t sipeed_spi_set_clk_rate(sipeed_spi_device_num_t spi_num, sipeed_uint32_t spi_clk)
{
    return spi_set_clk_rate( spi_num, spi_clk);
}


static spi_transfer_width_t sipeed_spi_get_frame_size(size_t data_bit_length)
{
    if (data_bit_length < 8)
        return SPI_TRANS_CHAR;
    else if (data_bit_length < 16)
        return SPI_TRANS_SHORT;
    return SPI_TRANS_INT;
}

static void sipeed_spi_set_tmod(uint8_t spi_num, uint32_t tmod)
{
    configASSERT(spi_num < SPI_DEVICE_MAX && spi_num != 2);
    volatile spi_t *spi_handle = spi[spi_num];
    uint8_t tmod_offset = 0;
    switch(spi_num)
    {
        case 0:
        case 1:
            tmod_offset = 8;
            break;
        case 2:
            configASSERT(!"Spi Bus 2 Not Support!");
            break;
        case 3:
        default:
            tmod_offset = 10;
            break;
    }
    set_bit(&spi_handle->ctrlr0, 3 << tmod_offset, tmod << tmod_offset);
}

void sipeed_spi_transfer_data_standard(spi_device_num_t spi_num, spi_chip_select_t chip_select, const uint8_t *tx_buff,uint8_t *rx_buff,  size_t tx_len, size_t rx_len)
{
    configASSERT(spi_num < SPI_DEVICE_MAX && spi_num != 2);
    configASSERT(tx_len > 0 && tx_len >= rx_len);
    size_t index, fifo_len;
    sipeed_spi_set_tmod(spi_num, SPI_TMOD_TRANS_RECV);

    volatile spi_t *spi_handle = spi[spi_num];

    uint8_t dfs_offset;
    switch(spi_num){
        case 0:
        case 1:
            dfs_offset = 16;
            break;
        case 2:
            configASSERT(!"Spi Bus 2 Not Support!");
            break;
        case 3:
        default:
            dfs_offset = 0;
            break;
    }
    uint32_t data_bit_length = (spi_handle->ctrlr0 >> dfs_offset) & 0x1F;
    spi_transfer_width_t frame_width = sipeed_spi_get_frame_size(data_bit_length);
    spi_handle->ctrlr1 = (uint32_t)(tx_len/frame_width - 1);
    spi_handle->ssienr = 0x01;
    spi_handle->ser = 1U << chip_select;
    uint32_t index_tx = 0, index_rx = 0;
    while (tx_len)
    {
        fifo_len = 32 - spi_handle->txflr;
        fifo_len = fifo_len < tx_len ? fifo_len : tx_len;
        switch(frame_width)
        {
            case SPI_TRANS_INT:
                fifo_len = fifo_len / 4 * 4;
                for (index = 0; index < fifo_len / 4; index++)
                    spi_handle->dr[0] = ((uint32_t *)tx_buff)[index_tx++];
                break;
            case SPI_TRANS_SHORT:
                fifo_len = fifo_len / 2 * 2;
                for (index = 0; index < fifo_len / 2; index++)
                    spi_handle->dr[0] = ((uint16_t *)tx_buff)[index_tx++];
                break;
            default:
                for (index = 0; index < fifo_len; index++)
                    spi_handle->dr[0] = tx_buff[index_tx++];
                break;
        }
        tx_len -= fifo_len;
        while(rx_len)
        {
            fifo_len = spi_handle->rxflr;
            if(fifo_len==0)
            {
                if(index_tx - index_rx < 32)
                    break;
            }
            fifo_len = fifo_len < rx_len ? fifo_len : rx_len;
            switch(frame_width)
            {
                case SPI_TRANS_INT:
                    fifo_len = fifo_len / 4 * 4;
                    for (index = 0; index < fifo_len / 4; index++)
                    ((uint32_t *)rx_buff)[index_rx++] = spi_handle->dr[0];
                    break;
                case SPI_TRANS_SHORT:
                    fifo_len = fifo_len / 2 * 2;
                    for (index = 0; index < fifo_len / 2; index++)
                    ((uint16_t *)rx_buff)[index_rx++] = (uint16_t)spi_handle->dr[0];
                    break;
                default:
                    for (index = 0; index < fifo_len; index++)
                        rx_buff[index_rx++] = (uint8_t)spi_handle->dr[0];
                    break;
            }

            rx_len -= fifo_len;
            
        }
    }

    while ((spi_handle->sr & 0x05) != 0x04)
        ;
    while (rx_len)
    {
        fifo_len = spi_handle->rxflr;
        fifo_len = fifo_len < rx_len ? fifo_len : rx_len;
        switch(frame_width)
        {
            case SPI_TRANS_INT:
                fifo_len = fifo_len / 4 * 4;
                for (index = 0; index < fifo_len / 4; index++)
                  ((uint32_t *)rx_buff)[index_rx++] = spi_handle->dr[0];
                break;
            case SPI_TRANS_SHORT:
                fifo_len = fifo_len / 2 * 2;
                for (index = 0; index < fifo_len / 2; index++)
                  ((uint16_t *)rx_buff)[index_rx++] = (uint16_t)spi_handle->dr[0];
                 break;
            default:
                  for (index = 0; index < fifo_len; index++)
                      rx_buff[index_rx++] = (uint8_t)spi_handle->dr[0];
                break;
        }

        rx_len -= fifo_len;
    }
    spi_handle->ser = 0x00;
    spi_handle->ssienr = 0x00;
}

void sipeed_spi_deinit(spi_device_num_t spi_num)
{
    volatile spi_t *spi_handle = spi[spi_num];
    spi_handle->ssienr = 0x00;
    sysctl_clock_disable(SYSCTL_CLOCK_SPI0 + spi_num);
}

