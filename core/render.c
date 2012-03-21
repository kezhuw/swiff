#include "render.h"
#include <base/slab.h>
#include <base/compat.h>
#include <base/helper.h>
#include <base/geometry.h>

#include <stddef.h>
#include <stdint.h>

#define ac_type		ac_union._ac_info.__ac_type
#define ac_transparent	ac_union._ac_info.__ac_transparent
#define ac_visnum	ac_union._ac_info.__ac_visnum
#define ac_init		ac_union._ac_init
struct active_color {
	union {
		struct {
			uint8_t __ac_type;
			uint8_t __ac_transparent;
			int16_t __ac_visnum;
		} _ac_info;
		uint32_t _ac_init;
	} ac_union;
	union color ac_color[];
};

#define SolidColorSize		(sizeof(struct active_color)+sizeof(struct rgba8))
#define GradientColorSize	(sizeof(struct active_color)+sizeof(struct rgba8))

#define COLOR_OFFSET		offsetof(struct active_color, ac_color)
#define COLOR2ACTIVE(co)	((struct active_color *)(((char*)co) - COLOR_OFFSET))

struct spoint {
	int16_t x;
	int16_t y;
};

static inline void
spoint_import_point(struct spoint *po, const struct point *pt) {
	po->x = (int16_t)pt->x;
	po->y = (int16_t)pt->y;
}

static inline void
spoint_export_point(const struct spoint *po, struct point *pt) {
	pt->x = (coord_t)po->x;
	pt->y = (coord_t)po->y;
}

#define EdgeCommonFields(prefix)	\
struct edge * prefix ## next;		\
uint8_t prefix ## edge_type;		\
uint8_t prefix ## fill_rule;		\
int16_t prefix ## direction;		\
struct spoint prefix ## anchor0;	\
struct spoint prefix ## anchor1;	\
struct active_color * prefix ## color0

struct curve {
	EdgeCommonFields(ce_);
	struct spoint ce_control;
	struct active_color *ce_color1;
};

struct line {
	EdgeCommonFields(le_);
	struct active_color *le_color1;
};

struct edge {
	EdgeCommonFields(ee_);
};

// line + evenodd	0
// curve + evenodd	1
// line + swfedge	2
// curve + swfedge	3

#define EdgeSlabIndex0Isize	offsetof(struct line, le_color1)
#define EdgeSlabIndex1Isize	offsetof(struct curve, ce_color1)
#define EdgeSlabIndex2Isize	sizeof(struct line)
#define EdgeSlabIndex3Isize	sizeof(struct curve)

#define EdgeSlabIndex0Nitem	30
#define EdgeSlabIndex1Nitem	30
#define EdgeSlabIndex2Nitem	15
#define EdgeSlabIndex3Nitem	15

#define EdgeSlabIndexz		4

enum edge_type {
	EdgeTypeLine	= 0,
	EdgeTypeCurve	= 1,
};

enum fill_rule {
	FillRuleEvenodd	= 0,
	FillRuleSwfedge	= 2,
	FillRuleWinding	= 4,
};

enum {
	EdgeDirectionPositive	= 1,
	EdgeDirectionNegative	= -1,
};

static inline size_t
edge_slab_index(enum edge_type type, enum fill_rule rule) {
	size_t index = (size_t)((type+rule) & 0x03);
	return index;
}

#define edge2texture(ee)	((struct texture *)ee);
#define texture2edge(tu)	((struct edge*)tu)

// We don't care about edge order in same layer.
struct painter {
	struct edge *pn_first;
	struct edge **pn_inset;
	struct render *pn_render;
	enum fill_rule pn_fill_rule;
	struct active_color *pn_color0;
	struct active_color *pn_color1;
	struct point pn_anchor0;
};

static inline void
painter_init(struct painter *pn) {
	pn->pn_inset = &pn->pn_first;
}

static inline void
painter_inset(struct painter *pn, struct edge *ee) {
	*pn->pn_inset = ee;
	pn->pn_inset = &ee->ee_next;
}

static inline struct edge *
painter_spawn(struct painter *pn, struct edge ***inset) {
	if (pn->pn_inset != &pn->pn_first) {
		*inset = pn->pn_inset;
		pn->pn_inset = &pn->pn_first;
		return pn->pn_first;
	}
	return NULL;
}

#define sk_closeo	sk_painter.pn_anchor0
struct stroker {
	struct painter sk_painter;
	struct point sk_closel;
	struct point sk_closer;
	struct point sk_startl;
	struct point sk_starto;
	struct point sk_startr;
};

static inline void
stroker_inset(struct stroker *sk, struct edge *ee) {
	painter_inset(&sk->sk_painter, ee);
}

static inline struct edge *
stroker_spawn(struct stroker *sk, struct edge ***inset) {
	return painter_spawn(&sk->sk_painter, inset);
}

struct render {
	struct painter rd_painter;
	struct stroker rd_stroker;
	struct texture *current_texture;
	struct memory *memctx;
	MemfaceAllocFunc_t malloc;
	MemfaceDeallocFunc_t dealloc;
	struct slab *active_color_slabs[ColorTypeNumber];
	struct slab *edge_slabs[EdgeSlabIndexz];
};

static inline void *
render_malloc(struct render *rd, size_t size, const char *file, int line) {
	return rd->malloc(rd->memctx, size, file, line);
}

static inline void
render_dealloc(struct render *rd, void *ptr, const char *file, int line) {
	rd->dealloc(rd->memctx, ptr, file, line);
}

static struct edge *
render_create_edge(struct render *rd, enum edge_type type, enum fill_rule rule) {
	size_t idx = edge_slab_index(type, rule);
	return slab_alloc(rd->edge_slabs[idx]);
}

static void
render_delete_edge(struct render *rd, struct edge *ee) {
	size_t idx = edge_slab_index((enum edge_type)ee->ee_edge_type, (enum fill_rule)ee->ee_fill_rule);
	slab_dealloc(rd->edge_slabs[idx], ee);
}

void
render_struct_texture(struct render *rd) {
	struct edge *first, **inset;
	if ((first = painter_spawn(&rd->rd_painter, &inset)) != NULL) {
		*inset = texture2edge(rd->current_texture);
		rd->current_texture = edge2texture(first);
	}
	if ((first = stroker_spawn(&rd->rd_stroker, &inset)) != NULL) {
		*inset = texture2edge(rd->current_texture);
		rd->current_texture = edge2texture(first);
	}
}

struct texture *
render_return_texture(struct render *rd) {
	render_struct_texture(rd);
	struct texture *tu = rd->current_texture;
	rd->current_texture = NULL;
	return tu;
}

void
render_delete_texture(struct render *rd, struct texture *tu) {
	struct edge *ee = texture2edge(tu);
	while (ee != NULL) {
		struct edge *ne = ee->ee_next;
		render_delete_edge(rd, ee);
		ee = ne;
	}
}

struct render *
render_create(struct memface *mem, struct logface *log, struct errface *err) {
	(void)log; (void)err;
	struct render *rd = mem->alloc(mem->ctx, sizeof(*rd), __FILE__, __LINE__);
	rd->memctx = mem->ctx;
	rd->malloc = mem->alloc;
	rd->dealloc = mem->dealloc;
	rd->active_color_slabs[ColorTypeSolid] = slab_create(mem, 30, SolidColorSize);
	rd->active_color_slabs[ColorTypeLinearGradient] = slab_create(mem, 5, GradientColorSize);
	rd->active_color_slabs[ColorTypeRadialGradient] = rd->active_color_slabs[ColorTypeLinearGradient];
	rd->edge_slabs[0] = slab_create(mem, EdgeSlabIndex0Nitem, EdgeSlabIndex0Isize);
	rd->edge_slabs[1] = slab_create(mem, EdgeSlabIndex1Nitem, EdgeSlabIndex1Isize);
	rd->edge_slabs[2] = slab_create(mem, EdgeSlabIndex2Nitem, EdgeSlabIndex2Isize);
	rd->edge_slabs[3] = slab_create(mem, EdgeSlabIndex3Nitem, EdgeSlabIndex3Isize);
	return rd;
}

void
render_delete(struct render *rd) {
	slab_delete(rd->active_color_slabs[ColorTypeSolid]);
	slab_delete(rd->active_color_slabs[ColorTypeLinearGradient]);
	render_dealloc(rd, rd, __FILE__, __LINE__);
}

union color *
render_malloc_color(struct render *rd, enum color_type type) {
	assert(type < ColorTypeNumber);
	struct active_color *ac = slab_alloc(rd->active_color_slabs[type]);
	ac->ac_init = 0;
	ac->ac_type = type;
	return ac->ac_color;
}

void
render_dealloc_color(struct render *rd, union color *co) {
	struct active_color *ac = COLOR2ACTIVE(co);
	slab_dealloc(rd->active_color_slabs[ac->ac_type], ac);
}

static void
point_average(const struct point *anchor0, const struct point *anchor1, struct point *control) {
	control->x = (anchor0->x + anchor1->x) >> 1;
	control->y = (anchor0->y + anchor1->y) >> 1;
}

static void
curve_divide(const struct point *anchor0, const struct point *control, const struct point *anchor1, struct point *control0, struct point *anchorz, struct point *control1) {
	point_average(anchor0, control, control0);
	point_average(control, anchor1, control1);
	point_average(control0, control1, anchorz);
}

static void
point_average_ratio(const struct point *anchor0, const struct point *anchor1, fixed_t ratio, struct point *control) {
	control->x = anchor0->x + fixed_mul(ratio, anchor1->x - anchor0->x);
	control->y = anchor0->y + fixed_mul(ratio, anchor1->y - anchor0->y);
}

static void
curve_divide_ratio(const struct point *anchor0, const struct point *control, const struct point *anchor1, fixed_t ratio, struct point *control0, struct point *anchorz, struct point *control1) {
	point_average_ratio(anchor0, control, ratio, control0);
	point_average_ratio(control, anchor1, ratio, control1);
	point_average_ratio(control0, control1, ratio, anchorz);
}

static void
painter_add_line(struct painter *pn, const struct point *anchor0, const struct point *anchor1, intreg_t direction) {
	assert(anchor0->y < anchor1->y);
	enum fill_rule rule = pn->pn_fill_rule;
	struct line *le = (void*)render_create_edge(pn->pn_render, EdgeTypeLine, rule);
	le->le_direction = direction;
	spoint_import_point(&le->le_anchor0, anchor0);
	spoint_import_point(&le->le_anchor1, anchor1);
	le->le_color0 = pn->pn_color0;
	if (rule == FillRuleSwfedge) {
		le->le_color1 = pn->pn_color1;
	}
	painter_inset(pn, (struct edge *)le);
}

#define CurveMaxError	3
#define CurveMaxDelta	256

static void
painter_add_curve(struct painter *pn, const struct point *anchor0, struct point *control, const struct point *anchor1, intreg_t direction) {
	assert(anchor0->y <= anchor1->y);
	if (control->y < anchor0->y || control->y > anchor1->y) {
		if (control->y < anchor0->y && anchor0->y - control->y < CurveMaxError) {
			control->y = anchor0->y;
		} else if (control->y > anchor1->y && control->y - anchor1->y < CurveMaxError) {
			control->y = anchor1->y;
		} else {
			coord_t a = anchor0->y - 2*control->y + anchor1->y;
			coord_t b = anchor0->y - control->y;
			fixed_t ratio = fixed_div((fixed_t)b, (fixed_t)a);
			struct point control0[1], anchorz[1], control1[1];
			curve_divide_ratio(anchor0, control, anchor1, ratio, control0, anchorz, control1);
			if (anchorz->y < anchor0->y) {
				direction = -direction;
			}
			painter_add_curve(pn, anchor0, control0, anchorz, direction);
			painter_add_curve(pn, anchorz, control1, anchor1, -direction);
			return;
		}
	}
	if (anchor1->y - anchor0->y > CurveMaxDelta) {
		coord_t a = anchor1->y - anchor0->y;
		coord_t b = control->y - anchor0->y;
		fixed_t ratio = fixed_div((fixed_t)b, (fixed_t)a);
		struct point control0[1], anchorz[1], control1[1];
		curve_divide_ratio(anchor0, control, anchor1, ratio, control0, anchorz, control1);
		painter_add_curve(pn, anchor0, control0, anchorz, direction);
		painter_add_curve(pn, anchorz, control1, anchor1, direction);
		return;
	}
	enum fill_rule rule = pn->pn_fill_rule;
	struct curve *ce = (void*)render_create_edge(pn->pn_render, EdgeTypeCurve, rule);
	ce->ce_direction = direction;
	spoint_import_point(&ce->ce_anchor0, anchor0);
	spoint_import_point(&ce->ce_control, control);
	spoint_import_point(&ce->ce_anchor1, anchor1);
	ce->ce_color0 = pn->pn_color0;
	if (rule == FillRuleSwfedge) {
		ce->ce_color1 = pn->pn_color1;
	}
	painter_inset(pn, (struct edge *)ce);
}

static inline void
painter_move_to(struct painter *pn, const struct point *anchor1) {
	pn->pn_anchor0 = *anchor1;
}

static void
painter_line_to(struct painter *pn, const struct point *anchor1) {
	struct point *anchor0 = &pn->pn_anchor0;
	if (anchor0->y != anchor1->y) {
		if (anchor0->y < anchor1->y) {
			painter_add_line(pn, anchor0, anchor1, EdgeDirectionPositive);
		} else {
			painter_add_line(pn, anchor1, anchor0, EdgeDirectionNegative);
		}
	}
	pn->pn_anchor0 = *anchor1;
}

static void
painter_curve_to(struct painter *pn, const struct point *control, const struct point *anchor1) {
	struct point *anchor0 = &pn->pn_anchor0;
	struct point controlz[1];
	*controlz = *control;
	if (anchor0->y <= anchor1->y) {
		painter_add_curve(pn, anchor0, controlz, anchor1, EdgeDirectionPositive);
	} else {
		painter_add_curve(pn, anchor1, controlz, anchor0, EdgeDirectionNegative);
	}
	pn->pn_anchor0 = *anchor1;
}

static void
painter_set_fill_rule(struct painter *pn, enum fill_rule rule) {
	if (rule == FillRuleSwfedge && pn->pn_color1 == NULL) {
		return;
	}
	pn->pn_fill_rule = rule;
}

static void
painter_set_fillcolor(struct painter *pn, struct active_color *color0, struct active_color *color1) {
	if (color1 != NULL && color0 == NULL) {
		color0 = color1;
		color1 = NULL;
	}
	pn->pn_color0 = color0;
	pn->pn_color1 = color1;
	if (color1 != NULL) {
		pn->pn_fill_rule = FillRuleSwfedge;
	}
}

void
render_set_fillcolor(struct render *rd, union color *fill0, union color *fill1) {
	struct active_color *color0, *color1;
	color0 = COLOR2ACTIVE(fill0);
	color1 = COLOR2ACTIVE(fill1);
	painter_set_fillcolor(&rd->rd_painter, color0, color1);
}
