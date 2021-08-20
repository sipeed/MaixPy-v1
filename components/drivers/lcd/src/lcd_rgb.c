#include <string.h>
#include <stdlib.h>

#include "gpiohs.h"
#include "sleep.h"
#include "timer.h"

#include "printf.h"
#include "spi.h"
#include "dmac.h"
#include "utils.h"
#include "sysctl.h"
#include "fpioa.h"

#include "lcd.h"

#define LCD_WRITE_REG (0x80)

#define LCD_SCALE_DISABLE (0x00)
#define LCD_SCALE_ENABLE (0x01)

#define LCD_SCALE_NONE (0x00)
#define LCD_SCALE_2X2 (0x01)
#define LCD_SCALE_3X3 (0x02)
#define LCD_SCALE_4X4 (0x03)

#define LCD_SCALE_X (0x00)
#define LCD_SCALE_Y (0x01)

#define LCD_PHASE_PCLK (0x00)
#define LCD_PHASE_HSYNC (0x01)
#define LCD_PHASE_VSYNC (0x02)
#define LCD_PHASE_DE (0x03)

#define LCD_ADDR_VBP (0x00)
#define LCD_ADDR_H (0x01)
#define LCD_ADDR_VFP (0x02)
#define LCD_ADDR_HBP (0x03)
#define LCD_ADDR_W (0x04)
#define LCD_ADDR_HFP (0x05)
#define LCD_ADDR_DIVCFG (0x06) //b7:0 dis,1 en; b6: 0x,1y; b[5:3]: mul part1; b[2:0]: mul part2
#define LCD_ADDR_POS (0x07)
#define LCD_ADDR_PHASE (0x08) //b0: pclk b1: hsync b2:vsync b3:de
#define LCD_ADDR_START (0x09)
#define LCD_ADDR_INITDONE (0x0A)

//实用最高频率100M，更高FPC延长线无法工作，需要onboard。另外FPGA的FIFO IP读写时钟速率限制在120M左右
#define LCD_INIT_LINE (0) /* 在timer_callback耽搁的行数 */

#define LCD_TIMER TIMER_DEVICE_2
#define LCD_TIMER_CHN TIMER_CHANNEL_3

volatile uint8_t dis_flag = 0;

static int32_t line_count = 0; // 当前发送的行号
static uint8_t *lcd_main = NULL;
static uint8_t *disp_banner_buf = NULL;
static uint16_t sipeed_lcd_w = 0;
static uint16_t sipeed_lcd_h = 0;

static void (*lcd_irq_rs_sync)(void); // 发送行数据回调

static uint8_t invert = 0;          // 反色
static bool bgr_to_rgb = false;     // 是否启用 bgr 转 rgb
static lcd_dir_t dir = DIR_YX_RLDU; // 屏幕方向

// sipeed 转接板配置参数
typedef struct
{ //屏幕基础参数
    uint16_t width;
    uint16_t height;
    uint8_t vbp;
    uint8_t h;
    uint8_t vfp;
    uint8_t hbp;
    uint8_t w;
    uint8_t hfp;
    //以下是屏幕分割和倍率配置
    //b7:0 dis,1 en; b6: 0x,1y; b[5:3]: mul part1; b[2:0]: mul part2
    uint8_t enable;
    uint8_t div_xy;
    uint8_t div_pos;
    // 缩放参数
    uint8_t mula_M;
    uint8_t mula_N;
    uint8_t mulb_M;
    uint8_t mulb_N;
    //以下是信号极性配置
    uint8_t phase_de;
    uint8_t phase_vsync;
    uint8_t phase_hsync;
    uint8_t phase_pclk;
    //刷新周期设置,ms
    uint8_t refresh_time; // 定时器周期
    uint32_t spi_speed;   // Hz为单位
} rgb_lcd_ctl_t;
static rgb_lcd_ctl_t *converter_ctl_para = NULL; // 转接板专用参数

extern const uint16_t gray2rgb565[64];
extern void gpiohs_irq_disable(size_t pin);
extern void spi_send_data_normal(spi_device_num_t spi_num,
                                 spi_chip_select_t chip_select,
                                 const uint8_t *tx_buff, size_t tx_len);

// 不同型号屏幕的不同参数
static rgb_lcd_ctl_t lcd_para_4p3 = {
    .width = 480, .height = 272, .hbp = 43, .w = 480 / 4, .hfp = 12, .vbp = 4, .h = 272 / 4, .vfp = 12, .enable = 0, .div_xy = 0, .div_pos = 640 / 4, .mula_M = 1, .mula_N = 1, .mulb_M = 1, .mulb_N = 1, .phase_de = 1, .phase_vsync = 1, .phase_hsync = 1, .phase_pclk = 0, .refresh_time = 23, .spi_speed = 80 * 1000 * 1000};

static rgb_lcd_ctl_t lcd_para_5p0_ips = {
    .vbp = 30, .h = 856 / 4, .vfp = 12, .hbp = 252, .w = 484 / 4, .hfp = 40, .enable = 1, .div_xy = 1, .div_pos = 640 / 4, //y方向分割到640
    .mula_M = 1,
    .mula_N = 2, //2倍缩放
    .mulb_M = 1,
    .mulb_N = 1, //不缩放
    .phase_de = 1,
    .phase_vsync = 1,
    .phase_hsync = 1,
    .phase_pclk = 0,
    .refresh_time = 25,
    .spi_speed = 80 * 1000 * 1000};

static rgb_lcd_ctl_t lcd_para_5p0_7p0 = {
    .width = 800, .height = 480, .vbp = 0, .vfp = 0, .hbp = 180, .hfp = 1,.w = 800 / 4, .h = 480 / 4, .enable = 1, .div_xy = 0, .div_pos = 800 / 4, //x方向分割到800
    .mula_M = 1,
    .mula_N = 2, //2倍缩放
    .mulb_M = 1,
    .mulb_N = 1, //不缩放
    .phase_de = 1,
    .phase_vsync = 10,
    .phase_hsync = 1,
    .phase_pclk = 1,
    .refresh_time = 28,
    .spi_speed = 80 * 1000 * 1000};

// 数据传输函数
static void rgb_lcd_send_cmd(uint8_t CMDData)
{
    gpiohs_set_drive_mode(DCX_GPIONUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(DCX_GPIONUM, GPIO_PV_HIGH);

    spi_init(SPI_CHANNEL, SPI_WORK_MODE_0, SPI_FF_OCTAL, 8, 0);
    spi_init_non_standard(SPI_CHANNEL, 8 /*instrction length*/, 0 /*address length*/, 0 /*wait cycles*/,
                          SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);

    spi_send_data_normal(SPI_CHANNEL, LCD_SPI_SLAVE_SELECT, (uint8_t *)(&CMDData), 1);
}

static inline void rgb_lcd_send_dat(uint8_t *DataBuf, uint32_t Length)
{
    spi_send_data_normal_dma(SPI_DMA_CH, SPI_CHANNEL, LCD_SPI_SLAVE_SELECT, DataBuf, Length / 4, SPI_TRANS_INT);
}

static void lcd_480272_irq_rs_sync(void)
{
    if (line_count >= 0)
    {
        if (line_count < 272)
        {
            dis_flag = 1;
            rgb_lcd_send_dat(&lcd_main[line_count / converter_ctl_para->mula_N * sipeed_lcd_w * 2], sipeed_lcd_w * 2);
        }
        else if ((line_count >= (272 + converter_ctl_para->vfp - 1)))
        {
            dis_flag = 0;
        }
    }
    line_count++;
}

static void lcd_480854_irq_rs_sync(void)
{
    int32_t tmp = line_count;
    line_count -= converter_ctl_para->vbp;

    if (line_count >= 0)
    {
        if (line_count < 640)
        {
            dis_flag = 1;
            rgb_lcd_send_dat(&lcd_main[(line_count / converter_ctl_para->mula_N) * sipeed_lcd_w * 2], sipeed_lcd_w * 2 + 8);
        }
        else if (line_count < 854)
        {
            dis_flag = 1;
#if LCD_SIPEED_SWAP_LINE
            uint16_t tt = (line_count - 640);
            tt = (tt % 2) ? (tt - 1) : (tt + 1);
            rgb_lcd_send_dat(&disp_banner_buf[tt * 480 * 2], 480 * 2 + 8);
#else
            rgb_lcd_send_dat(&disp_banner_buf[(line_count - 640) * 480 * 2], 480 * 2 + 8);
#endif
        }
        else if ((line_count >= (856 + converter_ctl_para->vfp - 1)))
        {
            dis_flag = 0;
        }
    }

    line_count = tmp;
    line_count++;
}

static void lcd_800480_irq_rs_sync(void)
{
    int32_t tmp = line_count;
    line_count -= LCD_INIT_LINE;

    if (line_count >= 0)
    {
        if (line_count < 480)
        {
            dis_flag = 1;
            rgb_lcd_send_dat(&lcd_main[line_count / converter_ctl_para->mula_N * sipeed_lcd_w * 2], sipeed_lcd_w * 2);
        }
        else if ((line_count >= (480 + converter_ctl_para->vfp - 1)))
        {
            dis_flag = 0;
        }
    }

    line_count = tmp;
    line_count++;
    return;
}

//excute in 50Hz lcd refresh irq
static void rgb_lcd_display(void)
{
    gpiohs_irq_disable(DCX_GPIONUM);
    line_count = LCD_INIT_LINE;

    rgb_lcd_send_cmd(LCD_WRITE_REG | LCD_ADDR_START);
    rgb_lcd_send_cmd(0x0d);

    spi_init(SPI_CHANNEL, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 1);
    spi_init_non_standard(SPI_CHANNEL, 0, 32, 0, SPI_AITM_AS_FRAME_FORMAT);

    gpiohs_set_drive_mode(DCX_GPIONUM, GPIO_DM_INPUT);
    gpiohs_set_pin_edge(DCX_GPIONUM, GPIO_PE_RISING);

    gpiohs_set_irq(DCX_GPIONUM, 1, lcd_irq_rs_sync);
}

int timer_callback(void *ctx)
{
    dis_flag = 1;
    rgb_lcd_display();
    return 0;
}

// 转接板相关函数
static uint8_t cal_div_reg(uint8_t enable, uint8_t div_xy, uint8_t mula_M, uint8_t mula_N, uint8_t mulb_M, uint8_t mulb_N)
{
    if (mula_M > 2 || mulb_M > 2)
    {
        printk("mul_M should = 1 or 2\r\n");
    }
    mula_N -= (mula_M - 1);
    mulb_N -= (mulb_M - 1);
    return (enable << 7) | (div_xy << 6) | ((mula_M - 1) << 5) | ((mula_N - 1) << 3) | ((mulb_M - 1) << 2) | ((mulb_N - 1) << 0);
}

// 配置转接板
static void rgb_lcd_init_seq(const rgb_lcd_ctl_t *para)
{
    rgb_lcd_send_cmd(LCD_WRITE_REG | LCD_ADDR_VBP);
    rgb_lcd_send_cmd((para->vbp + 2));
    rgb_lcd_send_cmd(LCD_WRITE_REG | LCD_ADDR_H);
    rgb_lcd_send_cmd(para->h); //多了2行
    rgb_lcd_send_cmd(LCD_WRITE_REG | LCD_ADDR_VFP);
    rgb_lcd_send_cmd(para->vfp);
    rgb_lcd_send_cmd(LCD_WRITE_REG | LCD_ADDR_HBP);
    rgb_lcd_send_cmd(para->hbp);
    rgb_lcd_send_cmd(LCD_WRITE_REG | LCD_ADDR_W);
    rgb_lcd_send_cmd(para->w);
    rgb_lcd_send_cmd(LCD_WRITE_REG | LCD_ADDR_HFP);
    rgb_lcd_send_cmd(para->hfp);

    //b7:0 dis,1 en; b6: 0x,1y; b[5:3]: mul part1; b[2:0]: mul part2
    rgb_lcd_send_cmd(LCD_WRITE_REG | LCD_ADDR_DIVCFG);
    rgb_lcd_send_cmd(cal_div_reg(para->enable, para->div_xy, para->mula_M, para->mula_N, para->mulb_M, para->mulb_N));

    rgb_lcd_send_cmd(LCD_WRITE_REG | LCD_ADDR_POS);
    rgb_lcd_send_cmd(para->div_pos);

    rgb_lcd_send_cmd(LCD_WRITE_REG | LCD_ADDR_PHASE);
    rgb_lcd_send_cmd((para->phase_de << 3) | (para->phase_vsync << 2) | (para->phase_hsync << 1) | (para->phase_pclk << 0));

    rgb_lcd_send_cmd(LCD_WRITE_REG | LCD_ADDR_INITDONE);
    rgb_lcd_send_cmd(0x01);

    return;
}

// 以下定时器函数均交给 core 1 调用
typedef int (*dual_func_t)(int);
extern volatile dual_func_t dual_func;

static int rgb_lcd_timer_start(int core)
{
    timer_irq_register(LCD_TIMER, LCD_TIMER_CHN, 0, 1, timer_callback, NULL); //1th pri
    timer_set_enable(LCD_TIMER, LCD_TIMER_CHN, 1);
    return 0;
}

static int rgb_lcd_timer_stop(int core)
{
    timer_irq_unregister(LCD_TIMER, LCD_TIMER_CHN); //1th pri
    timer_set_enable(LCD_TIMER, LCD_TIMER_CHN, 0);
    return 0;
}

// rgb 接口 lcd 屏初始化
static int rgb_lcd_init(lcd_para_t *lcd_para)
{
    invert = lcd_para->invert;
    switch (lcd_para->lcd_type)
    {
    case LCD_TYPE_5P0_7P0:
        converter_ctl_para = &lcd_para_5p0_7p0;
        lcd_irq_rs_sync = lcd_800480_irq_rs_sync;
        break;
    case LCD_TYPE_5P0_IPS:
        lcd_irq_rs_sync = lcd_480854_irq_rs_sync;
        converter_ctl_para = &lcd_para_5p0_ips;
        break;
    case LCD_TYPE_480_272_4P3:
        converter_ctl_para = &lcd_para_4p3;
        lcd_irq_rs_sync = lcd_480272_irq_rs_sync;
        break;
    default:
        printk("error rgb lcd type!\r\n");
        return -1;
    }

    uint8_t mula_M = converter_ctl_para->mula_M;
    uint8_t mula_N = converter_ctl_para->mula_N;
    uint16_t div_xy = converter_ctl_para->div_xy;
    uint16_t div_pos = converter_ctl_para->div_pos * 4;
    uint32_t spi_speed = converter_ctl_para->spi_speed;

    if (converter_ctl_para->enable)
    {
        if (div_xy == 0)
        {
            sipeed_lcd_w = div_pos * mula_M / mula_N;
            sipeed_lcd_h = converter_ctl_para->height * mula_M / mula_N;
        }
        else
        {
            sipeed_lcd_w = converter_ctl_para->width * mula_M / mula_N;
            sipeed_lcd_h = div_pos * mula_M / mula_N;
        }
    }
    else
    {
        sipeed_lcd_w = converter_ctl_para->width;
        sipeed_lcd_h = converter_ctl_para->height;
    }

    lcd_main = (uint8_t *)malloc(sipeed_lcd_w * sipeed_lcd_h * 2);

    // printk("setup lcd (%d x %d)\r\n", sipeed_lcd_w, sipeed_lcd_h);
    // printk("rst: %d dcx:%d, cs: %d, sck: %d, freq: %d rt: %d\r\n", lcd_para->rst_pin,
    //         lcd_para->dcx_pin, lcd_para->cs_pin, lcd_para->clk_pin, spi_speed, converter_ctl_para->refresh_time);
    if (!lcd_main)
    {
        printk("rgb lcd malloc failed!\r\n");
        return -1;
    }

    gpiohs_set_drive_mode(DCX_GPIONUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(DCX_GPIONUM, GPIO_PV_HIGH);

    gpiohs_set_drive_mode(RST_GPIONUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(RST_GPIONUM, GPIO_PV_HIGH);

    spi_set_clk_rate(SPI_CHANNEL, spi_speed);

    //480x272:40/50M测试可用,搭配100M，9M时钟
    //800x480:50/60/70M 配166~125/25M 或 40M 配111/100/83 :16.66
    //似乎FIFO最高速率到不了200M，只能使用6分频166M
    rgb_lcd_init_seq(converter_ctl_para);

    // start display use core1
    timer_init(LCD_TIMER);
    timer_set_interval(LCD_TIMER, LCD_TIMER_CHN, converter_ctl_para->refresh_time * 1000 * 1000);
    //SPI传输时间(dis_flag=1)约20ms，需要留出10~20ms给缓冲区准备(dis_flag=0)
    //如果图像中断处理时间久，可以调慢FPGA端的像素时钟，但是注意不能比刷屏中断慢(否则就会垂直滚动画面)。
    //33M->18ms  25M->23ms  20M->29ms
    while(dual_func){} // 等待sd卡初始化结束
    dual_func = rgb_lcd_timer_start;
    while(dual_func){} // 等待 core1 执行完毕
    return 0;
}

static void rgb_lcd_deinit(void)
{
    if (!lcd_main)
    {
        dual_func = rgb_lcd_timer_stop;
        while(dual_func){} // wait core1
        free(lcd_main);
        lcd_main = NULL;
    }
    return;
}

static uint16_t rgb_lcd_get_width()
{
    return sipeed_lcd_w;
}

static uint16_t rgb_lcd_get_height()
{
    return sipeed_lcd_h;
}

void rgb_lcd_set_direction(lcd_dir_t dir_set)
{
    dir = dir_set;
    return;
}

void rgb_lcd_set_offset(uint16_t offset_w, uint16_t offset_h)
{
    return;
}

static void rgb_lcd_bgr_to_rgb(bool enable)
{
    bgr_to_rgb = enable;
    return;
}

static uint32_t rgb_lcd_get_freq()
{
    return converter_ctl_para->spi_speed;
}

static void rgb_lcd_set_freq(uint32_t freq)
{ // freq : hz 为单位
    converter_ctl_para->spi_speed = freq;
    spi_set_clk_rate(SPI_CHANNEL, freq);
    return;
}

///////////// UI ////////////
// 清屏
static void rgb_lcd_clear(uint16_t color)
{
    if (!lcd_main)
    {
        printk("rgb lcd not init!\r\n");
        return;
    }
    uint32_t size = sipeed_lcd_h * sipeed_lcd_w;
#if LCD_SWAP_COLOR_BYTES
    color = SWAP_16(color);
#endif
    for (int i = 0; i < size; i++)
    {
        ((uint16_t *)lcd_main)[i] = color;
    }
}

static inline void rgb_lcd_draw_point_rgb(uint16_t x, uint16_t y, uint16_t color)
{
    color = bgr_to_rgb ? ((color >> 11) | (color & 0x07e0) | (color << 11)) : color;

    int px = x, py = y;
    switch (dir)
    {
    case DIR_YX_RLDU:
        break;
    case DIR_YX_RLUD:
        px = sipeed_lcd_w - x;
        break;

    case DIR_XY_RLUD:
        py = x;
        px = sipeed_lcd_w - y;
        break;
    case DIR_XY_LRUD:
        py = sipeed_lcd_h - x;
        px = sipeed_lcd_w - y;
        break;

    case DIR_YX_LRUD:
        py = sipeed_lcd_h - y;
        px = sipeed_lcd_w - x;
        break;
    case DIR_YX_LRDU:
        py = sipeed_lcd_h - y;
        px = x;
        break;

    case DIR_XY_LRDU:
        px = y;
        py = sipeed_lcd_h - x;
        break;
    case DIR_XY_RLDU:
        px = y;
        py = x;
        break;
    default:
        break;
    }

    ((uint16_t *)lcd_main)[(py * sipeed_lcd_w) + px] = invert ? ~color : color;
}

static inline void rgb_lcd_draw_point_gbrg(uint16_t x, uint16_t y, uint16_t color)
{
#if LCD_SWAP_COLOR_BYTES
    color = SWAP_16(color);
#endif
    color = bgr_to_rgb ? ((color >> 11) | (color & 0x07e0) | (color << 11)) : color;

    int px = x, py = y;
    switch (dir)
    {
    case DIR_YX_RLDU:
        break;
    case DIR_YX_RLUD:
        px = sipeed_lcd_w - x;
        break;

    case DIR_XY_RLUD:
        py = x;
        px = sipeed_lcd_w - y;
        break;
    case DIR_XY_LRUD:
        py = sipeed_lcd_h - x;
        px = sipeed_lcd_w - y;
        break;

    case DIR_YX_LRUD:
        py = sipeed_lcd_h - y;
        px = sipeed_lcd_w - x;
        break;
    case DIR_YX_LRDU:
        py = sipeed_lcd_h - y;
        px = x;
        break;

    case DIR_XY_LRDU:
        px = y;
        py = sipeed_lcd_h - x;
        break;
    case DIR_XY_RLDU:
        px = y;
        py = x;
        break;
    default:
        break;
    }

    ((uint16_t *)lcd_main)[py * sipeed_lcd_w + px] = invert ? ~color : color;
}

static void rgb_lcd_fill_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    if (!lcd_main)
    {
        printk("rgb lcd not init!\r\n");
        return;
    }
    if ((x1 == x2) || (y1 == y2))
        return;
    for (int j = y1; j < y2; j++)
    {
        for (int i = x1; i < x2; i++)
        {
            rgb_lcd_draw_point_gbrg(i, j, color);
        }
    }
}

static void rgb_lcd_draw_picture(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint8_t *img)
{
    if (!lcd_main)
    {
        printk("rgb lcd not init!\r\n");
        return;
    }
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            rgb_lcd_draw_point_gbrg(x0 + j, y0 + i, ((uint16_t *)img)[i * w + j]);
        }
    }
}

//draw pic's roi on (x,y)
//x,y of LCD, w,h is pic; rx,ry,rw,rh is roi
static void rgb_lcd_draw_pic_roi(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh, uint8_t *ptr)
{
    if (!lcd_main)
    {
        printk("rgb lcd not init!\r\n");
        return;
    }

    int y_oft, x_end;
    uint8_t *p;
    for (y_oft = 0; y_oft < rh; y_oft++)
    { //draw line by line
        p = (uint8_t *)(ptr) + w * 2 * (y_oft + ry) + 2 * rx;
        x_end = x + rw;
        for (int i = x; i < x_end; i++)
        {
            rgb_lcd_draw_point_gbrg(i, y + y_oft, (*(uint16_t *)p));
            p += 2;
        }
    }
}

static void rgb_lcd_draw_pic_gray(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint8_t *ptr)
{
    if (!lcd_main)
    {
        printk("rgb lcd not init!\r\n");
        return;
    }

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j += 2)
        {
            ((uint16_t *)lcd_main)[(i + x1) * sipeed_lcd_w + j + x1] = gray2rgb565[ptr[i * width + j + 1] >> 2];
            ((uint16_t *)lcd_main)[(i + x1) * sipeed_lcd_w + j + x1 + 1] = gray2rgb565[ptr[i * width + j] >> 2];
        }
    }
}

static void rgb_lcd_draw_pic_grayroi(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh, uint8_t *ptr)
{
    if (!lcd_main)
    {
        printk("rgb lcd not init!\r\n");
        return;
    }
    int y_oft;
    uint8_t *p;
    for (y_oft = 0; y_oft < rh; y_oft++)
    { //draw line by line
        p = (uint8_t *)(ptr) + w * (y_oft + ry) + rx;
        rgb_lcd_draw_pic_gray(x, y + y_oft, rw, 1, p);
    }
    return;
}

static void rgb_lcd_draw_char(uint16_t x, uint16_t y, char c, uint16_t color)
{
    // uint8_t i = 0;
    // uint8_t j = 0;
    // uint8_t data = 0;

    // for (i = 0; i < 16; i++)
    // {
    //     data = ascii0816[c * 16 + i];
    //     for (j = 0; j < 8; j++)
    //     {
    //         if (data & 0x80)
    //             rgb_lcd_draw_point_gbrg(x + j, y, color);
    //         data <<= 1;
    //     }
    //     y++;
    // }
}

static void rgb_lcd_draw_string(uint16_t x, uint16_t y, char *str, uint16_t color)
{
    // if(!lcd_main) {
    //     printk("rgb lcd not init!\r\n");
    //     return -1;
    // }
    // while (*str)
    // {
    //     rgb_lcd_draw_char(x, y, *str, color);
    //     str++;
    //     x += 8;
    // }
}

/************************* RGB 屏参数  ****************************/

static lcd_para_t rgb_lcd_default = {
    .lcd_type = LCD_TYPE_5P0_7P0,
    .width = 800,
    .height = 480,
    .dir = DIR_YX_RLDU,
    .extra_para = NULL};

lcd_t lcd_rgb = {
    .lcd_para           = &rgb_lcd_default,

    .init               = rgb_lcd_init,
    .deinit             = rgb_lcd_deinit,
    .clear              = rgb_lcd_clear,
    .set_direction      = rgb_lcd_set_direction,
    .set_offset         = rgb_lcd_set_offset,
    .set_freq           = rgb_lcd_set_freq,
    .get_freq           = rgb_lcd_get_freq,
    .get_width          = rgb_lcd_get_width,
    .get_height         = rgb_lcd_get_height,
    .bgr_to_rgb         = rgb_lcd_bgr_to_rgb,

    .draw_point         = rgb_lcd_draw_point_rgb,
    .draw_picture       = rgb_lcd_draw_picture,
    .draw_pic_roi       = rgb_lcd_draw_pic_roi,
    .draw_pic_gray      = rgb_lcd_draw_pic_gray,
    .draw_pic_grayroi   = rgb_lcd_draw_pic_grayroi,
    .fill_rectangle     = rgb_lcd_fill_rectangle,
    // .draw_string     = rgb_lcd_draw_string,
};