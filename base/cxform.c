#include "cxform.h"
#include "compat.h"
#include "bitval.h"
#include "struct.h"

#include <stdint.h>

enum {
	Multiple	= 1,
	Addition	= 2
};

// lo => store first.
#if BYTE_ORDER == LITTLE_ENDIAN
#define C16to32(lo, hi)		((lo)|((hi)<<16))
#define C32to64(lo, hi)		((lo)|((hi)<<32))
#elif BYTE_ORDER == BIG_ENDIAN
#define C16to32(lo, hi)		((hi)|((lo)<<16))
#define C32to64(lo, hi)		((hi)|((lo)<<32))
#else
#error unsupported BYTE_ORDER(only support LITTLE_ENDIAN and BIG_ENDIAN)
#endif

#if __ILP32__
#undef C32to64
#endif

// XXX premature ??? oops!
void
cxform_reflag(struct cxform * restrict cx) {
	cx->flag = 0;
	if ((cx->rb | cx->gb | cx->bb | cx->ab) != 0) {
		cx->flag = Addition;
	}
	long or = cx->ra | cx->ga | cx->ba | cx->aa;
	long and = cx->ra & cx->ga & cx->ba & cx->aa;
	if (or != 256 || and != 256) {
		cx->flag |= Multiple;
	}
}

#define cconv(mul, ext)	(((mul)*(ext)) >> 8)

#define DEFINE_TRANSFORM_RGBA(ttype, ctype)		\
void cxform_transform_ ## ttype				\
(const struct cxform *cx, struct ttype *c) {		\
	long r = (long)c->r;				\
	long g = (long)c->g;				\
	long b = (long)c->b;				\
	long a = (long)c->a;				\
	switch (cx->flag) {				\
	case 0:						\
		return;					\
	case Addition:					\
		r += cx->rb;				\
		g += cx->gb;				\
		b += cx->bb;				\
		a += cx->ab;				\
		break;					\
	case Multiple:					\
		r = cconv(cx->ra, r);			\
		g = cconv(cx->ga, g);			\
		b = cconv(cx->ba, b);			\
		a = cconv(cx->aa, a);			\
		break;					\
	case (Addition|Multiple):			\
		r = cconv(cx->ra, r) + cx->rb;		\
		g = cconv(cx->ga, g) + cx->gb;		\
		b = cconv(cx->ba, b) + cx->bb;		\
		a = cconv(cx->aa, a) + cx->ab;		\
		break;					\
	default:					\
		assert(!"invalid cxform flag.");	\
	}						\
	long or = r|g|b|a;				\
	if ((or & ~255) == 0) {				\
		goto done;				\
	}						\
	if (or < 0) {					\
		if (r < 0) r = 0;			\
		if (g < 0) g = 0;			\
		if (b < 0) b = 0;			\
		if (a < 0) a = 0;			\
	}						\
	if (r > 255) r = 255;				\
	if (g > 255) g = 255;				\
	if (b > 255) b = 255;				\
	if (a > 255) a = 255;				\
done:							\
	c->r = (ctype)r;				\
	c->g = (ctype)g;				\
	c->b = (ctype)b;				\
	c->a = (ctype)a;				\
}

DEFINE_TRANSFORM_RGBA(rgba8, uint8_t)
DEFINE_TRANSFORM_RGBA(rgba16, uint16_t)

#undef DEFINE_TRANSFORM_RGBA

void
cxform_concat(struct cxform * restrict cx, const struct cxform * restrict ex) {
	if (ex->flag == 0) {
		return;
	}
	switch (cx->flag) {
	case 0:
		*cx = *ex;
		return;
	case Addition:
		cx->rb += ex->rb;
		cx->gb += ex->gb;
		cx->bb += ex->bb;
		cx->ab += ex->ab;

		cx->ra = ex->ra;
		cx->ga = ex->ga;
		cx->ba = ex->ba;
		cx->aa = ex->aa;
		break;
	case Multiple:
	case (Addition|Multiple):
		switch (ex->flag) {
		case Addition:
			cx->rb += cconv(cx->ra, ex->rb);
			cx->gb += cconv(cx->ga, ex->gb);
			cx->bb += cconv(cx->ba, ex->bb);
			cx->ab += cconv(cx->aa, ex->ab);
			break;
		case Multiple:
			cx->ra = cconv(cx->ra, ex->ra);
			cx->ga = cconv(cx->ga, ex->ga);
			cx->ba = cconv(cx->ba, ex->ba);
			cx->aa = cconv(cx->aa, ex->aa);
			break;
		case (Addition|Multiple):
			cx->rb += cconv(cx->ra, ex->rb);
			cx->gb += cconv(cx->ga, ex->gb);
			cx->bb += cconv(cx->ba, ex->bb);
			cx->ab += cconv(cx->aa, ex->ab);

			cx->ra = cconv(cx->ra, ex->ra);
			cx->ga = cconv(cx->ga, ex->ga);
			cx->ba = cconv(cx->ba, ex->ba);
			cx->aa = cconv(cx->aa, ex->aa);
			break;
		default:
			assert(!"invalid cxform flag value in the second argument of cxform_concat.");
		}
		break;
	default:
		assert(!"invalid cxform flag value in the first argument of cxform_concat.");
	}
	cxform_reflag(cx);
}

#if defined(__ILP32__)
#define set_mul		ux->u32.mul[0] = mul0; ux->u32.mul[1] = mul1
#define set_add		ux->u32.add[0] = add0; ux->u32.add[1] = add1
#define raw_mul		ux->u32.mul[0] = MUL32; ux->u32.mul[1] = MUL32
#define raw_add		ux->u32.add[0] = ADD32; ux->u32.add[1] = ADD32
#elif defined(__LP64__)
#define set_mul		ux->u64.mul[0] = C32to64(mul0, mul1)
#define set_add		ux->u64.add[0] = C32to64(add0, add1)
#define raw_mul		ux->u64.mul[0] = MUL64
#define raw_add		ux->u64.add[0] = ADD64
#endif

#define get_aa		bitval_read_sbits(bv, nbit)
#define get_ab		bitval_read_sbits(bv, nbit)

#define raw_aa		256
#define raw_ab		0

#define DEFINE_READ_CXFORM(suffix, aa, ab)				\
void									\
bitval_read_cxform ## suffix(struct bitval *bv, struct cxform *cx) {	\
	assert(bitval_synced(bv));					\
	uintreg_t bit6 = bitval_read_ubits(bv, 6);			\
	uintreg_t flag = bit6 >> 4;					\
	size_t nbit = (size_t)(bit6 & 0x0F);				\
	union unionx *ux = (void *)cx;					\
	uintreg_t lo0, hi0, lo1, hi1;					\
	uintreg_t mul0, mul1, add0, add1;				\
	switch (flag) {							\
	case 0:								\
		cxform_identify(cx);					\
		break;							\
	case Multiple:							\
		lo0 = (uint16_t)bitval_read_sbits(bv, nbit);		\
		hi0 = (uint16_t)bitval_read_sbits(bv, nbit);		\
		lo1 = (uint16_t)bitval_read_sbits(bv, nbit);		\
		hi1 = (uint16_t)aa;					\
		mul0 = C16to32(lo0, hi0);				\
		mul1 = C16to32(lo1, hi1);				\
		set_mul;						\
		raw_add;						\
		break;							\
	case Addition:							\
		lo0 = (uint16_t)bitval_read_sbits(bv, nbit);		\
		hi0 = (uint16_t)bitval_read_sbits(bv, nbit);		\
		lo1 = (uint16_t)bitval_read_sbits(bv, nbit);		\
		hi1 = (uint16_t)ab;					\
		add0 = C16to32(lo0, hi0);				\
		add1 = C16to32(lo1, hi1);				\
		set_add;						\
		raw_mul;						\
		break;							\
	case (Multiple|Addition):					\
		lo0 = (uint16_t)bitval_read_sbits(bv, nbit);		\
		hi0 = (uint16_t)bitval_read_sbits(bv, nbit);		\
		lo1 = (uint16_t)bitval_read_sbits(bv, nbit);		\
		hi1 = (uint16_t)aa;					\
		mul0 = C16to32(lo0, hi0);				\
		mul1 = C16to32(lo1, hi1);				\
		lo0 = (uint16_t)bitval_read_sbits(bv, nbit);		\
		hi0 = (uint16_t)bitval_read_sbits(bv, nbit);		\
		lo1 = (uint16_t)bitval_read_sbits(bv, nbit);		\
		hi1 = (uint16_t)ab;					\
		add0 = C16to32(lo0, hi0);				\
		add1 = C16to32(lo1, hi1);				\
		set_mul;						\
		set_add;						\
		break;							\
	}								\
	assert(Multiple == 1);						\
	assert(Addition == 2);						\
	cx->flag = flag;						\
}

DEFINE_READ_CXFORM(, get_aa, get_ab)
DEFINE_READ_CXFORM(_without_alpha, raw_aa, raw_ab)

#undef DEFINE_READ_CXFORM
