#ifndef __BITVAL_H
#define __BITVAL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Bitval provides utilities to read:
// 	no more than 4 bytes integer value;
// 	C string value.

// For all functions that take 'bitval_t bv' parameter, assert(bv != NULL).
// For all read/peek bits functions, error will raise when n > 4*CHAR_BIT.
// For all read/peek/skip functions, error will raise when there is no enough data.
// For all read/peek/skip bytes functions, error will raise when bitval_synced() is false.

typedef int8_t byte1_t;
typedef int16_t byte2_t;
typedef int32_t byte3_t;
typedef int32_t byte4_t;

typedef uint8_t ubyte1_t;
typedef uint16_t ubyte2_t;
typedef uint32_t ubyte3_t;
typedef uint32_t ubyte4_t;

// Change [u]byte_fastN_t to capably fastest integer type.
typedef int_fast32_t byte_fast1_t;
typedef int_fast32_t byte_fast2_t;
typedef int_fast32_t byte_fast3_t;
typedef int_fast32_t byte_fast4_t;

typedef uint_fast32_t ubyte_fast1_t;
typedef uint_fast32_t ubyte_fast2_t;
typedef uint_fast32_t ubyte_fast3_t;
typedef uint_fast32_t ubyte_fast4_t;

// At least 4-bytes width.
typedef byte_fast4_t intbv_t;
typedef ubyte_fast4_t uintbv_t;

typedef struct {
	char _ [10*sizeof(void *)];
} bitval_t[1];

static inline void
bitval_copy(bitval_t bv, const bitval_t src) {
	bv[0] = src[0];
}

typedef void (*BitvalErrorFunc_t)(const char *);

void bitval_init(bitval_t bv, const ubyte1_t *data, size_t size, BitvalErrorFunc_t err);
const ubyte1_t *bitval_getptr(const bitval_t bv);

void bitval_sync(bitval_t bv);
bool bitval_synced(const bitval_t bv);

void bitval_skip_bits(bitval_t bv, size_t n);
void bitval_skip_bytes(bitval_t bv, size_t n);

size_t bitval_number_bits(const bitval_t bv);
size_t bitval_number_bytes(const bitval_t bv);

bool bitval_ensure_bits(const bitval_t bv, size_t n);
bool bitval_ensure_bytes(const bitval_t bv, size_t n);

const char *bitval_read_string(bitval_t bv, size_t *lenp);
const char *bitval_peek_string(const bitval_t bv, size_t *lenp);
void bitval_skip_string(bitval_t bv);

int bitval_read_bit(bitval_t bv);
intbv_t bitval_read_sbits(bitval_t bv, size_t n);
uintbv_t bitval_read_ubits(bitval_t bv, size_t n);

byte_fast1_t bitval_read_byte1(bitval_t bv);
byte_fast2_t bitval_read_byte2(bitval_t bv);
byte_fast3_t bitval_read_byte3(bitval_t bv);
byte_fast4_t bitval_read_byte4(bitval_t bv);
ubyte_fast1_t bitval_read_ubyte1(bitval_t bv);
ubyte_fast2_t bitval_read_ubyte2(bitval_t bv);
ubyte_fast3_t bitval_read_ubyte3(bitval_t bv);
ubyte_fast4_t bitval_read_ubyte4(bitval_t bv);

byte_fast1_t bitval_read_bigendian_byte1(bitval_t bv);
byte_fast2_t bitval_read_bigendian_byte2(bitval_t bv);
byte_fast3_t bitval_read_bigendian_byte3(bitval_t bv);
byte_fast4_t bitval_read_bigendian_byte4(bitval_t bv);
ubyte_fast1_t bitval_read_bigendian_ubyte1(bitval_t bv);
ubyte_fast2_t bitval_read_bigendian_ubyte2(bitval_t bv);
ubyte_fast3_t bitval_read_bigendian_ubyte3(bitval_t bv);
ubyte_fast4_t bitval_read_bigendian_ubyte4(bitval_t bv);


int bitval_peek_bit(const bitval_t bv);
intbv_t bitval_peek_sbits(const bitval_t bv, size_t n);
uintbv_t bitval_peek_ubits(const bitval_t bv, size_t n);

byte_fast1_t bitval_peek_byte1(const bitval_t bv);
byte_fast2_t bitval_peek_byte2(const bitval_t bv);
byte_fast3_t bitval_peek_byte3(const bitval_t bv);
byte_fast4_t bitval_peek_byte4(const bitval_t bv);
ubyte_fast1_t bitval_peek_ubyte1(const bitval_t bv);
ubyte_fast2_t bitval_peek_ubyte2(const bitval_t bv);
ubyte_fast3_t bitval_peek_ubyte3(const bitval_t bv);
ubyte_fast4_t bitval_peek_ubyte4(const bitval_t bv);

byte_fast1_t bitval_peek_bigendian_byte1(const bitval_t bv);
byte_fast2_t bitval_peek_bigendian_byte2(const bitval_t bv);
byte_fast3_t bitval_peek_bigendian_byte3(const bitval_t bv);
byte_fast4_t bitval_peek_bigendian_byte4(const bitval_t bv);
ubyte_fast1_t bitval_peek_bigendian_ubyte1(const bitval_t bv);
ubyte_fast2_t bitval_peek_bigendian_ubyte2(const bitval_t bv);
ubyte_fast3_t bitval_peek_bigendian_ubyte3(const bitval_t bv);
ubyte_fast4_t bitval_peek_bigendian_ubyte4(const bitval_t bv);
#endif
