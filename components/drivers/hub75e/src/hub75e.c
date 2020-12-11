#include "hub75e.h"

#include "colorTable.h"

void hub75e_init(hub75e_t* hub75e_obj)
{   
    if(hub75e_obj->spi == 1){
        fpioa_set_function(hub75e_obj->r1_pin, HUB75E_FUN_SPIxDv(1, 7));
        fpioa_set_function(hub75e_obj->g1_pin, HUB75E_FUN_SPIxDv(1, 6));
        fpioa_set_function(hub75e_obj->b1_pin, HUB75E_FUN_SPIxDv(1, 5));
        fpioa_set_function(hub75e_obj->r2_pin, HUB75E_FUN_SPIxDv(1, 4));
        fpioa_set_function(hub75e_obj->g2_pin, HUB75E_FUN_SPIxDv(1, 3));
        fpioa_set_function(hub75e_obj->b2_pin, HUB75E_FUN_SPIxDv(1, 2));
        fpioa_set_function(hub75e_obj->clk_pin, FUNC_SPI1_SCLK);
    }else if(hub75e_obj->spi == 0){
        fpioa_set_function(hub75e_obj->r1_pin, HUB75E_FUN_SPIxDv(0, 7));
        fpioa_set_function(hub75e_obj->g1_pin, HUB75E_FUN_SPIxDv(0, 6));
        fpioa_set_function(hub75e_obj->b1_pin, HUB75E_FUN_SPIxDv(0, 5));
        fpioa_set_function(hub75e_obj->r2_pin, HUB75E_FUN_SPIxDv(0, 4));
        fpioa_set_function(hub75e_obj->g2_pin, HUB75E_FUN_SPIxDv(0, 3));
        fpioa_set_function(hub75e_obj->b2_pin, HUB75E_FUN_SPIxDv(0, 2));
        fpioa_set_function(hub75e_obj->clk_pin, FUNC_SPI0_SCLK);
    }else{
        printf("Error spi num should be 1 or 0\r\n");
        return;
    }

    dmac_init();
    spi_init(hub75e_obj->spi, SPI_WORK_MODE_0, SPI_FF_OCTAL, 8, 0);
    spi_init_non_standard(hub75e_obj->spi, 0 /*instrction length*/,
                          8 /*address length*/, 0 /*wait cycles*/,
                          SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
    fpioa_set_function(hub75e_obj->cs_pin, FUNC_SPI1_SS3);
    spi_set_clk_rate(hub75e_obj->spi, 45000000);

    // fpioa_set_function(HUB75E_ADDR_A_PIN, HUB75E_ADDR_A_GPIOHSNUM + 24);
    // fpioa_set_io_pull(HUB75E_ADDR_A_PIN, FPIOA_PULL_UP);
    gpiohs_set_drive_mode(hub75e_obj->a_gpio, GPIO_DM_OUTPUT);
    gpiohs_set_pin(hub75e_obj->a_gpio, GPIO_PV_LOW);

    // fpioa_set_function(HUB75E_ADDR_B_PIN, HUB75E_ADDR_B_GPIOHSNUM + 24);
    // fpioa_set_io_pull(HUB75E_ADDR_B_PIN, FPIOA_PULL_UP);
    gpiohs_set_drive_mode(hub75e_obj->b_gpio, GPIO_DM_OUTPUT);
    gpiohs_set_pin(hub75e_obj->b_gpio, GPIO_PV_LOW);

    // fpioa_set_function(HUB75E_ADDR_C_PIN, HUB75E_ADDR_C_GPIOHSNUM + 24);
    // fpioa_set_io_pull(HUB75E_ADDR_C_PIN, FPIOA_PULL_UP);
    gpiohs_set_drive_mode(hub75e_obj->c_gpio, GPIO_DM_OUTPUT);
    gpiohs_set_pin(hub75e_obj->c_gpio, GPIO_PV_LOW);

    // fpioa_set_function(HUB75E_ADDR_D_PIN, HUB75E_ADDR_D_GPIOHSNUM + 24);
    // fpioa_set_io_pull(HUB75E_ADDR_D_PIN, FPIOA_PULL_UP);
    gpiohs_set_drive_mode(hub75e_obj->d_gpio, GPIO_DM_OUTPUT);
    gpiohs_set_pin(hub75e_obj->d_gpio, GPIO_PV_LOW);

    // fpioa_set_function(HUB75E_ADDR_E_PIN, HUB75E_ADDR_E_GPIOHSNUM + 24);
    // fpioa_set_io_pull(HUB75E_ADDR_E_PIN, FPIOA_PULL_UP);
    gpiohs_set_drive_mode(hub75e_obj->e_gpio, GPIO_DM_OUTPUT);
    gpiohs_set_pin(hub75e_obj->e_gpio, GPIO_PV_LOW);

    // fpioa_set_function(HUB75E_OE_PIN, HUB75E_OE_GPIOHSNUM + 24);
    // fpioa_set_io_pull(HUB75E_OE_PIN, FPIOA_PULL_UP);
    gpiohs_set_drive_mode(hub75e_obj->oe_gpio, GPIO_DM_OUTPUT);
    gpiohs_set_pin(hub75e_obj->oe_gpio, GPIO_PV_LOW);

    // fpioa_set_function(HUB75E_LATCH_PIN, HUB75E_LATCH_GPIOHSNUM + 24);
    // fpioa_set_io_pull(HUB75E_LATCH_PIN, FPIOA_PULL_UP);
    gpiohs_set_drive_mode(hub75e_obj->latch_gpio, GPIO_DM_OUTPUT);
    gpiohs_set_pin(hub75e_obj->latch_gpio, GPIO_PV_LOW);
}

static inline void my_set_gpiohs(uint8_t io, uint8_t val)
{
    set_bit(gpiohs->output_val.u32, 1 << io, val << io);
}

static inline void hub75e_set_addr(hub75e_t* hub75e_obj, uint8_t addr)
{
    my_set_gpiohs(hub75e_obj->a_gpio, addr & 0x01);
    my_set_gpiohs(hub75e_obj->b_gpio, (addr & 0x02) >> 1);
    my_set_gpiohs(hub75e_obj->c_gpio, (addr & 0x04) >> 2);
    my_set_gpiohs(hub75e_obj->d_gpio, (addr & 0x08) >> 3);
    my_set_gpiohs(hub75e_obj->e_gpio, (addr & 0x10) >> 4);
}


static inline void enable_hub75e(hub75e_t* hub75e_obj)
{
    // set_gpio_bit(gpiohs->output_val.u32, HUB75E_OE_GPIOHSNUM, GPIO_PV_LOW);
    GPIOHS_OUT_LOW(hub75e_obj->oe_gpio);
}
static inline void disable_hub75e(hub75e_t* hub75e_obj)
{
    // gpiohs_set_pin(HUB75E_OE_GPIOHSNUM, GPIO_PV_HIGH);
    // set_gpio_bit(gpiohs->output_val.u32, HUB75E_OE_GPIOHSNUM, GPIO_PV_HIGH);
    GPIOHS_OUT_HIGH(hub75e_obj->oe_gpio);
}

static inline void latch_hub75e(hub75e_t* hub75e_obj)
{
    // set_gpio_bit(gpiohs->output_val.u32, HUB75E_LATCH_GPIOHSNUM, GPIO_PV_HIGH);
    GPIOHS_OUT_HIGH(hub75e_obj->latch_gpio);
}

static inline void unlatch_hub75e(hub75e_t* hub75e_obj)
{
    // set_gpio_bit(gpiohs->output_val.u32, HUB75E_LATCH_GPIOHSNUM, GPIO_PV_LOW);
    GPIOHS_OUT_LOW(hub75e_obj->latch_gpio);
}

static inline void fill_line(hub75e_t *hub75e_obj, spi_t *spi_handle, uint32_t *line_buf, const int line_num)
{
    disable_hub75e(hub75e_obj); // 停止显示

    // 传输行数据, 以下相当于 spi_send_data_normal_dma 函数
    set_bit(&spi_handle->ctrlr0, 3 << 8, SPI_TMOD_TRANS << 8);

    spi_handle->dmacr = 0x2; /*enable dma transmit*/
    spi_handle->ssienr = 0x01;
    sysctl_dma_select((sysctl_dma_channel_t)(hub75e_obj->dma_channel),
                      SYSCTL_DMA_SELECT_SSI0_TX_REQ + hub75e_obj->spi * 2);
    dmac_set_single_mode(hub75e_obj->dma_channel, line_buf,
                         (void *)(&spi_handle->dr[0]), DMAC_ADDR_INCREMENT,
                         DMAC_ADDR_NOCHANGE, DMAC_MSIZE_4, DMAC_TRANS_WIDTH_32,
                         64);
    spi_handle->ser = 1U << SPI_CHIP_SELECT_3;
    dmac_wait_done(hub75e_obj->dma_channel);
    while ((spi_handle->sr & 0x05) != 0x04)
        ;
    spi_handle->ser = 0x00;
    spi_handle->ssienr = 0x00;

    latch_hub75e(hub75e_obj);
    unlatch_hub75e(hub75e_obj);
    hub75e_set_addr(hub75e_obj, line_num); // 选择行地址
    enable_hub75e(hub75e_obj);           // 开始显示
}

void hub75e_display(hub75e_t* hub75e_obj, uint16_t image[][HORIZONTAL_PIXELS])
{
    int y, t, x;
    volatile spi_t *spi_handle = spi[hub75e_obj->spi];
    uint16_t rgb555[hub75e_obj->width][hub75e_obj->height];
    uint32_t line_buffer[hub75e_obj->height];

    // rgb565 -> rgb555
    for (y = 0; y < hub75e_obj->height; y++)
    {
        for (x = 0; x < hub75e_obj->width; x++)
        {
            rgb555[y][x] = rgb565_to_rgb555[image[y][x]];
        }
    }

#define HUB75E_ENCODE(x)                               \
    line_buffer[x] = ((table_upper[t][rgb555[y][x]]) | \
                      table_upper[t][rgb555[y + (hub75e_obj->height / 2)][x]] >> 3);

    // 每张图刷新 32 次, 可以达到用占空比控制的效果, 形成多种颜色
    for (t = 0; t < 32; t++)
    {
        // 填充第 y 和 y+32 行, 共 64 行, 1/32 扫, 所以每次填充两行之间的间隔为 32
        for (y = 0; y < hub75e_obj->height / 2; y++)
        {
            // 编码每两行的 rgb1(高三位: 7,6,5) rgb2(后三位: 4,3,2), 每行共 64 个点
            HUB75E_ENCODE(0);
            HUB75E_ENCODE(1);
            HUB75E_ENCODE(2);
            HUB75E_ENCODE(3);
            HUB75E_ENCODE(4);
            HUB75E_ENCODE(5);
            HUB75E_ENCODE(6);
            HUB75E_ENCODE(7);
            HUB75E_ENCODE(8);
            HUB75E_ENCODE(9);
            HUB75E_ENCODE(10);
            HUB75E_ENCODE(11);
            HUB75E_ENCODE(12);
            HUB75E_ENCODE(13);
            HUB75E_ENCODE(14);
            HUB75E_ENCODE(15);
            HUB75E_ENCODE(16);
            HUB75E_ENCODE(17);
            HUB75E_ENCODE(18);
            HUB75E_ENCODE(19);
            HUB75E_ENCODE(20);
            HUB75E_ENCODE(21);
            HUB75E_ENCODE(22);
            HUB75E_ENCODE(23);
            HUB75E_ENCODE(24);
            HUB75E_ENCODE(25);
            HUB75E_ENCODE(26);
            HUB75E_ENCODE(27);
            HUB75E_ENCODE(28);
            HUB75E_ENCODE(29);
            HUB75E_ENCODE(30);
            HUB75E_ENCODE(31);
            HUB75E_ENCODE(32);
            HUB75E_ENCODE(33);
            HUB75E_ENCODE(34);
            HUB75E_ENCODE(35);
            HUB75E_ENCODE(36);
            HUB75E_ENCODE(37);
            HUB75E_ENCODE(38);
            HUB75E_ENCODE(39);
            HUB75E_ENCODE(40);
            HUB75E_ENCODE(41);
            HUB75E_ENCODE(42);
            HUB75E_ENCODE(43);
            HUB75E_ENCODE(44);
            HUB75E_ENCODE(45);
            HUB75E_ENCODE(46);
            HUB75E_ENCODE(47);
            HUB75E_ENCODE(48);
            HUB75E_ENCODE(49);
            HUB75E_ENCODE(50);
            HUB75E_ENCODE(51);
            HUB75E_ENCODE(52);
            HUB75E_ENCODE(53);
            HUB75E_ENCODE(54);
            HUB75E_ENCODE(55);
            HUB75E_ENCODE(56);
            HUB75E_ENCODE(57);
            HUB75E_ENCODE(58);
            HUB75E_ENCODE(59);
            HUB75E_ENCODE(60);
            HUB75E_ENCODE(61);
            HUB75E_ENCODE(62);
            HUB75E_ENCODE(63);

            fill_line(hub75e_obj, spi_handle, line_buffer, y);
        }
    }
    disable_hub75e(hub75e_obj); //防止 31 和 64 行太亮
}