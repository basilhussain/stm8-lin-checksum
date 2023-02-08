/*******************************************************************************
 *
 * lin_checksum.c - LIN checksum calculation routines
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

#include <stdint.h>
#include <stdbool.h>
#include "lin_checksum.h"

#if !defined(__SDCCCALL) || __SDCCCALL != 1
#error "SDCC calling convention other than 1 not supported"
#endif

#ifdef __SDCC_MODEL_LARGE
#define ASM_SP_ARGS_OFFSET 3
#define ASM_RETURN retf
#else
#define ASM_CALLEE_CLEANUP
#define ASM_SP_ARGS_OFFSET 2
#define ASM_RETURN ret
#endif

static uint8_t lin_calculate_checksum_intermediate(uint8_t cksum_init, const void *data, uint8_t data_len) __naked {
	(void)cksum_init; // a
	(void)data; // x
	(void)data_len; // stack
	
	// The checksum calculation is done with assembly here because that makes it
	// trivial to handle the addition of carry resulting from any overflow.
	
	__asm
		; Offsets and sizes for all stack-held arguments.
		DATA_LEN_SP_OFFSET = ASM_SP_ARGS_OFFSET + 1
		DATA_LEN_SIZE = 1
	
		; Given initial value for checksum is already in A reg.
		
		; Bail out early if data length is zero.
		tnz (DATA_LEN_SP_OFFSET, sp)
		jreq 0002$
		
		; Ensure carry is zero before we begin.
		rcf
		
	0001$:
		; Add next data byte to checksum, including carry from any overflow from
		; previous addition. Increment the data pointer.
		adc a, (x)
		incw x
		
		; Decrement data length. Loop around if not yet zero.
		dec (DATA_LEN_SP_OFFSET, sp)
		jrne 0001$
		
		; There might be leftover carry from the final addition, so add it too.
		adc a, #0
		
	0002$:
		; Return value is un-inverted checksum in A reg.
		
#ifdef ASM_CALLEE_CLEANUP
		; Callee must adjust stack on medium memory model where return value is
		; 16 bits or smaller (or void). So we must discard stack args and return
		; a different way.
		ldw x, (1, sp)
		addw sp, #(DATA_LEN_SIZE + ASM_SP_ARGS_OFFSET)
		jp (x)
#else
		ASM_RETURN
#endif
	__endasm;
}

uint8_t lin_calculate_checksum_classic(const void *data, const uint8_t data_len) {
	return ~lin_calculate_checksum_intermediate(0, data, data_len);
}

uint8_t lin_calculate_checksum_enhanced(const uint8_t pid, const void *data, const uint8_t data_len) {
	return ~lin_calculate_checksum_intermediate(pid, data, data_len);
}

bool lin_verify_checksum_classic(const uint8_t cksum, const void *data, const uint8_t data_len) {
	return (cksum + lin_calculate_checksum_intermediate(0, data, data_len) == 0xFF);
}

bool lin_verify_checksum_enhanced(const uint8_t cksum, const uint8_t pid, const void *data, const uint8_t data_len) {
	return (cksum + lin_calculate_checksum_intermediate(pid, data, data_len) == 0xFF);
}

uint8_t lin_get_protected_id(const uint8_t fid) {
	// This is actually smaller than the code to do the calculation
	// (73 versus 83 bytes).
	static const uint8_t lut[64] = {
		0x80, 0xC1, 0x42, 0x03, 0xC4, 0x85, 0x06, 0x47,
		0x08, 0x49, 0xCA, 0x8B, 0x4C, 0x0D, 0x8E, 0xCF,
		0x50, 0x11, 0x92, 0xD3, 0x14, 0x55, 0xD6, 0x97,
		0xD8, 0x99, 0x1A, 0x5B, 0x9C, 0xDD, 0x5E, 0x1F,
		0x20, 0x61, 0xE2, 0xA3, 0x64, 0x25, 0xA6, 0xE7,
		0xA8, 0xE9, 0x6A, 0x2B, 0xEC, 0xAD, 0x2E, 0x6F,
		0xF0, 0xB1, 0x32, 0x73, 0xB4, 0xF5, 0x76, 0x37,
		0x78, 0x39, 0xBA, 0xFB, 0x3C, 0x7D, 0xFE, 0xBF,
	};
	
	return lut[fid & 0x3F];
}
