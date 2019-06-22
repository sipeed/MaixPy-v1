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
#ifndef _DRIVER_DVP_H
#define _DRIVER_DVP_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* clang-format off */
/**
 * @brief       DVP object
 */
typedef struct _dvp
{
    uint32_t dvp_cfg;
    uint32_t r_addr;
    uint32_t g_addr;
    uint32_t b_addr;
    uint32_t cmos_cfg;
    uint32_t sccb_cfg;
    uint32_t sccb_ctl;
    uint32_t axi;
    uint32_t sts;
    uint32_t reverse;
    uint32_t rgb_addr;
} __attribute__((packed, aligned(4))) dvp_t;

/* DVP Config Register */
#define DVP_CFG_START_INT_ENABLE                0x00000001U
#define DVP_CFG_FINISH_INT_ENABLE               0x00000002U
#define DVP_CFG_AI_OUTPUT_ENABLE                0x00000004U
#define DVP_CFG_DISPLAY_OUTPUT_ENABLE           0x00000008U
#define DVP_CFG_AUTO_ENABLE                     0x00000010U
#define DVP_CFG_BURST_SIZE_4BEATS               0x00000100U
#define DVP_CFG_FORMAT_MASK                     0x00000600U
#define DVP_CFG_RGB_FORMAT                      0x00000000U
#define DVP_CFG_YUV_FORMAT                      0x00000200U
#define DVP_CFG_Y_FORMAT                        0x00000600U
#define DVP_CFG_HREF_BURST_NUM_MASK             0x000FF000U
#define DVP_CFG_HREF_BURST_NUM(x)               ((x) << 12)
#define DVP_CFG_LINE_NUM_MASK                   0x3FF00000U
#define DVP_CFG_LINE_NUM(x)                     ((x) << 20)

/* DVP CMOS Config Register */
#define DVP_CMOS_CLK_DIV_MASK                   0x000000FFU
#define DVP_CMOS_CLK_DIV(x)                     ((x) << 0)
#define DVP_CMOS_CLK_ENABLE                     0x00000100U
#define DVP_CMOS_RESET                          0x00010000U
#define DVP_CMOS_POWER_DOWN                     0x01000000U

/* DVP SCCB Config Register */
#define DVP_SCCB_BYTE_NUM_MASK                  0x00000003U
#define DVP_SCCB_BYTE_NUM_2                     0x00000001U
#define DVP_SCCB_BYTE_NUM_3                     0x00000002U
#define DVP_SCCB_BYTE_NUM_4                     0x00000003U
#define DVP_SCCB_SCL_LCNT_MASK                  0x0000FF00U
#define DVP_SCCB_SCL_LCNT(x)                    ((x) << 8)
#define DVP_SCCB_SCL_HCNT_MASK                  0x00FF0000U
#define DVP_SCCB_SCL_HCNT(x)                    ((x) << 16)
#define DVP_SCCB_RDATA_BYTE(x)                  ((x) >> 24)

/* DVP SCCB Control Register */
#define DVP_SCCB_WRITE_DATA_ENABLE                   0x00000001U
#define DVP_SCCB_DEVICE_ADDRESS(x)              ((x) << 0)
#define DVP_SCCB_REG_ADDRESS(x)                 ((x) << 8)
#define DVP_SCCB_WDATA_BYTE0(x)                 ((x) << 16)
#define DVP_SCCB_WDATA_BYTE1(x)                 ((x) << 24)

/* DVP AXI Register */
#define DVP_AXI_GM_MLEN_MASK                    0x000000FFU
#define DVP_AXI_GM_MLEN_1BYTE                   0x00000000U
#define DVP_AXI_GM_MLEN_4BYTE                   0x00000003U

/* DVP STS Register */
#define DVP_STS_FRAME_START                     0x00000001U
#define DVP_STS_FRAME_START_WE                  0x00000002U
#define DVP_STS_FRAME_FINISH                    0x00000100U
#define DVP_STS_FRAME_FINISH_WE                 0x00000200U
#define DVP_STS_DVP_EN                          0x00010000U
#define DVP_STS_DVP_EN_WE                       0x00020000U
#define DVP_STS_SCCB_EN                         0x01000000U
#define DVP_STS_SCCB_EN_WE                      0x02000000U
/* clang-format on */

typedef enum _dvp_output_mode
{
    DVP_OUTPUT_AI,
    DVP_OUTPUT_DISPLAY,
} dvp_output_mode_t;

/**
 * @brief       DVP object instance
 */
extern volatile dvp_t* const dvp;

/**
 * @brief       Initialize DVP
 */
void dvp_init(uint8_t reg_len);

/**
 * @brief       Set image format
 *
 * @param[in]   format      The image format
 */
void dvp_set_image_format(uint32_t format);

/**
 * @brief       Set image size
 *
 * @param[in]   width   The width  of image
 * @param[in]   height  The height of image
 */
void dvp_set_image_size(uint32_t width, uint32_t height);

/**
 * @brief       Set the address of RGB for AI
 *
 * @param[in]   r_addr      The R address of RGB
 * @param[in]   g_addr      The G address of RGB
 * @param[in]   b_addr      The B address of RGB
 */
void dvp_set_ai_addr(uint32_t r_addr, uint32_t g_addr, uint32_t b_addr);

/**
 * @brief       Set the address of RGB for display
 *
 * @param[in]   r_addr      The R address of RGB
 * @param[in]   g_addr      The G address of RGB
 * @param[in]   b_addr      The B address of RGB
 */
void dvp_set_display_addr(uint32_t addr);

/**
 * @brief       The frame start transfer
 */
void dvp_start_frame(void);

/**
 * @brief       The DVP convert start
 */
void dvp_start_convert(void);

/**
 * @brief       The DVP convert finish
 */
void dvp_finish_convert(void);

/**
 * @brief       Get the image data
 *
 * @note        The image data stored in the address of RGB
 */
void dvp_get_image(void);

/**
 * @brief       Use SCCB write register
 *
 * @param[in]   dev_addr        The device address
 * @param[in]   reg_addr        The register address
 * @param[in]   reg_data        The register data
 */
void dvp_sccb_send_data(uint8_t dev_addr, uint16_t reg_addr, uint8_t reg_data);

/**
 * @brief       Use SCCB read register
 *
 * @param[in]   dev_addr        The device address
 * @param[in]   reg_addr        The register address
 *
 * @return      The register value
 */
uint8_t dvp_sccb_receive_data(uint8_t dev_addr, uint16_t reg_addr);

/**
 * @brief       Enable dvp burst
 */
void dvp_enable_burst(void);

/**
 * @brief       Disable dvp burst
 */
void dvp_disable_burst(void);

/**
 * @brief       Enable or disable dvp interrupt
 *
 * @param[in]   interrupt       Dvp interrupt
 * @param[in]   status          0:disable 1:enable
 *
 */
void dvp_config_interrupt(uint32_t interrupt, uint8_t enable);

/**
 * @brief       Get dvp interrupt status
 *
 * @param[in]   interrupt       Dvp interrupt
 *
 *
 * @return      Interrupt status
 *     - 0      false
 *     - 1      true
 */
int dvp_get_interrupt(uint32_t interrupt);

/**
 * @brief       Clear dvp interrupt status
 *
 * @param[in]   interrupt       Dvp interrupt
 *
 */
void dvp_clear_interrupt(uint32_t interrupt);

/**
 * @brief       Enable dvp auto mode
 */
void dvp_enable_auto(void);

/**
 * @brief       Disable dvp auto mode
 */
void dvp_disable_auto(void);

/**
 * @brief       Dvp ouput data enable or not
 *
 * @param[in]   index       0:AI, 1:display
 * @param[in]   enable      0:disable, 1:enable
 *
 */
void dvp_set_output_enable(dvp_output_mode_t index, int enable);

/**
 * @brief       Set sccb clock rate
 *
 * @param[in]   clk_rate       Sccb clock rate
 *
 * @return      The real sccb clock rate
 */
uint32_t dvp_sccb_set_clk_rate(uint32_t clk_rate);

/**
 * @brief       Set xclk rate
 *
 * @param[in]   clk_rate       xclk rate
 *
 * @return      The real xclk rate
 */
uint32_t dvp_set_xclk_rate(uint32_t xclk_rate);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_DVP_H */
