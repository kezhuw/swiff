#ifndef __BITVAL_H
#define __BITVAL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Bitval provides utilities to read:
// 	no more than 4 bytes integer value;
// 	C string value.

// For bitval_init function, caller must provide 'BitvalErrorFunc_t err' parameter.
// For all functions that take 'struct bitval *b' parameter, assert(b != NULL).
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

struct bitval;
size_t bitval_size(void);

typedef void (*BitvalErrorFunc_t)(const char *);
struct bitval *bitval_init(struct bitval *b, const ubyte1_t *data, size_t size, BitvalErrorFunc_t err);
struct bitval *bitval_copy(struct bitval *b, const struct bitval *src);
const ubyte1_t *bitval_getptr(const struct bitval *b);

void bitval_sync(struct bitval *b);
bool bitval_synced(const struct bitval *b);

void bitval_skip_bits(struct bitval *b, size_t n);
void bitval_skip_bytes(struct bitval *b, size_t n);

size_t bitval_number_bits(const struct bitval *b);
size_t bitval_number_bytes(const struct bitval *b);

bool bitval_ensure_bits(const struct bitval *b, size_t n);
bool bitval_ensure_bytes(const struct bitval *b, size_t n);

const char *bitval_read_string(struct bitval *b, size_t *lenp);
const char *bitval_peek_string(const struct bitval *b, size_t *lenp);
void bitval_skip_string(struct bitval *b);

int bitval_read_bit(struct bitval *b);
intbv_t bitval_read_sbits(struct bitval *b, size_t n);
uintbv_t bitval_read_ubits(struct bitval *b, size_t n);

byte_fast1_t bitval_read_byte1(struct bitval *b);
byte_fast2_t bitval_read_byte2(struct bitval *b);
byte_fast3_t bitval_read_byte3(struct bitval *b);
byte_fast4_t bitval_read_byte4(struct bitval *b);
ubyte_fast1_t bitval_read_ubyte1(struct bitval *b);
ubyte_fast2_t bitval_read_ubyte2(struct bitval *b);
ubyte_fast3_t bitval_read_ubyte3(struct bitval *b);
ubyte_fast4_t bitval_read_ubyte4(struct bitval *b);

byte_fast1_t bitval_read_bigendian_byte1(struct bitval *b);
byte_fast2_t bitval_read_bigendian_byte2(struct bitval *b);
byte_fast3_t bitval_read_bigendian_byte3(struct bitval *b);
byte_fast4_t bitval_read_bigendian_byte4(struct bitval *b);
ubyte_fast1_t bitval_read_bigendian_ubyte1(struct bitval *b);
ubyte_fast2_t bitval_read_bigendian_ubyte2(struct bitval *b);
ubyte_fast3_t bitval_read_bigendian_ubyte3(struct bitval *b);
ubyte_fast4_t bitval_read_bigendian_ubyte4(struct bitval *b);


int bitval_peek_bit(const struct bitval *b);
intbv_t bitval_peek_sbits(const struct bitval *b, size_t n);
uintbv_t bitval_peek_ubits(const struct bitval *b, size_t n);

byte_fast1_t bitval_peek_byte1(const struct bitval *b);
byte_fast2_t bitval_peek_byte2(const struct bitval *b);
byte_fast3_t bitval_peek_byte3(const struct bitval *b);
byte_fast4_t bitval_peek_byte4(const struct bitval *b);
ubyte_fast1_t bitval_peek_ubyte1(const struct bitval *b);
ubyte_fast2_t bitval_peek_ubyte2(const struct bitval *b);
ubyte_fast3_t bitval_peek_ubyte3(const struct bitval *b);
ubyte_fast4_t bitval_peek_ubyte4(const struct bitval *b);

byte_fast1_t bitval_peek_bigendian_byte1(const struct bitval *b);
byte_fast2_t bitval_peek_bigendian_byte2(const struct bitval *b);
byte_fast3_t bitval_peek_bigendian_byte3(const struct bitval *b);
byte_fast4_t bitval_peek_bigendian_byte4(const struct bitval *b);
ubyte_fast1_t bitval_peek_bigendian_ubyte1(const struct bitval *b);
ubyte_fast2_t bitval_peek_bigendian_ubyte2(const struct bitval *b);
ubyte_fast3_t bitval_peek_bigendian_ubyte3(const struct bitval *b);
ubyte_fast4_t bitval_peek_bigendian_ubyte4(const struct bitval *b);
#endif
