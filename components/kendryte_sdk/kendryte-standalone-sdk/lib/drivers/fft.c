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
#include <stddef.h>
#include "dmac.h"
#include "utils.h"
#include "sysctl.h"
#include "fft.h"

static volatile fft_t *const fft = (volatile fft_t *)FFT_BASE_ADDR;

static void fft_init(uint8_t point, uint8_t mode, uint16_t shift, uint8_t is_dma, uint8_t input_mode, uint8_t data_mode)
{
    fft->fft_ctrl.fft_point = point;
    fft->fft_ctrl.fft_mode = mode;
    fft->fft_ctrl.fft_shift = shift;
    fft->fft_ctrl.dma_send = is_dma;
    fft->fft_ctrl.fft_enable = 1;
    fft->fft_ctrl.fft_input_mode = input_mode;
    fft->fft_ctrl.fft_data_mode = data_mode;
}

void fft_complex_uint16_dma(dmac_channel_number_t dma_send_channel_num, dmac_channel_number_t dma_receive_channel_num,
                        uint16_t shift, fft_direction_t direction, const uint64_t *input, size_t point_num, uint64_t *output)
{
    fft_point_t point = FFT_512;
    switch(point_num)
    {
        case 512:
            point = FFT_512;
            break;
        case 256:
            point = FFT_256;
            break;
        case 128:
            point = FFT_128;
            break;
        case 64:
            point = FFT_64;
            break;
        default:
            configASSERT(!"fft point error");
            break;
    }
    sysctl_clock_enable(SYSCTL_CLOCK_FFT);
    sysctl_reset(SYSCTL_RESET_FFT);
    fft_init(point, direction, shift, 1, 0, 0);
    sysctl_dma_select(dma_receive_channel_num, SYSCTL_DMA_SELECT_FFT_RX_REQ);
    sysctl_dma_select(dma_send_channel_num, SYSCTL_DMA_SELECT_FFT_TX_REQ);
    dmac_set_single_mode(dma_receive_channel_num, (void *)(&fft->fft_output_fifo), output, DMAC_ADDR_NOCHANGE, DMAC_ADDR_INCREMENT,
        DMAC_MSIZE_4, DMAC_TRANS_WIDTH_64, point_num>>1);
    dmac_set_single_mode(dma_send_channel_num, input, (void *)(&fft->fft_input_fifo), DMAC_ADDR_INCREMENT, DMAC_ADDR_NOCHANGE,
        DMAC_MSIZE_4, DMAC_TRANS_WIDTH_64, point_num>>1);
    dmac_wait_done(dma_receive_channel_num);
}


