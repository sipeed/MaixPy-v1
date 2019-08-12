#include <stdlib.h>
#include <string.h>

#include <sysctl.h>
#include <fpioa.h>

#include "ws2812.h"

ws2812_info *ws2812_init_buf(uint32_t num)
{
    ws2812_info *ws = malloc(sizeof(ws2812_info));

    if (ws == NULL)
        return NULL;
    ws->ws_num = num;
    ws->ws_buf = malloc(num * sizeof(ws2812_data));
    if (ws->ws_buf == NULL) {
        free(ws);
        return NULL;
    }
    memset(ws->ws_buf, 0, num * sizeof(ws2812_data));
    return ws;
}

bool ws2812_release_buf(ws2812_info *ws)
{
    if (ws == NULL)
        return false;

    if (ws->ws_buf == NULL)
        return false;

    free(ws->ws_buf);
    free(ws);
    return true;
}

bool ws2812_clear(ws2812_info *ws)
{
    if (ws == NULL)
        return false;

    if (ws->ws_buf == NULL)
        return false;

    memset(ws->ws_buf, 0, ws->ws_num * sizeof(ws2812_data));

    return true;
}

bool ws2812_set_data(ws2812_info *ws, uint32_t num, uint8_t r, uint8_t g, uint8_t b)
{
    if (ws == NULL)
        return false;

    if (ws->ws_buf == NULL)
        return false;
    if (num >= ws->ws_num)
        return false;

    (ws->ws_buf + num)->red = r;
    (ws->ws_buf + num)->green = g;
    (ws->ws_buf + num)->blue = b;

    return true;
}

void ws2812_init_i2s(uint8_t pin, i2s_device_number_t i2s_num, i2s_channel_num_t channel)
{
    fpioa_set_function(pin, FUNC_I2S0_OUT_D0 + channel + i2s_num * 11);
    uint32_t pll2_rate = sysctl_clock_get_freq(SYSCTL_SOURCE_PLL2);
    uint32_t sample_rate = pll2_rate / (2 * 32 * 2) - 1;
    i2s_set_sample_rate(i2s_num, sample_rate);
    i2s_init(i2s_num, I2S_TRANSMITTER, 0x3 << 2 *channel);
    i2s_tx_channel_config(i2s_num, channel,
                        RESOLUTION_32_BIT, SCLK_CYCLES_32,
                        TRIGGER_LEVEL_4,
                        LEFT_JUSTIFYING_MODE
                        );
}

void ws2812_i2s_enable_channel(i2s_device_number_t i2s_num, i2s_channel_num_t channel)
{
    uint32_t pll2_rate = sysctl_clock_get_freq(SYSCTL_SOURCE_PLL2);
    uint32_t sample_rate = pll2_rate / (2 * 32 * 2) - 1;
    i2s_set_sample_rate(i2s_num, sample_rate);
    i2s_init(i2s_num, I2S_TRANSMITTER, 0x3 << 2 *channel);
    i2s_tx_channel_config(i2s_num, channel,
                        RESOLUTION_32_BIT, SCLK_CYCLES_32,
                        TRIGGER_LEVEL_4,
                        LEFT_JUSTIFYING_MODE
                        );
}

bool ws2812_send_data_i2s(i2s_device_number_t i2s_num, dmac_channel_number_t dmac_num, ws2812_info *ws)
{
    uint32_t longbit;
    uint32_t shortbit;
    uint32_t resbit;

    if (ws == NULL)
        return false;

    if (ws->ws_buf == NULL)
        return false;

    size_t ws_cnt = ws->ws_num;
    uint32_t *ws_data = (uint32_t *)ws->ws_buf;

    uint32_t freq_i2s = sysctl_clock_get_freq(SYSCTL_CLOCK_I2S0 + i2s_num);

    double clk_time = 1e9 / freq_i2s;   // ns per clk

    uint32_t longtime = 850 / clk_time * clk_time;

    if (longtime < 700)
        longbit = 850 / clk_time + 1;
    else
        longbit = 850 / clk_time;

    uint32_t shortime = 400 / clk_time * clk_time;

    if (shortime < 250)
        shortbit = 400 / clk_time + 1;
    else
        shortbit = 400 / clk_time;

    resbit = (400000 / clk_time);   // > 300us

    uint32_t spi_send_cnt = (((ws_cnt * 24 * (longbit + shortbit) + resbit) / 8) + 7) / 4; //加最后的reset
    uint32_t reset_cnt = (resbit + 7) / 8 / 4;      //先reset
    uint32_t *tmp_i2s_data = malloc((spi_send_cnt + reset_cnt) * 4);

    if (tmp_i2s_data == NULL)
        return false;

    memset(tmp_i2s_data, 0, (spi_send_cnt + reset_cnt) * 4);
    uint32_t *spi_data = tmp_i2s_data;

    spi_data += reset_cnt;
    int pos = 31;
    uint32_t long_cnt = longbit;
    uint32_t short_cnt = shortbit;

    for (int i = 0; i < ws_cnt; i++) {

        for (uint32_t mask = 0x800000; mask > 0; mask >>= 1) {

            if (ws_data[i] & mask) {

                while (long_cnt--) {
                    *(spi_data) |= (1 << (pos--));
                    if (pos < 0) {
                        spi_data++;
                        pos = 31;
                    }
                }
                long_cnt = longbit;

                while (short_cnt--) {
                    *(spi_data) &= ~(1 << (pos--));
                    if (pos < 0) {
                        spi_data++;
                        pos = 31;
                    }
                }
                short_cnt = shortbit;

            } else{
                while (short_cnt--) {
                    *(spi_data) |= (1 << (pos--));
                    if (pos < 0) {
                        spi_data++;
                        pos = 31;
                    }
                }
                short_cnt = shortbit;

                while (long_cnt--) {
                    *(spi_data) &= ~(1 << (pos--));
                    if (pos < 0) {
                        spi_data++;
                        pos = 31;
                    }
                }
                long_cnt = longbit;


            }
        }
    }
    i2s_send_data_dma(i2s_num, tmp_i2s_data, spi_send_cnt + reset_cnt, dmac_num);
    free(tmp_i2s_data);
    return true;
}
