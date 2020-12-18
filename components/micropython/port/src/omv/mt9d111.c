/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS},
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include "mt9d111.h"
#include "dvp.h"
#include "plic.h"
#include "sleep.h"
#include "sensor.h"
#include "mphalport.h"
#include "cambus.h"
#include "printf.h"

static i2c_device_number_t i2c_dev_tmp = -2; // i2c_device_number_t components/micropython/port/src/omv/cambus.c

/**
 * \brief Struct to store an register address and its value.
 */
struct Register
{
    uint8_t     address;    /**< struct Register address. */
    uint16_t    value;      /**< struct Register value. */
};

/**
 * \brief Default registers values (Values after boot).
 */
const struct Register reg_default_vals[] = 
{
    {MT9D111_REG_ROW_START,                                 0x001C},
    {MT9D111_REG_COLUMN_START,                              0x003C},
    {MT9D111_REG_ROW_WIDTH,                                 0x04B0},
    {MT9D111_REG_COL_WIDTH,                                 0x0640},
    {MT9D111_REG_HORIZONTAL_BLANKING_B,                     0x0204},
    {MT9D111_REG_VERTICAL_BLANKING_B,                       0x002F},
    {MT9D111_REG_HORIZONTAL_BLANKING_A,                     0x00FE},
    {MT9D111_REG_VERTICAL_BLANKING_A,                       0x000C},
    {MT9D111_REG_SHUTTER_WIDTH,                             0x04D0},
    {MT9D111_REG_ROW_SPEED,                                 0x0001},
    {MT9D111_REG_EXTRA_DELAY,                               0x0000},
    {MT9D111_REG_SHUTTER_DELAY,                             0x0000},
    {MT9D111_REG_RESET,                                     0x0000},
    {MT9D111_REG_FRAME_VALID_CONTROL,                       0x0000},
    {MT9D111_REG_READ_MODE_B,                               0x0300},
    {MT9D111_REG_READ_MODE_A,                               0x8400},
    {MT9D111_REG_DARK_COL_ROWS,                             0x010F},
    {MT9D111_REG_FLASH,                                     0x0608},
    {MT9D111_REG_EXTRA_RESET,                               0x8000},
    {MT9D111_REG_LINE_VALID_CONTROL,                        0x0000},
    {MT9D111_REG_BOTTOM_DARK_ROWS,                          0x0007},
    {MT9D111_REG_GREEN_1_GAIN,                              0x0020},
    {MT9D111_REG_BLUE_GAIN,                                 0x0020},
    {MT9D111_REG_RED_GAIN,                                  0x0020},
    {MT9D111_REG_GREEN_2_GAIN,                              0x0020},
    {MT9D111_REG_GLOBAL_GAIN,                               0x0020},
    {MT9D111_REG_ROW_NOISE,                                 0x042A},
    {MT9D111_REG_BLACK_ROWS,                                0x00FF},
    {MT9D111_REG_DARK_G1_AVERAGE,                           0x0000},
    {MT9D111_REG_DARK_B_AVERAGE,                            0x0000},
    {MT9D111_REG_DARK_R_AVERAGE,                            0x0000},
    {MT9D111_REG_DARK_G2_AVERAGE,                           0x0000},
    {MT9D111_REG_CALIB_THRESHOLD,                           0x231D},
    {MT9D111_REG_CALIB_CONTROL,                             0x0080},
    {MT9D111_REG_CALIB_GREEN_1,                             0x0000},
    {MT9D111_REG_CALIB_BLUE,                                0x0000},
    {MT9D111_REG_CALIB_RED,                                 0x0000},
    {MT9D111_REG_CALIB_GREEN_2,                             0x0000},
    {MT9D111_REG_CLOCK_CONTROL,                             0xE000},
    {MT9D111_REG_PLL_CONTROL_1,                             0x1000},
    {MT9D111_REG_PLL_CONTROL_2,                             0x0500},
    {MT9D111_REG_GLOBAL_SHUTTER_CONTROL,                    0x0000},
    {MT9D111_REG_START_INTEGRATION_T1,                      0x0064},
    {MT9D111_REG_START_READOUT_T2,                          0x0064},
    {MT9D111_REG_ASSERT_STROBE_T3,                          0x0096},
    {MT9D111_REG_DEASSERT_STROBE_T4,                        0x00C8},
    {MT9D111_REG_ASSERT_FLASH,                              0x0064},
    {MT9D111_REG_DEASSERT_FLASH,                            0x0078},
    {MT9D111_REG_EXTERNAL_SAMPLE_1,                         0x0000},
    {MT9D111_REG_EXTERNAL_SAMPLE_2,                         0x0000},
    {MT9D111_REG_EXTERNAL_SAMPLE_3,                         0x0000},
    {MT9D111_REG_EXTERNAL_SAMPLING_CONTROL,                 0x0000},
    {MT9D111_REG_PAGE_REGISTER,                             0x0000},
    {MT9D111_REG_BYTEWISE_ADDRESS,                          0x0000},
    {MT9D111_REG_CONTEXT_CONTROL,                           0x0000}
};

/**
 * \brief QVGA (320x240) at 30 FPS.
 *
 * \see https://github.com/ArduCAM/Arduino/blob/master/ArduCAM/mt9d111_regs.h
 */
const struct Register reg_vals_qvga_30fps[] =
{
    {MT9D111_REG_PAGE_REGISTER                                          ,   MT9D111_REG_PAGE_0},
    {0x33                                                               ,   0x0343},    // RESERVED_CORE_33

    {MT9D111_REG_PAGE_REGISTER                                          ,   MT9D111_REG_PAGE_1},
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0xA115},    // SEQ_LLMODE
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0020},    // SEQ_LLMODE

    {MT9D111_REG_PAGE_REGISTER                                          ,   MT9D111_REG_PAGE_0},
    {0x38                                                               ,   0x0866},    // RESERVED_CORE_38

    {MT9D111_REG_PAGE_REGISTER                                          ,   MT9D111_REG_PAGE_2},
    {MT9D111_REG_LENS_CORRECTION_CONTROL                                ,   0x0168},    // LENS_CORRECTION_CONTROL
    {MT9D111_REG_ZONE_BOUNDARIES_X1_AND_X2                              ,   0x6432},    // ZONE_BOUNDS_X1_X2
    {MT9D111_REG_ZONE_BOUNDARIES_X0_AND_X3                              ,   0x3296},    // ZONE_BOUNDS_X0_X3
    {MT9D111_REG_ZONE_BOUNDARIES_X4_AND_X5                              ,   0x9664},    // ZONE_BOUNDS_X4_X5
    {MT9D111_REG_ZONE_BOUNDARIES_Y1_AND_Y2                              ,   0x5028},    // ZONE_BOUNDS_Y1_Y2
    {MT9D111_REG_ZONE_BOUNDARIES_Y0_AND_Y3                              ,   0x2878},    // ZONE_BOUNDS_Y0_Y3
    {MT9D111_REG_ZONE_BOUNDARIES_Y4_AND_Y5                              ,   0x7850},    // ZONE_BOUNDS_Y4_Y5
    {MT9D111_REG_CENTER_OFFSET                                          ,   0x0000},    // CENTER_OFFSET
    {MT9D111_REG_FX_FOR_RED_COLOR_AT_THE_FIRST_PIXEL_OF_THE_ARRAY       ,   0x0152},    // FX_RED
    {MT9D111_REG_FX_FOR_GREEN_COLOR_AT_THE_FIRST_PIXEL_OF_THE_ARRAY     ,   0x015C},    // FX_GREEN
    {MT9D111_REG_FX_FOR_BLUE_COLOR_AT_THE_FIRST_PIXEL_OF_THE_ARRAY      ,   0x00F4},    // FX_BLUE
    {MT9D111_REG_FY_FOR_RED_COLOR_AT_THE_FIRST_PIXEL_OF_THE_ARRAY       ,   0x0108},    // FY_RED
    {MT9D111_REG_FY_FOR_GREEN_COLOR_AT_THE_FIRST_PIXEL_OF_THE_ARRAY     ,   0x00FA},    // FY_GREEN
    {MT9D111_REG_FY_FOR_BLUE_COLOR_AT_THE_FIRST_PIXEL_OF_THE_ARRAY      ,   0x00CF},    // FY_BLUE
    {MT9D111_REG_DF_DX_FOR_RED_COLOR_AT_THE_FIRST_PIXEL_OF_THE_ARRAY    ,   0x09AD},    // DF_DX_RED
    {MT9D111_REG_DF_DX_FOR_GREEN_COLOR_AT_THE_FIRST_PIXEL_OF_THE_ARRAY  ,   0x091E},    // DF_DX_GREEN
    {MT9D111_REG_DF_DX_FOR_BLUE_COLOR_AT_THE_FIRST_PIXEL_OF_THE_ARRAY   ,   0x0B3F},    // DF_DX_BLUE
    {MT9D111_REG_DF_DY_FOR_RED_COLOR_AT_THE_FIRST_PIXEL_OF_THE_ARRAY    ,   0x0C85},    // DF_DY_RED
    {MT9D111_REG_DF_DY_FOR_GREEN_COLOR_AT_THE_FIRST_PIXEL_OF_THE_ARRAY  ,   0x0CFF},    // DF_DY_GREEN
    {MT9D111_REG_DF_DY_FOR_BLUE_COLOR_AT_THE_FIRST_PIXEL_OF_THE_ARRAY   ,   0x0D86},    // DF_DY_BLUE
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_0_RED_COLOR                 ,   0x163A},    // SECOND_DERIV_ZONE_0_RED
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_0_GREEN_COLOR               ,   0x0E47},    // SECOND_DERIV_ZONE_0_GREEN
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_0_BLUE_COLOR                ,   0x103C},    // SECOND_DERIV_ZONE_0_BLUE
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_1_RED_COLOR                 ,   0x1D35},    // SECOND_DERIV_ZONE_1_RED
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_1_GREEN_COLOR               ,   0x173E},    // SECOND_DERIV_ZONE_1_GREEN
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_1_BLUE_COLOR                ,   0x1119},    // SECOND_DERIV_ZONE_1_BLUE
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_2_RED_COLOR                 ,   0x1663},    // SECOND_DERIV_ZONE_2_RED
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_2_GREEN_COLOR               ,   0x1569},    // SECOND_DERIV_ZONE_2_GREEN
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_2_BLUE_COLOR                ,   0x104C},    // SECOND_DERIV_ZONE_2_BLUE
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_3_RED_COLOR                 ,   0x1015},    // SECOND_DERIV_ZONE_3_RED
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_3_GREEN_COLOR               ,   0x1010},    // SECOND_DERIV_ZONE_3_GREEN
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_3_BLUE_COLOR                ,   0x0B0A},    // SECOND_DERIV_ZONE_3_BLUE
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_4_RED_COLOR                 ,   0x0D53},    // SECOND_DERIV_ZONE_4_RED
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_4_GREEN_COLOR               ,   0x0D51},    // SECOND_DERIV_ZONE_4_GREEN
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_4_BLUE_COLOR                ,   0x0A44},    // SECOND_DERIV_ZONE_4_BLUE
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_5_RED_COLOR                 ,   0x1545},    // SECOND_DERIV_ZONE_5_RED
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_5_GREEN_COLOR               ,   0x1643},    // SECOND_DERIV_ZONE_5_GREEN
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_5_BLUE_COLOR                ,   0x1231},    // SECOND_DERIV_ZONE_5_BLUE
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_6_RED_COLOR                 ,   0x0047},    // SECOND_DERIV_ZONE_6_RED
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_6_GREEN_COLOR               ,   0x035C},    // SECOND_DERIV_ZONE_6_GREEN
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_6_BLUE_COLOR                ,   0xFE30},    // SECOND_DERIV_ZONE_6_BLUE
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_7_RED_COLOR                 ,   0x4625},    // SECOND_DERIV_ZONE_7_RED
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_7_GREEN_COLOR               ,   0x47F3},    // SECOND_DERIV_ZONE_7_GREEN
    {MT9D111_REG_SECOND_DERIVATIVE_FOR_ZONE_7_BLUE_COLOR                ,   0x5859},    // SECOND_DERIV_ZONE_7_BLUE
    {MT9D111_REG_X2_FACTORS                                             ,   0x0000},    // X2_FACTORS
    {MT9D111_REG_GLOBAL_OFFSET_OF_FXY_FUNCTION                          ,   0x0000},    // GLOBAL_OFFSET_FXY_FUNCTION
    {MT9D111_REG_K_FACTOR_IN_K_FX_FY                                    ,   0x0000},    // K_FACTOR_IN_K_FX_FY

    {MT9D111_REG_PAGE_REGISTER                                          ,   MT9D111_REG_PAGE_1},
    {MT9D111_REG_COLOR_PIPELINE_CONTROL                                 ,   0x01FC},    // COLOR_PIPELINE_CONTROL

    {MT9D111_REG_PAGE_REGISTER                                          ,   MT9D111_REG_PAGE_1},
    {MT9D111_REG_YUV_YCbCr_CONTROL                                      ,   0x0004},    // YUV_YCBCR_CONTROL

    {MT9D111_REG_PAGE_REGISTER                                          ,   MT9D111_REG_PAGE_0},
    {MT9D111_REG_CLOCK_CONTROL                                          ,   0xA000},    // CLOCK_ENABLING

    {MT9D111_REG_PAGE_REGISTER                                          ,   MT9D111_REG_PAGE_1},
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0xA102},    // SEQ_MODE
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x001F},    // SEQ_MODE
    {MT9D111_REG_COLOR_PIPELINE_CONTROL                                 ,   0x01FC},    // COLOR_PIPELINE_CONTROL
    {MT9D111_REG_COLOR_PIPELINE_CONTROL                                 ,   0x01EC},    // COLOR_PIPELINE_CONTROL
    {MT9D111_REG_COLOR_PIPELINE_CONTROL                                 ,   0x01FC},    // COLOR_PIPELINE_CONTROL
    {MT9D111_REG_2D_APERTURE_CORRECTION_PARAMETERS                      ,   0x0F08},    // APERTURE_PARAMETERS
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x270B},    // MODE_CONFIG
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0030},    // MODE_CONFIG, JPEG disabled for A and B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0xA121},    // SEQ_CAP_MODE
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x007F},    // SEQ_CAP_MODE (127 frames before switching to Preview)

    {MT9D111_REG_PAGE_REGISTER                                          ,   MT9D111_REG_PAGE_0},
    {MT9D111_REG_HORIZONTAL_BLANKING_B                                  ,   0x00ED},    // HORZ_BLANK_B
    {MT9D111_REG_VERTICAL_BLANKING_B                                    ,   0x000B},    // VERT_BLANK_B
    {MT9D111_REG_HORIZONTAL_BLANKING_A                                  ,   0x0193},    // HORZ_BLANK_A
    {MT9D111_REG_VERTICAL_BLANKING_A                                    ,   0x000B},    // VERT_BLANK_A
    {MT9D111_REG_READ_MODE_B                                            ,   0x0700},    // READ_MODE_B (Image flip settings)
    {MT9D111_REG_READ_MODE_A                                            ,   0x8400},    // READ_MODE_A (1ADC)

    {MT9D111_REG_PAGE_REGISTER                                          ,   MT9D111_REG_PAGE_1},
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x2717},    // MODE_SENSOR_X_DELAY_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0187},    // MODE_SENSOR_X_DELAY_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x270F},    // MODE_SENSOR_ROW_START_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x001C},    // MODE_SENSOR_ROW_START_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x2711},    // MODE_SENSOR_COL_START_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x003C},    // MODE_SENSOR_COL_START_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x2713},    // MODE_SENSOR_ROW_HEIGHT_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x04B0},    // MODE_SENSOR_ROW_HEIGHT_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x2715},    // MODE_SENSOR_COL_WIDTH_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0640},    // MODE_SENSOR_COL_WIDTH_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x2719},    // MODE_SENSOR_ROW_SPEED_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0011},    // MODE_SENSOR_ROW_SPEED_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x2707},    // MODE_OUTPUT_WIDTH_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0640},    // MODE_OUTPUT_WIDTH_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x2709},    // MODE_OUTPUT_HEIGHT_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x04B0},    // MODE_OUTPUT_HEIGHT_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x271B},    // MODE_SENSOR_ROW_START_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x001C},    // MODE_SENSOR_ROW_START_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x271D},    // MODE_SENSOR_COL_START_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x003C},    // MODE_SENSOR_COL_START_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x271F},    // MODE_SENSOR_ROW_HEIGHT_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x04B0},    // MODE_SENSOR_ROW_HEIGHT_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x2721},    // MODE_SENSOR_COL_WIDTH_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0640},    // MODE_SENSOR_COL_WIDTH_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x2723},    // MODE_SENSOR_X_DELAY_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x03B1},    // MODE_SENSOR_X_DELAY_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x2725},    // MODE_SENSOR_ROW_SPEED_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0011},    // MODE_SENSOR_ROW_SPEED_B

    // Maximum Slew-Rate on IO-Pads (for Mode A)
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x276B},    // MODE_FIFO_CONF0_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0027},    // MODE_FIFO_CONF0_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x276D},    // MODE_FIFO_CONF1_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0xE0E2},    // MODE_FIFO_CONF1_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0xA76F},    // MODE_FIFO_CONF2_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x00E1},    // MODE_FIFO_CONF2_A

    // Maximum Slew-Rate on IO-Pads (for Mode B)
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x2772},    // MODE_FIFO_CONF0_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0027},    // MODE_FIFO_CONF0_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x2774},    // MODE_FIFO_CONF1_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0xE0E1},    // MODE_FIFO_CONF1_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0xA776},    // MODE_FIFO_CONF2_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x00E1},    // MODE_FIFO_CONF2_B

    // Set maximum integration time to get a minimum of 15 fps at 45MHz
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0xA20E},    // AE_MAX_INDEX
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0004},    // AE_MAX_INDEX

    // Set minimum integration time to get a maximum of 15 fps at 45MHz
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0xA20D},    // AE_MAX_INDEX
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0004},    // AE_MAX_INDEX

    // Configue all GPIO for output and set low to save power
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x9078},
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0000},
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x9079},
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0000},
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x9070},
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0000},
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x9071},
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0000},

    // Gamma and contrast
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0xA743},    // MODE_GAM_CONT_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0003},    // MODE_GAM_CONT_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0xA744},    // MODE_GAM_CONT_B
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0003},    // MODE_GAM_CONT_B

    // Set PLL (MCLK = 19 MHz , PCLK = 79 MHz)
    // Set PLL (MCLK = 45 MHz , PCLK = 45 MHz)  -> From Wizard
    {MT9D111_REG_PAGE_REGISTER                                          ,   MT9D111_REG_PAGE_0},
    {MT9D111_REG_CONTEXT_CONTROL                                        ,   0x0000},

    {MT9D111_REG_PAGE_REGISTER                                          ,   MT9D111_REG_PAGE_0},

    {MT9D111_REG_PAGE_REGISTER                                          ,   MT9D111_REG_PAGE_1},
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x2735},    // MODE_CROP_X0_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0000},    // MODE_CROP_X0_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x2737},    // MODE_CROP_X1_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   1600},      // MODE_CROP_X1_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x2739},    // MODE_CROP_Y0_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0000},    // MODE_CROP_Y0_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0x273B},    // MODE_CROP_Y1_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   1200},      // MODE_CROP_Y1_A
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_ADDRESS                       ,   0xA103},    // SEQ_CMD, Do capture
    {MT9D111_REG_MICROCONTROLLER_VARIABLE_DATA                          ,   0x0005},

    {MT9D111_REG_PAGE_REGISTER                                          ,   MT9D111_REG_PAGE_0},
};

static int mt9d111_read_reg(sensor_t *sensor, uint8_t reg_addr)
{
    printk("%s sensor %p\r\n", __func__, sensor);
    return 0;
}

static int mt9d111_write_reg(sensor_t *sensor, uint8_t reg_addr, uint16_t reg_data)
{
    printk("%s sensor %p\r\n", __func__, sensor);
    return 0;
}

static int mt9d111_set_pixformat(sensor_t *sensor, pixformat_t pixformat)
{
    printk("%s sensor %p\r\n", __func__, sensor);
    return 0;
}

static int mt9d111_set_framesize(sensor_t *sensor, framesize_t framesize)
{
    printk("%s sensor %p framesize %d\r\n", __func__, sensor, framesize);
    uint16_t width = resolution[framesize][0];
    uint16_t height = resolution[framesize][1];

    // if (framesize == FRAMESIZE_QQVGA)

    // this->SetResolution(MT9D111_MODE_CAPTURE, width, height);

    /* delay n ms */
    mp_hal_delay_ms(100);
    dvp_set_image_size(width, height);
    return 0;
}

static int mt9d111_set_framerate(sensor_t *sensor, framerate_t framerate)
{
    printk("%s %s %d\r\n", __func__, __FILE__, __LINE__);
    return 0;
}

static int mt9d111_set_contrast(sensor_t *sensor, int level)
{
    printk("%s %s %d\r\n", __func__, __FILE__, __LINE__);
    return 0;
}

static int mt9d111_set_brightness(sensor_t *sensor, int level)
{
    printk("%s %s %d\r\n", __func__, __FILE__, __LINE__);
    return 0;
}

static int mt9d111_set_saturation(sensor_t *sensor, int level)
{
    printk("%s %s %d\r\n", __func__, __FILE__, __LINE__);
    return 0;
}

static int mt9d111_set_gainceiling(sensor_t *sensor, gainceiling_t gainceiling)
{
    printk("%s %s %d\r\n", __func__, __FILE__, __LINE__);
    return 0;
}

static int mt9d111_set_quality(sensor_t *sensor, int qs)
{
    printk("%s %s %d\r\n", __func__, __FILE__, __LINE__);
    return 0;
}

static int mt9d111_set_colorbar(sensor_t *sensor, int enable)
{
    printk("%s %s %d\r\n", __func__, __FILE__, __LINE__);
    return 0;
}

static int mt9d111_set_auto_gain(sensor_t *sensor, int enable, float gain_db, float gain_db_ceiling)
{
    printk("%s %s %d\r\n", __func__, __FILE__, __LINE__);
    return 0;
}

static int mt9d111_get_gain_db(sensor_t *sensor, float *gain_db)
{
    printk("%s %s %d\r\n", __func__, __FILE__, __LINE__);
    return 0;
}

static int mt9d111_set_auto_exposure(sensor_t *sensor, int enable, int exposure_us)
{
    printk("%s %s %d\r\n", __func__, __FILE__, __LINE__);
    return 0;
}

static int mt9d111_get_exposure_us(sensor_t *sensor, int *exposure_us)
{
    printk("%s %s %d\r\n", __func__, __FILE__, __LINE__);
    return 0;
}

static int mt9d111_set_auto_whitebal(sensor_t *sensor, int enable, float r_gain_db, float g_gain_db, float b_gain_db)
{
    printk("%s %s %d\r\n", __func__, __FILE__, __LINE__);
    return 0;
}

static int mt9d111_get_rgb_gain_db(sensor_t *sensor, float *r_gain_db, float *g_gain_db, float *b_gain_db)
{
    printk("%s %s %d\r\n", __func__, __FILE__, __LINE__);
    return 0;
}

static int mt9d111_set_hmirror(sensor_t *sensor, int enable)
{
    printk("%s %s %d\r\n", __func__, __FILE__, __LINE__);
    return 0;
}

static int mt9d111_set_vflip(sensor_t *sensor, int enable)
{
    printk("%s %s %d\r\n", __func__, __FILE__, __LINE__);
    return 0;
}

static int mt9d11_write(i2c_device_number_t i2c_dev_tmp, uint8_t reg, uint16_t val)
{
    uint8_t tmp[3] = { 0 };
    tmp[0] = (uint8_t)(reg & 0xff);
    tmp[1] = (uint8_t)((val >> 8) & 0xff);
    tmp[2] = (uint8_t)(val & 0xff);
    int ret = maix_i2c_send_data(i2c_dev_tmp, MT9D111_CONFIG_I2C_ID, tmp, 3, 20);
    msleep(20);
    return ret;
}

static uint16_t mt9d11_read(i2c_device_number_t i2c_dev_tmp, uint8_t reg)
{
    uint8_t buf[2] = { 0 };
    maix_i2c_send_data(i2c_dev_tmp, MT9D111_CONFIG_I2C_ID, reg, 1, 20);
    int ret = maix_i2c_recv_data(i2c_dev_tmp, MT9D111_CONFIG_I2C_ID, NULL, 0, buf, 2, 30);
    msleep(20);
    if (ret == 0)
    {
        return (buf[0] << 8) + buf[1];
    }
    return -1;
}

uint16_t mt9d111_read_id(i2c_device_number_t extern_i2c_dev_tmp)
{
    i2c_dev_tmp = extern_i2c_dev_tmp; // TODO remove after implementation cambus_readw and cambus_writew
    // return 0x1519;
    mt9d11_write(i2c_dev_tmp, MT9D111_REG_PAGE_REGISTER, MT9D111_REG_PAGE_0);
    uint16_t id = mt9d11_read(i2c_dev_tmp, MT9D111_REG_RESERVED);
    // printk("[mt9d11] Sensor ID:0x%x,req:0x1519\r\n", id);
    return id;
}

static int mt9d111_reset(sensor_t *sensor)
{
    for (uint8_t i = 0; i < (sizeof(reg_vals_qvga_30fps) / sizeof(struct Register)); i++)
    {
        mt9d11_write(i2c_dev_tmp, reg_vals_qvga_30fps[i].address, reg_vals_qvga_30fps[i].value);
    }
    return 0;
}

int mt9d111_init(sensor_t *sensor)
{
    //Initialize sensor structure.
    sensor->gs_bpp = 2;
    sensor->reset = mt9d111_reset;
    sensor->read_reg = mt9d111_read_reg;
    sensor->write_reg = mt9d111_write_reg;
    sensor->set_pixformat = mt9d111_set_pixformat;
    sensor->set_framesize = mt9d111_set_framesize;
    sensor->set_framerate = mt9d111_set_framerate;
    sensor->set_contrast = mt9d111_set_contrast;
    sensor->set_brightness = mt9d111_set_brightness;
    sensor->set_saturation = mt9d111_set_saturation;
    sensor->set_gainceiling = mt9d111_set_gainceiling;
    sensor->set_quality = mt9d111_set_quality;
    sensor->set_colorbar = mt9d111_set_colorbar;
    sensor->set_auto_gain = mt9d111_set_auto_gain;
    sensor->get_gain_db = mt9d111_get_gain_db;
    sensor->set_auto_exposure = mt9d111_set_auto_exposure;
    sensor->get_exposure_us = mt9d111_get_exposure_us;
    sensor->set_auto_whitebal = mt9d111_set_auto_whitebal;
    sensor->get_rgb_gain_db = mt9d111_get_rgb_gain_db;
    sensor->set_hmirror = mt9d111_set_hmirror;
    sensor->set_vflip = mt9d111_set_vflip;

    // Set sensor flags
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_VSYNC, 0);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_HSYNC, 0);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_PIXCK, 1);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_FSYNC, 0);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_JPEGE, 1);

    return 0;
}