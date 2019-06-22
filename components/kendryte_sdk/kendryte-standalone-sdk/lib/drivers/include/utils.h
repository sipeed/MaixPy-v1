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
#ifndef _DRIVER_UTILS_H
#define _DRIVER_UTILS_H

#ifdef __cplusplus
#include <cstdbool>
#include <cstddef>
#include <cstdint>
#else /* __cplusplus */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define KENDRYTE_MIN(a, b) ((a) > (b) ? (b) : (a))
#define KENDRYTE_MAX(a, b) ((a) > (b) ? (a) : (b))

#ifdef __ASSEMBLY__
#define KENDRYTE_CAST(type, ptr) ptr
#else /* __ASSEMBLY__ */
/**
 * @brief       Cast the pointer to specified pointer type.
 *
 * @param[in]   type        The pointer type to cast to
 * @param[in]   ptr         The pointer to apply the type cast to
 */
#define KENDRYTE_CAST(type, ptr) ((type)(ptr))
#endif /* __ASSEMBLY__ */

/**
 * @addtogroup      UTIL_RW_FUNC Memory Read/Write Utilities
 *
 * This section implements read and write functionality for various
 * memory untis. The memory unit terms used for these functions are
 * consistent with those used in the ARM Architecture Reference Manual
 * ARMv7-A and ARMv7-R edition manual. The terms used for units of memory are:
 *
 *  Unit of Memory | Abbreviation | Size in Bits
 * :---------------|:-------------|:------------:
 *  Byte           | byte         |       8
 *  Half Word      | hword        |      16
 *  Word           | word         |      32
 *  Double Word    | dword        |      64
 *
 */

/**
 * @brief       Write the 8 bit byte to the destination address in device memory.
 *
 * @param[in]   dest        Write destination pointer address
 * @param[in]   src         8 bit data byte to write to memory
 */
#define kendryte_write_byte(dest, src) \
    (*KENDRYTE_CAST(volatile uint8_t*, (dest)) = (src))

/**
 * @brief       Read and return the 8 bit byte from the source address in device memory.
 *
 * @param[in]   src     Read source pointer address
 *
 * @return      8 bit data byte value
 */
#define kendryte_read_byte(src) (*KENDRYTE_CAST(volatile uint8_t*, (src)))

/**
 * @brief       Write the 16 bit half word to the destination address in device memory.
 *
 * @param[in]   dest        Write destination pointer address
 * @param[in]   src         16 bit data half word to write to memory
 */
#define kendryte_write_hword(dest, src) \
    (*KENDRYTE_CAST(volatile uint16_t*, (dest)) = (src))

/**
 * @brief       Read and return the 16 bit half word from the source address in device
 *
 * @param[in]   src     Read source pointer address
 *
 * @return      16 bit data half word value
 */
#define kendryte_read_hword(src) (*KENDRYTE_CAST(volatile uint16_t*, (src)))

/**
 * @brief       Write the 32 bit word to the destination address in device memory.
 *
 * @param[in]   dest        Write destination pointer address
 * @param[in]   src         32 bit data word to write to memory
 */
#define kendryte_write_word(dest, src) \
    (*KENDRYTE_CAST(volatile uint32_t*, (dest)) = (src))

/**
 * @brief       Read and return the 32 bit word from the source address in device memory.
 *
 * @param[in]   src     Read source pointer address
 *
 * @return      32 bit data half word value
 */
#define kendryte_read_word(src) (*KENDRYTE_CAST(volatile uint32_t*, (src)))

/**
 * @brief       Write the 64 bit double word to the destination address in device memory.
 *
 * @param[in]   dest        Write destination pointer address
 * @param[in]   src         64 bit data word to write to memory
 */
#define kendryte_write_dword(dest, src) \
    (*KENDRYTE_CAST(volatile uint64_t*, (dest)) = (src))

/**
 * @brief       Read and return the 64 bit double word from the source address in device
 *
 * @param[in]   src     Read source pointer address
 *
 * @return      64 bit data half word value
 */
#define kendryte_read_dword(src) (*KENDRYTE_CAST(volatile uint64_t*, (src)))

/**
 * @brief       Set selected bits in the 8 bit byte at the destination address in device
 *
 * @param[in]   dest        Destination pointer address
 * @param[in]   bits        Bits to set in destination byte
 */
#define kendryte_setbits_byte(dest, bits) \
    (kendryte_write_byte(dest, kendryte_read_byte(dest) | (bits)))

/**
 * @brief       Clear selected bits in the 8 bit byte at the destination address in device
 *
 * @param[in]   dest        Destination pointer address
 * @param[in]   bits        Bits to clear in destination byte
 */
#define kendryte_clrbits_byte(dest, bits) \
    (kendryte_write_byte(dest, kendryte_read_byte(dest) & ~(bits)))

/**
 * @brief       Change or toggle selected bits in the 8 bit byte at the destination address
 *
 * @param[in]   dest        Destination pointer address
 * @param[in]   bits        Bits to change in destination byte
 */
#define kendryte_xorbits_byte(dest, bits) \
    (kendryte_write_byte(dest, kendryte_read_byte(dest) ^ (bits)))

/**
 * @brief       Replace selected bits in the 8 bit byte at the destination address in device
 *
 * @param[in]   dest        Destination pointer address
 * @param[in]   msk         Bits to replace in destination byte
 * @param[in]   src         Source bits to write to cleared bits in destination byte
 */
#define kendryte_replbits_byte(dest, msk, src) \
    (kendryte_write_byte(dest, (kendryte_read_byte(dest) & ~(msk)) | ((src) & (msk))))

/**
 * @brief       Set selected bits in the 16 bit halfword at the destination address in
 *
 * @param[in]   dest        Destination pointer address
 * @param[in]   bits        Bits to set in destination halfword
 */
#define kendryte_setbits_hword(dest, bits) \
    (kendryte_write_hword(dest, kendryte_read_hword(dest) | (bits)))

/**
 * @brief       Clear selected bits in the 16 bit halfword at the destination address in
 *
 * @param[in]   dest        Destination pointer address
 * @param[in]   bits        Bits to clear in destination halfword
 */
#define kendryte_clrbits_hword(dest, bits) \
    (kendryte_write_hword(dest, kendryte_read_hword(dest) & ~(bits)))

/**
 * @brief       Change or toggle selected bits in the 16 bit halfword at the destination
 *
 * @param[in]   dest        Destination pointer address
 * @param[in]   bits        Bits to change in destination halfword
 */
#define kendryte_xorbits_hword(dest, bits) \
    (kendryte_write_hword(dest, kendryte_read_hword(dest) ^ (bits)))

/**
 * @brief       Replace selected bits in the 16 bit halfword at the destination address in
 *
 * @param[in]   dest        Destination pointer address
 * @param[in]   msk         Bits to replace in destination byte
 * @param[in]   src         Source bits to write to cleared bits in destination halfword
 */
#define kendryte_replbits_hword(dest, msk, src) \
    (kendryte_write_hword(dest, (kendryte_read_hword(dest) & ~(msk)) | ((src) & (msk))))

/**
 * @brief       Set selected bits in the 32 bit word at the destination address in device
 *
 * @param[in]   dest        Destination pointer address
 * @param[in]   bits        Bits to set in destination word
 */
#define kendryte_setbits_word(dest, bits) \
    (kendryte_write_word(dest, kendryte_read_word(dest) | (bits)))

/**
 * @brief       Clear selected bits in the 32 bit word at the destination address in device
 *
 * @param[in]   dest        Destination pointer address
 * @param[in]   bits        Bits to clear in destination word
 */
#define kendryte_clrbits_word(dest, bits) \
    (kendryte_write_word(dest, kendryte_read_word(dest) & ~(bits)))

/**
 * @brief       Change or toggle selected bits in the 32 bit word at the destination address
 *
 * @param[in]   dest        Destination pointer address
 * @param[in]   bits        Bits to change in destination word
 */
#define kendryte_xorbits_word(dest, bits) \
    (kendryte_write_word(dest, kendryte_read_word(dest) ^ (bits)))

/**
 * @brief       Replace selected bits in the 32 bit word at the destination address in
 *
 * @param[in]   dest        Destination pointer address
 * @param[in]   msk         Bits to replace in destination word
 * @param[in]   src         Source bits to write to cleared bits in destination word
 */
#define kendryte_replbits_word(dest, msk, src) \
    (kendryte_write_word(dest, (kendryte_read_word(dest) & ~(msk)) | ((src) & (msk))))

/**
 * @brief      Set selected bits in the 64 bit doubleword at the destination address in
 *
 * @param[in]   dest     Destination pointer address
 * @param[in]   bits     Bits to set in destination doubleword
 */
#define kendryte_setbits_dword(dest, bits) \
    (kendryte_write_dword(dest, kendryte_read_dword(dest) | (bits)))

/**
 * @brief       Clear selected bits in the 64 bit doubleword at the destination address in
 *
 * @param[in]   dest        Destination pointer address
 * @param[in]   bits        Bits to clear in destination doubleword
 */
#define kendryte_clrbits_dword(dest, bits) \
    (kendryte_write_dword(dest, kendryte_read_dword(dest) & ~(bits)))

/**
 * @brief       Change or toggle selected bits in the 64 bit doubleword at the destination
 *
 * @param[in]   dest        Destination pointer address
 * @param[in]   bits        Bits to change in destination doubleword
 */
#define kendryte_xorbits_dword(dest, bits) \
    (kendryte_write_dword(dest, kendryte_read_dword(dest) ^ (bits)))

/**
 * @brief       Replace selected bits in the 64 bit doubleword at the destination address in
 *
 * @param[in]   dest        Destination pointer address
 * @param[in]   msk         its to replace in destination doubleword
 * @param[in]   src         Source bits to write to cleared bits in destination word
 */
#define kendryte_replbits_dword(dest, msk, src) \
    (kendryte_write_dword(dest, (kendryte_read_dword(dest) & ~(msk)) | ((src) & (msk))))

#define configASSERT(x)                                 \
    if ((x) == 0)                                       \
    {                                                   \
        printf("(%s:%d) %s\r\n", __FILE__, __LINE__, #x); \
        for (;;)                                        \
            ;                                           \
    }

/**
 * @brief       Set value by mask
 *
 * @param[in]   bits        The one be set
 * @param[in]   mask        mask value
 * @param[in]   value       The value to set
 */
void set_bit(volatile uint32_t *bits, uint32_t mask, uint32_t value);

/**
 * @brief       Set value by mask
 *
 * @param[in]   bits        The one be set
 * @param[in]   mask        Mask value
 * @param[in]   offset      Mask's offset
 * @param[in]   value       The value to set
 */
void set_bit_offset(volatile uint32_t *bits, uint32_t mask, size_t offset, uint32_t value);

/**
 * @brief       Set bit for gpio, only set one bit
 *
 * @param[in]   bits        The one be set
 * @param[in]   idx         Offset value
 * @param[in]   value       The value to set
 */
void set_gpio_bit(volatile uint32_t *bits, size_t idx, uint32_t value);

/**
 * @brief      Get bits value of mask
 *
 * @param[in]   bits        The source data
 * @param[in]   mask        Mask value
 * @param[in]   offset      Mask's offset
 *
 * @return      The bits value of mask
 */
uint32_t get_bit(volatile uint32_t *bits, uint32_t mask, size_t offset);

/**
 * @brief       Get a bit value by offset
 *
 * @param[in]   bits        The source data
 * @param[in]   offset      Bit's offset
 *
 *
 * @return      The bit value
 */
uint32_t get_gpio_bit(volatile uint32_t *bits, size_t offset);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _DRIVER_COMMON_H */

