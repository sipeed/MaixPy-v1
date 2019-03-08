#include "sipeed_sk9822.h"

#define LED_NUM 12

#define SK9822_DAT_SET()                 \
    {                                    \
        gpiohs->output_val.bits.b14 = 1; \
    }

#define SK9822_DAT_CLR()                 \
    {                                    \
        gpiohs->output_val.bits.b14 = 0; \
    }

#define SK9822_CLK_SET()                 \
    {                                    \
        gpiohs->output_val.bits.b15 = 1; \
    }

#define SK9822_CLK_CLR()                 \
    {                                    \
        gpiohs->output_val.bits.b15 = 0; \
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
        printf("0x%x\r\n", led_frame[index]);
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//led colormap

uint8_t led_brightness[12] = {0}; //mic array led brightness,to figure the direction

uint8_t voice_strength_len[12] = {14, 20, 14, 14, 20, 14, 14, 20, 14, 14, 20, 14};

//voice strength, to calc direction
uint8_t voice_strength[12][32] = {
    {197, 198, 199, 213, 214, 215, 228, 229, 230, 231, 244, 245, 246, 247},                               //14
    {178, 179, 192, 193, 194, 195, 196, 208, 209, 210, 211, 212, 224, 225, 226, 227, 240, 241, 242, 243}, //20
    {128, 129, 130, 131, 144, 145, 146, 147, 160, 161, 162, 163, 176, 177},
    {64, 65, 80, 81, 82, 83, 96, 97, 98, 99, 112, 113, 114, 115},
    {0, 1, 2, 3, 16, 17, 18, 19, 32, 33, 34, 35, 36, 48, 49, 50, 51, 52, 66, 67},
    {4, 5, 6, 7, 20, 21, 22, 23, 37, 38, 39, 53, 54, 55},
    {8, 9, 10, 11, 24, 25, 26, 27, 40, 41, 42, 56, 57, 58},
    {12, 13, 14, 15, 28, 29, 30, 31, 43, 44, 45, 46, 47, 59, 60, 61, 62, 63, 76, 77},
    {78, 79, 92, 93, 94, 95, 108, 109, 110, 111, 124, 125, 126, 127},
    {140, 141, 142, 143, 156, 157, 158, 159, 173, 172, 174, 175, 190, 191},
    {188, 189, 203, 204, 205, 206, 207, 219, 220, 221, 222, 223, 236, 237, 238, 239, 252, 253, 254, 255},
    {200, 201, 202, 216, 217, 218, 232, 233, 234, 235, 248, 249, 250, 251},
};

void sipeed_init_mic_array_led(void)
{
    sk9822_init();
    //flash 3 times
    sk9822_flash(0xffeec900, 0xff000000, 200);
    sk9822_flash(0xffeec900, 0xff000000, 200);
    sk9822_flash(0xffeec900, 0xff000000, 200);
}

void sipeed_calc_voice_strength(uint8_t voice_data[])
{
    uint32_t tmp_sum[12] = {0};
    uint32_t led_color[12];
    uint8_t i, index, tmp;

    for (index = 0; index < 12; index++)
    {
        tmp_sum[index] = 0;
        for (i = 0; i < voice_strength_len[index]; i++)
        {
            tmp_sum[index] += voice_data[voice_strength[index][i]];
        }
        tmp = (uint8_t)tmp_sum[index] / voice_strength_len[index];
        led_brightness[index] = tmp > 15 ? 15 : tmp;
    }
    sk9822_start_frame();
    for (index = 0; index < 12; index++)
    {
        led_color[index] = (led_brightness[index] / 2) > 1 ? (((0xe0 | (led_brightness[index] * 2)) << 24) | 0xcd3333) : 0xe0000000;
        sk9822_send_data(led_color[index]);
    }
    sk9822_stop_frame();
}
