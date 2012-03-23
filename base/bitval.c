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

#define NBYTE	(sizeof(uintreg_t))
#define METER(val, nbit, bit4)		\
uintreg_t __v = val;			\
if (__v != 0) {				\
	uintreg_t hi = __v >> 4;	\
	if (hi != 0) {			\
		nbit -= bit4[hi];	\
		break;			\
	}				\
	nbit -= 4 + bit4[__v & 0x0F];	\
}					\
nbit -= 8

size_t
bitval_meter_nbits(uintreg_t val) {
	size_t bit4[0x0F] = {4, 3, 2, 2, 1, 1, 1, 1, 0, };
	union {
		uintreg_t val;
		uint8_t bytes[NBYTE];
	} u;
	u.val = val;
	size_t nbit = NBYTE*8;
#if BYTE_ORDER == LITTLE_ENDIAN
	size_t i=NBYTE;
	while (i--) {
		METER(u.bytes[i], nbit, bit4);
	}
#elif BYTE_ORDER == BIG_ENDIAN
	for (size_t i=0; i<NBYTE; i++) {
		METER(u.bytes[i], nbit, bit4);
	}
#else
#error specify byte order(BYTE_ORDER <= LITTLE_ENDIAN or BIG_ENDIAN)
#endif
	return nbit;
}

void
bitval_flush_write(struct bitval *bv) {
	size_t num = bv->num;
	if (num != 0) {
		num = (num+7)/8;
		buffer_t buf = bv->buf;
		uint8_t *ptr = (void*)bv->ptr;
		bv->buf = 0;
		bv->num = 0;
		bv->ptr += num;
		switch (num) {
		case 4:
			ptr[3] = (uint8_t)(buf >> (sizeof(buffer_t)*8 - 32));
		case 3:
			ptr[2] = (uint8_t)(buf >> (sizeof(buffer_t)*8 - 24));
		case 2:
			ptr[1] = (uint8_t)(buf >> (sizeof(buffer_t)*8 - 16));
		case 1:
			ptr[0] = (uint8_t)(buf >> (sizeof(buffer_t)*8 - 8));
		case 0:
			break;
		default:
			do {
				*ptr++ = (uint8_t)(buf >> (sizeof(buffer_t)*8 - 8));
				num --;
				buf <<= 8;
			} while (num != 0);
			break;
		}
	}
}

void
bitval_write_ubits(struct bitval *bv, uintreg_t val, size_t n) {
	assert(sizeof(buffer_t) >= sizeof(uintreg_t));
	buffer_t v = ((buffer_t)val) << (sizeof(buffer_t)*8 - n);
	size_t num = bv->num;
	while (n != 0) {
		size_t wrt = sizeof(buffer_t)*8 - num;
		if (wrt == 0) {
			bitval_flush_write(bv);
			num = 0;
			wrt = sizeof(buffer_t)*8;
		}
		if (n < wrt) {
			wrt = n;
		}
		assert(wrt == n || num+wrt == sizeof(buffer_t)*8);
		bv->buf |= v>>num;
		v <<= wrt;
		n -= wrt;
		num += wrt;
	}
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
		*lenp = (size_t)(end - (char*)bv->ptr);

		char *str = (char*)bv->ptr;
		bv->ptr = (byte_t*)end+1;
		return str;
	}
	return NULL;
}
