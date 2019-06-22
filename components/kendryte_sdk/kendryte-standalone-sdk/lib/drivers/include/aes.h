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
#ifndef _DRIVER_AES_H
#define _DRIVER_AES_H
#include <stdlib.h>
#include <stdint.h>
#include "platform.h"
#include "dmac.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _aes_cipher_mode
{
    AES_ECB = 0,
    AES_CBC = 1,
    AES_GCM = 2,
    AES_CIPHER_MAX,
} aes_cipher_mode_t;

typedef enum _aes_kmode
{
    AES_128 = 16,
    AES_192 = 24,
    AES_256 = 32,
} aes_kmode_t;

typedef enum _aes_iv_len
{
    IV_LEN_96 = 12,
    IV_LEN_128 = 16,
} aes_iv_len_t;

typedef enum _aes_encrypt_sel
{
    AES_HARD_ENCRYPTION = 0,
    AES_HARD_DECRYPTION = 1,
} aes_encrypt_sel_t;

typedef struct _aes_mode_ctl
{
    /* [2:0]:000:ecb; 001:cbc,010:gcm */
    uint32_t cipher_mode : 3;
    /* [4:3]:00:aes-128; 01:aes-192; 10:aes-256;11:reserved*/
    uint32_t kmode : 2;
    /* [6:5]:input key order 1：little endian; 0: big endian */
    uint32_t key_order : 2;
    /* [8:7]:input data order 1：little endian; 0: big endian */
    uint32_t input_order : 2;
    /* [10:9]:output data order 1：little endian; 0: big endian */
    uint32_t output_order : 2;
    uint32_t reserved : 21;
} __attribute__((packed, aligned(4))) aes_mode_ctl_t;

/**
 * @brief       AES
 */
typedef struct _aes
{
    /* (0x00) customer key.1st~4th byte key */
    uint32_t aes_key[4];
    /* (0x10) 0: encryption; 1: decryption */
    uint32_t encrypt_sel;
    /* (0x14) aes mode reg */
    aes_mode_ctl_t mode_ctl;
    /* (0x18) Initialisation Vector. GCM support 96bit. CBC support 128bit */
    uint32_t aes_iv[4];
    /* (0x28) input data endian;1:little endian; 0:big endian */
    uint32_t aes_endian;
    /* (0x2c) calculate status. 1:finish; 0:not finish */
    uint32_t aes_finish;
    /* (0x30) aes out data to dma 0:cpu 1:dma */
    uint32_t dma_sel;
    /* (0x34) gcm Additional authenticated data number */
    uint32_t gb_aad_num;
    uint32_t reserved;
    /* (0x3c) aes plantext/ciphter text input data number */
    uint32_t gb_pc_num;
    /* (0x40) aes plantext/ciphter text input data */
    uint32_t aes_text_data;
    /* (0x44) Additional authenticated data */
    uint32_t aes_aad_data;
    /**
     * (0x48) [1:0],b'00:check not finish; b'01:check fail; b'10:check success;
     * b'11:reversed
     */
    uint32_t tag_chk;
    /* (0x4c) data can input flag. 1: data can input; 0 : data cannot input */
    uint32_t data_in_flag;
    /* (0x50) gcm input tag for compare with the calculate tag */
    uint32_t gcm_in_tag[4];
    /* (0x60) aes plantext/ciphter text output data */
    uint32_t aes_out_data;
    /* (0x64) aes module enable */
    uint32_t gb_aes_en;
    /* (0x68) data can output flag 1: data ready 0: data not ready */
    uint32_t data_out_flag;
    /* (0x6c) allow tag input when use gcm */
    uint32_t tag_in_flag;
    /* (0x70) clear tag_chk */
    uint32_t tag_clear;
    uint32_t gcm_out_tag[4];
    /* (0x84) customer key for aes-192 aes-256.5th~8th byte key */
    uint32_t aes_key_ext[4];
} __attribute__((packed, aligned(4))) aes_t;

typedef struct _gcm_context
{
    /* The buffer holding the encryption or decryption key. */
    uint8_t *input_key;
    /* The initialization vector. must be 96 bit */
    uint8_t *iv;
    /* The buffer holding the Additional authenticated data. or NULL */
    uint8_t *gcm_aad;
    /* The length of the Additional authenticated data. or 0L */
    size_t gcm_aad_len;
} gcm_context_t;

typedef struct _cbc_context
{
    /* The buffer holding the encryption or decryption key. */
    uint8_t *input_key;
    /* The initialization vector. must be 128 bit */
    uint8_t *iv;
} cbc_context_t;

/**
 * @brief       AES-ECB-128 decryption
 *
 * @param[in]   input_key       The decryption key. must be 16bytes.
 * @param[in]   input_data      The buffer holding the input data.
 * @param[in]   input_len       The length of a data unit in bytes.
 *                              This can be any length between 16 bytes and 2^31 bytes inclusive
 *                              (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data     The buffer holding the output data.
 *                              The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_ecb128_hard_decrypt(uint8_t *input_key, uint8_t *input_data, size_t input_len, uint8_t *output_data);

/**
 * @brief       AES-ECB-128 encryption
 *
 * @param[in]   input_key       The encryption key. must be 16bytes.
 * @param[in]   input_data      The buffer holding the input data.
 * @param[in]   input_len       The length of a data unit in bytes.
 *                              This can be any length between 16 bytes and 2^31 bytes inclusive
 *                              (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data     The buffer holding the output data.
 *                              The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_ecb128_hard_encrypt(uint8_t *input_key, uint8_t *input_data, size_t input_len, uint8_t *output_data);

/**
 * @brief       AES-ECB-192 decryption
 *
 * @param[in]   input_key       The decryption key. must be 24bytes.
 * @param[in]   input_data      The buffer holding the input data.
 * @param[in]   input_len       The length of a data unit in bytes.
 *                              This can be any length between 16 bytes and 2^31 bytes inclusive
 *                              (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data     The buffer holding the output data.
 *                              The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_ecb192_hard_decrypt(uint8_t *input_key, uint8_t *input_data, size_t input_len, uint8_t *output_data);

/**
 * @brief       AES-ECB-192 encryption
 *
 * @param[in]   input_key       The encryption key. must be 24bytes.
 * @param[in]   input_data      The buffer holding the input data.
 * @param[in]   input_len       The length of a data unit in bytes.
 *                              This can be any length between 16 bytes and 2^31 bytes inclusive
 *                              (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data     The buffer holding the output data.
 *                              The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_ecb192_hard_encrypt(uint8_t *input_key, uint8_t *input_data, size_t input_len, uint8_t *output_data);

/**
 * @brief       AES-ECB-256 decryption
 *
 * @param[in]   input_key       The decryption key. must be 32bytes.
 * @param[in]   input_data      The buffer holding the input data.
 * @param[in]   input_len       The length of a data unit in bytes.
 *                              This can be any length between 16 bytes and 2^31 bytes inclusive
 *                              (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data     The buffer holding the output data.
 *                              The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_ecb256_hard_decrypt(uint8_t *input_key, uint8_t *input_data, size_t input_len, uint8_t *output_data);

/**
 * @brief       AES-ECB-256 encryption
 *
 * @param[in]   input_key       The encryption key. must be 32bytes.
 * @param[in]   input_data      The buffer holding the input data.
 * @param[in]   input_len       The length of a data unit in bytes.
 *                              This can be any length between 16 bytes and 2^31 bytes inclusive
 *                              (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data     The buffer holding the output data.
 *                              The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_ecb256_hard_encrypt(uint8_t *input_key, uint8_t *input_data, size_t input_len, uint8_t *output_data);

/**
 * @brief       AES-CBC-128 decryption
 *
 * @param[in]   context         The cbc context to use for encryption or decryption.
 * @param[in]   input_key       The decryption key. must be 16bytes.
 * @param[in]   input_data      The buffer holding the input data.
 * @param[in]   input_len       The length of a data unit in bytes.
 *                              This can be any length between 16 bytes and 2^31 bytes inclusive
 *                              (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data     The buffer holding the output data.
 *                              The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_cbc128_hard_decrypt(cbc_context_t *context, uint8_t *input_data, size_t input_len, uint8_t *output_data);

/**
 * @brief       AES-CBC-128 encryption
 *
 * @param[in]   context         The cbc context to use for encryption or decryption.
 * @param[in]   input_key       The encryption key. must be 16bytes.
 * @param[in]   input_data      The buffer holding the input data.
 * @param[in]   input_len       The length of a data unit in bytes.
 *                              This can be any length between 16 bytes and 2^31 bytes inclusive
 *                              (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data     The buffer holding the output data.
 *                              The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_cbc128_hard_encrypt(cbc_context_t *context, uint8_t *input_data, size_t input_len, uint8_t *output_data);

/**
 * @brief       AES-CBC-192 decryption
 *
 * @param[in]   context         The cbc context to use for encryption or decryption.
 * @param[in]   input_key       The decryption key. must be 24bytes.
 * @param[in]   input_data      The buffer holding the input data.
 * @param[in]   input_len       The length of a data unit in bytes.
 *                              This can be any length between 16 bytes and 2^31 bytes inclusive
 *                              (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data     The buffer holding the output data.
 *                              The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_cbc192_hard_decrypt(cbc_context_t *context, uint8_t *input_data, size_t input_len, uint8_t *output_data);

/**
 * @brief       AES-CBC-192 encryption
 *
 * @param[in]   context         The cbc context to use for encryption or decryption.
 * @param[in]   input_key       The encryption key. must be 24bytes.
 * @param[in]   input_data      The buffer holding the input data.
 * @param[in]   input_len       The length of a data unit in bytes.
 *                              This can be any length between 16 bytes and 2^31 bytes inclusive
 *                              (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data     The buffer holding the output data.
 *                              The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_cbc192_hard_encrypt(cbc_context_t *context, uint8_t *input_data, size_t input_len, uint8_t *output_data);

/**
 * @brief       AES-CBC-256 decryption
 *
 * @param[in]   context         The cbc context to use for encryption or decryption.
 * @param[in]   input_key       The decryption key. must be 32bytes.
 * @param[in]   input_data      The buffer holding the input data.
 * @param[in]   input_len       The length of a data unit in bytes.
 *                              This can be any length between 16 bytes and 2^31 bytes inclusive
 *                              (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data     The buffer holding the output data.
 *                              The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_cbc256_hard_decrypt(cbc_context_t *context, uint8_t *input_data, size_t input_len, uint8_t *output_data);

/**
 * @brief       AES-CBC-256 encryption
 *
 * @param[in]   context         The cbc context to use for encryption or decryption.
 * @param[in]   input_key       The encryption key. must be 32bytes.
 * @param[in]   input_data      The buffer holding the input data.
 * @param[in]   input_len       The length of a data unit in bytes.
 *                              This can be any length between 16 bytes and 2^31 bytes inclusive
 *                              (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data     The buffer holding the output data.
 *                              The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_cbc256_hard_encrypt(cbc_context_t *context, uint8_t *input_data, size_t input_len, uint8_t *output_data);

/**
 * @brief       AES-GCM-128 decryption
 *
 * @param[in]   context         The gcm context to use for encryption or decryption.
 * @param[in]   input_key       The decryption key. must be 16bytes.
 * @param[in]   input_data      The buffer holding the input data.
 * @param[in]   input_len       The length of a data unit in bytes.
 *                              This can be any length between 16 bytes and 2^31 bytes inclusive
 *                              (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data     The buffer holding the output data.
 * @param[out]  gcm_tag         The buffer for holding the tag.The length of the tag must be 4 bytes.
 */
void aes_gcm128_hard_decrypt(gcm_context_t *context, uint8_t *input_data, size_t input_len, uint8_t *output_data, uint8_t *gcm_tag);

/**
 * @brief       AES-GCM-128 encryption
 *
 * @param[in]   context         The gcm context to use for encryption or decryption.
 * @param[in]   input_key       The encryption key. must be 16bytes.
 * @param[in]   input_data      The buffer holding the input data.
 * @param[in]   input_len       The length of a data unit in bytes.
 *                              This can be any length between 16 bytes and 2^31 bytes inclusive
 *                              (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data     The buffer holding the output data.
 * @param[out]  gcm_tag         The buffer for holding the tag.The length of the tag must be 4 bytes.
 */
void aes_gcm128_hard_encrypt(gcm_context_t *context, uint8_t *input_data, size_t input_len, uint8_t *output_data, uint8_t *gcm_tag);

/**
 * @brief       AES-GCM-192 decryption
 *
 * @param[in]   context         The gcm context to use for encryption or decryption.
 * @param[in]   input_key       The decryption key. must be 24bytes.
 * @param[in]   input_data      The buffer holding the input data.
 * @param[in]   input_len       The length of a data unit in bytes.
 *                              This can be any length between 16 bytes and 2^31 bytes inclusive
 *                              (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data     The buffer holding the output data.
 * @param[out]  gcm_tag         The buffer for holding the tag.The length of the tag must be 4 bytes.
 */
void aes_gcm192_hard_decrypt(gcm_context_t *context, uint8_t *input_data, size_t input_len, uint8_t *output_data, uint8_t *gcm_tag);

/**
 * @brief       AES-GCM-192 encryption
 *
 * @param[in]   context         The gcm context to use for encryption or decryption.
 * @param[in]   input_key       The encryption key. must be 24bytes.
 * @param[in]   input_data      The buffer holding the input data.
 * @param[in]   input_len       The length of a data unit in bytes.
 *                              This can be any length between 16 bytes and 2^31 bytes inclusive
 *                              (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data     The buffer holding the output data.
 * @param[out]  gcm_tag         The buffer for holding the tag.The length of the tag must be 4 bytes.
 */
void aes_gcm192_hard_encrypt(gcm_context_t *context, uint8_t *input_data, size_t input_len, uint8_t *output_data, uint8_t *gcm_tag);

/**
 * @brief       AES-GCM-256 decryption
 *
 * @param[in]   context         The gcm context to use for encryption or decryption.
 * @param[in]   input_key       The decryption key. must be 32bytes.
 * @param[in]   input_data      The buffer holding the input data.
 * @param[in]   input_len       The length of a data unit in bytes.
 *                              This can be any length between 16 bytes and 2^31 bytes inclusive
 *                              (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data     The buffer holding the output data.
 * @param[out]  gcm_tag         The buffer for holding the tag.The length of the tag must be 4 bytes.
 */
void aes_gcm256_hard_decrypt(gcm_context_t *context, uint8_t *input_data, size_t input_len, uint8_t *output_data, uint8_t *gcm_tag);

/**
 * @brief       AES-GCM-256 encryption
 *
 * @param[in]   context         The gcm context to use for encryption or decryption.
 * @param[in]   input_key       The encryption key. must be 32bytes.
 * @param[in]   input_data      The buffer holding the input data.
 * @param[in]   input_len       The length of a data unit in bytes.
 *                              This can be any length between 16 bytes and 2^31 bytes inclusive
 *                              (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data     The buffer holding the output data.
 * @param[out]  gcm_tag         The buffer for holding the tag.The length of the tag must be 4 bytes.
 */
void aes_gcm256_hard_encrypt(gcm_context_t *context, uint8_t *input_data, size_t input_len, uint8_t *output_data, uint8_t *gcm_tag);

/**
 * @brief       AES-ECB-128 decryption by dma
 *
 * @param[in]   dma_receive_channel_num     Dmac receive channel number.
 * @param[in]   input_key                   The decryption key. must be 16bytes.
 * @param[in]   input_data                  The buffer holding the input data.
 * @param[in]   input_len                   The length of a data unit in bytes.
 *                                          This can be any length between 16 bytes and 2^31 bytes inclusive
 *                                          (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data                 The buffer holding the output data.
 *                                          The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_ecb128_hard_decrypt_dma(dmac_channel_number_t dma_receive_channel_num,
    uint8_t *input_key,
    uint8_t *input_data,
    size_t input_len,
    uint8_t *output_data);

/**
 * @brief       AES-ECB-128 encryption by dma
 *
 * @param[in]   dma_receive_channel_num     Dmac receive channel number.
 * @param[in]   input_key                   The encryption key. must be 16bytes.
 * @param[in]   input_data                  The buffer holding the input data.
 * @param[in]   input_len                   The length of a data unit in bytes.
 *                                          This can be any length between 16 bytes and 2^31 bytes inclusive
 *                                          (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data                 The buffer holding the output data.
 *                                          The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_ecb128_hard_encrypt_dma(dmac_channel_number_t dma_receive_channel_num,
    uint8_t *input_key,
    uint8_t *input_data,
    size_t input_len,
    uint8_t *output_data);

/**
 * @brief       AES-ECB-192 decryption by dma
 *
 * @param[in]   dma_receive_channel_num     Dmac receive channel number.
 * @param[in]   input_key                   The decryption key. must be 16bytes.
 * @param[in]   input_data                  The buffer holding the input data.
 * @param[in]   input_len                   The length of a data unit in bytes.
 *                                          This can be any length between 16 bytes and 2^31 bytes inclusive
 *                                          (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data                 The buffer holding the output data.
 *                                          The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_ecb192_hard_decrypt_dma(dmac_channel_number_t dma_receive_channel_num,
    uint8_t *input_key,
    uint8_t *input_data,
    size_t input_len,
    uint8_t *output_data);

/**
 * @brief       AES-ECB-192 encryption by dma
 *
 * @param[in]   dma_receive_channel_num     Dmac receive channel number.
 * @param[in]   input_key                   The encryption key. must be 16bytes.
 * @param[in]   input_data                  The buffer holding the input data.
 * @param[in]   input_len                   The length of a data unit in bytes.
 *                                          This can be any length between 16 bytes and 2^31 bytes inclusive
 *                                          (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data                 The buffer holding the output data.
 *                                          The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_ecb192_hard_encrypt_dma(dmac_channel_number_t dma_receive_channel_num,
    uint8_t *input_key,
    uint8_t *input_data,
    size_t input_len,
    uint8_t *output_data);

/**
 * @brief       AES-ECB-256 decryption by dma
 *
 * @param[in]   dma_receive_channel_num     Dmac receive channel number.
 * @param[in]   input_key                   The decryption key. must be 16bytes.
 * @param[in]   input_data                  The buffer holding the input data.
 * @param[in]   input_len                   The length of a data unit in bytes.
 *                                          This can be any length between 16 bytes and 2^31 bytes inclusive
 *                                          (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data                 The buffer holding the output data.
 *                                          The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_ecb256_hard_decrypt_dma(dmac_channel_number_t dma_receive_channel_num,
    uint8_t *input_key,
    uint8_t *input_data,
    size_t input_len,
    uint8_t *output_data);

/**
 * @brief       AES-ECB-256 encryption by dma
 *
 * @param[in]   dma_receive_channel_num     Dmac receive channel number.
 * @param[in]   input_key                   The encryption key. must be 16bytes.
 * @param[in]   input_data                  The buffer holding the input data.
 * @param[in]   input_len                   The length of a data unit in bytes.
 *                                          This can be any length between 16 bytes and 2^31 bytes inclusive
 *                                          (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data                 The buffer holding the output data.
 *                                          The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_ecb256_hard_encrypt_dma(dmac_channel_number_t dma_receive_channel_num,
    uint8_t *input_key,
    uint8_t *input_data,
    size_t input_len,
    uint8_t *output_data);

/**
 * @brief       AES-CBC-128 decryption
 *
 * @param[in]   dma_receive_channel_num     Dmac receive channel number.
 * @param[in]   context                     The cbc context to use for encryption or decryption.
 * @param[in]   input_key                   The encryption key. must be 24bytes.
 * @param[in]   input_data                  The buffer holding the input data.
 * @param[in]   input_len                   The length of a data unit in bytes.
 *                                          This can be any length between 16 bytes and 2^31 bytes inclusive
 *                                          (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data                 The buffer holding the output data.
 *                                          The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_cbc128_hard_decrypt_dma(dmac_channel_number_t dma_receive_channel_num,
    cbc_context_t *context,
    uint8_t *input_data,
    size_t input_len,
    uint8_t *output_data);

/**
 * @brief       AES-CBC-128 encryption
 *
 * @param[in]   dma_receive_channel_num     Dmac receive channel number.
 * @param[in]   context                     The cbc context to use for encryption or decryption.
 * @param[in]   input_key                   The encryption key. must be 24bytes.
 * @param[in]   input_data                  The buffer holding the input data.
 * @param[in]   input_len                   The length of a data unit in bytes.
 *                                          This can be any length between 16 bytes and 2^31 bytes inclusive
 *                                          (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data                 The buffer holding the output data.
 *                                          The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_cbc128_hard_encrypt_dma(dmac_channel_number_t dma_receive_channel_num,
    cbc_context_t *context,
    uint8_t *input_data,
    size_t input_len,
    uint8_t *output_data);

/**
 * @brief       AES-CBC-192 decryption
 *
 * @param[in]   dma_receive_channel_num     Dmac receive channel number.
 * @param[in]   context                     The cbc context to use for encryption or decryption.
 * @param[in]   input_key                   The decryption key. must be 24bytes.
 * @param[in]   input_data                  The buffer holding the input data.
 * @param[in]   input_len                   The length of a data unit in bytes.
 *                                          This can be any length between 16 bytes and 2^31 bytes inclusive
 *                                          (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data                 The buffer holding the output data.
 *                                          The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_cbc192_hard_decrypt_dma(dmac_channel_number_t dma_receive_channel_num,
    cbc_context_t *context,
    uint8_t *input_data,
    size_t input_len,
    uint8_t *output_data);

/**
 * @brief       AES-CBC-192 encryption
 *
 * @param[in]   dma_receive_channel_num     Dmac receive channel number.
 * @param[in]   context                     The cbc context to use for encryption or decryption.
 * @param[in]   input_key                   The encryption key. must be 24bytes.
 * @param[in]   input_data                  The buffer holding the input data.
 * @param[in]   input_len                   The length of a data unit in bytes.
 *                                          This can be any length between 16 bytes and 2^31 bytes inclusive
 *                                          (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data                 The buffer holding the output data.
 *                                          The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_cbc192_hard_encrypt_dma(dmac_channel_number_t dma_receive_channel_num,
    cbc_context_t *context,
    uint8_t *input_data,
    size_t input_len,
    uint8_t *output_data);

/**
 * @brief       AES-CBC-256 decryption
 *
 * @param[in]   dma_receive_channel_num     Dmac receive channel number.
 * @param[in]   context                     The cbc context to use for encryption or decryption.
 * @param[in]   input_key                   The decryption key. must be 24bytes.
 * @param[in]   input_data                  The buffer holding the input data.
 * @param[in]   input_len                   The length of a data unit in bytes.
 *                                          This can be any length between 16 bytes and 2^31 bytes inclusive
 *                                          (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data                 The buffer holding the output data.
 *                                          The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_cbc256_hard_decrypt_dma(dmac_channel_number_t dma_receive_channel_num,
    cbc_context_t *context,
    uint8_t *input_data,
    size_t input_len,
    uint8_t *output_data);

/**
 * @brief       AES-CBC-256 encryption
 *
 * @param[in]   dma_receive_channel_num     Dmac receive channel number.
 * @param[in]   context                     The cbc context to use for encryption or decryption.
 * @param[in]   input_key                   The encryption key. must be 24bytes.
 * @param[in]   input_data                  The buffer holding the input data.
 * @param[in]   input_len                   The length of a data unit in bytes.
 *                                          This can be any length between 16 bytes and 2^31 bytes inclusive
 *                                          (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data                 The buffer holding the output data.
 *                                          The buffer size must be larger than the size after padding by 16 byte multiples.
 */
void aes_cbc256_hard_encrypt_dma(dmac_channel_number_t dma_receive_channel_num,
    cbc_context_t *context,
    uint8_t *input_data,
    size_t input_len,
    uint8_t *output_data);

/**
 * @brief       AES-GCM-128 decryption
 *
 * @param[in]   dma_receive_channel_num     Dmac receive channel number.
 * @param[in]   context                     The gcm context to use for encryption or decryption.
 * @param[in]   input_key                   The decryption key. must be 16bytes.
 * @param[in]   input_data                  The buffer holding the input data.
 * @param[in]   input_len                   The length of a data unit in bytes. Must be 4 byte multiples.
 *                                          This can be any length between 16 bytes and 2^31 bytes inclusive
 *                                          (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data                 The buffer holding the output data.
 * @param[out]  gcm_tag                     The buffer for holding the tag.The length of the tag must be 4 bytes.
 */
void aes_gcm128_hard_decrypt_dma(dmac_channel_number_t dma_receive_channel_num,
    gcm_context_t *context,
    uint8_t *input_data,
    size_t input_len,
    uint8_t *output_data,
    uint8_t *gcm_tag);

/**
 * @brief       AES-GCM-128 encryption
 *
 * @param[in]   dma_receive_channel_num     Dmac receive channel number.
 * @param[in]   context                     The gcm context to use for encryption or decryption.
 * @param[in]   input_key                   The encryption key. must be 16bytes.
 * @param[in]   input_data                  The buffer holding the input data.
 * @param[in]   input_len                   The length of a data unit in bytes. Must be 4 byte multiples.
 *                                          This can be any length between 16 bytes and 2^31 bytes inclusive
 *                                          (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data                 The buffer holding the output data.
 * @param[out]  gcm_tag                     The buffer for holding the tag.The length of the tag must be 4 bytes.
 */
void aes_gcm128_hard_encrypt_dma(dmac_channel_number_t dma_receive_channel_num,
    gcm_context_t *context,
    uint8_t *input_data,
    size_t input_len,
    uint8_t *output_data,
    uint8_t *gcm_tag);

/**
 * @brief       AES-GCM-192 decryption
 *
 * @param[in]   dma_receive_channel_num     Dmac receive channel number.
 * @param[in]   context                     The gcm context to use for encryption or decryption.
 * @param[in]   input_key                   The decryption key. must be 16bytes.
 * @param[in]   input_data                  The buffer holding the input data.
 * @param[in]   input_len                   The length of a data unit in bytes. Must be 4 byte multiples.
 *                                          This can be any length between 16 bytes and 2^31 bytes inclusive
 *                                          (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data                 The buffer holding the output data.
 * @param[out]  gcm_tag                     The buffer for holding the tag.The length of the tag must be 4 bytes.
 */
void aes_gcm192_hard_decrypt_dma(dmac_channel_number_t dma_receive_channel_num,
    gcm_context_t *context,
    uint8_t *input_data,
    size_t input_len,
    uint8_t *output_data,
    uint8_t *gcm_tag);

/**
 * @brief       AES-GCM-192 encryption
 *
 * @param[in]   dma_receive_channel_num     Dmac receive channel number.
 * @param[in]   context                     The gcm context to use for encryption or decryption.
 * @param[in]   input_key                   The encryption key. must be 16bytes.
 * @param[in]   input_data                  The buffer holding the input data.
 * @param[in]   input_len                   The length of a data unit in bytes. Must be 4 byte multiples.
 *                                          This can be any length between 16 bytes and 2^31 bytes inclusive
 *                                          (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data                 The buffer holding the output data.
 * @param[out]  gcm_tag                     The buffer for holding the tag.The length of the tag must be 4 bytes.
 */
void aes_gcm192_hard_encrypt_dma(dmac_channel_number_t dma_receive_channel_num,
    gcm_context_t *context,
    uint8_t *input_data,
    size_t input_len,
    uint8_t *output_data,
    uint8_t *gcm_tag);

/**
 * @brief       AES-GCM-256 decryption
 *
 * @param[in]   dma_receive_channel_num     Dmac receive channel number.
 * @param[in]   context                     The gcm context to use for encryption or decryption.
 * @param[in]   input_key                   The decryption key. must be 16bytes.
 * @param[in]   input_data                  The buffer holding the input data.
 * @param[in]   input_len                   The length of a data unit in bytes. Must be 4 byte multiples.
 *                                          This can be any length between 16 bytes and 2^31 bytes inclusive
 *                                          (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data                 The buffer holding the output data.
 * @param[out]  gcm_tag                     The buffer for holding the tag.The length of the tag must be 4 bytes.
 */
void aes_gcm256_hard_decrypt_dma(dmac_channel_number_t dma_receive_channel_num,
    gcm_context_t *context,
    uint8_t *input_data,
    size_t input_len,
    uint8_t *output_data,
    uint8_t *gcm_tag);

/**
 * @brief       AES-GCM-256 encryption
 *
 * @param[in]   dma_receive_channel_num     Dmac receive channel number.
 * @param[in]   context                     The gcm context to use for encryption or decryption.
 * @param[in]   input_key                   The encryption key. must be 16bytes.
 * @param[in]   input_data                  The buffer holding the input data.
 * @param[in]   input_len                   The length of a data unit in bytes. Must be 4 byte multiples.
 *                                          This can be any length between 16 bytes and 2^31 bytes inclusive
 *                                          (between 1 and 2^27 block cipher blocks).
 * @param[out]  output_data                 The buffer holding the output data.
 * @param[out]  gcm_tag                     The buffer for holding the tag.The length of the tag must be 4 bytes.
 */
void aes_gcm256_hard_encrypt_dma(dmac_channel_number_t dma_receive_channel_num,
    gcm_context_t *context,
    uint8_t *input_data,
    size_t input_len,
    uint8_t *output_data,
    uint8_t *gcm_tag);

/**
 * @brief       This function initializes the AES hard module.
 *
 * @param[in]   input_key       The buffer holding the encryption or decryption key.
 * @param[in]   input_key_len   The length of the input_key.must be 16bytes || 24bytes || 32bytes.
 * @param[in]   iv              The initialization vector.
 * @param[in]   iv_len          The length of the iv.GCM must be 12bytes. CBC must be 16bytes. ECB set 0L.
 * @param[in]   gcm_aad         The buffer holding the Additional authenticated data. or NULL
 * @param[in]   cipher_mode     Cipher Modes.must be AES_CBC || AES_ECB || AES_GCM.
 *                              Other cipher modes, please look forward to the next generation of kendryte.
 * @param[in]   encrypt_sel     The operation to perform:encryption or decryption.
 * @param[in]   gcm_aad_len     The length of the gcm_aad.
 * @param[in]   input_data_len  The length of the input_data.
 */
void aes_init(uint8_t *input_key, size_t input_key_len, uint8_t *iv,size_t iv_len, uint8_t *gcm_aad,
                aes_cipher_mode_t cipher_mode, aes_encrypt_sel_t encrypt_sel, size_t gcm_aad_len, size_t input_data_len);

/**
 * @brief       This function feeds an input buffer into an encryption or decryption operation.
 *
 * @param[in]   input_data      The buffer holding the input data.
 * @param[out]  output_data     The buffer holding the output data.
 * @param[in]   input_data_len  The length of the input_data.
 * @param[in]   cipher_mode     Cipher Modes.must be AES_CBC || AES_ECB || AES_GCM.
 *                              Other cipher modes, please look forward to the next generation of kendryte.
 */
void aes_process(uint8_t *input_data, uint8_t *output_data, size_t input_data_len, aes_cipher_mode_t cipher_mode);

/**
 * @brief       This function get the gcm tag to verify.
 *
 * @param[out]  gcm_tag         The buffer holding the gcm tag.The length of the tag must be 16bytes.
 */
void gcm_get_tag(uint8_t *gcm_tag);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_AES_H */
