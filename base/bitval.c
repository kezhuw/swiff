#include "bitval.h"
#include "multip.h"
#include <limits.h>
#include <stddef.h>
#include <stdbool.h>

#define BitvalStrerrNoEnoughData	"Bitval:{no enough data}"
#define BitvalStrerrUnaligned		"Bitval:{unaligned data}"
#define BitvalStrerrInvalidBitwidth	"Bitval:{invalid bitwidth}"

typedef uintbv_t buffer_t;	// Assert(sizeof(buffer_t) >= sizeof(uintbv_t) && (sizeof(buffer_t) >= sizeof(ubyte4_t))).
typedef uintbv_t number_t;

struct bitval {
	buffer_t buf;		// Valid bits are the highest bits.
	number_t num;		// Number of valid bits in buf.
	const ubyte1_t *ptr;
	const ubyte1_t *beg;
	const ubyte1_t *end;
	BitvalErrorFunc_t err;
};

static inline ubyte_fast1_t
read_ubyte1(const ubyte1_t *ptr) {
	return (ubyte_fast1_t)ptr[0];
}

static inline ubyte_fast2_t
read_ubyte2(const ubyte1_t *ptr) {
	return (ubyte_fast2_t)(ptr[0] | (ptr[1] << CHAR_BIT));
}

static inline ubyte_fast3_t
read_ubyte3(const ubyte1_t *ptr) {
	return (ubyte_fast3_t)(ptr[0] | (ptr[1] << CHAR_BIT) | (ptr[2] << CHAR_BIT*2));
}

static inline ubyte_fast4_t
read_ubyte4(const ubyte1_t *ptr) {
	return (ubyte_fast4_t)(ptr[0] | (ptr[1] << CHAR_BIT) | (ptr[2] << CHAR_BIT*2) | (ptr[3] << CHAR_BIT*3));
}

static inline byte_fast1_t
read_byte1(const ubyte1_t *ptr) {
	return (byte_fast1_t)((const byte1_t *)ptr)[0];
}

static inline byte_fast2_t
read_byte2(const ubyte1_t *ptr) {
	return (byte_fast2_t)(ptr[0] | ((((const byte1_t *)ptr)[1]) << CHAR_BIT));
}

static inline byte_fast3_t
read_byte3(const ubyte1_t *ptr) {
	return (byte_fast3_t)(ptr[0] | (ptr[1] << CHAR_BIT) | ((((const byte1_t *)ptr)[2]) << CHAR_BIT*2));
}

static inline byte_fast4_t
read_byte4(const ubyte1_t *ptr) {
	return (byte_fast4_t)(ptr[0] | (ptr[1] << CHAR_BIT) | (ptr[2] << CHAR_BIT*2) | ((((const byte1_t *)ptr)[3]) << CHAR_BIT*3));
}

static inline ubyte_fast1_t
read_bigendian_ubyte1(const ubyte1_t *ptr) {
	return (ubyte_fast1_t)ptr[0];
}

static inline ubyte_fast2_t
read_bigendian_ubyte2(const ubyte1_t *ptr) {
	return (ubyte_fast2_t)((ptr[0] << CHAR_BIT) | ptr[1]);
}

static inline ubyte_fast3_t
read_bigendian_ubyte3(const ubyte1_t *ptr) {
	return (ubyte_fast3_t)((ptr[0] << CHAR_BIT*2) | (ptr[1] << CHAR_BIT) | ptr[2]);
}

static inline ubyte_fast4_t
read_bigendian_ubyte4(const ubyte1_t *ptr) {
	return (ubyte_fast4_t)((ptr[0] << CHAR_BIT*3) | (ptr[1] << CHAR_BIT*2) | (ptr[2] << CHAR_BIT) | ptr[3]);
}

static inline byte_fast1_t
read_bigendian_byte1(const ubyte1_t *ptr) {
	return (byte_fast1_t)(((const byte1_t *)ptr)[0]);
}

static inline byte_fast2_t
read_bigendian_byte2(const ubyte1_t *ptr) {
	return (byte_fast2_t)(((((const byte1_t *)ptr)[0]) << CHAR_BIT) | ptr[1]);
}

static inline byte_fast3_t
read_bigendian_byte3(const ubyte1_t *ptr) {
	return (byte_fast3_t)(((((const byte1_t *)ptr)[0]) << CHAR_BIT*2) | (ptr[1] << CHAR_BIT) | ptr[2]);
}

static inline byte_fast4_t
read_bigendian_byte4(const ubyte1_t *ptr) {
	return (byte_fast4_t)(((((const byte1_t *)ptr)[0]) << CHAR_BIT*3) | (ptr[1] << CHAR_BIT*2) | (ptr[2] << CHAR_BIT) | ptr[3]);
}

static inline ubyte_fast4_t
read_bigendian_ubyte1_h4(const ubyte1_t *ptr) {
	return (ubyte_fast4_t)(read_bigendian_ubyte1(ptr) << CHAR_BIT*3);
}

static inline ubyte_fast4_t
read_bigendian_ubyte2_h4(const ubyte1_t *ptr) {
	return (ubyte_fast4_t)(read_bigendian_ubyte2(ptr) << CHAR_BIT*2);
}

static inline ubyte_fast4_t
read_bigendian_ubyte3_h4(const ubyte1_t *ptr) {
	return (ubyte_fast4_t)(read_bigendian_ubyte3(ptr) << CHAR_BIT);
}

static inline ubyte_fast4_t
read_bigendian_ubyte4_h4(const ubyte1_t *ptr) {
	return read_bigendian_ubyte4(ptr);
}

size_t
bitval_size(void) {
	return sizeof(struct bitval);
}

struct bitval *
bitval_init(struct bitval *b, const ubyte1_t *data, size_t size, BitvalErrorFunc_t err) {
	assert(b != NULL && data != NULL && err != NULL);
	b->buf = b->num = 0;
	b->beg = b->ptr = data;
	b->end = data + size;
	b->err = err;
	return b;
}

struct bitval *
bitval_copy(struct bitval *b, const struct bitval *src) {
	assert(b != NULL && src != NULL);
	*b = *src;
	return b;
}

const ubyte1_t *
bitval_getptr(const struct bitval *b) {
	assert(b != NULL);
	return b->ptr;
}

void
bitval_sync(struct bitval *b) {
	assert(b != NULL);
	b->ptr -= b->num/CHAR_BIT;
	b->buf = b->num = 0;
}

bool
bitval_synced(const struct bitval *b) {
	assert(b != NULL);
	return b->num == 0;
}

size_t
bitval_number_bits(const struct bitval *b) {
	assert(b != NULL);
	return (size_t)(b->num + (b->end-b->ptr)*CHAR_BIT);
}

bool
bitval_ensure_bits(const struct bitval *b, size_t n) {
	assert(b != NULL);
	return bitval_number_bits(b) >= n;
}

size_t
bitval_number_bytes(const struct bitval *b) {
	assert(b != NULL);
	return (size_t)(b->num/CHAR_BIT + (b->end-b->ptr));
}

bool
bitval_ensure_bytes(const struct bitval *b, size_t n) {
	assert(b != NULL);
	return bitval_number_bytes(b) >= n;
}

static inline void
bitval_assert_bits(const struct bitval *b, size_t n) {
#ifdef BITVAL_ERROR_REPORT
	if (!bitval_ensure_bits(b, n)) {
		b->err(BitvalStrerrNoEnoughData);
	}
#endif
	assert(bitval_ensure_bits(b, n));
}

static inline void
bitval_assert_bytes(const struct bitval *b, size_t n) {
#ifdef BITVAL_ERROR_REPORT
	if (!bitval_ensure_bytes(b, n)) {
		b->err(BitvalStrerrNoEnoughData);
	}
#endif
	assert((b->ptr+n) <= b->end);
}

static inline void
bitval_assert_synced(const struct bitval *b) {
#ifdef BITVAL_ERROR_REPORT
	if (!bitval_synced(b)) {
		b->err(BitvalStrerrUnaligned);
	}
#endif
	assert(bitval_synced(b));
}

static inline void
bitval_assert_bitwidth(const struct bitval *b, size_t n) {
#ifdef BITVAL_ERROR_REPORT
	if (n > sizeof(ubyte4_t)*CHAR_BIT) {
		b->err(BitvalStrerrInvalidBitwidth);
	}
#endif
	assert(n <= sizeof(ubyte4_t)*CHAR_BIT);
}

void
bitval_skip_bytes(struct bitval *b, size_t n) {
	assert(b != NULL);
	bitval_assert_synced(b);
	bitval_assert_bytes(b, n);

	b->ptr += n;
}

// Move no more than 4-bytes ptr to buf's highest bits.
static void
bitval_fill_hibuf(struct bitval *b) {
	ubyte4_t val;
	number_t num;
	switch (b->end - b->ptr) {
	case 0:
		val = 0;
		num = 0;
		break;
	case 1:
		val = read_bigendian_ubyte1_h4(b->ptr);
		num = CHAR_BIT;
		b->ptr += 1;
		break;
	case 2:
		val = read_bigendian_ubyte2_h4(b->ptr);
		num = CHAR_BIT*2;
		b->ptr += 2;
		break;
	case 3:
		val = read_bigendian_ubyte3_h4(b->ptr);
		num = CHAR_BIT*3;
		b->ptr += 3;
		break;
	default:
		val = read_bigendian_ubyte4_h4(b->ptr);
		num = CHAR_BIT*4;
		b->ptr += 4;
		break;
	}
	b->buf = ((buffer_t)val) << ((sizeof(buffer_t)-sizeof(ubyte4_t))*CHAR_BIT);
	b->num = num;
}

// Read n bits to highest bits of return value.
static uintbv_t
bitval_read_hbits(struct bitval *b, size_t n) {
	assert(b != NULL);
	bitval_assert_bits(b, n);
	bitval_assert_bitwidth(b, n);

	buffer_t val = 0;
	number_t num = 0;

	if (n > b->num) {
		n -= b->num;

		val = b->buf;
		num = b->num;

		bitval_fill_hibuf(b);
	}

	val |= (b->buf & (~0 << (sizeof(buffer_t)*CHAR_BIT - n))) >> num;

	b->buf <<= n;
	b->num -= n;

	return (uintbv_t)(val >> ((sizeof(buffer_t)-sizeof(uintbv_t))*CHAR_BIT));
}

uintbv_t
bitval_read_ubits(struct bitval *b, size_t n) {
	assert(b != NULL);
	return bitval_read_hbits(b, n) >> (sizeof(uintbv_t)*CHAR_BIT - n);
}

uintbv_t
bitval_peek_ubits(const struct bitval *b, size_t n) {
	assert(b != NULL);
	struct bitval bv = *b;
	return bitval_read_ubits(&bv, n);
}

intbv_t
bitval_read_sbits(struct bitval *b, size_t n) {
	// The resulting value of right-shift to negative-signed integer is
	// implementation-defined. Use implementation below rather than
	// return ((intbv_t)bitval_read_hbits(b, n)) >> (sizeof(intbv_t)*CHAR_BIT - n);
	assert(b != NULL);
	uintbv_t val = bitval_read_ubits(b, n);
	if ((val & (1 << (n-1))) != 0) {
		val |= ~0 << n;
	}
	return val;
}

intbv_t
bitval_peek_sbits(const struct bitval *b, size_t n) {
	assert(b != NULL);
	struct bitval bv = *b;
	return bitval_read_sbits(&bv, n);
}

void
bitval_skip_bits(struct bitval *b, size_t n) {
	assert(b != NULL);
	if (!bitval_ensure_bits(b, n)) {
		b->err(BitvalStrerrNoEnoughData);
	}

	if (n > b->num) {
		n -= b->num;

		b->ptr += n/CHAR_BIT;
		n %= CHAR_BIT;

		bitval_fill_hibuf(b);
	}

	b->buf <<= n;
	b->num -= n;
}

int
bitval_read_bit(struct bitval *b) {
	assert(b != NULL);
	return (int)(bitval_read_hbits(b, 1) >> (sizeof(uintbv_t)*CHAR_BIT - 1));
}

int
bitval_peek_bit(const struct bitval *b) {
	assert(b != NULL);
	struct bitval bv = *b;
	return bitval_read_bit(&bv);
}

#define DEFINE_READ(kind, nbyte, rettype)		\
rettype							\
bitval_read_ ## kind ## nbyte (struct bitval *b) {	\
	assert(b != NULL);				\
	bitval_assert_synced(b);			\
	bitval_assert_bytes(b, nbyte);			\
	rettype val = read_ ## kind ## nbyte (b->ptr);	\
	b->ptr += nbyte;				\
	return val;					\
}

#define DEFINE_PEEK(kind, nbyte, rettype)		\
rettype							\
bitval_peek_ ## kind ## nbyte (const struct bitval *b) {\
	assert(b != NULL);				\
	struct bitval bv = *b;				\
	return bitval_read_ ## kind ## nbyte (&bv);	\
}

DEFINE_READ(ubyte, 1, ubyte_fast1_t)
DEFINE_READ(ubyte, 2, ubyte_fast2_t)
DEFINE_READ(ubyte, 3, ubyte_fast3_t)
DEFINE_READ(ubyte, 4, ubyte_fast4_t)
DEFINE_READ(byte, 1, byte_fast1_t)
DEFINE_READ(byte, 2, byte_fast2_t)
DEFINE_READ(byte, 3, byte_fast3_t)
DEFINE_READ(byte, 4, byte_fast4_t)
DEFINE_READ(bigendian_ubyte, 1, ubyte_fast1_t)
DEFINE_READ(bigendian_ubyte, 2, ubyte_fast2_t)
DEFINE_READ(bigendian_ubyte, 3, ubyte_fast3_t)
DEFINE_READ(bigendian_ubyte, 4, ubyte_fast4_t)
DEFINE_READ(bigendian_byte, 1, byte_fast1_t)
DEFINE_READ(bigendian_byte, 2, byte_fast2_t)
DEFINE_READ(bigendian_byte, 3, byte_fast3_t)
DEFINE_READ(bigendian_byte, 4, byte_fast4_t)

DEFINE_PEEK(ubyte, 1, ubyte_fast1_t)
DEFINE_PEEK(ubyte, 2, ubyte_fast2_t)
DEFINE_PEEK(ubyte, 3, ubyte_fast3_t)
DEFINE_PEEK(ubyte, 4, ubyte_fast4_t)
DEFINE_PEEK(byte, 1, byte_fast1_t)
DEFINE_PEEK(byte, 2, byte_fast2_t)
DEFINE_PEEK(byte, 3, byte_fast3_t)
DEFINE_PEEK(byte, 4, byte_fast4_t)
DEFINE_PEEK(bigendian_ubyte, 1, ubyte_fast1_t)
DEFINE_PEEK(bigendian_ubyte, 2, ubyte_fast2_t)
DEFINE_PEEK(bigendian_ubyte, 3, ubyte_fast3_t)
DEFINE_PEEK(bigendian_ubyte, 4, ubyte_fast4_t)
DEFINE_PEEK(bigendian_byte, 1, byte_fast1_t)
DEFINE_PEEK(bigendian_byte, 2, byte_fast2_t)
DEFINE_PEEK(bigendian_byte, 3, byte_fast3_t)
DEFINE_PEEK(bigendian_byte, 4, byte_fast4_t)

const char *
bitval_read_string(struct bitval *b, size_t *lenp) {
	assert(b != NULL && lenp != NULL);
	bitval_assert_synced(b);

	char *end = memchr(b->ptr, '\0', b->end-b->ptr);
	if (end != NULL) {
		*lenp = (size_t)(end - (char *)b->ptr);

		char *str = (char *)b->ptr;
		b->ptr = (ubyte1_t *)end+1;
		return str;
	}
	return NULL;
}

const char *
bitval_peek_string(const struct bitval *b, size_t *lenp) {
	assert(b != NULL);
	struct bitval bv = *b;
	return bitval_read_string(&bv, lenp);
}

void
bitval_skip_string(struct bitval *b) {
	assert(b != NULL);
	size_t len;
	bitval_read_string(b, &len);
}
