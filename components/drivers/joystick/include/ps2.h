#ifndef __PS2_H
#define __PS2_H

#include <stdio.h>
#include <stdint.h>

/* clang-format off */
#define CTRL_CLK        10
#define CTRL_CLK_HIGH   10
#define CTRL_BYTE_DELAY 5

//These are our button constants
#define PSB_SELECT      0x0001
#define PSB_L3          0x0002
#define PSB_R3          0x0004
#define PSB_START       0x0008
#define PSB_PAD_UP      0x0010
#define PSB_PAD_RIGHT   0x0020
#define PSB_PAD_DOWN    0x0040
#define PSB_PAD_LEFT    0x0080
#define PSB_L2          0x0100
#define PSB_R2          0x0200
#define PSB_L1          0x0400
#define PSB_R1          0x0800
#define PSB_GREEN       0x1000
#define PSB_RED         0x2000
#define PSB_BLUE        0x4000
#define PSB_PINK        0x8000
#define PSB_TRIANGLE    0x1000
#define PSB_CIRCLE      0x2000
#define PSB_CROSS       0x4000
#define PSB_SQUARE      0x8000

//Guitar  button constants
#define UP_STRUM		0x0010
#define DOWN_STRUM		0x0040
#define STAR_POWER		0x0100
#define GREEN_FRET		0x0200
#define YELLOW_FRET		0x1000
#define RED_FRET		0x2000
#define BLUE_FRET		0x4000
#define ORANGE_FRET		0x8000
#define WHAMMY_BAR		8

//These are stick values
#define PSS_RX 5
#define PSS_RY 6
#define PSS_LX 7
#define PSS_LY 8

//These are analog buttons
#define PSAB_PAD_RIGHT    9
#define PSAB_PAD_UP      11
#define PSAB_PAD_DOWN    12
#define PSAB_PAD_LEFT    10
#define PSAB_L2          19
#define PSAB_R2          20
#define PSAB_L1          17
#define PSAB_R1          18
#define PSAB_GREEN       13
#define PSAB_RED         14
#define PSAB_BLUE        15
#define PSAB_PINK        16
#define PSAB_TRIANGLE    13
#define PSAB_CIRCLE      14
#define PSAB_CROSS       15
#define PSAB_SQUARE      16

#define SET(x, y) (x |= (1 << y))
#define CLR(x, y) (x &= (~(1 << y)))
#define CHK(x, y) (x & (1 << y))
#define TOG(x, y) (x ^= (1 << y))

/* clang-format on */
uint8_t PS2X_Button(uint16_t button);
uint8_t PS2X_Analog(uint8_t button);
void PS2X_confg_io(uint8_t cs, uint8_t clk, uint8_t mosi, uint8_t miso);
uint8_t PS2X_read_gamepad(uint8_t motor1, uint8_t motor2);
uint8_t PS2X_config_gamepad(uint8_t pressures, uint8_t rumble);
uint8_t PS2X_readType(void);
void PS2X_enableRumble(void);
void PS2X_enableRumble(void);
void PS2X_reconfig_gamepad(void);

#endif
