/*******************************************************************************
 *
 * lin_checksum.h - LIN checksum calculation routines header
 *
 * Copyright (c) 2023 Basil Hussain
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 ******************************************************************************/

#ifndef LIN_CHECKSUM_H__
#define LIN_CHECKSUM_H__

#include <stdint.h>
#include <stdbool.h>

extern uint8_t lin_calculate_checksum_classic(const void *data, const uint8_t data_len);
extern uint8_t lin_calculate_checksum_enhanced(const uint8_t pid, const void *data, const uint8_t data_len);
extern bool lin_verify_checksum_classic(const uint8_t cksum, const void *data, const uint8_t data_len);
extern bool lin_verify_checksum_enhanced(const uint8_t cksum, const uint8_t pid, const void *data, const uint8_t data_len);
extern uint8_t lin_get_protected_id(const uint8_t fid);

#endif // LIN_CHECKSUM_H__
