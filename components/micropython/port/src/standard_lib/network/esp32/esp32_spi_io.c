#include <stdlib.h>
#include "esp32_spi_io.h"
#include "gpiohs.h"
#include "sleep.h"
#include "sysctl.h"

#include <utils.h>
#include "spi.h"
#include "esp32_spi.h"

#if 1 /* ESP32_SOFT_SPI */

#define GPIOHS_OUT_HIGH(io) (*(volatile uint32_t *)0x3800100CU) |= (1 << (io))
#define GPIOHS_OUT_LOWX(io) (*(volatile uint32_t *)0x3800100CU) &= ~(1 << (io))

#define GET_GPIOHS_VALX(io) (((*(volatile uint32_t *)0x38001000U) >> (io)) & 1)

uint8_t cs_num = 0, rst_num = 0, rdy_num = 0;

static uint8_t _mosi_num = -1;
static uint8_t _miso_num = -1;
static uint8_t _sclk_num = -1;

/* SPI端口初始化 */
//should check io value
void esp32_spi_config_io(uint8_t cs, uint8_t rst, uint8_t rdy, uint8_t mosi, uint8_t miso, uint8_t sclk)
{
    //clk
    gpiohs_set_drive_mode(sclk, GPIO_DM_OUTPUT);
    gpiohs_set_pin(sclk, 0);
    //mosi
    gpiohs_set_drive_mode(mosi, GPIO_DM_OUTPUT);
    gpiohs_set_pin(mosi, 0);
    //miso
    gpiohs_set_drive_mode(miso, GPIO_DM_INPUT_PULL_UP);

    _mosi_num = mosi;
    _miso_num = miso;
    _sclk_num = sclk;

    cs_num = cs;
    rdy_num = rdy;
    rst_num = rst; //if rst <0, use soft reset
}

uint8_t soft_spi_rw(uint8_t data)
{
    uint8_t i;
    uint8_t temp = 0;
    for (i = 0; i < 8; i++)
    {
        if (data & 0x80)
        {
            GPIOHS_OUT_HIGH(_mosi_num);
        }
        else
        {
            GPIOHS_OUT_LOWX(_mosi_num);
        }
        data <<= 1;
        GPIOHS_OUT_HIGH(_sclk_num);

        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");

        temp <<= 1;
        if (GET_GPIOHS_VALX(_miso_num))
        {
            temp++;
        }
        GPIOHS_OUT_LOWX(_sclk_num);

        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
    }
    return temp;
}

void soft_spi_rw_len(uint8_t *send, uint8_t *recv, uint32_t len)
{
    if (send == NULL && recv == NULL)
    {
        printf(" buffer is null\r\n");
        return;
    }

#if 0
    uint32_t i = 0;
    do
    {
        *(recv + i) = soft_spi_rw(*(send + i));
        i++;
    } while (--len);
#else

    uint32_t i = 0;
    uint8_t *stmp = NULL, sf = 0;

    if (send == NULL)
    {
        stmp = (uint8_t *)malloc(len * sizeof(uint8_t));
        // memset(stmp, 'A', len);
        sf = 1;
    }
    else
        stmp = send;

    if (recv == NULL)
    {
        do
        {
            soft_spi_rw(*(stmp + i));
            i++;
        } while (--len);
    }
    else
    {
        do
        {
            *(recv + i) = soft_spi_rw(*(stmp + i));
            i++;
        } while (--len);
    }

    if (sf)
        free(stmp);
#endif
}

#else

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
    switch (spi_num)
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

static void sipeed_spi_transfer_data_standard(spi_device_num_t spi_num, spi_chip_select_t chip_select, const uint8_t *tx_buff, uint8_t *rx_buff, size_t len)
{
    configASSERT(spi_num < SPI_DEVICE_MAX && spi_num != 2);
    configASSERT(len > 0);
    size_t index, fifo_len;
    size_t rx_len = len;
    size_t tx_len = rx_len;
    sipeed_spi_set_tmod(spi_num, SPI_TMOD_TRANS_RECV);

    volatile spi_t *spi_handle = spi[spi_num];

    uint8_t dfs_offset;
    switch (spi_num)
    {
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
    spi_handle->ctrlr1 = (uint32_t)(tx_len / frame_width - 1);
    spi_handle->ssienr = 0x01;
    spi_handle->ser = 1U << chip_select;
    uint32_t i = 0;
    while (tx_len)
    {
        fifo_len = 32 - spi_handle->txflr;
        fifo_len = fifo_len < tx_len ? fifo_len : tx_len;
        switch (frame_width)
        {
        case SPI_TRANS_INT:
            fifo_len = fifo_len / 4 * 4;
            for (index = 0; index < fifo_len / 4; index++)
                spi_handle->dr[0] = ((uint32_t *)tx_buff)[i++];
            break;
        case SPI_TRANS_SHORT:
            fifo_len = fifo_len / 2 * 2;
            for (index = 0; index < fifo_len / 2; index++)
                spi_handle->dr[0] = ((uint16_t *)tx_buff)[i++];
            break;
        default:
            for (index = 0; index < fifo_len; index++)
                spi_handle->dr[0] = tx_buff[i++];
            break;
        }
        tx_len -= fifo_len;
    }

    while ((spi_handle->sr & 0x05) != 0x04)
        ;
    i = 0;
    while (rx_len)
    {
        fifo_len = spi_handle->rxflr;
        fifo_len = fifo_len < rx_len ? fifo_len : rx_len;
        switch (frame_width)
        {
        case SPI_TRANS_INT:
            fifo_len = fifo_len / 4 * 4;
            for (index = 0; index < fifo_len / 4; index++)
                ((uint32_t *)rx_buff)[i++] = spi_handle->dr[0];
            break;
        case SPI_TRANS_SHORT:
            fifo_len = fifo_len / 2 * 2;
            for (index = 0; index < fifo_len / 2; index++)
                ((uint16_t *)rx_buff)[i++] = (uint16_t)spi_handle->dr[0];
            break;
        default:
            for (index = 0; index < fifo_len; index++)
                rx_buff[i++] = (uint8_t)spi_handle->dr[0];
            break;
        }

        rx_len -= fifo_len;
    }
    spi_handle->ser = 0x00;
    spi_handle->ssienr = 0x00;
}

/* SPI端口初始化 */
void soft_spi_init(void)
{
    printf("hard spi\r\n");
    //cs
    gpiohs_set_drive_mode(ESP32_SPI_CSX_HS_NUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(ESP32_SPI_CSX_HS_NUM, 1);
    //init SPI_DEVICE_1
    spi_init(SPI_DEVICE_1, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
    printf("set spi clk:%d\r\n", spi_set_clk_rate(SPI_DEVICE_1, 1000000 * 8)); /*set clk rate*/
}

uint8_t soft_spi_rw(uint8_t data)
{
    uint8_t c;
    sipeed_spi_transfer_data_standard(SPI_DEVICE_1, SPI_CHIP_SELECT_0, &data, &c, 1);
    return c;
}

void soft_spi_rw_len(uint8_t *send, uint8_t *recv, uint32_t len)
{
    if (send == NULL && recv == NULL)
    {
        printf(" buffer is null\r\n");
        return;
    }

#if 0
    spi_init(SPI_DEVICE_1, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
#endif

    //only send
    if (send && recv == NULL)
    {
        spi_send_data_standard(SPI_DEVICE_1, SPI_CHIP_SELECT_0, NULL, 0, send, len);
        return;
    }

    //only recv
    if (send == NULL && recv)
    {
        spi_receive_data_standard(SPI_DEVICE_1, SPI_CHIP_SELECT_0, NULL, 0, recv, len);
        return;
    }

    //send and recv
    if (send && recv)
    {
        sipeed_spi_transfer_data_standard(SPI_DEVICE_1, SPI_CHIP_SELECT_0, send, recv, len);
        return;
    }
    return;
}

#endif

uint64_t get_millis(void)
{
    return sysctl_get_time_us() / 1000;
}
