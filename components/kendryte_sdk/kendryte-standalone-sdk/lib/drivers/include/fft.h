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
#ifndef _DRIVER_FFT_H
#define _DRIVER_FFT_H

#include <stdint.h>
#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _complex_hard
{
    int16_t real;
    int16_t imag;
} complex_hard_t;

typedef struct _fft_data
{
    int16_t I1;
    int16_t R1;
    int16_t I2;
    int16_t R2;
} fft_data_t;

typedef enum _fft_point
{
    FFT_512,
    FFT_256,
    FFT_128,
    FFT_64,
} fft_point_t;

typedef enum _fft_direction
{
    FFT_DIR_BACKWARD,
    FFT_DIR_FORWARD,
    FFT_DIR_MAX,
} fft_direction_t;

/**
 * @brief      FFT algorithm accelerator register
 *
 * @note       FFT algorithm accelerator register table
 *
 * | Offset    | Name           | Description                         |
 * |-----------|----------------|-------------------------------------|
 * | 0x00      | fft_input_fifo | input data fifo                     |
 * | 0x08      | fft_ctrl       | fft ctrl reg                        |
 * | 0x10      | fifo_ctrl      | fifo ctrl                           |
 * | 0x18      | intr_mask      | interrupt mask                      |
 * | 0x20      | intr_clear     | interrupt clear                     |
 * | 0x28      | fft_status     | fft status reg                      |
 * | 0x30      | fft_status_raw | fft_status_raw                      |
 * | 0x38      | fft_output_fifo| fft_output_fifo                     |
 *
 */

/**
 * @brief      The calculation data is input through this register
 *
 *             No. 0 Register (0x00)
 */
typedef struct _fft_input_fifo
{
    uint64_t fft_input_fifo : 64;
} __attribute__((packed, aligned(8))) fft_input_fifo_t;

/**
 * @brief      fft ctrl reg
 *
 *             No. 1 Register (0x08)
 */
typedef struct _fft_fft_ctrl
{
    /**
     *FFT calculation data length:
     *b'000:512 point; b'001:256 point; b'010:128 point; b'011:64 point;
     */
    uint64_t fft_point : 3;
    /* FFT mode: b'0:FFT b'1:IFFT */
    uint64_t fft_mode : 1;
    /* Corresponding to the nine layer butterfly shift operation, 0x0: does not shift; 0x1: shift 1st layer. ...*/
    uint64_t fft_shift : 9;
    /* FFT enable: b'0:disable b'1:enable */
    uint64_t fft_enable : 1;
    /* FFT DMA enable: b'0:disable b'1:enable */
    uint64_t dma_send : 1;
    /**
     *Input data arrangement: b'00:RIRI; b'01:only real part exist, RRRR;
     *b'10:First input the real part and then input the imaginary part.
     */
    uint64_t fft_input_mode : 2;
    /* Effective width of input data. b'0:64bit effective; b'1:32bit effective  */
    uint64_t fft_data_mode : 1;
    uint64_t reserved : 46;
} __attribute__((packed, aligned(8))) fft_fft_ctrl_t;

/**
 * @brief      fifo ctrl
 *
 *             No. 2 Register (0x10)
 */
typedef struct _fft_fifo_ctrl
{
    /* Response memory initialization flag.b'1:initialization */
    uint64_t resp_fifo_flush_n : 1;
    /* Command memory initialization flag.b'1:initialization */
    uint64_t cmd_fifo_flush_n : 1;
    /* Output interface memory initialization flag.b'1:initialization */
    uint64_t gs_fifo_flush_n : 1;
    uint64_t reserved : 61;
} __attribute__((packed, aligned(8))) fft_fifo_ctrl_t;

/**
 * @brief      interrupt mask
 *
 *             No. 3 Register (0x18)
 */
typedef struct _fft_intr_mask
{
    /**
     *FFT return status set.
     *b'0:FFT returns to the state after completion.
     *b'1:FFT does not return to the state after completion
     */
    uint64_t fft_done_mask : 1;
    uint64_t reserved : 63;
} __attribute__((packed, aligned(8))) fft_intr_mask_t;

/**
 * @brief      interrupt clear
 *
 *             No. 4 Register (0x20)
 */
typedef struct _fft_intr_clear
{
    /* The interrupt state clears. b'1:clear current interrupt request */
    uint64_t fft_done_clear : 1;
    uint64_t reserved1 : 63;
} __attribute__((packed, aligned(8))) fft_intr_clear_t;

/**
 * @brief      fft status reg
 *
 *             No. 5 Register (0x28)
 */
typedef struct _fft_status
{
    /* FFT calculation state.b'0:not completed; b'1:completed */
    uint64_t fft_done_status : 1;
    uint64_t reserved1 : 63;
} __attribute__((packed, aligned(8))) fft_status_t;

/**
 * @brief      fft status raw
 *
 *             No. 6 Register (0x30)
 */
typedef struct _fft_status_raw
{
    /* FFT calculation state. b'1:done */
    uint64_t fft_done_status_raw : 1;
    /* FFT calculation state. b'1:working */
    uint64_t fft_work_status_raw : 1;
    uint64_t reserved : 62;
} __attribute__((packed, aligned(8))) fft_status_raw_t;

/**
 * @brief      Output of FFT calculation data through this register
 *
 *             No. 7 Register (0x38)
 */
typedef struct _fft_output_fifo
{
    uint64_t fft_output_fifo : 64;
} __attribute__((packed, aligned(8))) fft_output_fifo_t;

/**
 * @brief      Fast Fourier transform (FFT) algorithm accelerator object
 *
 *             A fast Fourier transform (FFT) algorithm computes the discrete
 *             Fourier transform (DFT) of a sequence, or its inverse (IFFT).
 *             Fourier analysis converts a signal from its original domain
 *             (often time or space) to a representation in the frequency
 *             domain and vice versa. An FFT rapidly computes such
 *             transformations by factorizing the DFT matrix into a product of
 *             sparse (mostly zero) factors.
 */
typedef struct _fft
{
    /* No. 0 (0x00): input data fifo */
    fft_input_fifo_t fft_input_fifo;
    /* No. 1 (0x08): fft ctrl reg */
    fft_fft_ctrl_t fft_ctrl;
    /* No. 2 (0x10): fifo ctrl */
    fft_fifo_ctrl_t fifo_ctrl;
    /* No. 3 (0x18): interrupt mask */
    fft_intr_mask_t intr_mask;
    /* No. 4 (0x20): interrupt clear */
    fft_intr_clear_t intr_clear;
    /* No. 5 (0x28): fft status reg */
    fft_status_t fft_status;
    /* No. 6 (0x30): fft_status_raw */
    fft_status_raw_t fft_status_raw;
    /* No. 7 (0x38): fft_output_fifo */
    fft_output_fifo_t fft_output_fifo;
} __attribute__((packed, aligned(8))) fft_t;

/**
 * @brief       Do 16bit quantized complex FFT by DMA
 *
 * @param[in]   dma_send_channel_num        Dmac send channel number.
 * @param[in]   dma_receive_channel_num     Dmac receive channel number.
 * @param[in]   shift                       The shifts selection in 9 stage
 * @param[in]   direction                   The direction
 * @param[in]   input                       The input data
 * @param[in]   point                       The FFT points count
 * @param[out]  output                      The output data
 */
void fft_complex_uint16_dma(dmac_channel_number_t dma_send_channel_num,
    dmac_channel_number_t dma_receive_channel_num,
    uint16_t shift,
    fft_direction_t direction,
    const uint64_t *input,
    size_t point_num,
    uint64_t *output);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_FFT_H */

