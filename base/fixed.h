#ifndef __BASE_FIXED_POINT_H
#define __BASE_FIXED_POINT_H

#include "intreg.h"
#include "compat.h"
#include <stdint.h>

typedef int32_t fixed_t;

#define FIXED_1		(1 << 16)

static inline intreg_t
fixed_mul(fixed_t ratio, intreg_t num) {
	return (intreg_t)((((int64_t)ratio)*((int64_t)num)) >> 16);
}

static inline fixed_t
fixed_div(fixed_t a, fixed_t b) {
	return (fixed_t)(((int64_t)a << 16)/(int64_t)b);
}

#endif
