#ifndef __BASE_MATRIX
#define __BASE_MATRIX

#include "fixed.h"
#include "geometry.h"

#include <stdint.h>

typedef fixed_t scale_t;
typedef coord_t trans_t;

struct matrix {
	scale_t sx;	// a in ActionScript
	scale_t sy;	// d
	scale_t shx;	// b
	scale_t shy;	// c
	trans_t tx;
	trans_t ty;
};

void matrix_identify(struct matrix *mx);
void matrix_concat(struct matrix *mx, const struct matrix *in);
void matrix_invert(struct matrix *mx);

coord_t matrix_transform_xcoord(struct matrix *mx, const struct point *pt);
coord_t matrix_transform_ycoord(struct matrix *mx, const struct point *pt);
void matrix_transform_point(struct matrix *mx, struct point *pt);

struct bitval;
void bitval_read_matrix(struct bitval *bv, struct matrix *mx);
#endif
