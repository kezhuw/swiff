#ifndef __CXFORM_H
#define __CXFORM_H
#include "compat.h"
#include "intreg.h"
#include <stdint.h>
#include <stdbool.h>

struct cxform {
	uintreg_t flag;
	// Multiple items are 8.8 fixed point.
	int16_t ra;
	int16_t ga;
	int16_t ba;
	int16_t aa;
	// Addition items are 16 bits integers.
	int16_t rb;
	int16_t gb;
	int16_t bb;
	int16_t ab;
};

// API {

// After changing components of cxform, caller need to call reflag().
void cxform_reflag(struct cxform *cx);

// Cx is the outer cxform.
void cxform_concat(struct cxform * restrict cx, const struct cxform * restrict ex);

static inline void cxform_identify(struct cxform *cx);
static inline bool cxform_identity(const struct cxform *cx);

struct rgba8;
struct rgba16;
void cxform_transform_rgba8(const struct cxform *cx, struct rgba8 *c);
void cxform_transform_rgba16(const struct cxform *cx, struct rgba16 *c);

struct bitval;
void bitval_read_cxform(struct bitval *bv, struct cxform *cx);
void bitval_read_cxform_without_alpha(struct bitval *bv, struct cxform *cx);
// }



// XXX dirty
// XXX depend on struct cxform layout!!!
union unionx {
	struct cxform _;
#if defined(__ILP32__)
	struct {
		uintreg_t _;
		uint32_t mul[2];
		uint32_t add[2];
	} u32;
#elif defined(__LP64__)
	struct {
		uintreg_t _;
		uint64_t mul[1];
		uint64_t add[1];
	} u64;
#else
#error unsupported architecture(only support __ILP32__ and __LP64__)
#endif
};

#define ADD32	UINT32_C(0)
#define MUL32	UINT32_C(0x01000100)

#define ADD64	UINT64_C(0)
#define MUL64	UINT64_C(0x0100010001000100)

static inline void
cxform_identify(struct cxform *cx) {
	cx->flag = 0;
	union unionx *ux = (void *)cx;
#if defined(__ILP32__)
	ux->u32.mul[0] = MUL32;
	ux->u32.mul[1] = MUL32;
	ux->u32.add[0] = ADD32;
	ux->u32.add[1] = ADD32;
#elif defined(__LP64__)
	ux->u64.mul[0] = MUL64;
	ux->u64.add[0] = MUL64;
#endif
}

static inline bool
cxform_identity(const struct cxform *cx) {
	return cx->flag == 0;
}
#endif
