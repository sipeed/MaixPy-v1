#include "st7789.h"
#include "sipeed_spi.h"
#include "sleep.h"
#include "lcd.h"

static bool standard_spi = false;
static spi_work_mode_t standard_work_mode = SPI_WORK_MODE_0;

static void init_dcx(void)
{
    gpiohs_set_drive_mode(DCX_GPIONUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(DCX_GPIONUM, GPIO_PV_HIGH);
}

static void set_dcx_control(void)
{
    gpiohs_set_pin(DCX_GPIONUM, GPIO_PV_LOW);
}

static void set_dcx_data(void)
{
    gpiohs_set_pin(DCX_GPIONUM, GPIO_PV_HIGH);
}

static void init_rst(void)
{
    gpiohs_set_drive_mode(RST_GPIONUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(RST_GPIONUM, GPIO_PV_HIGH);
}

static void set_rst(uint8_t val)
{
    gpiohs_set_pin(RST_GPIONUM, val);
}

void tft_set_clk_freq(uint32_t freq)
{
    spi_set_clk_rate(SPI_CHANNEL, freq);
}

void tft_hard_init(uint32_t freq, bool oct)
{
    standard_spi = !oct;
    init_dcx();
    init_rst();
    set_rst(0);
    if(standard_spi)
    {
        /* Init SPI IO map and function settings */
        // sysctl_set_spi0_dvp_data(1);
        spi_init(SPI_CHANNEL, standard_work_mode, SPI_FF_STANDARD, 8, 0);
    }
    else
    {
        /* Init SPI IO map and function settings */
        // sysctl_set_spi0_dvp_data(0);
        spi_init(SPI_CHANNEL, SPI_WORK_MODE_0, SPI_FF_OCTAL, 8, 0);
    }
    tft_set_clk_freq(freq);
    msleep(50);
    set_rst(1);
    msleep(50);
}

void tft_write_command(uint8_t cmd)
{
    set_dcx_control();
    if(!standard_spi)
    {
        spi_init(SPI_CHANNEL, SPI_WORK_MODE_0, SPI_FF_OCTAL, 8, 0);
        spi_init_non_standard(SPI_CHANNEL, 8 /*instrction length*/, 0 /*address length*/, 0 /*wait cycles*/,
                                SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
        spi_send_data_normal_dma(SPI_DMA_CH, SPI_CHANNEL, LCD_SPI_SLAVE_SELECT, (uint8_t *)(&cmd), 1, SPI_TRANS_CHAR);
    }
    else
    {
        spi_init(SPI_CHANNEL, standard_work_mode, SPI_FF_STANDARD, 8, 0);
        spi_send_data_normal_dma(SPI_DMA_CH, SPI_CHANNEL, LCD_SPI_SLAVE_SELECT, (uint8_t *)(&cmd), 1, SPI_TRANS_CHAR);
    }
}

void tft_write_byte(uint8_t *data_buf, uint32_t length)
{
    set_dcx_data();
    if(!standard_spi)
    {
        spi_init(SPI_CHANNEL, SPI_WORK_MODE_0, SPI_FF_OCTAL, 8, 0);
        spi_init_non_standard(SPI_CHANNEL, 0 /*instrction length*/, 8 /*address length*/, 0 /*wait cycles*/,
                                SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
        spi_send_data_normal_dma(SPI_DMA_CH, SPI_CHANNEL, LCD_SPI_SLAVE_SELECT, data_buf, length, SPI_TRANS_CHAR);
    }
    else
    {
        spi_init(SPI_CHANNEL, standard_work_mode, SPI_FF_STANDARD, 8, 0);
        spi_send_data_normal_dma(SPI_DMA_CH, SPI_CHANNEL, LCD_SPI_SLAVE_SELECT, data_buf, length, SPI_TRANS_CHAR);
    }
}

void tft_write_half(uint16_t *data_buf, uint32_t length)
{
    set_dcx_data();
    if(!standard_spi)
    {
        spi_init(SPI_CHANNEL, SPI_WORK_MODE_0, SPI_FF_OCTAL, 16, 0);
        spi_init_non_standard(SPI_CHANNEL, 0 /*instrction length*/, 16 /*address length*/, 0 /*wait cycles*/,
                            SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
        spi_send_data_normal_dma(SPI_DMA_CH, SPI_CHANNEL, LCD_SPI_SLAVE_SELECT, data_buf, length, SPI_TRANS_SHORT);
    }
    else
    {
        spi_init(SPI_CHANNEL, standard_work_mode, SPI_FF_STANDARD, 16, 0);
        spi_send_data_normal_dma(SPI_DMA_CH, SPI_CHANNEL, LCD_SPI_SLAVE_SELECT, data_buf, length, SPI_TRANS_SHORT);
    }
}

void tft_write_word(uint32_t *data_buf, uint32_t length)
{
    set_dcx_data();
    if(!standard_spi)
    {
        spi_init(SPI_CHANNEL, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 0);

        spi_init_non_standard(SPI_CHANNEL, 0 /*instrction length*/, 32 /*address length*/, 0 /*wait cycles*/,
                            SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
        spi_send_data_normal_dma(SPI_DMA_CH, SPI_CHANNEL, LCD_SPI_SLAVE_SELECT, data_buf, length, SPI_TRANS_INT);
    }
    else
    {
        spi_init(SPI_CHANNEL, standard_work_mode, SPI_FF_STANDARD, 32, 0);
        spi_send_data_normal_dma(SPI_DMA_CH, SPI_CHANNEL, LCD_SPI_SLAVE_SELECT, data_buf, length, SPI_TRANS_INT);
    }
}

void tft_fill_data(uint32_t *data_buf, uint32_t length)
{
    set_dcx_data();
    if(!standard_spi)
    {
        spi_init(SPI_CHANNEL, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 0);
        spi_init_non_standard(SPI_CHANNEL, 0 /*instrction length*/, 32 /*address length*/, 0 /*wait cycles*/,
                            SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
        spi_fill_data_dma(SPI_DMA_CH, SPI_CHANNEL, LCD_SPI_SLAVE_SELECT, data_buf, length);
    }
    else
    {
        spi_init(SPI_CHANNEL, standard_work_mode, SPI_FF_STANDARD, 32, 0);
        spi_fill_data_dma(SPI_DMA_CH, SPI_CHANNEL, LCD_SPI_SLAVE_SELECT, data_buf, length);
    }
}

void tft_set_datawidth(uint8_t width)
{

}


