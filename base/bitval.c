#include "bitval.h"
#include "compat.h"
#include <stddef.h>
#include <stdbool.h>

static inline uintreg_t
read_bigendian_uint8_hi(const void *p) {
	return (read_bigendian_uint8(p) << 24);
}

static inline uintreg_t
read_bigendian_uint16_hi(const void *p) {
	return (read_bigendian_uint16(p) << 16);
}

static inline uintreg_t
read_bigendian_uint24(const void *p) {
	const uint8_t *u = p;
	return ((u[0]<<16) | (u[1]<<8) | u[2]);
}

static inline uintreg_t
read_bigendian_uint24_hi(const void *p) {
	return (read_bigendian_uint24(p) << 8);
}

static inline uintreg_t
read_bigendian_uint32_hi(const void *p) {
	return read_bigendian_uint32(p);
}

// Move no more than 4-bytes ptr to buf's highest bits.
static void
bitval_fill_hibuf(struct bitval *bv) {
	uint32_t val;
	number_t num;
	switch (bitval_remain_bytes(bv)) {
	case 0:
		val = 0;
		num = 0;
		break;
	case 1:
		val = read_bigendian_uint8_hi(bv->ptr);
		num = 8;
		bv->ptr += 1;
		break;
	case 2:
		val = read_bigendian_uint16_hi(bv->ptr);
		num = 16;
		bv->ptr += 2;
		break;
	case 3:
		val = read_bigendian_uint24_hi(bv->ptr);
		num = 24;
		bv->ptr += 3;
		break;
	default:
		val = read_bigendian_uint32_hi(bv->ptr);
		num = 32;
		bv->ptr += 4;
		break;
	}
	bv->buf = ((buffer_t)val) << ((sizeof(buffer_t)-sizeof(uint32_t))*8);
	bv->num = num;
}

// Read n bits to highest bits of return value.
uintreg_t
bitval_read_hbits(struct bitval *bv, size_t n) {
	assert(bv != NULL);
	assert(bitval_ensure_bits(bv, n));
	assert(n <= 32);

	buffer_t val = 0;
	number_t num = 0;

	if (n > bv->num) {
		n -= bv->num;

		val = bv->buf;
		num = bv->num;

		bitval_fill_hibuf(bv);
	}

	val |= (bv->buf & (~0 << (sizeof(buffer_t)*8 - n))) >> num;

	bv->buf <<= n;
	bv->num -= n;

	return (uintreg_t)(val >> ((sizeof(buffer_t)-sizeof(uintreg_t))*8));
}

void
bitval_skip_bits(struct bitval *bv, size_t n) {
	assert(bv != NULL);
	assert(bitval_ensure_bits(bv, n));

	if (n > bv->num) {
		n -= bv->num;

		bv->ptr += (n>>3);
		n &= (8-1);

		bitval_fill_hibuf(bv);
	}

	bv->buf <<= n;
	bv->num -= n;
}

const char *
bitval_read_string(struct bitval *bv, size_t *lenp) {
	assert(bv != NULL && lenp != NULL);
	assert(bitval_synced(bv));
	char *end = memchr(bv->ptr, '\0', bitval_remain_bytes(bv));
	if (end != NULL) {
		*lenp = (size_t)(end - (char *)bv->ptr);

		char *str = (char *)bv->ptr;
		bv->ptr = (uint8_t *)end+1;
		return str;
	}
	return NULL;
}
