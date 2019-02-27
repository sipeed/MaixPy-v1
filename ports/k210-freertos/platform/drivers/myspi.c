#include "myspi.h"
#include "gpiohs.h"
#include "sleep.h"
#include "sysctl.h"

#include "spi.h"

#if SOFT_SPI
/* SPI端口初始化 */
void soft_spi_init(void)
{
    //cs
    gpiohs_set_drive_mode(SS_GPIONUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(SS_GPIONUM, 0);
    //clk
    gpiohs_set_drive_mode(SCLK_GPIONUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(SCLK_GPIONUM, 1);
    //mosi
    gpiohs_set_drive_mode(MOSI_GPIONUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(MOSI_GPIONUM, 1);
    //miso
    gpiohs_set_drive_mode(MISO_GPIONUM, GPIO_DM_INPUT_PULL_UP);
    // gpiohs_set_pin(MISO_GPIONUM, 1);
}

uint8_t soft_spi_rw(uint8_t data)
{
    uint8_t i;
    uint8_t temp = 0;
    for (i = 8; i > 0; i--)
    {
        if (data & 0x80)
        {
            SOFT_SPI_MOSI_SET();
        }
        else
        {
            SOFT_SPI_MOSI_CLR();
        }
        data <<= 1;
        SOFT_SPI_CLK_SET();
        asm volatile("nop");
        // asm volatile("nop");
        temp <<= 1;
        // if (gpiohs_get_pin(MISO_GPIONUM))
        if (gpiohs->input_val.bits.b13)
        {
            temp++;
        }
        SOFT_SPI_CLK_CLR();
        asm volatile("nop");
        // asm volatile("nop");
    }
    return temp;
}
#else

// /* SPI端口初始化 */
// void soft_spi_init(void)
// {
//     //cs
//     gpiohs_set_drive_mode(SS_GPIONUM, GPIO_DM_OUTPUT);
//     gpiohs_set_pin(SS_GPIONUM, 0);
//     spi_set_clk_rate(SPI_DEVICE_1, 1000000 * 9); /*set clk rate*/
// }

// uint8_t soft_spi_rw(uint8_t data)
// {
//     uint8_t c;
//     sipeed_spi_transfer_data_standard(SPI_DEVICE_1, SPI_CHIP_SELECT_0, &data, &c, 1);
//     return c;
// }

// void soft_spi_rw_len(uint8_t *send, uint8_t *recv, uint8_t len)
// {
//     sipeed_spi_transfer_data_standard(SPI_DEVICE_1, SPI_CHIP_SELECT_0, send, recv, len);
// }

#endif

uint64_t get_millis(void)
{
    return sysctl_get_time_us() / 1000;
}
