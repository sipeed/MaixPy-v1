#include "sipeed_sk9822.h"

#define LED_NUM 12

#define SK9822_DAT_SET()                 \
    {                                    \
        gpiohs->output_val.bits.b27 = 1; \
    }

#define SK9822_DAT_CLR()                 \
    {                                    \
        gpiohs->output_val.bits.b27 = 0; \
    }

#define SK9822_CLK_SET()                 \
    {                                    \
        gpiohs->output_val.bits.b28 = 1; \
    }

#define SK9822_CLK_CLR()                 \
    {                                    \
        gpiohs->output_val.bits.b28 = 0; \
    }

static void sk9822_init(void)
{
    gpiohs_set_drive_mode(SK9822_DAT_GPIONUM, GPIO_DM_OUTPUT);
    gpiohs_set_drive_mode(SK9822_CLK_GPIONUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(SK9822_DAT_GPIONUM, 0);
    gpiohs_set_pin(SK9822_CLK_GPIONUM, 0);
}

void sk9822_send_data(uint32_t data)
{
    for (uint32_t mask = 0x80000000; mask > 0; mask >>= 1)
    {
        SK9822_CLK_CLR();
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        // usleep(1);
        if (data & mask)
        {
            SK9822_DAT_SET();
        }
        else
        {
            SK9822_DAT_CLR();
        }
        SK9822_CLK_SET();
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        // usleep(2);
    }
}

//32bit 0
void sk9822_start_frame(void)
{
    sk9822_send_data(0);
}

//32bit 1
void sk9822_stop_frame(void)
{
    sk9822_send_data(0xffffffff);
}

//1 1 1 1 gray | b | g | r
void sk9822_data_one_led(uint8_t gray, uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t tosend;

    gray &= 0x1f; //for make sure no error data in
    tosend = ((0xe0 | gray) << 24) | (b << 16) | (g << 8) | r;
    sk9822_send_data(tosend);
}

uint32_t sk9822_gen_data_one_led(uint8_t gray, uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t tosend;

    gray &= 0x1f; //for make sure no error data in
    tosend = ((0xe0 | gray) << 24) | (b << 16) | (g << 8) | r;
    return tosend;
}

//first color1, then msleep interval, then color2, last msleep interval
void sk9822_flash(uint32_t color1, uint32_t color2, uint32_t interval)
{
    uint8_t index;

    color1 |= 0xe0000000;
    color2 |= 0xe0000000;

    sk9822_start_frame();
    for (index = 0; index < LED_NUM; index++)
    {
        sk9822_send_data(color1);
    }
    sk9822_stop_frame();
    msleep(interval);

    sk9822_start_frame();
    for (index = 0; index < LED_NUM; index++)
    {
        sk9822_send_data(color2);
    }
    sk9822_stop_frame();
    msleep(interval);
}

static void arraymove(uint32_t array[], uint8_t len)
{
    uint8_t index;
    uint32_t tmp;

    tmp = array[0];
    for (index = 0; index < len - 1; index++)
    {
        array[index] = array[index + 1];
    }
    array[len - 1] = tmp;
}
//呼吸
//跑马灯
//方位
void sk9822_horse_race(uint8_t r, uint8_t g, uint8_t b, uint32_t interval, uint8_t times)
{
    uint32_t led_frame[LED_NUM] = {0};
    uint8_t i, index;

    for (index = 0; index < 12; index++)
    {
        led_frame[index] = 0xff000000;
    }

    for (index = 0; index < 6; index++)
    {
        // led_frame[index] = sk9822_gen_data_one_led((0xe0|(index*4)),r-40*index,g-30*index,b-20*index);
        led_frame[index] = sk9822_gen_data_one_led((0xe0 | (32 - index * 4)), r, g, b);
    }

    for (index = 0; index < times; index++)
    {
        while (1)
        {
            sk9822_start_frame();
            for (i = 0; i < 12; i++)
            {
                sk9822_send_data(led_frame[i]);
            }
            sk9822_stop_frame();
            arraymove(led_frame, LED_NUM);
            msleep(interval);
        }
    }
}

void sk9822_breath(uint8_t r, uint8_t g, uint8_t b, uint32_t interval)
{
    uint8_t index, cnt, dir;

    uint32_t color = sk9822_gen_data_one_led(0xff, r, g, b);

    cnt = 0;
    dir = 1;

    while (1)
    {
        if (cnt >= 30)
        {
            dir = !dir;
            cnt = 0;
        }
        cnt++;
        color = sk9822_gen_data_one_led((0xe0 | (dir ? cnt : 31 - cnt)), r, g, b);
        sk9822_start_frame();
        for (index = 0; index < LED_NUM; index++)
        {
            sk9822_send_data(color);
        }
        sk9822_stop_frame();
        msleep(interval);
    }
}

void sipeed_init_mic_array_led(void)
{
    sk9822_init();
    //flash 3 times
    sk9822_flash(0xffeec900, 0xffff0000, 200);
    sk9822_flash(0xffeec900, 0xff00ff00, 200);
    sk9822_flash(0xffeec900, 0xff0000ff, 200);
}

// void sipeed_calc_voice_strength(uint8_t voice_data[])
// {
//     uint32_t tmp_sum[12] = {0};
//     uint32_t led_color[12];
//     uint8_t i, index, tmp;

//     for (index = 0; index < 12; index++)
//     {
//         tmp_sum[index] = 0;
//         for (i = 0; i < voice_strength_len[index]; i++)
//         {
//             tmp_sum[index] += voice_data[voice_strength[index][i]];
//         }
//         tmp = (uint8_t)tmp_sum[index] / voice_strength_len[index];
//         led_brightness[index] = tmp > 15 ? 15 : tmp;
//     }
//     sk9822_start_frame();
//     for (index = 0; index < 12; index++)
//     {
//         led_color[index] = (led_brightness[index] / 2) > 1 ? (((0xe0 | (led_brightness[index] * 2)) << 24) | 0xcd3333) : 0xe0000000;
//         sk9822_send_data(led_color[index]);
//     }
//     sk9822_stop_frame();
// }
