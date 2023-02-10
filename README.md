# Overview

This is a library for the STM8 microcontroller and [SDCC](http://sdcc.sourceforge.net/) compiler providing routines for calculating and verifying [LIN bus](https://en.wikipedia.org/wiki/Local_Interconnect_Network) checksums, as well as calculating and verifying parity bits of protected frame IDs.

Checksum calculation routines for both 'classic' (according to LIN specification version 1.x) and 'enhanced' (specification version 2.x) checksums are supported.

In addition to the library, a test suite program is also included.

# Requirements

Requires SDCC version 4.1.10 or greater. The inline assembly code present in this library is written for STM8 calling convention version 1 only, which was introduced and became the default in SDCC version 4.1.10.

# Building

Run `make` in the code's root folder. Some arguments may be required; see below. The output `.lib` file is placed in the `lib` folder.

By default, without specifying any further options to `make`, the library compiles for the medium memory model.

To compile for the large memory model (i.e. SDCC's `--model-large` option), give an additional argument of `MODEL=large` to `make`. If your code that you will be linking with is compiled using `--model-large` (typically the case for STM8 devices with 32 kB or more of flash), then you will need to build this library as such too.

# Usage

1. Include the `lin_checksum.h` file in your C code wherever you want to use the library functions.
2. When linking, provide the path to the `.lib` file with the `-l` SDCC command-line option.

## Function Reference

### `uint8_t lin_calculate_checksum_classic(const void *data, const uint8_t data_len)`

Calculates a 'classic' checksum from the given data. Takes a pointer `data` to a buffer of data bytes, from which `data_len` bytes will be read when calculating the checksum. Returns the checksum value.

### `uint8_t lin_calculate_checksum_enhanced(const uint8_t pid, const void *data, const uint8_t data_len)`

Calculates an 'enhanced' checksum from the given data and protected identifier. Takes a protected ID value `pid`, as well as a pointer `data` to a buffer of data bytes, from which `data_len` bytes will be read when calculating the checksum. Returns the checksum value.

### `bool lin_verify_checksum_classic(const uint8_t cksum, const void *data, const uint8_t data_len)`

Verifies that a 'classic' checksum matches the given data. Takes a pointer `data` to a buffer of data bytes, from which `data_len` bytes will be read, and a new checksum value calculated and compared to the given `cksum` value. Returns a boolean value indicating whether `cksum` matched.

### `bool lin_verify_checksum_enhanced(const uint8_t cksum, const uint8_t pid, const void *data, const uint8_t data_len)`

Verifies that an 'enhanced' checksum matches the given data and protected ID. Takes a protected ID value `pid`, as well as a pointer `data` to a buffer of data bytes, from which `data_len` bytes will be read, and a new checksum value calculated and compared to the given `cksum` value. Returns a boolean value indicating whether `cksum` matched.

### `uint8_t lin_get_protected_id(const uint8_t fid)`

Constructs a protected identifier value from the given frame identifier `fid` by calculating the two necessary parity bits and appending them as the most-significant bits to the frame ID. Any `fid` value greater than 63 (0x3F) will be wrapped at that value (e.g. 65 → 1). Returns the protected ID value.

### `bool lin_verify_protected_id(const uint8_t pid, uint8_t *fid_out)`

Verifies that the parity bits of the given protected ID are correct for its encapsulated frame ID. Takes as input a protected ID value `pid`, and outputs a frame ID via the pointer `fid_out`. The `fid_out` argument is non-optional and a valid pointer must always be provided. The frame ID is also output regardless of parity correctness. Returns a boolean value indicating whether the parity is correct.

## Notes

* The checksum calculation and verification functions do not impose or respect any LIN frame length limitations on the data buffer they read from. For example, if your data resides in a buffer of size 20, but you wish to calculate a checksum for a LIN frame carrying 8 data bytes, then you should pass a length of 8. Similarly, if your buffer is of size 8, but the frame you'll be sending is 4 data bytes, pass a length of 4.
* Use of the `lin_verify_protected_id` function is not typically needed because STM8 UARTs that are LIN-capable already incorporate protected ID parity error checking in hardware. However, the function is included in the library not only for completeness, but also for the scenario where a UART is being used that does not possess LIN features.

## Example

```c
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "lin_checksum.h"

void send() {
	uint8_t frame_id = 0x3F;
	uint8_t frame_data[8] = { 0x4A, 0x55, 0x93, 0xE5 };
	
	uint8_t protected_id = lin_get_protected_id(frame_id);
	uint8_t checksum = lin_calculate_checksum_enhanced(protected_id, frame_data, 4);
	
	/* Send frame using protected_id, frame_data, and checksum... */
}

void receive() {
	uint8_t frame_pid; /* received protected ID */
	uint8_t frame_data[8]; /* received data */
	uint8_t frame_length; /* number of data bytes in frame */
	uint8_t frame_checksum; /* received checksum */
	
	bool verified_ok = lin_verify_checksum_enhanced(frame_checksum, frame_pid, frame_data, frame_length);
	
	if(verified_ok) {
		/* Process frame_data, etc... */
	} else {
		/* Error handling... */
	}
}
```

# Test Program

A test suite program, `main.c`, is included in the source repository. It is designed to be run with the [μCsim](http://mazsola.iit.uni-miskolc.hu/~drdani/embedded/ucsim/) microcontroller simulator included with SDCC.

The program will run tests using a variety of input data and validate the correct operation of all library functions. A string of "PASS" or "FAIL" is given for each test case. At the conclusion of all tests, total pass/fail counts will be output.

To build the test program, run `make test`. If you previously built the library with the `MODEL=large` argument, pass it here too, or run `make clean` before running `make test`.

To then run the test program in the simulator, run `make sim`.

# Licence

This library is licenced under the MIT Licence. Please see file LICENSE.txt for full licence text.
