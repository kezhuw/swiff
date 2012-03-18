#ifndef __BASE_GEOMETRY_H
#define __BASE_GEOMETRY_H

#include "bitval.h"
#include <stddef.h>
#include <stdint.h>

typedef uint32_t coord_t;

struct point {
	coord_t x;
	coord_t y;
};

struct rectangle {
	coord_t xmin;
	coord_t xmax;
	coord_t ymin;
	coord_t ymax;
};

static inline void
bitval_read_rectangle(struct bitval *bv, struct rectangle *rt) {
	size_t n = bitval_read_ubits(bv, 5);
	rt->xmin = bitval_read_sbits(bv, n);
	rt->xmax = bitval_read_sbits(bv, n);
	rt->ymin = bitval_read_sbits(bv, n);
	rt->ymax = bitval_read_sbits(bv, n);
}
#endif
