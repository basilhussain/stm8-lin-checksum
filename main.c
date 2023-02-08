/*******************************************************************************
 *
 * main.c - LIN checksum calculation test suite
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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include "ucsim.h"
#include "lin_checksum.h"

#define CLK_CKDIVR (*(volatile uint8_t *)(0x50C6))

#define ANSI_BOLD "\x1B[1m"
#define ANSI_GREEN "\x1B[32m"
#define ANSI_RED "\x1B[31m"
#define ANSI_YELLOW "\x1B[33m"
#define ANSI_RESET "\x1B[0m"

typedef struct {
	uint16_t pass_count;
	uint16_t fail_count;
} test_result_t;

static const char pass_str[] = ANSI_BOLD ANSI_GREEN "PASS" ANSI_RESET;
static const char fail_str[] = ANSI_BOLD ANSI_RED "FAIL" ANSI_RESET;
static const char hrule_str[] = "----------------------------------------";

#define print_test_name() \
	do { \
		puts(hrule_str); \
		printf(ANSI_BOLD ANSI_YELLOW "%s" ANSI_RESET "\n", __func__); \
		puts(hrule_str); \
	} while(0)

#define print_test_num(n) \
	do { \
		printf(ANSI_YELLOW "TEST %02u" ANSI_RESET ":\n", (n) + 1); \
	} while(0)

#define print_pass_fail(x) \
	do { \
		puts((x) ? pass_str : fail_str); \
	} while(0)

#define count_test_result(x, r) \
	do { \
		if(x) { \
			(r)->pass_count++; \
		} else { \
			(r)->fail_count++; \
		} \
	} while(0)

/******************************************************************************/

static void print_hex_data(const uint8_t *data, const size_t len) {
	const size_t row_len = 16;
	bool end = false;

	for(size_t i = 0; i < len; i += row_len) {
		printf("0x%04X:", i);
		for(size_t j = 0; j < row_len; j++) {
			end = (i + j >= len);
			if(!end) {
				printf(" %02X", data[i + j]);
			} else {
				printf("   ");
			}
		}
		putchar(' ');
		putchar(' ');
		for(size_t j = 0; j < row_len; j++) {
			end = (i + j >= len);
			if(!end) {
				putchar(isprint(data[i + j]) ? data[i + j] : '.');
			} else {
				break;
			}
		}
		putchar('\n');
		if(end) break;
	}
}

static void test_calculate_classic(test_result_t *results) {
	static const struct {
		uint8_t data[8];
		uint8_t data_len;
		uint8_t expected_cksum;
	} tests[] = {
		{ { 0x00 }, 0, 0xFF }, // Zero-length data
		{ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 8, 0xFF },
		{ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, 8, 0x00 },
		{ { 0x91, 0xFA }, 2, 0x73 },
		{ { 0x4A, 0x55, 0x93, 0xE5 }, 4, 0xE6 }, // LIN Spec 2.2A example calculation (§ 2.8.3)
		{ { 0xA9, 0xD3, 0x76, 0x3D, 0x4F, 0xD9, 0xD3, 0x5B }, 8, 0x76 },
	};
	uint8_t cksum;
	bool pass;
	
	print_test_name();
	
	for(size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); i++) {
		print_test_num(i);
		printf("length = %u\n", tests[i].data_len);
		print_hex_data((const uint8_t *)&tests[i].data, tests[i].data_len);
		cksum = lin_calculate_checksum_classic(&tests[i].data, tests[i].data_len);
		pass = (cksum == tests[i].expected_cksum);
		printf("expected = 0x%02X, checksum = 0x%02X\n", tests[i].expected_cksum, cksum);
		print_pass_fail(pass);
		count_test_result(pass, results);
	}
}

static void test_calculate_enhanced(test_result_t *results) {
	static const struct {
		uint8_t pid;
		uint8_t data[8];
		uint8_t data_len;
		uint8_t expected_cksum;
	} tests[] = {
		{ 0xBF, { 0x00 }, 0, 0x40 }, // Zero-length data
		{ 0xBF, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 8, 0x40 },
		{ 0xBF, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, 8, 0x40 },
		{ 0xBF, { 0x91, 0xFA }, 2, 0xB3 },
		{ 0xBF, { 0x4A, 0x55, 0x93, 0xE5 }, 4, 0x27 }, // LIN Spec 2.2A example calculation (§ 2.8.3)
		{ 0xBF, { 0xA9, 0xD3, 0x76, 0x3D, 0x4F, 0xD9, 0xD3, 0x5B }, 8, 0xB6 },
	};
	uint8_t cksum;
	bool pass;
	
	print_test_name();
	
	for(size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); i++) {
		print_test_num(i);
		printf("pid = 0x%02X, length = %u\n", tests[i].pid, tests[i].data_len);
		print_hex_data((const uint8_t *)&tests[i].data, tests[i].data_len);
		cksum = lin_calculate_checksum_enhanced(tests[i].pid, &tests[i].data, tests[i].data_len);
		pass = (cksum == tests[i].expected_cksum);
		printf("expected = 0x%02X, checksum = 0x%02X\n", tests[i].expected_cksum, cksum);
		print_pass_fail(pass);
		count_test_result(pass, results);
	}
}

static void test_verify_classic(test_result_t *results) {
	static const struct {
		uint8_t data[8];
		uint8_t data_len;
		uint8_t cksum;
		bool expected_result;
	} tests[] = {
		{ { 0x00 }, 0, 0xFF, true }, // Zero-length data
		{ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 8, 0xFF, true },
		{ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, 8, 0x00, true },
		{ { 0x91, 0xFA }, 2, 0x73, true },
		{ { 0x4A, 0x55, 0x93, 0xE5 }, 4, 0xE6, true }, // LIN Spec 2.2A example calculation (§ 2.8.3)
		{ { 0xA9, 0xD3, 0x76, 0x3D, 0x4F, 0xD9, 0xD3, 0x5B }, 8, 0x76, true },
		{ { 0x00 }, 0, 0xF0, false }, // Zero-length data
		{ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 8, 0x12, false },
		{ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, 8, 0x34, false },
		{ { 0x91, 0xFA }, 2, 0x42, false },
		{ { 0x4A, 0x55, 0x93, 0xE5 }, 4, 0x55, false },
		{ { 0xA9, 0xD3, 0x76, 0x3D, 0x4F, 0xD9, 0xD3, 0x5B }, 8, 0xAA, false },
	};
	bool result, pass;
	
	print_test_name();
	
	for(size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); i++) {
		print_test_num(i);
		printf("length = %u, checksum = 0x%02X\n", tests[i].data_len, tests[i].cksum);
		print_hex_data((const uint8_t *)&tests[i].data, tests[i].data_len);
		result = lin_verify_checksum_classic(tests[i].cksum, &tests[i].data, tests[i].data_len);
		pass = (result == tests[i].expected_result);
		printf("expected = %u, result = %u\n", tests[i].expected_result, result);
		print_pass_fail(pass);
		count_test_result(pass, results);
	}
}

static void test_verify_enhanced(test_result_t *results) {
	static const struct {
		uint8_t pid;
		uint8_t data[8];
		uint8_t data_len;
		uint8_t cksum;
		bool expected_result;
	} tests[] = {
		{ 0xBF, { 0x00 }, 0, 0x40, true }, // Zero-length data
		{ 0xBF, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 8, 0x40, true },
		{ 0xBF, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, 8, 0x40, true },
		{ 0xBF, { 0x91, 0xFA }, 2, 0xB3, true },
		{ 0xBF, { 0x4A, 0x55, 0x93, 0xE5 }, 4, 0x27, true }, // LIN Spec 2.2A example calculation (§ 2.8.3)
		{ 0xBF, { 0xA9, 0xD3, 0x76, 0x3D, 0x4F, 0xD9, 0xD3, 0x5B }, 8, 0xB6, true },
		{ 0xBF, { 0x00 }, 0, 0xF0, false }, // Zero-length data
		{ 0xBF, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 8, 0x12, false },
		{ 0xBF, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, 8, 0x34, false },
		{ 0xBF, { 0x91, 0xFA }, 2, 0x42, false },
		{ 0xBF, { 0x4A, 0x55, 0x93, 0xE5 }, 4, 0x55, false },
		{ 0xBF, { 0xA9, 0xD3, 0x76, 0x3D, 0x4F, 0xD9, 0xD3, 0x5B }, 8, 0xAA, false },
	};
	bool result, pass;
	
	print_test_name();
	
	for(size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); i++) {
		print_test_num(i);
		printf("pid = 0x%02X, length = %u, checksum = 0x%02X\n", tests[i].pid, tests[i].data_len, tests[i].cksum);
		print_hex_data((const uint8_t *)&tests[i].data, tests[i].data_len);
		result = lin_verify_checksum_enhanced(tests[i].cksum, tests[i].pid, &tests[i].data, tests[i].data_len);
		pass = (result == tests[i].expected_result);
		printf("expected = %u, result = %u\n", tests[i].expected_result, result);
		print_pass_fail(pass);
		count_test_result(pass, results);
	}
}

static void test_protected_id(test_result_t *results) {
	static const struct {
		uint8_t fid;
		uint8_t expected_pid;
	} tests[] = {
		{ 0x00, 0x80 },
		{ 0x3F, 0xBF },
		{ 0x01, 0xC1 },
		{ 0x10, 0x50 },
		{ 0x28, 0xA8 },
		{ 0x1F, 0x1F },
		{ 0x08, 0x08 },
		// Illegal frame IDs, should be truncated to 6 bits:
		{ 0x40, 0x80 },
		{ 0xFF, 0xBF },
	};
	uint8_t pid;
	bool pass;
	
	print_test_name();
	
	for(size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); i++) {
		print_test_num(i);
		printf("fid = 0x%02X\n", tests[i].fid);
		pid = lin_get_protected_id(tests[i].fid);
		pass = (pid == tests[i].expected_pid);
		printf("expected = 0x%02X, pid = 0x%02X\n", tests[i].expected_pid, pid);
		print_pass_fail(pass);
		count_test_result(pass, results);
	}
}

void main(void) {
	test_result_t results = { 0, 0 };

	CLK_CKDIVR = 0;

	test_calculate_classic(&results);
	test_calculate_enhanced(&results);
	test_verify_classic(&results);
	test_verify_enhanced(&results);
	test_protected_id(&results);

	puts(hrule_str);

	printf("TOTAL RESULTS: passed = %u, failed = %u\n", results.pass_count, results.fail_count);
	
	ucsim_if_stop();
}

int putchar(int c) {
	return ucsim_if_putchar(c);
}
