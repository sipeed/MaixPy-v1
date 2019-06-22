/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * @file
 * @brief      Field Programmable GPIO Array (FPIOA)
 *
 *             The FPIOA peripheral supports the following features:
 *
 *             - 48 IO with 256 functions
 *
 *             - Schmitt trigger
 *
 *             - Invert input and output
 *
 *             - Pull up and pull down
 *
 *             - Driving selector
 *
 *             - Static input and output
 *
 */

#ifndef _DRIVER_FPIOA_H
#define _DRIVER_FPIOA_H

#include <stdint.h>
#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* clang-format off */
/* Pad number settings */
#define FPIOA_NUM_IO    (48)
/* clang-format on */

/**
 * @brief      FPIOA IO functions
 *
 * @note       FPIOA pin function table
 *
 * | Function  | Name             | Description                       |
 * |-----------|------------------|-----------------------------------|
 * | 0         | JTAG_TCLK        | JTAG Test Clock                   |
 * | 1         | JTAG_TDI         | JTAG Test Data In                 |
 * | 2         | JTAG_TMS         | JTAG Test Mode Select             |
 * | 3         | JTAG_TDO         | JTAG Test Data Out                |
 * | 4         | SPI0_D0          | SPI0 Data 0                       |
 * | 5         | SPI0_D1          | SPI0 Data 1                       |
 * | 6         | SPI0_D2          | SPI0 Data 2                       |
 * | 7         | SPI0_D3          | SPI0 Data 3                       |
 * | 8         | SPI0_D4          | SPI0 Data 4                       |
 * | 9         | SPI0_D5          | SPI0 Data 5                       |
 * | 10        | SPI0_D6          | SPI0 Data 6                       |
 * | 11        | SPI0_D7          | SPI0 Data 7                       |
 * | 12        | SPI0_SS0         | SPI0 Chip Select 0                |
 * | 13        | SPI0_SS1         | SPI0 Chip Select 1                |
 * | 14        | SPI0_SS2         | SPI0 Chip Select 2                |
 * | 15        | SPI0_SS3         | SPI0 Chip Select 3                |
 * | 16        | SPI0_ARB         | SPI0 Arbitration                  |
 * | 17        | SPI0_SCLK        | SPI0 Serial Clock                 |
 * | 18        | UARTHS_RX        | UART High speed Receiver          |
 * | 19        | UARTHS_TX        | UART High speed Transmitter       |
 * | 20        | RESV6            | Reserved function                 |
 * | 21        | RESV7            | Reserved function                 |
 * | 22        | CLK_SPI1         | Clock SPI1                        |
 * | 23        | CLK_I2C1         | Clock I2C1                        |
 * | 24        | GPIOHS0          | GPIO High speed 0                 |
 * | 25        | GPIOHS1          | GPIO High speed 1                 |
 * | 26        | GPIOHS2          | GPIO High speed 2                 |
 * | 27        | GPIOHS3          | GPIO High speed 3                 |
 * | 28        | GPIOHS4          | GPIO High speed 4                 |
 * | 29        | GPIOHS5          | GPIO High speed 5                 |
 * | 30        | GPIOHS6          | GPIO High speed 6                 |
 * | 31        | GPIOHS7          | GPIO High speed 7                 |
 * | 32        | GPIOHS8          | GPIO High speed 8                 |
 * | 33        | GPIOHS9          | GPIO High speed 9                 |
 * | 34        | GPIOHS10         | GPIO High speed 10                |
 * | 35        | GPIOHS11         | GPIO High speed 11                |
 * | 36        | GPIOHS12         | GPIO High speed 12                |
 * | 37        | GPIOHS13         | GPIO High speed 13                |
 * | 38        | GPIOHS14         | GPIO High speed 14                |
 * | 39        | GPIOHS15         | GPIO High speed 15                |
 * | 40        | GPIOHS16         | GPIO High speed 16                |
 * | 41        | GPIOHS17         | GPIO High speed 17                |
 * | 42        | GPIOHS18         | GPIO High speed 18                |
 * | 43        | GPIOHS19         | GPIO High speed 19                |
 * | 44        | GPIOHS20         | GPIO High speed 20                |
 * | 45        | GPIOHS21         | GPIO High speed 21                |
 * | 46        | GPIOHS22         | GPIO High speed 22                |
 * | 47        | GPIOHS23         | GPIO High speed 23                |
 * | 48        | GPIOHS24         | GPIO High speed 24                |
 * | 49        | GPIOHS25         | GPIO High speed 25                |
 * | 50        | GPIOHS26         | GPIO High speed 26                |
 * | 51        | GPIOHS27         | GPIO High speed 27                |
 * | 52        | GPIOHS28         | GPIO High speed 28                |
 * | 53        | GPIOHS29         | GPIO High speed 29                |
 * | 54        | GPIOHS30         | GPIO High speed 30                |
 * | 55        | GPIOHS31         | GPIO High speed 31                |
 * | 56        | GPIO0            | GPIO pin 0                        |
 * | 57        | GPIO1            | GPIO pin 1                        |
 * | 58        | GPIO2            | GPIO pin 2                        |
 * | 59        | GPIO3            | GPIO pin 3                        |
 * | 60        | GPIO4            | GPIO pin 4                        |
 * | 61        | GPIO5            | GPIO pin 5                        |
 * | 62        | GPIO6            | GPIO pin 6                        |
 * | 63        | GPIO7            | GPIO pin 7                        |
 * | 64        | UART1_RX         | UART1 Receiver                    |
 * | 65        | UART1_TX         | UART1 Transmitter                 |
 * | 66        | UART2_RX         | UART2 Receiver                    |
 * | 67        | UART2_TX         | UART2 Transmitter                 |
 * | 68        | UART3_RX         | UART3 Receiver                    |
 * | 69        | UART3_TX         | UART3 Transmitter                 |
 * | 70        | SPI1_D0          | SPI1 Data 0                       |
 * | 71        | SPI1_D1          | SPI1 Data 1                       |
 * | 72        | SPI1_D2          | SPI1 Data 2                       |
 * | 73        | SPI1_D3          | SPI1 Data 3                       |
 * | 74        | SPI1_D4          | SPI1 Data 4                       |
 * | 75        | SPI1_D5          | SPI1 Data 5                       |
 * | 76        | SPI1_D6          | SPI1 Data 6                       |
 * | 77        | SPI1_D7          | SPI1 Data 7                       |
 * | 78        | SPI1_SS0         | SPI1 Chip Select 0                |
 * | 79        | SPI1_SS1         | SPI1 Chip Select 1                |
 * | 80        | SPI1_SS2         | SPI1 Chip Select 2                |
 * | 81        | SPI1_SS3         | SPI1 Chip Select 3                |
 * | 82        | SPI1_ARB         | SPI1 Arbitration                  |
 * | 83        | SPI1_SCLK        | SPI1 Serial Clock                 |
 * | 84        | SPI_SLAVE_D0     | SPI Slave Data 0                  |
 * | 85        | SPI_SLAVE_SS     | SPI Slave Select                  |
 * | 86        | SPI_SLAVE_SCLK   | SPI Slave Serial Clock            |
 * | 87        | I2S0_MCLK        | I2S0 Master Clock                 |
 * | 88        | I2S0_SCLK        | I2S0 Serial Clock(BCLK)           |
 * | 89        | I2S0_WS          | I2S0 Word Select(LRCLK)           |
 * | 90        | I2S0_IN_D0       | I2S0 Serial Data Input 0          |
 * | 91        | I2S0_IN_D1       | I2S0 Serial Data Input 1          |
 * | 92        | I2S0_IN_D2       | I2S0 Serial Data Input 2          |
 * | 93        | I2S0_IN_D3       | I2S0 Serial Data Input 3          |
 * | 94        | I2S0_OUT_D0      | I2S0 Serial Data Output 0         |
 * | 95        | I2S0_OUT_D1      | I2S0 Serial Data Output 1         |
 * | 96        | I2S0_OUT_D2      | I2S0 Serial Data Output 2         |
 * | 97        | I2S0_OUT_D3      | I2S0 Serial Data Output 3         |
 * | 98        | I2S1_MCLK        | I2S1 Master Clock                 |
 * | 99        | I2S1_SCLK        | I2S1 Serial Clock(BCLK)           |
 * | 100       | I2S1_WS          | I2S1 Word Select(LRCLK)           |
 * | 101       | I2S1_IN_D0       | I2S1 Serial Data Input 0          |
 * | 102       | I2S1_IN_D1       | I2S1 Serial Data Input 1          |
 * | 103       | I2S1_IN_D2       | I2S1 Serial Data Input 2          |
 * | 104       | I2S1_IN_D3       | I2S1 Serial Data Input 3          |
 * | 105       | I2S1_OUT_D0      | I2S1 Serial Data Output 0         |
 * | 106       | I2S1_OUT_D1      | I2S1 Serial Data Output 1         |
 * | 107       | I2S1_OUT_D2      | I2S1 Serial Data Output 2         |
 * | 108       | I2S1_OUT_D3      | I2S1 Serial Data Output 3         |
 * | 109       | I2S2_MCLK        | I2S2 Master Clock                 |
 * | 110       | I2S2_SCLK        | I2S2 Serial Clock(BCLK)           |
 * | 111       | I2S2_WS          | I2S2 Word Select(LRCLK)           |
 * | 112       | I2S2_IN_D0       | I2S2 Serial Data Input 0          |
 * | 113       | I2S2_IN_D1       | I2S2 Serial Data Input 1          |
 * | 114       | I2S2_IN_D2       | I2S2 Serial Data Input 2          |
 * | 115       | I2S2_IN_D3       | I2S2 Serial Data Input 3          |
 * | 116       | I2S2_OUT_D0      | I2S2 Serial Data Output 0         |
 * | 117       | I2S2_OUT_D1      | I2S2 Serial Data Output 1         |
 * | 118       | I2S2_OUT_D2      | I2S2 Serial Data Output 2         |
 * | 119       | I2S2_OUT_D3      | I2S2 Serial Data Output 3         |
 * | 120       | RESV0            | Reserved function                 |
 * | 121       | RESV1            | Reserved function                 |
 * | 122       | RESV2            | Reserved function                 |
 * | 123       | RESV3            | Reserved function                 |
 * | 124       | RESV4            | Reserved function                 |
 * | 125       | RESV5            | Reserved function                 |
 * | 126       | I2C0_SCLK        | I2C0 Serial Clock                 |
 * | 127       | I2C0_SDA         | I2C0 Serial Data                  |
 * | 128       | I2C1_SCLK        | I2C1 Serial Clock                 |
 * | 129       | I2C1_SDA         | I2C1 Serial Data                  |
 * | 130       | I2C2_SCLK        | I2C2 Serial Clock                 |
 * | 131       | I2C2_SDA         | I2C2 Serial Data                  |
 * | 132       | CMOS_XCLK        | DVP System Clock                  |
 * | 133       | CMOS_RST         | DVP System Reset                  |
 * | 134       | CMOS_PWDN        | DVP Power Down Mode               |
 * | 135       | CMOS_VSYNC       | DVP Vertical Sync                 |
 * | 136       | CMOS_HREF        | DVP Horizontal Reference output   |
 * | 137       | CMOS_PCLK        | Pixel Clock                       |
 * | 138       | CMOS_D0          | Data Bit 0                        |
 * | 139       | CMOS_D1          | Data Bit 1                        |
 * | 140       | CMOS_D2          | Data Bit 2                        |
 * | 141       | CMOS_D3          | Data Bit 3                        |
 * | 142       | CMOS_D4          | Data Bit 4                        |
 * | 143       | CMOS_D5          | Data Bit 5                        |
 * | 144       | CMOS_D6          | Data Bit 6                        |
 * | 145       | CMOS_D7          | Data Bit 7                        |
 * | 146       | SCCB_SCLK        | SCCB Serial Clock                 |
 * | 147       | SCCB_SDA         | SCCB Serial Data                  |
 * | 148       | UART1_CTS        | UART1 Clear To Send               |
 * | 149       | UART1_DSR        | UART1 Data Set Ready              |
 * | 150       | UART1_DCD        | UART1 Data Carrier Detect         |
 * | 151       | UART1_RI         | UART1 Ring Indicator              |
 * | 152       | UART1_SIR_IN     | UART1 Serial Infrared Input       |
 * | 153       | UART1_DTR        | UART1 Data Terminal Ready         |
 * | 154       | UART1_RTS        | UART1 Request To Send             |
 * | 155       | UART1_OUT2       | UART1 User-designated Output 2    |
 * | 156       | UART1_OUT1       | UART1 User-designated Output 1    |
 * | 157       | UART1_SIR_OUT    | UART1 Serial Infrared Output      |
 * | 158       | UART1_BAUD       | UART1 Transmit Clock Output       |
 * | 159       | UART1_RE         | UART1 Receiver Output Enable      |
 * | 160       | UART1_DE         | UART1 Driver Output Enable        |
 * | 161       | UART1_RS485_EN   | UART1 RS485 Enable                |
 * | 162       | UART2_CTS        | UART2 Clear To Send               |
 * | 163       | UART2_DSR        | UART2 Data Set Ready              |
 * | 164       | UART2_DCD        | UART2 Data Carrier Detect         |
 * | 165       | UART2_RI         | UART2 Ring Indicator              |
 * | 166       | UART2_SIR_IN     | UART2 Serial Infrared Input       |
 * | 167       | UART2_DTR        | UART2 Data Terminal Ready         |
 * | 168       | UART2_RTS        | UART2 Request To Send             |
 * | 169       | UART2_OUT2       | UART2 User-designated Output 2    |
 * | 170       | UART2_OUT1       | UART2 User-designated Output 1    |
 * | 171       | UART2_SIR_OUT    | UART2 Serial Infrared Output      |
 * | 172       | UART2_BAUD       | UART2 Transmit Clock Output       |
 * | 173       | UART2_RE         | UART2 Receiver Output Enable      |
 * | 174       | UART2_DE         | UART2 Driver Output Enable        |
 * | 175       | UART2_RS485_EN   | UART2 RS485 Enable                |
 * | 176       | UART3_CTS        | UART3 Clear To Send               |
 * | 177       | UART3_DSR        | UART3 Data Set Ready              |
 * | 178       | UART3_DCD        | UART3 Data Carrier Detect         |
 * | 179       | UART3_RI         | UART3 Ring Indicator              |
 * | 180       | UART3_SIR_IN     | UART3 Serial Infrared Input       |
 * | 181       | UART3_DTR        | UART3 Data Terminal Ready         |
 * | 182       | UART3_RTS        | UART3 Request To Send             |
 * | 183       | UART3_OUT2       | UART3 User-designated Output 2    |
 * | 184       | UART3_OUT1       | UART3 User-designated Output 1    |
 * | 185       | UART3_SIR_OUT    | UART3 Serial Infrared Output      |
 * | 186       | UART3_BAUD       | UART3 Transmit Clock Output       |
 * | 187       | UART3_RE         | UART3 Receiver Output Enable      |
 * | 188       | UART3_DE         | UART3 Driver Output Enable        |
 * | 189       | UART3_RS485_EN   | UART3 RS485 Enable                |
 * | 190       | TIMER0_TOGGLE1   | TIMER0 Toggle Output 1            |
 * | 191       | TIMER0_TOGGLE2   | TIMER0 Toggle Output 2            |
 * | 192       | TIMER0_TOGGLE3   | TIMER0 Toggle Output 3            |
 * | 193       | TIMER0_TOGGLE4   | TIMER0 Toggle Output 4            |
 * | 194       | TIMER1_TOGGLE1   | TIMER1 Toggle Output 1            |
 * | 195       | TIMER1_TOGGLE2   | TIMER1 Toggle Output 2            |
 * | 196       | TIMER1_TOGGLE3   | TIMER1 Toggle Output 3            |
 * | 197       | TIMER1_TOGGLE4   | TIMER1 Toggle Output 4            |
 * | 198       | TIMER2_TOGGLE1   | TIMER2 Toggle Output 1            |
 * | 199       | TIMER2_TOGGLE2   | TIMER2 Toggle Output 2            |
 * | 200       | TIMER2_TOGGLE3   | TIMER2 Toggle Output 3            |
 * | 201       | TIMER2_TOGGLE4   | TIMER2 Toggle Output 4            |
 * | 202       | CLK_SPI2         | Clock SPI2                        |
 * | 203       | CLK_I2C2         | Clock I2C2                        |
 * | 204       | INTERNAL0        | Internal function signal 0        |
 * | 205       | INTERNAL1        | Internal function signal 1        |
 * | 206       | INTERNAL2        | Internal function signal 2        |
 * | 207       | INTERNAL3        | Internal function signal 3        |
 * | 208       | INTERNAL4        | Internal function signal 4        |
 * | 209       | INTERNAL5        | Internal function signal 5        |
 * | 210       | INTERNAL6        | Internal function signal 6        |
 * | 211       | INTERNAL7        | Internal function signal 7        |
 * | 212       | INTERNAL8        | Internal function signal 8        |
 * | 213       | INTERNAL9        | Internal function signal 9        |
 * | 214       | INTERNAL10       | Internal function signal 10       |
 * | 215       | INTERNAL11       | Internal function signal 11       |
 * | 216       | INTERNAL12       | Internal function signal 12       |
 * | 217       | INTERNAL13       | Internal function signal 13       |
 * | 218       | INTERNAL14       | Internal function signal 14       |
 * | 219       | INTERNAL15       | Internal function signal 15       |
 * | 220       | INTERNAL16       | Internal function signal 16       |
 * | 221       | INTERNAL17       | Internal function signal 17       |
 * | 222       | CONSTANT         | Constant function                 |
 * | 223       | INTERNAL18       | Internal function signal 18       |
 * | 224       | DEBUG0           | Debug function 0                  |
 * | 225       | DEBUG1           | Debug function 1                  |
 * | 226       | DEBUG2           | Debug function 2                  |
 * | 227       | DEBUG3           | Debug function 3                  |
 * | 228       | DEBUG4           | Debug function 4                  |
 * | 229       | DEBUG5           | Debug function 5                  |
 * | 230       | DEBUG6           | Debug function 6                  |
 * | 231       | DEBUG7           | Debug function 7                  |
 * | 232       | DEBUG8           | Debug function 8                  |
 * | 233       | DEBUG9           | Debug function 9                  |
 * | 234       | DEBUG10          | Debug function 10                 |
 * | 235       | DEBUG11          | Debug function 11                 |
 * | 236       | DEBUG12          | Debug function 12                 |
 * | 237       | DEBUG13          | Debug function 13                 |
 * | 238       | DEBUG14          | Debug function 14                 |
 * | 239       | DEBUG15          | Debug function 15                 |
 * | 240       | DEBUG16          | Debug function 16                 |
 * | 241       | DEBUG17          | Debug function 17                 |
 * | 242       | DEBUG18          | Debug function 18                 |
 * | 243       | DEBUG19          | Debug function 19                 |
 * | 244       | DEBUG20          | Debug function 20                 |
 * | 245       | DEBUG21          | Debug function 21                 |
 * | 246       | DEBUG22          | Debug function 22                 |
 * | 247       | DEBUG23          | Debug function 23                 |
 * | 248       | DEBUG24          | Debug function 24                 |
 * | 249       | DEBUG25          | Debug function 25                 |
 * | 250       | DEBUG26          | Debug function 26                 |
 * | 251       | DEBUG27          | Debug function 27                 |
 * | 252       | DEBUG28          | Debug function 28                 |
 * | 253       | DEBUG29          | Debug function 29                 |
 * | 254       | DEBUG30          | Debug function 30                 |
 * | 255       | DEBUG31          | Debug function 31                 |
 *
 * Any IO of FPIOA have 256 functions, it is a IO-function matrix.
 * All IO have default reset function, after reset, re-configure
 * IO function is required.
 */

/* clang-format off */
typedef enum _fpioa_function
{
    FUNC_JTAG_TCLK        = 0,  /*!< JTAG Test Clock */
    FUNC_JTAG_TDI         = 1,  /*!< JTAG Test Data In */
    FUNC_JTAG_TMS         = 2,  /*!< JTAG Test Mode Select */
    FUNC_JTAG_TDO         = 3,  /*!< JTAG Test Data Out */
    FUNC_SPI0_D0          = 4,  /*!< SPI0 Data 0 */
    FUNC_SPI0_D1          = 5,  /*!< SPI0 Data 1 */
    FUNC_SPI0_D2          = 6,  /*!< SPI0 Data 2 */
    FUNC_SPI0_D3          = 7,  /*!< SPI0 Data 3 */
    FUNC_SPI0_D4          = 8,  /*!< SPI0 Data 4 */
    FUNC_SPI0_D5          = 9,  /*!< SPI0 Data 5 */
    FUNC_SPI0_D6          = 10, /*!< SPI0 Data 6 */
    FUNC_SPI0_D7          = 11, /*!< SPI0 Data 7 */
    FUNC_SPI0_SS0         = 12, /*!< SPI0 Chip Select 0 */
    FUNC_SPI0_SS1         = 13, /*!< SPI0 Chip Select 1 */
    FUNC_SPI0_SS2         = 14, /*!< SPI0 Chip Select 2 */
    FUNC_SPI0_SS3         = 15, /*!< SPI0 Chip Select 3 */
    FUNC_SPI0_ARB         = 16, /*!< SPI0 Arbitration */
    FUNC_SPI0_SCLK        = 17, /*!< SPI0 Serial Clock */
    FUNC_UARTHS_RX        = 18, /*!< UART High speed Receiver */
    FUNC_UARTHS_TX        = 19, /*!< UART High speed Transmitter */
    FUNC_RESV6            = 20, /*!< Reserved function */
    FUNC_RESV7            = 21, /*!< Reserved function */
    FUNC_CLK_SPI1         = 22, /*!< Clock SPI1 */
    FUNC_CLK_I2C1         = 23, /*!< Clock I2C1 */
    FUNC_GPIOHS0          = 24, /*!< GPIO High speed 0 */
    FUNC_GPIOHS1          = 25, /*!< GPIO High speed 1 */
    FUNC_GPIOHS2          = 26, /*!< GPIO High speed 2 */
    FUNC_GPIOHS3          = 27, /*!< GPIO High speed 3 */
    FUNC_GPIOHS4          = 28, /*!< GPIO High speed 4 */
    FUNC_GPIOHS5          = 29, /*!< GPIO High speed 5 */
    FUNC_GPIOHS6          = 30, /*!< GPIO High speed 6 */
    FUNC_GPIOHS7          = 31, /*!< GPIO High speed 7 */
    FUNC_GPIOHS8          = 32, /*!< GPIO High speed 8 */
    FUNC_GPIOHS9          = 33, /*!< GPIO High speed 9 */
    FUNC_GPIOHS10         = 34, /*!< GPIO High speed 10 */
    FUNC_GPIOHS11         = 35, /*!< GPIO High speed 11 */
    FUNC_GPIOHS12         = 36, /*!< GPIO High speed 12 */
    FUNC_GPIOHS13         = 37, /*!< GPIO High speed 13 */
    FUNC_GPIOHS14         = 38, /*!< GPIO High speed 14 */
    FUNC_GPIOHS15         = 39, /*!< GPIO High speed 15 */
    FUNC_GPIOHS16         = 40, /*!< GPIO High speed 16 */
    FUNC_GPIOHS17         = 41, /*!< GPIO High speed 17 */
    FUNC_GPIOHS18         = 42, /*!< GPIO High speed 18 */
    FUNC_GPIOHS19         = 43, /*!< GPIO High speed 19 */
    FUNC_GPIOHS20         = 44, /*!< GPIO High speed 20 */
    FUNC_GPIOHS21         = 45, /*!< GPIO High speed 21 */
    FUNC_GPIOHS22         = 46, /*!< GPIO High speed 22 */
    FUNC_GPIOHS23         = 47, /*!< GPIO High speed 23 */
    FUNC_GPIOHS24         = 48, /*!< GPIO High speed 24 */
    FUNC_GPIOHS25         = 49, /*!< GPIO High speed 25 */
    FUNC_GPIOHS26         = 50, /*!< GPIO High speed 26 */
    FUNC_GPIOHS27         = 51, /*!< GPIO High speed 27 */
    FUNC_GPIOHS28         = 52, /*!< GPIO High speed 28 */
    FUNC_GPIOHS29         = 53, /*!< GPIO High speed 29 */
    FUNC_GPIOHS30         = 54, /*!< GPIO High speed 30 */
    FUNC_GPIOHS31         = 55, /*!< GPIO High speed 31 */
    FUNC_GPIO0            = 56, /*!< GPIO pin 0 */
    FUNC_GPIO1            = 57, /*!< GPIO pin 1 */
    FUNC_GPIO2            = 58, /*!< GPIO pin 2 */
    FUNC_GPIO3            = 59, /*!< GPIO pin 3 */
    FUNC_GPIO4            = 60, /*!< GPIO pin 4 */
    FUNC_GPIO5            = 61, /*!< GPIO pin 5 */
    FUNC_GPIO6            = 62, /*!< GPIO pin 6 */
    FUNC_GPIO7            = 63, /*!< GPIO pin 7 */
    FUNC_UART1_RX         = 64, /*!< UART1 Receiver */
    FUNC_UART1_TX         = 65, /*!< UART1 Transmitter */
    FUNC_UART2_RX         = 66, /*!< UART2 Receiver */
    FUNC_UART2_TX         = 67, /*!< UART2 Transmitter */
    FUNC_UART3_RX         = 68, /*!< UART3 Receiver */
    FUNC_UART3_TX         = 69, /*!< UART3 Transmitter */
    FUNC_SPI1_D0          = 70, /*!< SPI1 Data 0 */
    FUNC_SPI1_D1          = 71, /*!< SPI1 Data 1 */
    FUNC_SPI1_D2          = 72, /*!< SPI1 Data 2 */
    FUNC_SPI1_D3          = 73, /*!< SPI1 Data 3 */
    FUNC_SPI1_D4          = 74, /*!< SPI1 Data 4 */
    FUNC_SPI1_D5          = 75, /*!< SPI1 Data 5 */
    FUNC_SPI1_D6          = 76, /*!< SPI1 Data 6 */
    FUNC_SPI1_D7          = 77, /*!< SPI1 Data 7 */
    FUNC_SPI1_SS0         = 78, /*!< SPI1 Chip Select 0 */
    FUNC_SPI1_SS1         = 79, /*!< SPI1 Chip Select 1 */
    FUNC_SPI1_SS2         = 80, /*!< SPI1 Chip Select 2 */
    FUNC_SPI1_SS3         = 81, /*!< SPI1 Chip Select 3 */
    FUNC_SPI1_ARB         = 82, /*!< SPI1 Arbitration */
    FUNC_SPI1_SCLK        = 83, /*!< SPI1 Serial Clock */
    FUNC_SPI_SLAVE_D0     = 84, /*!< SPI Slave Data 0 */
    FUNC_SPI_SLAVE_SS     = 85, /*!< SPI Slave Select */
    FUNC_SPI_SLAVE_SCLK   = 86, /*!< SPI Slave Serial Clock */
    FUNC_I2S0_MCLK        = 87, /*!< I2S0 Master Clock */
    FUNC_I2S0_SCLK        = 88, /*!< I2S0 Serial Clock(BCLK) */
    FUNC_I2S0_WS          = 89, /*!< I2S0 Word Select(LRCLK) */
    FUNC_I2S0_IN_D0       = 90, /*!< I2S0 Serial Data Input 0 */
    FUNC_I2S0_IN_D1       = 91, /*!< I2S0 Serial Data Input 1 */
    FUNC_I2S0_IN_D2       = 92, /*!< I2S0 Serial Data Input 2 */
    FUNC_I2S0_IN_D3       = 93, /*!< I2S0 Serial Data Input 3 */
    FUNC_I2S0_OUT_D0      = 94, /*!< I2S0 Serial Data Output 0 */
    FUNC_I2S0_OUT_D1      = 95, /*!< I2S0 Serial Data Output 1 */
    FUNC_I2S0_OUT_D2      = 96, /*!< I2S0 Serial Data Output 2 */
    FUNC_I2S0_OUT_D3      = 97, /*!< I2S0 Serial Data Output 3 */
    FUNC_I2S1_MCLK        = 98, /*!< I2S1 Master Clock */
    FUNC_I2S1_SCLK        = 99, /*!< I2S1 Serial Clock(BCLK) */
    FUNC_I2S1_WS          = 100,    /*!< I2S1 Word Select(LRCLK) */
    FUNC_I2S1_IN_D0       = 101,    /*!< I2S1 Serial Data Input 0 */
    FUNC_I2S1_IN_D1       = 102,    /*!< I2S1 Serial Data Input 1 */
    FUNC_I2S1_IN_D2       = 103,    /*!< I2S1 Serial Data Input 2 */
    FUNC_I2S1_IN_D3       = 104,    /*!< I2S1 Serial Data Input 3 */
    FUNC_I2S1_OUT_D0      = 105,    /*!< I2S1 Serial Data Output 0 */
    FUNC_I2S1_OUT_D1      = 106,    /*!< I2S1 Serial Data Output 1 */
    FUNC_I2S1_OUT_D2      = 107,    /*!< I2S1 Serial Data Output 2 */
    FUNC_I2S1_OUT_D3      = 108,    /*!< I2S1 Serial Data Output 3 */
    FUNC_I2S2_MCLK        = 109,    /*!< I2S2 Master Clock */
    FUNC_I2S2_SCLK        = 110,    /*!< I2S2 Serial Clock(BCLK) */
    FUNC_I2S2_WS          = 111,    /*!< I2S2 Word Select(LRCLK) */
    FUNC_I2S2_IN_D0       = 112,    /*!< I2S2 Serial Data Input 0 */
    FUNC_I2S2_IN_D1       = 113,    /*!< I2S2 Serial Data Input 1 */
    FUNC_I2S2_IN_D2       = 114,    /*!< I2S2 Serial Data Input 2 */
    FUNC_I2S2_IN_D3       = 115,    /*!< I2S2 Serial Data Input 3 */
    FUNC_I2S2_OUT_D0      = 116,    /*!< I2S2 Serial Data Output 0 */
    FUNC_I2S2_OUT_D1      = 117,    /*!< I2S2 Serial Data Output 1 */
    FUNC_I2S2_OUT_D2      = 118,    /*!< I2S2 Serial Data Output 2 */
    FUNC_I2S2_OUT_D3      = 119,    /*!< I2S2 Serial Data Output 3 */
    FUNC_RESV0            = 120,    /*!< Reserved function */
    FUNC_RESV1            = 121,    /*!< Reserved function */
    FUNC_RESV2            = 122,    /*!< Reserved function */
    FUNC_RESV3            = 123,    /*!< Reserved function */
    FUNC_RESV4            = 124,    /*!< Reserved function */
    FUNC_RESV5            = 125,    /*!< Reserved function */
    FUNC_I2C0_SCLK        = 126,    /*!< I2C0 Serial Clock */
    FUNC_I2C0_SDA         = 127,    /*!< I2C0 Serial Data */
    FUNC_I2C1_SCLK        = 128,    /*!< I2C1 Serial Clock */
    FUNC_I2C1_SDA         = 129,    /*!< I2C1 Serial Data */
    FUNC_I2C2_SCLK        = 130,    /*!< I2C2 Serial Clock */
    FUNC_I2C2_SDA         = 131,    /*!< I2C2 Serial Data */
    FUNC_CMOS_XCLK        = 132,    /*!< DVP System Clock */
    FUNC_CMOS_RST         = 133,    /*!< DVP System Reset */
    FUNC_CMOS_PWDN        = 134,    /*!< DVP Power Down Mode */
    FUNC_CMOS_VSYNC       = 135,    /*!< DVP Vertical Sync */
    FUNC_CMOS_HREF        = 136,    /*!< DVP Horizontal Reference output */
    FUNC_CMOS_PCLK        = 137,    /*!< Pixel Clock */
    FUNC_CMOS_D0          = 138,    /*!< Data Bit 0 */
    FUNC_CMOS_D1          = 139,    /*!< Data Bit 1 */
    FUNC_CMOS_D2          = 140,    /*!< Data Bit 2 */
    FUNC_CMOS_D3          = 141,    /*!< Data Bit 3 */
    FUNC_CMOS_D4          = 142,    /*!< Data Bit 4 */
    FUNC_CMOS_D5          = 143,    /*!< Data Bit 5 */
    FUNC_CMOS_D6          = 144,    /*!< Data Bit 6 */
    FUNC_CMOS_D7          = 145,    /*!< Data Bit 7 */
    FUNC_SCCB_SCLK        = 146,    /*!< SCCB Serial Clock */
    FUNC_SCCB_SDA         = 147,    /*!< SCCB Serial Data */
    FUNC_UART1_CTS        = 148,    /*!< UART1 Clear To Send */
    FUNC_UART1_DSR        = 149,    /*!< UART1 Data Set Ready */
    FUNC_UART1_DCD        = 150,    /*!< UART1 Data Carrier Detect */
    FUNC_UART1_RI         = 151,    /*!< UART1 Ring Indicator */
    FUNC_UART1_SIR_IN     = 152,    /*!< UART1 Serial Infrared Input */
    FUNC_UART1_DTR        = 153,    /*!< UART1 Data Terminal Ready */
    FUNC_UART1_RTS        = 154,    /*!< UART1 Request To Send */
    FUNC_UART1_OUT2       = 155,    /*!< UART1 User-designated Output 2 */
    FUNC_UART1_OUT1       = 156,    /*!< UART1 User-designated Output 1 */
    FUNC_UART1_SIR_OUT    = 157,    /*!< UART1 Serial Infrared Output */
    FUNC_UART1_BAUD       = 158,    /*!< UART1 Transmit Clock Output */
    FUNC_UART1_RE         = 159,    /*!< UART1 Receiver Output Enable */
    FUNC_UART1_DE         = 160,    /*!< UART1 Driver Output Enable */
    FUNC_UART1_RS485_EN   = 161,    /*!< UART1 RS485 Enable */
    FUNC_UART2_CTS        = 162,    /*!< UART2 Clear To Send */
    FUNC_UART2_DSR        = 163,    /*!< UART2 Data Set Ready */
    FUNC_UART2_DCD        = 164,    /*!< UART2 Data Carrier Detect */
    FUNC_UART2_RI         = 165,    /*!< UART2 Ring Indicator */
    FUNC_UART2_SIR_IN     = 166,    /*!< UART2 Serial Infrared Input */
    FUNC_UART2_DTR        = 167,    /*!< UART2 Data Terminal Ready */
    FUNC_UART2_RTS        = 168,    /*!< UART2 Request To Send */
    FUNC_UART2_OUT2       = 169,    /*!< UART2 User-designated Output 2 */
    FUNC_UART2_OUT1       = 170,    /*!< UART2 User-designated Output 1 */
    FUNC_UART2_SIR_OUT    = 171,    /*!< UART2 Serial Infrared Output */
    FUNC_UART2_BAUD       = 172,    /*!< UART2 Transmit Clock Output */
    FUNC_UART2_RE         = 173,    /*!< UART2 Receiver Output Enable */
    FUNC_UART2_DE         = 174,    /*!< UART2 Driver Output Enable */
    FUNC_UART2_RS485_EN   = 175,    /*!< UART2 RS485 Enable */
    FUNC_UART3_CTS        = 176,    /*!< UART3 Clear To Send */
    FUNC_UART3_DSR        = 177,    /*!< UART3 Data Set Ready */
    FUNC_UART3_DCD        = 178,    /*!< UART3 Data Carrier Detect */
    FUNC_UART3_RI         = 179,    /*!< UART3 Ring Indicator */
    FUNC_UART3_SIR_IN     = 180,    /*!< UART3 Serial Infrared Input */
    FUNC_UART3_DTR        = 181,    /*!< UART3 Data Terminal Ready */
    FUNC_UART3_RTS        = 182,    /*!< UART3 Request To Send */
    FUNC_UART3_OUT2       = 183,    /*!< UART3 User-designated Output 2 */
    FUNC_UART3_OUT1       = 184,    /*!< UART3 User-designated Output 1 */
    FUNC_UART3_SIR_OUT    = 185,    /*!< UART3 Serial Infrared Output */
    FUNC_UART3_BAUD       = 186,    /*!< UART3 Transmit Clock Output */
    FUNC_UART3_RE         = 187,    /*!< UART3 Receiver Output Enable */
    FUNC_UART3_DE         = 188,    /*!< UART3 Driver Output Enable */
    FUNC_UART3_RS485_EN   = 189,    /*!< UART3 RS485 Enable */
    FUNC_TIMER0_TOGGLE1   = 190,    /*!< TIMER0 Toggle Output 1 */
    FUNC_TIMER0_TOGGLE2   = 191,    /*!< TIMER0 Toggle Output 2 */
    FUNC_TIMER0_TOGGLE3   = 192,    /*!< TIMER0 Toggle Output 3 */
    FUNC_TIMER0_TOGGLE4   = 193,    /*!< TIMER0 Toggle Output 4 */
    FUNC_TIMER1_TOGGLE1   = 194,    /*!< TIMER1 Toggle Output 1 */
    FUNC_TIMER1_TOGGLE2   = 195,    /*!< TIMER1 Toggle Output 2 */
    FUNC_TIMER1_TOGGLE3   = 196,    /*!< TIMER1 Toggle Output 3 */
    FUNC_TIMER1_TOGGLE4   = 197,    /*!< TIMER1 Toggle Output 4 */
    FUNC_TIMER2_TOGGLE1   = 198,    /*!< TIMER2 Toggle Output 1 */
    FUNC_TIMER2_TOGGLE2   = 199,    /*!< TIMER2 Toggle Output 2 */
    FUNC_TIMER2_TOGGLE3   = 200,    /*!< TIMER2 Toggle Output 3 */
    FUNC_TIMER2_TOGGLE4   = 201,    /*!< TIMER2 Toggle Output 4 */
    FUNC_CLK_SPI2         = 202,    /*!< Clock SPI2 */
    FUNC_CLK_I2C2         = 203,    /*!< Clock I2C2 */
    FUNC_INTERNAL0        = 204,    /*!< Internal function signal 0 */
    FUNC_INTERNAL1        = 205,    /*!< Internal function signal 1 */
    FUNC_INTERNAL2        = 206,    /*!< Internal function signal 2 */
    FUNC_INTERNAL3        = 207,    /*!< Internal function signal 3 */
    FUNC_INTERNAL4        = 208,    /*!< Internal function signal 4 */
    FUNC_INTERNAL5        = 209,    /*!< Internal function signal 5 */
    FUNC_INTERNAL6        = 210,    /*!< Internal function signal 6 */
    FUNC_INTERNAL7        = 211,    /*!< Internal function signal 7 */
    FUNC_INTERNAL8        = 212,    /*!< Internal function signal 8 */
    FUNC_INTERNAL9        = 213,    /*!< Internal function signal 9 */
    FUNC_INTERNAL10       = 214,    /*!< Internal function signal 10 */
    FUNC_INTERNAL11       = 215,    /*!< Internal function signal 11 */
    FUNC_INTERNAL12       = 216,    /*!< Internal function signal 12 */
    FUNC_INTERNAL13       = 217,    /*!< Internal function signal 13 */
    FUNC_INTERNAL14       = 218,    /*!< Internal function signal 14 */
    FUNC_INTERNAL15       = 219,    /*!< Internal function signal 15 */
    FUNC_INTERNAL16       = 220,    /*!< Internal function signal 16 */
    FUNC_INTERNAL17       = 221,    /*!< Internal function signal 17 */
    FUNC_CONSTANT         = 222,    /*!< Constant function */
    FUNC_INTERNAL18       = 223,    /*!< Internal function signal 18 */
    FUNC_DEBUG0           = 224,    /*!< Debug function 0 */
    FUNC_DEBUG1           = 225,    /*!< Debug function 1 */
    FUNC_DEBUG2           = 226,    /*!< Debug function 2 */
    FUNC_DEBUG3           = 227,    /*!< Debug function 3 */
    FUNC_DEBUG4           = 228,    /*!< Debug function 4 */
    FUNC_DEBUG5           = 229,    /*!< Debug function 5 */
    FUNC_DEBUG6           = 230,    /*!< Debug function 6 */
    FUNC_DEBUG7           = 231,    /*!< Debug function 7 */
    FUNC_DEBUG8           = 232,    /*!< Debug function 8 */
    FUNC_DEBUG9           = 233,    /*!< Debug function 9 */
    FUNC_DEBUG10          = 234,    /*!< Debug function 10 */
    FUNC_DEBUG11          = 235,    /*!< Debug function 11 */
    FUNC_DEBUG12          = 236,    /*!< Debug function 12 */
    FUNC_DEBUG13          = 237,    /*!< Debug function 13 */
    FUNC_DEBUG14          = 238,    /*!< Debug function 14 */
    FUNC_DEBUG15          = 239,    /*!< Debug function 15 */
    FUNC_DEBUG16          = 240,    /*!< Debug function 16 */
    FUNC_DEBUG17          = 241,    /*!< Debug function 17 */
    FUNC_DEBUG18          = 242,    /*!< Debug function 18 */
    FUNC_DEBUG19          = 243,    /*!< Debug function 19 */
    FUNC_DEBUG20          = 244,    /*!< Debug function 20 */
    FUNC_DEBUG21          = 245,    /*!< Debug function 21 */
    FUNC_DEBUG22          = 246,    /*!< Debug function 22 */
    FUNC_DEBUG23          = 247,    /*!< Debug function 23 */
    FUNC_DEBUG24          = 248,    /*!< Debug function 24 */
    FUNC_DEBUG25          = 249,    /*!< Debug function 25 */
    FUNC_DEBUG26          = 250,    /*!< Debug function 26 */
    FUNC_DEBUG27          = 251,    /*!< Debug function 27 */
    FUNC_DEBUG28          = 252,    /*!< Debug function 28 */
    FUNC_DEBUG29          = 253,    /*!< Debug function 29 */
    FUNC_DEBUG30          = 254,    /*!< Debug function 30 */
    FUNC_DEBUG31          = 255,    /*!< Debug function 31 */
    FUNC_MAX              = 256,    /*!< Function numbers */
} fpioa_function_t;
/* clang-format on */

/**
 * @brief      FPIOA pull settings
 *
 * @note       FPIOA pull settings description
 *
 * | PU  | PD  | Description                       |
 * |-----|-----|-----------------------------------|
 * | 0   | 0   | No Pull                           |
 * | 0   | 1   | Pull Down                         |
 * | 1   | 0   | Pull Up                           |
 * | 1   | 1   | Undefined                         |
 *
 */

/* clang-format off */
typedef enum _fpioa_pull
{
    FPIOA_PULL_NONE,      /*!< No Pull */
    FPIOA_PULL_DOWN,      /*!< Pull Down */
    FPIOA_PULL_UP,        /*!< Pull Up */
    FPIOA_PULL_MAX        /*!< Count of pull settings */
} fpioa_pull_t;
/* clang-format on */

/**
 * @brief      FPIOA driving settings
 *
 * @note       FPIOA driving settings description
 *             There are 16 kinds of driving settings
 *
 * @note       Low Level Output Current
 *
 * |DS[3:0] |Min(mA)|Typ(mA)|Max(mA)|
 * |--------|-------|-------|-------|
 * |0000    |3.2    |5.4    |8.3    |
 * |0001    |4.7    |8.0    |12.3   |
 * |0010    |6.3    |10.7   |16.4   |
 * |0011    |7.8    |13.2   |20.2   |
 * |0100    |9.4    |15.9   |24.2   |
 * |0101    |10.9   |18.4   |28.1   |
 * |0110    |12.4   |20.9   |31.8   |
 * |0111    |13.9   |23.4   |35.5   |
 *
 * @note       High Level Output Current
 *
 * |DS[3:0] |Min(mA)|Typ(mA)|Max(mA)|
 * |--------|-------|-------|-------|
 * |0000    |5.0    |7.6    |11.2   |
 * |0001    |7.5    |11.4   |16.8   |
 * |0010    |10.0   |15.2   |22.3   |
 * |0011    |12.4   |18.9   |27.8   |
 * |0100    |14.9   |22.6   |33.3   |
 * |0101    |17.4   |26.3   |38.7   |
 * |0110    |19.8   |30.0   |44.1   |
 * |0111    |22.3   |33.7   |49.5   |
 *
 */

/* clang-format off */
typedef enum _fpioa_driving
{
    FPIOA_DRIVING_0,      /*!< 0000 */
    FPIOA_DRIVING_1,      /*!< 0001 */
    FPIOA_DRIVING_2,      /*!< 0010 */
    FPIOA_DRIVING_3,      /*!< 0011 */
    FPIOA_DRIVING_4,      /*!< 0100 */
    FPIOA_DRIVING_5,      /*!< 0101 */
    FPIOA_DRIVING_6,      /*!< 0110 */
    FPIOA_DRIVING_7,      /*!< 0111 */
    FPIOA_DRIVING_8,      /*!< 1000 */
    FPIOA_DRIVING_9,      /*!< 1001 */
    FPIOA_DRIVING_10,     /*!< 1010 */
    FPIOA_DRIVING_11,     /*!< 1011 */
    FPIOA_DRIVING_12,     /*!< 1100 */
    FPIOA_DRIVING_13,     /*!< 1101 */
    FPIOA_DRIVING_14,     /*!< 1110 */
    FPIOA_DRIVING_15,     /*!< 1111 */
    FPIOA_DRIVING_MAX     /*!< Count of driving settings */
} fpioa_driving_t;
/* clang-format on */

/**
 * @brief      FPIOA IO
 *
 *             FPIOA IO is the specific pin of the chip package. Every IO
 *             has a 32bit width register that can independently implement
 *             schmitt trigger, invert input, invert output, strong pull
 *             up, driving selector, static input and static output. And more,
 *             it can implement any pin of any peripheral devices.
 *
 * @note       FPIOA IO's register bits Layout
 *
 * | Bits      | Name     |Description                                        |
 * |-----------|----------|---------------------------------------------------|
 * | 31        | PAD_DI   | Read current IO's data input.                     |
 * | 30:24     | NA       | Reserved bits.                                    |
 * | 23        | ST       | Schmitt trigger.                                  |
 * | 22        | DI_INV   | Invert Data input.                                |
 * | 21        | IE_INV   | Invert the input enable signal.                   |
 * | 20        | IE_EN    | Input enable. It can disable or enable IO input.  |
 * | 19        | SL       | Slew rate control enable.                         |
 * | 18        | SPU      | Strong pull up.                                   |
 * | 17        | PD       | Pull select: 0 for pull down, 1 for pull up.      |
 * | 16        | PU       | Pull enable.                                      |
 * | 15        | DO_INV   | Invert the result of data output select (DO_SEL). |
 * | 14        | DO_SEL   | Data output select: 0 for DO, 1 for OE.           |
 * | 13        | OE_INV   | Invert the output enable signal.                  |
 * | 12        | OE_EN    | Output enable.It can disable or enable IO output. |
 * | 11:8      | DS       | Driving selector.                                 |
 * | 7:0       | CH_SEL   | Channel select from 256 input.                    |
 *
 */
typedef struct _fpioa_io_config
{
    uint32_t ch_sel : 8;
    /*!< Channel select from 256 input. */
    uint32_t ds : 4;
    /*!< Driving selector. */
    uint32_t oe_en : 1;
    /*!< Static output enable, will AND with OE_INV. */
    uint32_t oe_inv : 1;
    /*!< Invert output enable. */
    uint32_t do_sel : 1;
    /*!< Data output select: 0 for DO, 1 for OE. */
    uint32_t do_inv : 1;
    /*!< Invert the result of data output select (DO_SEL). */
    uint32_t pu : 1;
    /*!< Pull up enable. 0 for nothing, 1 for pull up. */
    uint32_t pd : 1;
    /*!< Pull down enable. 0 for nothing, 1 for pull down. */
    uint32_t resv0 : 1;
    /*!< Reserved bits. */
    uint32_t sl : 1;
    /*!< Slew rate control enable. */
    uint32_t ie_en : 1;
    /*!< Static input enable, will AND with IE_INV. */
    uint32_t ie_inv : 1;
    /*!< Invert input enable. */
    uint32_t di_inv : 1;
    /*!< Invert Data input. */
    uint32_t st : 1;
    /*!< Schmitt trigger. */
    uint32_t resv1 : 7;
    /*!< Reserved bits. */
    uint32_t pad_di : 1;
    /*!< Read current IO's data input. */
} __attribute__((packed, aligned(4))) fpioa_io_config_t;

/**
 * @brief      FPIOA tie setting
 *
 *             FPIOA Object have 48 IO pin object and 256 bit input tie bits.
 *             All SPI arbitration signal will tie high by default.
 *
 * @note       FPIOA function tie bits RAM Layout
 *
 * | Address   | Name             |Description                       |
 * |-----------|------------------|----------------------------------|
 * | 0x000     | TIE_EN[31:0]     | Input tie enable bits [31:0]     |
 * | 0x004     | TIE_EN[63:32]    | Input tie enable bits [63:32]    |
 * | 0x008     | TIE_EN[95:64]    | Input tie enable bits [95:64]    |
 * | 0x00C     | TIE_EN[127:96]   | Input tie enable bits [127:96]   |
 * | 0x010     | TIE_EN[159:128]  | Input tie enable bits [159:128]  |
 * | 0x014     | TIE_EN[191:160]  | Input tie enable bits [191:160]  |
 * | 0x018     | TIE_EN[223:192]  | Input tie enable bits [223:192]  |
 * | 0x01C     | TIE_EN[255:224]  | Input tie enable bits [255:224]  |
 * | 0x020     | TIE_VAL[31:0]    | Input tie value bits [31:0]      |
 * | 0x024     | TIE_VAL[63:32]   | Input tie value bits [63:32]     |
 * | 0x028     | TIE_VAL[95:64]   | Input tie value bits [95:64]     |
 * | 0x02C     | TIE_VAL[127:96]  | Input tie value bits [127:96]    |
 * | 0x030     | TIE_VAL[159:128] | Input tie value bits [159:128]   |
 * | 0x034     | TIE_VAL[191:160] | Input tie value bits [191:160]   |
 * | 0x038     | TIE_VAL[223:192] | Input tie value bits [223:192]   |
 * | 0x03C     | TIE_VAL[255:224] | Input tie value bits [255:224]   |
 *
 * @note       Function which input tie high by default
 *
 * | Name          |Description                            |
 * |---------------|---------------------------------------|
 * | SPI0_ARB      | Arbitration function of SPI master 0  |
 * | SPI1_ARB      | Arbitration function of SPI master 1  |
 *
 *             Tie high means the SPI Arbitration input is 1
 *
 */
typedef struct _fpioa_tie
{
    uint32_t en[FUNC_MAX / 32];
    /*!< FPIOA GPIO multiplexer tie enable array */
    uint32_t val[FUNC_MAX / 32];
    /*!< FPIOA GPIO multiplexer tie value array */
} __attribute__((packed, aligned(4))) fpioa_tie_t;

/**
 * @brief      FPIOA Object
 *
 *             FPIOA Object have 48 IO pin object and 256 bit input tie bits.
 *             All SPI arbitration signal will tie high by default.
 *
 * @note       FPIOA IO Pin RAM Layout
 *
 * | Address   | Name     |Description                     |
 * |-----------|----------|--------------------------------|
 * | 0x000     | PAD0     | FPIOA GPIO multiplexer io 0    |
 * | 0x004     | PAD1     | FPIOA GPIO multiplexer io 1    |
 * | 0x008     | PAD2     | FPIOA GPIO multiplexer io 2    |
 * | 0x00C     | PAD3     | FPIOA GPIO multiplexer io 3    |
 * | 0x010     | PAD4     | FPIOA GPIO multiplexer io 4    |
 * | 0x014     | PAD5     | FPIOA GPIO multiplexer io 5    |
 * | 0x018     | PAD6     | FPIOA GPIO multiplexer io 6    |
 * | 0x01C     | PAD7     | FPIOA GPIO multiplexer io 7    |
 * | 0x020     | PAD8     | FPIOA GPIO multiplexer io 8    |
 * | 0x024     | PAD9     | FPIOA GPIO multiplexer io 9    |
 * | 0x028     | PAD10    | FPIOA GPIO multiplexer io 10   |
 * | 0x02C     | PAD11    | FPIOA GPIO multiplexer io 11   |
 * | 0x030     | PAD12    | FPIOA GPIO multiplexer io 12   |
 * | 0x034     | PAD13    | FPIOA GPIO multiplexer io 13   |
 * | 0x038     | PAD14    | FPIOA GPIO multiplexer io 14   |
 * | 0x03C     | PAD15    | FPIOA GPIO multiplexer io 15   |
 * | 0x040     | PAD16    | FPIOA GPIO multiplexer io 16   |
 * | 0x044     | PAD17    | FPIOA GPIO multiplexer io 17   |
 * | 0x048     | PAD18    | FPIOA GPIO multiplexer io 18   |
 * | 0x04C     | PAD19    | FPIOA GPIO multiplexer io 19   |
 * | 0x050     | PAD20    | FPIOA GPIO multiplexer io 20   |
 * | 0x054     | PAD21    | FPIOA GPIO multiplexer io 21   |
 * | 0x058     | PAD22    | FPIOA GPIO multiplexer io 22   |
 * | 0x05C     | PAD23    | FPIOA GPIO multiplexer io 23   |
 * | 0x060     | PAD24    | FPIOA GPIO multiplexer io 24   |
 * | 0x064     | PAD25    | FPIOA GPIO multiplexer io 25   |
 * | 0x068     | PAD26    | FPIOA GPIO multiplexer io 26   |
 * | 0x06C     | PAD27    | FPIOA GPIO multiplexer io 27   |
 * | 0x070     | PAD28    | FPIOA GPIO multiplexer io 28   |
 * | 0x074     | PAD29    | FPIOA GPIO multiplexer io 29   |
 * | 0x078     | PAD30    | FPIOA GPIO multiplexer io 30   |
 * | 0x07C     | PAD31    | FPIOA GPIO multiplexer io 31   |
 * | 0x080     | PAD32    | FPIOA GPIO multiplexer io 32   |
 * | 0x084     | PAD33    | FPIOA GPIO multiplexer io 33   |
 * | 0x088     | PAD34    | FPIOA GPIO multiplexer io 34   |
 * | 0x08C     | PAD35    | FPIOA GPIO multiplexer io 35   |
 * | 0x090     | PAD36    | FPIOA GPIO multiplexer io 36   |
 * | 0x094     | PAD37    | FPIOA GPIO multiplexer io 37   |
 * | 0x098     | PAD38    | FPIOA GPIO multiplexer io 38   |
 * | 0x09C     | PAD39    | FPIOA GPIO multiplexer io 39   |
 * | 0x0A0     | PAD40    | FPIOA GPIO multiplexer io 40   |
 * | 0x0A4     | PAD41    | FPIOA GPIO multiplexer io 41   |
 * | 0x0A8     | PAD42    | FPIOA GPIO multiplexer io 42   |
 * | 0x0AC     | PAD43    | FPIOA GPIO multiplexer io 43   |
 * | 0x0B0     | PAD44    | FPIOA GPIO multiplexer io 44   |
 * | 0x0B4     | PAD45    | FPIOA GPIO multiplexer io 45   |
 * | 0x0B8     | PAD46    | FPIOA GPIO multiplexer io 46   |
 * | 0x0BC     | PAD47    | FPIOA GPIO multiplexer io 47   |
 *
 */
typedef struct _fpioa
{
    fpioa_io_config_t io[FPIOA_NUM_IO];
    /*!< FPIOA GPIO multiplexer io array */
    fpioa_tie_t tie;
    /*!< FPIOA GPIO multiplexer tie */
} __attribute__((packed, aligned(4))) fpioa_t;

/**
 * @brief       FPIOA object instanse
 */
extern volatile fpioa_t *const fpioa;

/**
 * @brief       Initialize FPIOA user custom default settings
 *
 * @note        This function will set all FPIOA pad registers to user-defined
 *              values from kconfig
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int fpioa_init(void);

/**
 * @brief       Get IO configuration
 *
 * @param[in]   number      The IO number
 * @param       cfg         Pointer to struct of IO configuration for specified IO
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int fpioa_get_io(int number, fpioa_io_config_t *cfg);

/**
 * @brief       Set IO configuration
 *
 * @param[in]   number      The IO number
 * @param[in]   cfg         Pointer to struct of IO configuration for specified IO
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int fpioa_set_io(int number, fpioa_io_config_t *cfg);

/**
 * @brief       Set IO configuration with function number
 *
 * @note        The default IO configuration which bind to function number will
 *              set automatically
 *
 * @param[in]   number      The IO number
 * @param[in]   function    The function enum number
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int fpioa_set_function_raw(int number, fpioa_function_t function);

/**
 * @brief       Set only IO configuration with function number
 *
 * @note        The default IO configuration which bind to function number will
 *              set automatically
 *
 * @param[in]   number      The IO number
 * @param[in]   function    The function enum number
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int fpioa_set_function(int number, fpioa_function_t function);

/**
 * @brief       Set tie enable to function
 *
 * @param[in]   function    The function enum number
 * @param[in]   enable      Tie enable to set, 1 is enable, 0 is disable
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int fpioa_set_tie_enable(fpioa_function_t function, int enable);

/**
 * @brief       Set tie value to function
 *
 * @param[in]   function    The function enum number
 * @param[in]   value       Tie value to set, 1 is HIGH, 0 is LOW
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int fpioa_set_tie_value(fpioa_function_t function, int value);

/**
 * @brief      Set IO pull function
 *
 * @param[in]   number  The IO number
 * @param[in]   pull    The pull enum number
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int fpioa_set_io_pull(int number, fpioa_pull_t pull);

/**
 * @brief       Get IO pull function
 *
 * @param[in]   number  The IO number
 *
 * @return      result
 *     - -1     Fail
 *     - Other  The pull enum number
 */
int fpioa_get_io_pull(int number);

/**
 * @brief       Set IO driving
 *
 * @param[in]   number   The IO number
 * @param[in]   driving  The driving enum number
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int fpioa_set_io_driving(int number, fpioa_driving_t driving);

/**
 * @brief       Get IO driving
 *
 * @param[in]   number  The IO number
 *
 * @return      result
 *     - -1     Fail
 *     - Other  The driving enum number
 */
int fpioa_get_io_driving(int number);

/**
 * @brief       Get IO by function
 *
 * @param[in]   function  The function enum number
 *
 * @return      result
 *     - -1     Fail
 *     - Other  The IO number
 */
int fpioa_get_io_by_function(fpioa_function_t function);

/**
 * @brief       Set IO slew rate control
 *
 * @param[in]   number      The IO number
 * @param[in]   sl_value    Enable slew rate. 0: disable 1:enable
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int fpioa_set_sl(int number, uint8_t sl_enable);

/**
 * @brief       Set IO schmitt trigger
 *
 * @param[in]   number       The IO number
 * @param[in]   st_enable    Enable schmitt trigger. 0: disable 1:enable
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int fpioa_set_st(int number, uint8_t st_enable);

/**
 * @brief       Set IO output invert enable
 *
 * @param[in]   number       The IO number
 * @param[in]   inv_enable   Enable output invert. 0: disable 1:enable
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int fpioa_set_oe_inv(int number, uint8_t inv_enable);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_FPIOA_H */

