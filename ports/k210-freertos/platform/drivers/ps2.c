#include "ps2.h"
#include "myspi.h"
#include "sleep.h"

uint8_t ps2_soft_spi_rw(uint8_t data)
{
    uint8_t i, temp = 0;
    for (i = 0; i < 8; i++)
    {
        if (data & 0x01)
            gpiohs->output_val.bits.b12 = 1; //mosi
        else
            gpiohs->output_val.bits.b12 = 0; //mosi
        usleep(5);
        data >>= 1;
        gpiohs->output_val.bits.b11 = 1; //clk
        usleep(1);
        gpiohs->output_val.bits.b11 = 0; //clk
        usleep(1);
        gpiohs->output_val.bits.b11 = 1; //clk
        usleep(5);
        if (gpiohs->input_val.bits.b13)
            temp += (1 << i);
    }
    return temp;
}

// //Pre-request:CS=0
// //Use PS2_DO to shift bit out
// //lsb first
// uint8_t ps2_ps2_soft_spi_rw(uint8_t dat)
// {
//     uint8_t i, ret = 0;
//     for (i = 0; i < 8; i++)
//     {
//         if (dat & 0x01)
//             PS2_DO_H;
//         else
//             PS2_DO_L;
//         dat >>= 1;
//         PS2_CLK_H;
//         XD_Delay_us(6);
//         PS2_CLK_L;
//         XD_Delay_us(6);
//         PS2_CLK_H;
//         if (PS2_DI_VAL)
//             ret += (1 << i);
//     }
//     return ret;
// }

void ps2_mode_config(void)
{
    SOFT_SPI_CS_CLR();
    usleep(20);
    ps2_soft_spi_rw(0x01);
    usleep(1);
    ps2_soft_spi_rw(0x44);
    usleep(1);
    ps2_soft_spi_rw(0X00);
    usleep(1);
    ps2_soft_spi_rw(0x01); //analog=0x01;digital=0x00
    usleep(1);
    ps2_soft_spi_rw(0xee); //Ox03 Lock button Mode,0xEE do not lock
    usleep(1);
    ps2_soft_spi_rw(0x00);
    usleep(1);
    ps2_soft_spi_rw(0x00);
    usleep(1);
    ps2_soft_spi_rw(0x00);
    usleep(1);
    ps2_soft_spi_rw(0x00);
    usleep(1);
    SOFT_SPI_CS_SET();
    usleep(20);
}

void ps2_read_status(uint8_t *pbuff)
{
    SOFT_SPI_CS_CLR();
    usleep(20);
    pbuff[0] = ps2_soft_spi_rw(0x01);
    usleep(1);
    pbuff[1] = ps2_soft_spi_rw(0x42);
    usleep(1);
    pbuff[2] = ps2_soft_spi_rw(0x00);
    usleep(1);
    pbuff[3] = ps2_soft_spi_rw(0xff);
    usleep(1);
    pbuff[4] = ps2_soft_spi_rw(0xff);
    usleep(1);
    pbuff[5] = ps2_soft_spi_rw(0x00);
    usleep(1);
    pbuff[6] = ps2_soft_spi_rw(0x00);
    usleep(1);
    pbuff[7] = ps2_soft_spi_rw(0x00);
    usleep(1);
    pbuff[8] = ps2_soft_spi_rw(0x00);
    SOFT_SPI_CS_SET();
    usleep(20);
}

/*
    motor1:0x00=OFF,others=ON
    motor2:0x40~0xff, portion to value
    it seems that my PS2 do not support this function (T^T)
*/
void ps2_motor_ctrl(uint8_t motor1, uint8_t motor2)
{
    SOFT_SPI_CS_CLR();
    usleep(2);
    ps2_soft_spi_rw(0x01);
    ps2_soft_spi_rw(0x42);
    ps2_soft_spi_rw(0x00);
    ps2_soft_spi_rw(motor1);
    ps2_soft_spi_rw(motor2);
    ps2_soft_spi_rw(0x00);
    ps2_soft_spi_rw(0x00);
    ps2_soft_spi_rw(0x00);
    ps2_soft_spi_rw(0x00);
    SOFT_SPI_CS_SET();
}

/*
    RED LIGHT ON--ANALOG MODE,return 1
    RED LIGHT OFF--DIGITAL MODE,return 0
*/
uint8_t ps2_read_led_stat(void)
{
    uint8_t x;

    SOFT_SPI_CS_CLR();
    usleep(2);

    ps2_soft_spi_rw(0x01);
    x = ps2_soft_spi_rw(0x42);

    SOFT_SPI_CS_SET();
    usleep(2);

    if (x == 0x73)
        return 1;
    else
        return 0;
}
