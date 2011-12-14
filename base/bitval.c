#include "bitval.h"
#include "multip.h"
#include <limits.h>
#include <stddef.h>
#include <stdbool.h>

#define BitvalStrerrOutOfBound		"Bitval:{out of bound}"
#define BitvalStrerrUnaligned		"Bitval:{unaligned data}"
#define BitvalStrerrInvalidBitwidth	"Bitval:{invalid bitwidth}"

typedef uintbv_t buffer_t;	// Assert(sizeof(buffer_t) >= sizeof(uintbv_t) && (sizeof(buffer_t) >= sizeof(ubyte4_t))).
typedef uintbv_t number_t;

struct _bitval {
	buffer_t buf;		// Valid bits are the highest bits.
	number_t num;		// Number of valid bits in buf.
	const ubyte1_t *ptr;
	const ubyte1_t *beg;
	size_t cnt;
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

static inline size_t
_bitval_remain_bytes(const struct _bitval *_bv) {
	return (_bv->cnt - (size_t)(_bv->ptr-_bv->beg));
}

static inline size_t
_bitval_number_bits(const struct _bitval *_bv) {
	return (size_t)(_bv->num + _bitval_remain_bytes(_bv)*CHAR_BIT);
}

static inline size_t
_bitval_number_bytes(const struct _bitval *_bv) {
	return (size_t)(_bv->num/CHAR_BIT + _bitval_remain_bytes(_bv));
}

static inline bool
_bitval_ensure_bits(const struct _bitval *_bv, size_t n) {
	return (_bitval_number_bits(_bv) >= n);
}

static inline void
_bitval_assert_bits(const struct _bitval *_bv, size_t n) {
	if (!_bitval_ensure_bits(_bv, n) && _bv->err != NULL) {
		_bv->err(BitvalStrerrOutOfBound);
	}
	assert(_bitval_ensure_bits(_bv, n));
}

static inline bool
_bitval_ensure_bytes(const struct _bitval *_bv, size_t n) {
	return (_bitval_number_bytes(_bv) >= n);
}

static inline void
_bitval_assert_bytes(const struct _bitval *_bv, size_t n) {
	if (!_bitval_ensure_bytes(_bv, n) && _bv->err != NULL) {
		_bv->err(BitvalStrerrOutOfBound);
	}
	assert(_bitval_ensure_bytes(_bv, n));
}

static inline bool
_bitval_ensure_bound(const struct _bitval *_bv, long n) {
	if (n >= 0) {
		return _bitval_ensure_bytes(_bv, (size_t)n);
	} else {
		return _bv->ptr+n >= _bv->beg;
	}
}

static inline void
_bitval_assert_bound(const struct _bitval *_bv, long n) {
	if (!_bitval_ensure_bound(_bv, n) && _bv->err != NULL) {
		_bv->err(BitvalStrerrOutOfBound);
	}
	assert(_bitval_ensure_bound(_bv, n));
}

static inline bool
_bitval_synced(const struct _bitval *_bv) {
	return (_bv->num == 0);
}

static inline void
_bitval_assert_synced(const struct _bitval *_bv) {
	if (!_bitval_synced(_bv) && _bv->err != NULL) {
		_bv->err(BitvalStrerrUnaligned);
	}
	assert(_bitval_synced(_bv));
}

static inline void
_bitval_assert_bitwidth(const struct _bitval *_bv, size_t n) {
	if (!(n <= sizeof(ubyte4_t)*CHAR_BIT) && _bv->err != NULL) {
		_bv->err(BitvalStrerrInvalidBitwidth);
	}
	assert(n <= sizeof(ubyte4_t)*CHAR_BIT);
}

// Move no more than 4-bytes ptr to buf's highest bits.
static void
_bitval_fill_hibuf(struct _bitval *_bv) {
	ubyte4_t val;
	number_t num;
	switch (_bitval_remain_bytes(_bv)) {
	case 0:
		val = 0;
		num = 0;
		break;
	case 1:
		val = read_bigendian_ubyte1_h4(_bv->ptr);
		num = CHAR_BIT;
		_bv->ptr += 1;
		break;
	case 2:
		val = read_bigendian_ubyte2_h4(_bv->ptr);
		num = CHAR_BIT*2;
		_bv->ptr += 2;
		break;
	case 3:
		val = read_bigendian_ubyte3_h4(_bv->ptr);
		num = CHAR_BIT*3;
		_bv->ptr += 3;
		break;
	default:
		val = read_bigendian_ubyte4_h4(_bv->ptr);
		num = CHAR_BIT*4;
		_bv->ptr += 4;
		break;
	}
	_bv->buf = ((buffer_t)val) << ((sizeof(buffer_t)-sizeof(ubyte4_t))*CHAR_BIT);
	_bv->num = num;
}

// Read n bits to highest bits of return value.
static uintbv_t
_bitval_read_hbits(struct _bitval *_bv, size_t n) {
	assert(_bv != NULL);
	_bitval_assert_bits(_bv, n);
	_bitval_assert_bitwidth(_bv, n);

	buffer_t val = 0;
	number_t num = 0;

	if (n > _bv->num) {
		n -= _bv->num;

		val = _bv->buf;
		num = _bv->num;

		_bitval_fill_hibuf(_bv);
	}

	val |= (_bv->buf & (~0 << (sizeof(buffer_t)*CHAR_BIT - n))) >> num;

	_bv->buf <<= n;
	_bv->num -= n;

	return (uintbv_t)(val >> ((sizeof(buffer_t)-sizeof(uintbv_t))*CHAR_BIT));
}

void
bitval_init(bitval_t bv, const ubyte1_t *data, size_t size, BitvalErrorFunc_t err) {
	assert(bv != NULL && data != NULL);
	struct _bitval *_bv = (struct _bitval *)bv;
	_bv->buf = _bv->num = 0;
	_bv->beg = _bv->ptr = data;
	_bv->cnt = size;
	_bv->err = err;
}

const ubyte1_t *
bitval_getptr(const bitval_t bv) {
	assert(bv != NULL);
	struct _bitval *_bv = (struct _bitval *)bv;
	return _bv->ptr;
}

void
bitval_sync(bitval_t bv) {
	assert(bv != NULL);
	struct _bitval *_bv = (struct _bitval *)bv;
	_bv->ptr -= _bv->num/CHAR_BIT;
	_bv->buf = _bv->num = 0;
}

bool
bitval_synced(const bitval_t bv) {
	struct _bitval *_bv = (struct _bitval *)bv;
	return _bitval_synced(_bv);
}

size_t
bitval_number_bits(const bitval_t bv) {
	assert(bv != NULL);
	struct _bitval *_bv = (struct _bitval *)bv;
	return _bitval_number_bits(_bv);
}

bool
bitval_ensure_bits(const bitval_t bv, size_t n) {
	assert(bv != NULL);
	struct _bitval *_bv = (struct _bitval *)bv;
	return _bitval_ensure_bits(_bv, n);
}

size_t
bitval_number_bytes(const bitval_t bv) {
	assert(bv != NULL);
	struct _bitval *_bv = (struct _bitval *)bv;
	return _bitval_number_bytes(_bv);
}

bool
bitval_ensure_bytes(const bitval_t bv, size_t n) {
	assert(bv != NULL);
	struct _bitval *_bv = (struct _bitval *)bv;
	return _bitval_ensure_bytes(_bv, n);
}

void
bitval_skip_bytes(bitval_t bv, size_t n) {
	assert(bv != NULL);
	struct _bitval *_bv = (struct _bitval *)bv;
	_bitval_assert_synced(_bv);
	_bitval_assert_bytes(_bv, n);
	_bv->ptr += n;
}

void
bitval_jump_bytes(bitval_t bv, long n) {
	assert(bv != NULL);
	struct _bitval *_bv = (struct _bitval *)bv;
	_bitval_assert_synced(_bv);
	_bitval_assert_bound(_bv, n);
	_bv->ptr += n;
}

uintbv_t
bitval_read_ubits(bitval_t bv, size_t n) {
	assert(bv != NULL);
	struct _bitval *_bv = (struct _bitval *)bv;
	return _bitval_read_hbits(_bv, n) >> (sizeof(uintbv_t)*CHAR_BIT - n);
}

uintbv_t
bitval_peek_ubits(const bitval_t _bv, size_t n) {
	assert(_bv != NULL);
	bitval_t tmp;
	bitval_copy(tmp, _bv);
	return bitval_read_ubits(tmp, n);
}

intbv_t
bitval_read_sbits(bitval_t bv, size_t n) {
	// The resulting value of right-shift to negative-signed integer is
	// implementation-defined. Use implementation below rather than
	// return ((intbv_t)bitval_read_hbits(bv, n)) >> (sizeof(intbv_t)*CHAR_BIT - n);
	assert(bv != NULL);
	uintbv_t val = bitval_read_ubits(bv, n);
	if ((val & (1 << (n-1))) != 0) {
		val |= ~0 << n;
	}
	return val;
}

intbv_t
bitval_peek_sbits(const bitval_t bv, size_t n) {
	assert(bv != NULL);
	bitval_t tmp;
	bitval_copy(tmp, bv);
	return bitval_read_sbits(tmp, n);
}

void
bitval_skip_bits(bitval_t bv, size_t n) {
	assert(bv != NULL);
	struct _bitval *_bv = (struct _bitval *)bv;
	_bitval_assert_bits(_bv, n);

	if (n > _bv->num) {
		n -= _bv->num;

		_bv->ptr += n/CHAR_BIT;
		n %= CHAR_BIT;

		_bitval_fill_hibuf(_bv);
	}

	_bv->buf <<= n;
	_bv->num -= n;
}

int
bitval_read_bit(bitval_t bv) {
	assert(bv != NULL);
	struct _bitval *_bv = (struct _bitval *)bv;
	return (int)(_bitval_read_hbits(_bv, 1) >> (sizeof(uintbv_t)*CHAR_BIT - 1));
}

int
bitval_peek_bit(const bitval_t bv) {
	assert(bv != NULL);
	bitval_t tmp;
	bitval_copy(tmp, bv);
	return bitval_read_bit(tmp);
}

#define DEFINE_READ(kind, nbyte, rettype)		\
rettype							\
bitval_read_ ## kind ## nbyte (bitval_t bv) {		\
	assert(bv != NULL);				\
	struct _bitval *_bv = (struct _bitval *)bv;	\
	_bitval_assert_synced(_bv);			\
	_bitval_assert_bytes(_bv, nbyte);		\
	rettype val = read_ ## kind ## nbyte (_bv->ptr);\
	_bv->ptr += nbyte;				\
	return val;					\
}

#define DEFINE_PEEK(kind, nbyte, rettype)		\
rettype							\
bitval_peek_ ## kind ## nbyte (const bitval_t bv) {	\
	assert(bv != NULL);				\
	bitval_t tmp;					\
	bitval_copy(tmp, bv);				\
	return bitval_read_ ## kind ## nbyte (tmp);	\
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
bitval_read_string(bitval_t bv, size_t *lenp) {
	assert(bv != NULL && lenp != NULL);
	struct _bitval *_bv = (struct _bitval *)bv;
	_bitval_assert_synced(_bv);
	char *end = memchr(_bv->ptr, '\0', _bitval_remain_bytes(_bv));
	if (end != NULL) {
		*lenp = (size_t)(end - (char *)_bv->ptr);

		char *str = (char *)_bv->ptr;
		_bv->ptr = (ubyte1_t *)end+1;
		return str;
	}
	return NULL;
}

const char *
bitval_peek_string(const bitval_t bv, size_t *lenp) {
	assert(bv != NULL);
	bitval_t tmp;
	bitval_copy(tmp, bv);
	return bitval_read_string(tmp, lenp);
}

void
bitval_skip_string(bitval_t bv) {
	assert(bv != NULL);
	size_t len;
	bitval_read_string(bv, &len);
}
