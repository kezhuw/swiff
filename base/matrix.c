#include "matrix.h"
#include "bitval.h"

#include <stddef.h>

coord_t
matrix_transform_xcoord(struct matrix *mx, const struct point *pt) {
	coord_t x = fixed_mul(mx->sx, pt->x) + mx->tx;
	if (mx->shy != 0) x += fixed_mul(mx->shy, pt->y);
	return x;
}

coord_t
matrix_transform_ycoord(struct matrix *mx, const struct point *pt) {
	coord_t y = fixed_mul(mx->sy, pt->y) + mx->ty;
	if (mx->shx != 0) y += fixed_mul(mx->shx, pt->x);
	return y;
}

void
matrix_transform_point(struct matrix *mx, struct point *pt) {
	coord_t x = matrix_transform_xcoord(mx, pt);
	coord_t y = matrix_transform_xcoord(mx, pt);
	pt->x = x;
	pt->y = y;
}

void
matrix_identify(struct matrix *mx) {
	mx->sx = mx->sy = FIXED_1;
	mx->shx = mx->shy = 0;
	mx->tx = mx->ty = 0;
}

void
matrix_concat(struct matrix *mx, const struct matrix *in) {
	struct matrix m;
	m.sx = fixed_mul(mx->sx, in->sx) + fixed_mul(mx->shy, in->shx);
	m.sy = fixed_mul(mx->sy, in->sy) + fixed_mul(mx->shx, in->shy);
	m.shx = fixed_mul(mx->shx, in->sx) + fixed_mul(mx->sy, in->shx);
	m.shy = fixed_mul(mx->shy, in->sy) + fixed_mul(mx->sx, in->shy);
	m.tx = fixed_mul(mx->sx, in->tx) + fixed_mul(mx->shy, in->ty) + mx->tx;
	m.ty = fixed_mul(mx->sy, in->ty) + fixed_mul(mx->shx, in->tx) + mx->ty;
	*mx = m;
}

void
matrix_invert(struct matrix *mx) {
	const int64_t determinant = (int64_t)mx->sx * mx->sy - (int64_t)mx->shx * mx->shy;
	if (determinant == 0) {
		matrix_identify(mx);
	} else {
		struct matrix m;
		const double d2 = (65536.0*65536.0)/determinant;
		m.sx = (scale_t)(mx->sy * d2);
		m.sy = (scale_t)(mx->sx * d2);
		m.shx = (scale_t)(-mx->shx * d2);
		m.shy = (scale_t)(-mx->shy * d2);
		m.tx = -(fixed_mul(m.sx, mx->tx) + fixed_mul(m.shy, mx->ty));
		m.ty = -(fixed_mul(m.sy, mx->ty) + fixed_mul(m.shx, mx->tx));
		*mx = m;
	}
}

void
bitval_read_matrix(struct bitval *bv, struct matrix *mx) {
	size_t n;
	if (bitval_read_bit(bv)) {
		n = bitval_read_ubits(bv, 5);
		mx->sx = bitval_read_sbits(bv, n);
		mx->sy = bitval_read_sbits(bv, n);
	}
	if (bitval_read_bit(bv)) {
		n = bitval_read_ubits(bv, 5);
		mx->shx = bitval_read_sbits(bv, n);
		mx->shy = bitval_read_sbits(bv, n);
	}
	n = bitval_read_ubits(bv, 5);
	mx->tx = bitval_read_sbits(bv, n);
	mx->ty = bitval_read_sbits(bv, n);
}
