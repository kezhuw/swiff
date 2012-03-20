#ifndef __CORE_RENDER_H
#define __CORE_RENDER_H

#include <base/intreg.h>
#include <base/matrix.h>
#include <base/struct.h>

struct memface;
struct logface;
struct errface;
struct render;

struct render *render_create(struct memface *mc, struct logface *lc, struct errface *ec);
void render_delete(struct render *rd);

struct texture;
void render_struct_texture(struct render *rd);
struct texture *render_return_texture(struct render *rd);

void render_commit_texture(struct render *rd, struct texture *ca);
void render_delete_texture(struct render *rd, struct texture *ca);

struct gradient {
	struct matrix invmat;
	struct rgba8 ramps[256];
};

union color {
	struct rgba8 solid;
	struct gradient gradient;
};

enum color_type {
	ColorTypeMinimum,
	ColorTypeSolid = ColorTypeMinimum,
	ColorTypeLinearGradient,
	ColorTypeRadialGradient,
	ColorTypeNumber
};

// XXX If type is Gradient, return value's invmat is a revert matrix.
union color *render_malloc_color(struct render *rd, enum color_type type);
void render_dealloc_color(struct render *rd, union color *co);

struct cinfo {
	bool transparent;
};
void render_change_cinfo(struct render *rd, union color *co, struct cinfo *ci);

void render_set_fillcolor(struct render *rd, union color *fill0, union color *fill1);
void render_set_linewidth(struct render *rd, uintreg_t width);

void render_start_stroke(struct render *rd, union color *co);
void render_close_stroke(struct render *rd);

struct point;
void render_move_to(struct render *rd, struct point *pt);
void render_line_to(struct render *rd, struct point *pt);
void render_curve_to(struct render *rd, struct point *control, struct point *anchor1);
#endif
