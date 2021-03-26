#include "hub75e.h"
#include "color_table.h"

#define SWAP_TO_MP16(x) (((x)>>8) | ((x)<<8&0xff00)) // grbg -> rgb
#define HEIGHT_PER_BOARD  64
#define WIDTH_PER_BOARD 64
#define SCAN_TIMES 32

// cur -> current
static hub75e_t* hub75e_obj = 0;
static uint16_t* image = 0;
static uint16_t vertical_boards;
static uint16_t line_buf_size;
static uint16_t *rgb444;
// 建立两个libuffer0/1, 当发送 line_buffer0 时， 填充 line_buffer1, 发送 1 时， 填充 0，不必等待发送完毕
static uint8_t *line_buffer0;
static uint8_t *line_buffer1;
static uint8_t *line_buffer;
static volatile spi_t *spi_handle;

static inline void my_set_gpiohs(uint8_t io, uint8_t val)
{
    if(val) GPIOHS_OUT_HIGH(io);
    else GPIOHS_OUT_LOW(io);
}

static inline void hub75e_set_addr(hub75e_t* hub75e_obj, uint8_t addr)
{
    my_set_gpiohs(hub75e_obj->a_gpio, addr & 0x01);
    my_set_gpiohs(hub75e_obj->b_gpio, (addr & 0x02));
    my_set_gpiohs(hub75e_obj->c_gpio, (addr & 0x04));
    my_set_gpiohs(hub75e_obj->d_gpio, (addr & 0x08));
    my_set_gpiohs(hub75e_obj->e_gpio, (addr & 0x10));
}


static inline void enable_hub75e(hub75e_t* hub75e_obj)
{
    GPIOHS_OUT_LOW(hub75e_obj->oe_gpio);
}
static inline void disable_hub75e(hub75e_t* hub75e_obj)
{
    GPIOHS_OUT_HIGH(hub75e_obj->oe_gpio);
}

static inline void latch_hub75e(hub75e_t* hub75e_obj)
{
    GPIOHS_OUT_HIGH(hub75e_obj->latch_gpio);
}

static inline void unlatch_hub75e(hub75e_t* hub75e_obj)
{
    GPIOHS_OUT_LOW(hub75e_obj->latch_gpio);
}

void hub75e_init(hub75e_t* hub75e_obj)
{
    dmac_init();
    spi_init(hub75e_obj->spi, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 1);
    spi_init_non_standard(hub75e_obj->spi, 0 /*instrction length*/,
                          32 /*address length*/, 0 /*wait cycles*/,
                          SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
    spi_set_clk_rate(hub75e_obj->spi, 25000000);

    if(hub75e_obj->spi == 1){
        fpioa_set_function(hub75e_obj->r1_pin, HUB75E_FUN_SPIxDv(1, 7));
        fpioa_set_function(hub75e_obj->g1_pin, HUB75E_FUN_SPIxDv(1, 6));
        fpioa_set_function(hub75e_obj->b1_pin, HUB75E_FUN_SPIxDv(1, 5));
        fpioa_set_function(hub75e_obj->r2_pin, HUB75E_FUN_SPIxDv(1, 4));
        fpioa_set_function(hub75e_obj->g2_pin, HUB75E_FUN_SPIxDv(1, 3));
        fpioa_set_function(hub75e_obj->b2_pin, HUB75E_FUN_SPIxDv(1, 2));
        fpioa_set_function(hub75e_obj->clk_pin, FUNC_SPI1_SCLK);
        fpioa_set_function(hub75e_obj->cs_pin, FUNC_SPI1_SS3);
    }else if(hub75e_obj->spi == 0){
        fpioa_set_function(hub75e_obj->r1_pin, HUB75E_FUN_SPIxDv(0, 7));
        fpioa_set_function(hub75e_obj->g1_pin, HUB75E_FUN_SPIxDv(0, 6));
        fpioa_set_function(hub75e_obj->b1_pin, HUB75E_FUN_SPIxDv(0, 5));
        fpioa_set_function(hub75e_obj->r2_pin, HUB75E_FUN_SPIxDv(0, 4));
        fpioa_set_function(hub75e_obj->g2_pin, HUB75E_FUN_SPIxDv(0, 3));
        fpioa_set_function(hub75e_obj->b2_pin, HUB75E_FUN_SPIxDv(0, 2));
        fpioa_set_function(hub75e_obj->clk_pin, FUNC_SPI0_SCLK);
        fpioa_set_function(hub75e_obj->cs_pin, FUNC_SPI0_SS3);
        sysctl_set_spi0_dvp_data(1);
    }else{
        printf("Error spi num should be 1 or 0\r\n");
        return;
    }

    gpiohs_set_drive_mode(hub75e_obj->a_gpio, GPIO_DM_OUTPUT);
    gpiohs_set_pin(hub75e_obj->a_gpio, GPIO_PV_LOW);

    gpiohs_set_drive_mode(hub75e_obj->b_gpio, GPIO_DM_OUTPUT);
    gpiohs_set_pin(hub75e_obj->b_gpio, GPIO_PV_LOW);

    gpiohs_set_drive_mode(hub75e_obj->c_gpio, GPIO_DM_OUTPUT);
    gpiohs_set_pin(hub75e_obj->c_gpio, GPIO_PV_LOW);

    gpiohs_set_drive_mode(hub75e_obj->d_gpio, GPIO_DM_OUTPUT);
    gpiohs_set_pin(hub75e_obj->d_gpio, GPIO_PV_LOW);

    gpiohs_set_drive_mode(hub75e_obj->e_gpio, GPIO_DM_OUTPUT);
    gpiohs_set_pin(hub75e_obj->e_gpio, GPIO_PV_LOW);

    gpiohs_set_drive_mode(hub75e_obj->oe_gpio, GPIO_DM_OUTPUT);
    gpiohs_set_pin(hub75e_obj->oe_gpio, GPIO_PV_LOW);

    gpiohs_set_drive_mode(hub75e_obj->latch_gpio, GPIO_DM_OUTPUT);
    gpiohs_set_pin(hub75e_obj->latch_gpio, GPIO_PV_LOW);


    vertical_boards = hub75e_obj->height / HEIGHT_PER_BOARD;
    line_buf_size = hub75e_obj->width * vertical_boards;
    rgb444 = (uint16_t *)malloc(hub75e_obj->width * hub75e_obj->height * sizeof(uint16_t));
    line_buffer0 = (uint8_t *)malloc(line_buf_size);
    line_buffer1 = (uint8_t *)malloc(line_buf_size);
    line_buffer = line_buffer0;
    spi_handle = spi[hub75e_obj->spi];

    disable_hub75e(hub75e_obj); //防止初始化时闪烁
}

static inline void fill_line(hub75e_t *hub75e_obj, const int addr)
{
    dmac_wait_done(hub75e_obj->dma_channel);
    while ((spi_handle->sr & 0x05) != 0x04)
        ;
    spi_handle->ser = 0x00;
    spi_handle->ssienr = 0x00;
    
    latch_hub75e(hub75e_obj);
    unlatch_hub75e(hub75e_obj);
    hub75e_set_addr(hub75e_obj, addr-1); // 选择行地址
    enable_hub75e(hub75e_obj);           // 开始显示

    // 传输行数据, 以下相当于 spi_send_data_normal_dma 函数
    set_bit(&spi_handle->ctrlr0, 3 << 8, SPI_TMOD_TRANS << 8);

    spi_handle->dmacr = 0x2; /*enable dma transmit*/
    spi_handle->ssienr = 0x01;
    sysctl_dma_select((sysctl_dma_channel_t)(hub75e_obj->dma_channel),
                      SYSCTL_DMA_SELECT_SSI0_TX_REQ + hub75e_obj->spi * 2);
    dmac_set_single_mode(hub75e_obj->dma_channel, line_buffer,
                         (void *)(&spi_handle->dr[0]), DMAC_ADDR_INCREMENT,
                         DMAC_ADDR_NOCHANGE, DMAC_MSIZE_4, DMAC_TRANS_WIDTH_32,
                         line_buf_size/4);
    spi_handle->ser = 1U << SPI_CHIP_SELECT_3;

}

int hub75e_display(int core)
{
    if(!(hub75e_obj&&image)) return -1;
    int y, t, x;
    // rgb565 -> rgb444
    for (y = 0; y < hub75e_obj->height; y++)
    {
        for (x = 0; x < hub75e_obj->width; x++)
        {
            *(rgb444 + y * hub75e_obj->width + x) = rgb565_to_rgb444[SWAP_TO_MP16(*(image + y * hub75e_obj->width + x))];
        }
    }
    
    // 每张图刷新 16 次, 可以达到用占空比控制的效果, 16 为 4 位所能表示的全部色彩
    for (t = 0; t < 16; t++)
    {
        // 32 扫, 每次将 y 和 y+32 行填入 linebuffer       
        for (y = 0; y < SCAN_TIMES; y++)
        {
            // 每次发送一个 linebuffer ， linebuffer 大小为 每块的行宽*一行的块数*竖直的板子数
            for (int bs = vertical_boards; bs > 0; bs--)//刷新第 bs 行的板子， 板子总行数为 vertical_boards
            {
                // line_buffer 填充起点
                int line_buf_base_index = ((vertical_boards - bs)*hub75e_obj->width);
                if(bs & 0x01){// 垂直第奇数块所在行, 刷新顺序从上到下， 从左至右
                    // 填入 line-buffer 的 img 行号
                    int img_line_num = (bs - 1) * HEIGHT_PER_BOARD + y;
                    // 当前显示 img 行的出发点
                    int img_line_begin = img_line_num * hub75e_obj->width;
                    // 填入 linebuffer 的 img 行加 32 扫
                    int img_line_scan_begin = (img_line_num + SCAN_TIMES) * hub75e_obj->width;
                    // 显示第 bs 块板的第 y 行
                    for(int  x = 0; x < hub75e_obj->width; x++)
                    {
                        // 编码每行的点，高三位: 7(r1),6(g1),5(b1)为第 img_line_num 行), 后三位: 4(r2),3(g2),2(b2) 为第 img_line_num + SCAN_TIMES 行)
                        line_buffer[x+line_buf_base_index] = ((pwm_table[t][*(rgb444 + img_line_begin + x)]) | \
                                        pwm_table[t][*(rgb444 + img_line_scan_begin + x)] >> 3);
                    }
                }else{// 垂直第偶数行板子, 刷新顺序从下到上， 从右至左
                    // 填入 linebuffer 的 img 行号
                    int img_line_num = bs * HEIGHT_PER_BOARD - y - 1;
                    // 填入 linebuffer 的 img 出发点
                    int img_line_begin = img_line_num * hub75e_obj->width;
                    // 填入 linebuffer 的 img 行加 32 扫, 因为从下往上
                    int img_line_scan_begin  = (img_line_num - SCAN_TIMES) * hub75e_obj->width;
                    // 需要填充的 linebuffer 的结束点下标
                    int line_buf_end_index = line_buf_base_index + hub75e_obj->width - 1;
                    for(int  x = hub75e_obj->width - 1; x >= 0; x--)
                    {
                        // 编码每行的点， 一次两行， 高三位: 7(r1),6(g1),5(b1)为第 img_line_num 行), 后三位: 4(r2),3(g2),2(b2) 为第 img_line_num - SCAN_TIMES 行)
                        line_buffer[line_buf_end_index - x] = ((pwm_table[t][*(rgb444 + img_line_begin + x)]) | \
                                        pwm_table[t][*(rgb444 + img_line_scan_begin + x)] >> 3);
                    }
                }
            }
            fill_line(hub75e_obj, y); // 发送行数据
            if(line_buffer == line_buffer0)
                line_buffer = line_buffer1;
            else
                line_buffer = line_buffer0;
        }
    }   
    return 0;
}

typedef int (*dual_func_t)(int);
extern volatile dual_func_t dual_func;
volatile int hub75e_display_lock = 0;

void hub75e_display_start(hub75e_t* cur_hub75e_obj, uint16_t *cur_image)
{
    hub75e_obj = cur_hub75e_obj;
    image = cur_image;
    dual_func = hub75e_display;    
    hub75e_display_lock = 1;
    // printf("hub75e_display_start: %p\r\n", hub75e_display);
}

void hub75e_display_stop(void)
{
    hub75e_obj = 0;
    image = 0;
    dual_func = 0;
    free(line_buffer0);
    free(line_buffer1);
    free(rgb444);    
    hub75e_display_lock = 0;
}