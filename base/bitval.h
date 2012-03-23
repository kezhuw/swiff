#ifndef __BITVAL_H
#define __BITVAL_H

#include "compat.h"
#include "intreg.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Bitval provides utilities to read:
// 	no more than 32 bits integer value;
// 	C string value.

// For all functions that take 'struct bitval *bv' parameter, assert(bv != NULL).
// For all read/peek bits functions, assert(n <= 32).
// For all read/peek/skip functions, assert there is enough data.
// For all read/peek/skip bytes functions, assert(bitval_synced(bv)).

// At least 32-bits width.
typedef uintreg_t buffer_t;
typedef uintreg_t number_t;

#define ARITHMETIC_SHIFT_RIGHT

typedef uint8_t byte_t;

// XXX Export only for declaration.
typedef struct bitval {
	buffer_t buf;		// Valid bits are the highest bits.
	number_t num;		// Number of valid bits in buf.
	const byte_t *ptr;
	const byte_t *beg;
	size_t len;
	intreg_t opt;
} bitval_t[1];

// API {
static inline void bitval_copy(struct bitval *bv, const struct bitval *src);
static inline void bitval_sync(struct bitval *bv);
static inline bool bitval_synced(const struct bitval *bv);

static inline bool bitval_ensure_bits(const struct bitval *bv, size_t n);
static inline bool bitval_ensure_bytes(const struct bitval *bv, size_t n);

static inline size_t bitval_remain_bits(const struct bitval *bv);
static inline size_t bitval_remain_bytes(const struct bitval *bv);

// Reader API {{
static inline void bitval_init_read(struct bitval *bv, const byte_t *src, size_t len);

// Return current reading position.
// Caller need to sync bv.
static inline const byte_t *bitval_read_cursor(const struct bitval *bv);

static inline intreg_t bitval_read_bit(struct bitval *bv);
static inline intreg_t bitval_read_sbits(struct bitval *bv, size_t n);
static inline uintreg_t bitval_read_ubits(struct bitval *bv, size_t n);

static inline intreg_t bitval_read_int8(struct bitval *bv);
static inline intreg_t bitval_read_int16(struct bitval *bv);
static inline intreg_t bitval_read_int32(struct bitval *bv);

static inline uintreg_t bitval_read_uint8(struct bitval *bv);
static inline uintreg_t bitval_read_uint16(struct bitval *bv);
static inline uintreg_t bitval_read_uint32(struct bitval *bv);

static inline intreg_t bitval_read_bigendian_int8(struct bitval *bv);
static inline intreg_t bitval_read_bigendian_int16(struct bitval *bv);
static inline intreg_t bitval_read_bigendian_int32(struct bitval *bv);

static inline uintreg_t bitval_read_bigendian_uint8(struct bitval *bv);
static inline uintreg_t bitval_read_bigendian_uint16(struct bitval *bv);
static inline uintreg_t bitval_read_bigendian_uint32(struct bitval *bv);

const char *bitval_read_string(struct bitval *bv, size_t *lenp);
static inline const char *bitval_peek_string(const struct bitval *bv, size_t *lenp);
static inline void bitval_skip_string(struct bitval *bv);

void bitval_skip_bits(struct bitval *bv, size_t n);
static inline void bitval_skip_bytes(struct bitval *bv, size_t n);
static inline void bitval_jump_bytes(struct bitval *bv, long n);

static inline intreg_t bitval_peek_bit(const struct bitval *bv);
static inline intreg_t bitval_peek_sbits(const struct bitval *bv, size_t n);
static inline uintreg_t bitval_peek_ubits(const struct bitval *bv, size_t n);
// Reader API }}

// Writer API {{
static inline void bitval_init_write(struct bitval *bv, byte_t *dst, size_t len);

// Return writing position.
// Caller need to sync bv.
static inline byte_t *bitval_write_cursor(const struct bitval *bv);

void bitval_zero_bits(struct bitval *bv, size_t n);
static inline void bitval_zero_bytes(struct bitval *bv, size_t n);

static inline void bitval_write_bytes(struct bitval *bv, const byte_t *src, size_t n);
static inline void bitval_write_string(struct bitval *bv, const char *str, size_t len);

#define AssertNotnil(bv)	assert((bv) != NULL)
#define AssertSynced(bv)	AssertNotnil(bv); assert(bitval_synced(bv))
#define AssertNbytes(bv, n)	AssertNotnil(bv); assert(bitval_ensure_bytes((bv), (n)))
#define AssertNbits(bv, n)	AssertNotnil(bv); assert(bitval_ensure_bits((bv), (n)))

size_t bitval_meter_nbits(uintreg_t val);
static inline size_t bitval_meter_ubits(uintreg_t val);
static inline size_t bitval_meter_sbits(intreg_t val);

void bitval_write_ubits(struct bitval *bv, uintreg_t val, size_t n);
static inline void bitval_write_sbits(struct bitval *bv, intreg_t val, size_t n);

static inline void bitval_write_uint8(struct bitval *bv, uintreg_t val);
static inline void bitval_write_uint16(struct bitval *bv, uintreg_t val);
static inline void bitval_write_uint32(struct bitval *bv, uintreg_t val);

static inline void bitval_write_int8(struct bitval *bv, intreg_t val);
static inline void bitval_write_int16(struct bitval *bv, intreg_t val);
static inline void bitval_write_int32(struct bitval *bv, intreg_t val);

static inline void bitval_write_bigendian_uint8(struct bitval *bv, uintreg_t val);
static inline void bitval_write_bigendian_uint16(struct bitval *bv, uintreg_t val);
static inline void bitval_write_bigendian_uint32(struct bitval *bv, uintreg_t val);

static inline void bitval_write_bigendian_int8(struct bitval *bv, intreg_t val);
static inline void bitval_write_bigendian_int16(struct bitval *bv, intreg_t val);
static inline void bitval_write_bigendian_int32(struct bitval *bv, intreg_t val);

// Writer API }}

// Convenient functions {
static inline intreg_t read_int8(const byte_t *src);
static inline intreg_t read_int16(const byte_t *src);
static inline intreg_t read_int32(const byte_t *src);

static inline uintreg_t read_uint8(const byte_t *src);
static inline uintreg_t read_uint16(const byte_t *src);
static inline uintreg_t read_uint32(const byte_t *src);

static inline intreg_t read_bigendian_int8(const byte_t *src);
static inline intreg_t read_bigendian_int16(const byte_t *src);
static inline intreg_t read_bigendian_int32(const byte_t *src);

static inline uintreg_t read_bigendian_uint8(const byte_t *src);
static inline uintreg_t read_bigendian_uint16(const byte_t *src);
static inline uintreg_t read_bigendian_uint32(const byte_t *src);
// }

// }

static inline intreg_t
read_int8(const byte_t *src) {
	const int8_t *s = (void*)src;
	return s[0];
}

static inline intreg_t
read_int16(const byte_t *src) {
	const uint8_t *u = (void*)src;
	const int8_t *s = (void*)src;
	return (u[0] | (s[1]<<8));
}

static inline intreg_t
read_int32(const byte_t *src) {
	const uint8_t *u = (void*)src;
	const int8_t *s = (void*)src;
	return (u[0] | (u[1]<<8) | (u[2]<<16) | (s[3]<<24));
}

static inline uintreg_t
read_uint8(const byte_t *src) {
	return ((uint8_t *)src)[0];
}

static inline uintreg_t
read_uint16(const byte_t *src) {
	const uint8_t *u = (void*)src;
	return (u[0] | (u[1] << 8));
}

static inline uintreg_t
read_uint32(const byte_t *src) {
	const uint8_t *u = (void*)src;
	return (u[0] | (u[1]<<8) | (u[2]<<16) | (u[3]<<24));
}

static inline intreg_t
read_bigendian_int8(const byte_t *src) {
	const int8_t *s = (void*)src;
	return s[0];
}

static inline intreg_t
read_bigendian_int16(const byte_t *src) {
	const uint8_t *u = (void*)src;
	const int8_t *s = (void*)src;
	return ((s[0]<<8) | u[1]);
}

static inline intreg_t
read_bigendian_int32(const byte_t *src) {
	const uint8_t *u = (void*)src;
	const int8_t *s = (void*)src;
	return ((s[0]<<24) | (u[1]<<16) | (u[2]<<8) | u[3]);
}

static inline uintreg_t
read_bigendian_uint8(const byte_t *src) {
	const uint8_t *u = (void*)src;
	return u[0];
}

static inline uintreg_t
read_bigendian_uint16(const byte_t *src) {
	const uint8_t *u = (void*)src;
	return ((u[0]<<8) | u[1]);
}

static inline uintreg_t
read_bigendian_uint32(const byte_t *src) {
	const uint8_t *u = (void*)src;
	return ((u[0]<<24) | (u[1]<<16) | (u[2]<<8) | u[3]);
}

enum {
	OperationRead,
	OperationWrite,
};

#define AssertCorruptedData()	assert(!"corrupted bitval");

static inline void
bitval_init_read(struct bitval *bv, const byte_t *src, size_t len) {
	assert(sizeof(intreg_t) >= 4);
	assert(bv != NULL && src != NULL && len != 0);
	bv->buf = bv->num = 0;
	bv->beg = bv->ptr = src;
	bv->len = len;
	bv->opt = OperationRead;
}

static inline  void
bitval_init_write(struct bitval *bv, byte_t *dst, size_t len) {
	assert(sizeof(intreg_t) >= 4);
	assert(bv != NULL);
	assert(dst != NULL && len != 0);
	bv->buf = bv->num = 0;
	bv->beg = bv->ptr = dst;
	bv->len = len;
	bv->opt = OperationWrite;
}

static inline void
bitval_copy(struct bitval *bv, const struct bitval *src) {
	*bv = *src;
}

static inline const byte_t *
bitval_read_cursor(const struct bitval *bv) {
	assert(bv != NULL);
	return bv->ptr;
}

static inline byte_t *
bitval_write_cursor(const struct bitval *bv) {
	assert(bv != NULL);
	return (byte_t*)bv->ptr;
}

void bitval_flush_write(struct bitval *bv);

static inline void
bitval_sync(struct bitval *bv) {
	assert(bv != NULL);
	switch (bv->opt) {
	case OperationRead:
		bv->ptr -= bv->num/8;
		bv->buf = bv->num = 0;
		break;
	case OperationWrite:
		bitval_flush_write(bv);
		break;
	default:
		AssertCorruptedData();
	}
}

static inline bool
bitval_synced(const struct bitval *bv) {
	return bv->num == 0;
}

static inline bool
bitval_ensure_bound(const struct bitval *bv, long n) {
	if (n >= 0) {
		return bitval_ensure_bytes(bv, (size_t)n);
	} else {
		return bv->ptr+n >= bv->beg;
	}
}

static inline size_t
bitval_number_bytes(const struct bitval *bv) {
	return (bv->len - (size_t)(bv->ptr-bv->beg));
}

static inline size_t
bitval_number_bits(const struct bitval *bv) {
	return 8*bitval_number_bytes(bv);
}

static inline size_t
bitval_remain_bits(const struct bitval *bv) {
	switch (bv->opt) {
	case OperationRead:
		return (bitval_number_bits(bv) + bv->num);
	case OperationWrite:
		assert(bitval_number_bits(bv) >= bv->num);
		return (bitval_number_bits(bv) - bv->num);
	default:
		AssertCorruptedData();
	}
}

static inline size_t
bitval_remain_bytes(const struct bitval *bv) {
	switch (bv->opt) {
	case OperationRead:
		return (bitval_number_bytes(bv) + bv->num/8);
	case OperationWrite:
		return (bitval_number_bytes(bv) - ((bv->num+7)/8));
	default:
		AssertCorruptedData();
	}
}

static inline bool
bitval_ensure_bits(const struct bitval *bv, size_t n) {
	return bitval_remain_bits(bv) >= n;
}

static inline bool
bitval_ensure_bytes(const struct bitval *bv, size_t n) {
	return bitval_remain_bytes(bv) >= n;
}

static inline void
bitval_skip_bytes(struct bitval *bv, size_t n) {
	assert(bitval_synced(bv));
	assert(bitval_number_bytes(bv) >= n);
	bv->ptr += n;
}

static inline void
bitval_jump_bytes(struct bitval *bv, long n) {
	assert(bitval_synced(bv));
	assert(bitval_ensure_bound(bv, n));
	bv->ptr += n;
}

static inline const char *
bitval_peek_string(const struct bitval *bv, size_t *lenp) {
	assert(bv != NULL);
	struct bitval tmp;
	bitval_copy(&tmp, bv);
	return bitval_read_string(&tmp, lenp);
}

static inline void
bitval_skip_string(struct bitval *bv) {
	assert(bv != NULL);
	size_t len;
	bitval_read_string(bv, &len);
}

uintreg_t bitval_read_hbits(struct bitval *bv, size_t n);

uintreg_t
bitval_read_ubits(struct bitval *bv, size_t n) {
	assert(bv != NULL);
	return bitval_read_hbits(bv, n) >> (sizeof(uintreg_t)*8 - n);
}

intreg_t
bitval_read_sbits(struct bitval *bv, size_t n) {
	// XXX The resulting value of right-shift to negative-signed integer is
	// implementation-defined.
	assert(bv != NULL);
#ifdef ARITHMETIC_SHIFT_RIGHT
	return ((intreg_t)bitval_read_hbits(bv, n)) >> (sizeof(intreg_t)*8 - n);
#else
	uintreg_t val = bitval_read_ubits(bv, n);
	if ((val & (1 << (n-1))) != 0) {
		val |= ~0 << n;
	}
	return val;
#endif
}

static inline intreg_t
bitval_read_bit(struct bitval *bv) {
	assert(bv != NULL);
	return (intreg_t)(bitval_read_hbits(bv, 1) >> (sizeof(uintreg_t)*8 - 1));
}

static inline intreg_t
bitval_peek_bit(const struct bitval *bv) {
	assert(bv != NULL);
	struct bitval tmp;
	bitval_copy(&tmp, bv);
	return bitval_read_bit(&tmp);
}

uintreg_t
bitval_peek_ubits(const struct bitval *bv, size_t n) {
	assert(bv != NULL);
	struct bitval tmp;
	bitval_copy(&tmp, bv);
	return bitval_read_ubits(&tmp, n);
}

intreg_t
bitval_peek_sbits(const struct bitval *bv, size_t n) {
	assert(bv != NULL);
	struct bitval tmp;
	bitval_copy(&tmp, bv);
	return bitval_read_sbits(&tmp, n);
}

static inline size_t
bitval_meter_ubits(uintreg_t val) {
	if (val == 0) return 1;
	return bitval_meter_nbits(val);
}

static inline size_t
bitval_meter_sbits(intreg_t val) {
	if (val == 0 || val == -1) return 1;
	if (val > 0) {
		return bitval_meter_nbits(val)+1;
	} else {
		return bitval_meter_nbits(~val)+1;
	}
}

static inline void
bitval_write_sbits(struct bitval *bv, intreg_t val, size_t n) {
	bitval_write_ubits(bv, (uintreg_t)val, n);
}

static inline void
bitval_zero_bytes(struct bitval *bv, size_t n) {
	AssertSynced(bv);
	AssertNbytes(bv, n);
	memset((byte_t*)bv->ptr, 0, n);
	bv->ptr += n;
}

static inline void
bitval_write_bytes(struct bitval *bv, const byte_t *src, size_t n) {
	AssertSynced(bv);
	AssertNbytes(bv, n);
	memcpy((byte_t*)bv->ptr, src, n);
	bv->ptr += n;
}

static inline void
bitval_write_string(struct bitval *bv, const char *str, size_t len) {
	AssertSynced(bv);
	AssertNbytes(bv, len+1);
	char *ptr = (char*)bv->ptr;
	bv->ptr += len+1;
	memcpy(ptr, str, len);
	ptr[len] = '\0';
}

static inline void
bitval_write_uint8(struct bitval *bv, uintreg_t val) {
	AssertNbytes(bv, 1);
	uint8_t *ptr = (uint8_t*)bv->ptr;
	bv->ptr++;
	ptr[0] = (uint8_t)val;
}

static inline void
bitval_write_uint16(struct bitval *bv, uintreg_t val) {
	AssertNbytes(bv, 2);
	union {
		uint16_t val;
		uint8_t bytes[2];
	} u16;
	u16.val = (uint16_t)val;
	uint8_t *ptr = (uint8_t*)bv->ptr;
	bv->ptr += 2;
#if BYTE_ORDER == LITTLE_ENDIAN
	ptr[0] = u16.bytes[0];
	ptr[1] = u16.bytes[1];
#elif BYTE_ORDER == BIG_ENDIAN
	ptr[0] = u16.bytes[1];
	ptr[1] = u16.bytes[0];
#endif
}

static inline void
bitval_write_uint32(struct bitval *bv, uintreg_t val) {
	AssertNbytes(bv, 4);
	union {
		uint32_t val;
		uint8_t bytes[4];
	} u32;
	u32.val = (uint32_t)val;
	uint8_t *ptr = (uint8_t*)bv->ptr;
	bv->ptr += 4;
#if BYTE_ORDER == LITTLE_ENDIAN
	ptr[0] = u32.bytes[0];
	ptr[1] = u32.bytes[1];
	ptr[2] = u32.bytes[2];
	ptr[3] = u32.bytes[3];
#elif BYTE_ORDER == BIG_ENDIAN
	ptr[0] = u32.bytes[3];
	ptr[1] = u32.bytes[2];
	ptr[2] = u32.bytes[1];
	ptr[3] = u32.bytes[0];
#endif
}

static inline void
bitval_write_int8(struct bitval *bv, intreg_t val) {
	bitval_write_uint8(bv, (uintreg_t)val);
}

static inline void
bitval_write_int16(struct bitval *bv, intreg_t val) {
	bitval_write_uint16(bv, (uintreg_t)val);
}

static inline void
bitval_write_int32(struct bitval *bv, intreg_t val) {
	bitval_write_uint32(bv, (uintreg_t)val);
}

static inline void
bitval_write_bigendian_uint8(struct bitval *bv, uintreg_t val) {
	bitval_write_uint8(bv, val);
}

static inline void
bitval_write_bigendian_uint16(struct bitval *bv, uintreg_t val) {
	AssertNbytes(bv, 2);
	union {
		uint16_t val;
		uint8_t bytes[2];
	} u16;
	u16.val = (uint16_t)val;
	uint8_t *ptr = (uint8_t*)bv->ptr;
	bv->ptr += 2;
#if BYTE_ORDER == LITTLE_ENDIAN
	ptr[0] = u16.bytes[1];
	ptr[1] = u16.bytes[0];
#elif BYTE_ORDER == BIG_ENDIAN
	ptr[0] = u16.bytes[0];
	ptr[1] = u16.bytes[1];
#endif
}

static inline void
bitval_write_bigendian_uint32(struct bitval *bv, uintreg_t val) {
	AssertNbytes(bv, 4);
	union {
		uint32_t val;
		uint8_t bytes[4];
	} u32;
	u32.val = (uint32_t)val;
	uint8_t *ptr = (uint8_t*)bv->ptr;
	bv->ptr += 4;
#if BYTE_ORDER == LITTLE_ENDIAN
	ptr[0] = u32.bytes[3];
	ptr[1] = u32.bytes[2];
	ptr[2] = u32.bytes[1];
	ptr[3] = u32.bytes[0];
#elif BYTE_ORDER == BIG_ENDIAN
	ptr[0] = u32.bytes[0];
	ptr[1] = u32.bytes[1];
	ptr[2] = u32.bytes[2];
	ptr[3] = u32.bytes[3];
#endif
}

static inline void
bitval_write_bigendian_int8(struct bitval *bv, intreg_t val) {
	bitval_write_int8(bv, val);
}

static inline void
bitval_write_bigendian_int16(struct bitval *bv, intreg_t val) {
	bitval_write_bigendian_uint16(bv, (uintreg_t)val);
}

static inline void
bitval_write_bigendian_int32(struct bitval *bv, intreg_t val) {
	bitval_write_bigendian_uint32(bv, (uintreg_t)val);
}

#define DEFINE_READ_INTEGER(kind, nbit, type)		\
static inline type					\
bitval_read_ ## kind ## nbit (struct bitval *bv) {	\
	assert(bv != NULL);				\
	assert(bitval_synced(bv));			\
	assert(bitval_number_bytes(bv) >= (nbit>>3));	\
	type val = read_ ## kind ## nbit (bv->ptr);	\
	bv->ptr += (nbit>>3);				\
	return val;					\
}

#define DEFINE_PEEK_INTEGER(kind, nbit, type)		\
static inline type					\
bitval_peek_ ## kind ## nbit (const struct bitval *bv) {\
	assert(bv != NULL);				\
	struct bitval tmp = *bv;			\
	return bitval_read_ ## kind ## nbit (&tmp);	\
}

DEFINE_READ_INTEGER(uint, 8, uintreg_t)
DEFINE_READ_INTEGER(uint, 16, uintreg_t)
DEFINE_READ_INTEGER(uint, 32, uintreg_t)
DEFINE_READ_INTEGER(int, 8, intreg_t)
DEFINE_READ_INTEGER(int, 16, intreg_t)
DEFINE_READ_INTEGER(int, 32, intreg_t)
DEFINE_READ_INTEGER(bigendian_uint, 8, uintreg_t)
DEFINE_READ_INTEGER(bigendian_uint, 16, uintreg_t)
DEFINE_READ_INTEGER(bigendian_uint, 32, uintreg_t)
DEFINE_READ_INTEGER(bigendian_int, 8, intreg_t)
DEFINE_READ_INTEGER(bigendian_int, 16, intreg_t)
DEFINE_READ_INTEGER(bigendian_int, 32, intreg_t)

DEFINE_PEEK_INTEGER(uint, 8, uintreg_t)
DEFINE_PEEK_INTEGER(uint, 16, uintreg_t)
DEFINE_PEEK_INTEGER(uint, 32, uintreg_t)
DEFINE_PEEK_INTEGER(int, 8, intreg_t)
DEFINE_PEEK_INTEGER(int, 16, intreg_t)
DEFINE_PEEK_INTEGER(int, 32, intreg_t)
DEFINE_PEEK_INTEGER(bigendian_uint, 8, uintreg_t)
DEFINE_PEEK_INTEGER(bigendian_uint, 16, uintreg_t)
DEFINE_PEEK_INTEGER(bigendian_uint, 32, uintreg_t)
DEFINE_PEEK_INTEGER(bigendian_int, 8, intreg_t)
DEFINE_PEEK_INTEGER(bigendian_int, 16, intreg_t)
DEFINE_PEEK_INTEGER(bigendian_int, 32, intreg_t)
#undef DEFINE_READ_INTEGER
#undef DEFINE_PEEK_INTEGER
#endif
