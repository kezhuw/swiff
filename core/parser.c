#include "parser.h"
#include "define.h"
#include "render.h"
#include "stream.h"
#include "swftag.h"
#include "player.h"
#include "common.h"
#include <base/helper.h>
#include <base/bitval.h>
#include <base/matrix.h>
#include <base/cxform.h>

#include <stddef.h>
#include <stdint.h>

struct memory;

struct parser {
	struct pxface interface;
	struct memory *memctx;
	MemfaceAllocFunc_t malloc;
	MemfaceAllocFunc_t zalloc;
	MemfaceDeallocFunc_t dealloc;
};

struct character {
	uint16_t id;
	uint16_t tag;
	uintptr_t data;
	uintptr_t udef;
	struct character *next;
};

#define DICT_SIZE	128
#define DICT_MASK	(DICT_SIZE-1)

struct dictionary {
	size_t nchar;
	struct character *chars[DICT_SIZE];
};

static void
dictionary_add_char(struct dictionary *dc, uintreg_t id, struct character *ch) {
	size_t idx = id & DICT_MASK;
	ch->next = dc->chars[idx];
	dc->chars[idx] = ch;
}

static struct character *
dictionary_get_char(struct dictionary *dc, uintreg_t id) {
	struct character *ch = dc->chars[id & DICT_MASK];
	while (ch != NULL) {
		if (ch->id == id) {
			return ch;
		}
		ch = ch->next;
	}
	assert(!"character non found");
	return NULL;
}

static enum character_type
tag2type(intreg_t tag) {
	switch (tag) {
	case SwftagDefineShape:
	case SwftagDefineShape2:
	case SwftagDefineShape3:
		return CharacterShape;
	case SwftagDefineSprite:
		return CharacterSprite;
	default:
		assert(!"unsupported character");
		return -1;
	}
}

// For shape, return struct character;
// For sprite, return underlying data.
static uintptr_t
dictionary_get_mark(struct dictionary *dc, uintreg_t id, uintreg_t *type) {
	struct character *ch = dictionary_get_char(dc, id);
	*type = tag2type(ch->tag);
	uintptr_t mark;
	switch (*type) {
	case CharacterShape:
		mark = (uintptr_t)ch;
		break;
	case CharacterSprite:
		mark = (uintptr_t)ch->data;
	}
	return mark;
}

static inline void *
parser_malloc(struct parser *px, size_t size, const char *file, int line) {
	return px->malloc(px->memctx, size, file, line);
}

static inline void *
parser_zalloc(struct parser *px, size_t size, const char *file, int line) {
	return px->zalloc(px->memctx, size, file, line);
}

static inline void
parser_dealloc(struct parser *px, void *ptr, const void *file, int line) {
	px->dealloc(px->memctx, ptr, file, line);
}

static struct character *
parser_malloc_character(struct parser *px) {
	return parser_malloc(px, sizeof(struct character), __FILE__, __LINE__);
}

static void
parser_dealloc_character(struct parser *px, struct character *ch) {
	parser_dealloc(px, ch, __FILE__, __LINE__);
}

static struct rectangle *
parser_malloc_rectangle(struct parser *px, size_t num) {
	return parser_malloc(px, sizeof(struct rectangle)*num, __FILE__, __LINE__);
}

static void
parser_dealloc_rectangle(struct parser *px, struct rectangle *rt) {
	parser_dealloc(px, rt, __FILE__, __LINE__);
}

static struct dictionary *
parser_create_dictionary(struct parser *px) {
	return parser_zalloc(px, sizeof(struct dictionary), __FILE__, __LINE__);
}

static void
parser_delete_dictionary(struct parser *px, struct dictionary *dc) {
	// XXX Dealloc udef in here ?
	size_t i, n, nchar;
	nchar = dc->nchar;
	for (i=0, n=DICT_SIZE; i<n; i++) {
		struct character *ch = dc->chars[i];
		while (ch != NULL) {
			struct character *ne = ch->next;
			switch (ch->tag) {
			case SwftagDefineShape:
			case SwftagDefineShape2:
			case SwftagDefineShape3:
				parser_dealloc_rectangle(px, (void *)ch->udef);
				break;
			default:
				break;
			}
			parser_dealloc_character(px, ch);
			nchar--;
			ch = ne;
		}
	}
	assert(nchar == 0);
	parser_dealloc(px, dc, __FILE__, __LINE__);
}

static void
DefineShape(struct stream *stm, enum swftag tag, const uint8_t *pos, size_t len) {
	struct bitval bv[1];
	bitval_init(bv, pos, len);
	uintreg_t id = bitval_read_uint16(bv);
	struct character *ch = parser_malloc_character(stm->pxface->parser);
	struct rectangle *rt = parser_malloc_rectangle(stm->pxface->parser, 1);
	bitval_read_rectangle(bv, rt);
	bitval_sync(bv);
	dictionary_add_char(stm->dictionary, id, ch);
	ch->id = id;
	ch->tag = tag;
	ch->data = (uintptr_t)bitval_cursor(bv);
	ch->udef = (uintptr_t)rt;
}

static void
PlaceObject(struct sprite *si, struct stream *stm, const uint8_t *pos, size_t len) {
	struct place_info pi;
	pi.flag = 0;
	pi.character = dictionary_get_mark(stm->dictionary, read_uint16(pos), &pi.type);
	pi.chardepth = read_uint16(pos+2);
	pi.clipdepth = 0;
	pi.stepratio = 0;

	struct bitval bv[1];
	bitval_init(bv, pos+4, len-4);
	bitval_read_matrix(bv, &pi.transform.matrix);
	bitval_sync(bv);
	cxform_identify(&pi.transform.cxform);
	if (bitval_number_bytes(bv) > 0) {
		bitval_read_cxform_without_alpha(bv, &pi.transform.cxform);
	}
	sprite_place_object(si, &pi);
}

static void
PlaceObject2(struct sprite *si, struct stream *stm, const uint8_t *pos, size_t len) {
	struct bitval bv[1];
	bitval_init(bv, pos, len);

	struct place_info pi;
	uintreg_t flag = pi.flag = bitval_read_uint8(bv);
	pi.chardepth = bitval_read_uint16(bv);

	if ((flag & PlaceFlagHasCharacter) != 0) {
		uintreg_t id = bitval_read_uint16(bv);
		pi.character = dictionary_get_mark(stm->dictionary, id, &pi.type);
	}

	matrix_identify(&pi.transform.matrix);
	if ((flag & PlaceFlagHasMatrix) != 0) {
		bitval_read_matrix(bv, &pi.transform.matrix);
		bitval_sync(bv);
	}

	cxform_identify(&pi.transform.cxform);
	if ((flag & PlaceFlagHasCxform) != 0) {
		bitval_read_cxform(bv, &pi.transform.cxform);
		bitval_sync(bv);
	}

	pi.stepratio = 0;
	if ((flag & PlaceFlagHasRatio) != 0) {
		pi.stepratio = bitval_read_uint16(bv);
	}

	pi.moviename.str = NULL;
	pi.moviename.len = 0;
	if ((flag & PlaceFlagHasName) != 0) {
		pi.moviename.str = bitval_read_string(bv, &pi.moviename.len);
	}

	sprite_place_object(si, &pi);
}

static void
RemoveObject2(struct sprite *si, const uint8_t *pos, size_t len) {
	assert(len == 2);
	sprite_remove_object(si, read_uint16(pos));
}

static void
RemoveObject(struct sprite *si, const uint8_t *pos, size_t len) {
	assert(len == 4);
	sprite_remove_object(si, read_uint16(pos+2));
}

static enum swftag
bitval_read_swftag(struct bitval *bv, size_t *lenp) {
	uintreg_t u16 = bitval_read_uint16(bv);
	intreg_t l = (intreg_t)(u16 & 0x3F);
	if (l == 0x3F) {
		l = bitval_read_int32(bv);
	}
	assert(l >= 0);
	*lenp = (size_t)l;
	return (enum swftag)(u16 >> 6);
}

static uintptr_t
parser_progress_frame(struct parser *px, struct stream *stm, struct sprite *si, uintptr_t tagpos) {
	(void)px;
	struct bitval bv[1];
	bitval_init(bv, (void *)tagpos, (size_t)-1);
	for (;;) {
		size_t len;
		enum swftag tag = bitval_read_swftag(bv, &len);
		void *pos = (void *)bitval_cursor(bv);
		switch (tag) {
		case SwftagEnd:
		case SwftagShowFrame:
			return (uintptr_t)pos;
		case SwftagDefineShape:
		case SwftagDefineShape2:
		case SwftagDefineShape3:
			DefineShape(stm, tag, pos, len);
			break;
		case SwftagPlaceObject:
			PlaceObject(si, stm, pos, len);
			break;
		case SwftagPlaceObject2:
			PlaceObject2(si, stm, pos, len);
			break;
		case SwftagRemoveObject:
			RemoveObject(si, pos, len);
			break;
		case SwftagRemoveObject2:
			RemoveObject2(si, pos, len);
			break;
		default:
			assert(!"unsupported tag format");
		}
	}
}

static void
parser_struct_sprite(struct parser *px, struct stream *stm, uintptr_t chptr, struct sprite_define *def) {
	(void)px; (void)stm;
	const uint8_t *pos = (void *)chptr;
	def->nframe = (intreg_t)read_uint16(pos);
	def->tagbeg = (uintptr_t)(pos+2);
}

static void
parser_struct_stream(struct parser *px, struct stream *stm, const void *ud, struct stream_define *def) {
	// TODO Support StreamFile.
	assert(stm->type != StreamFile);
	assert(memcmp(ud, "FWS", 3) == 0);

	struct bitval bv[1];
	bitval_init(bv, ud, (size_t)-1);
	bitval_skip_bytes(bv, 3);
	stm->version = bitval_read_uint8(bv);
	stm->resource = (uintptr_t)ud;
	stm->userdef = (uintptr_t)bitval_read_uint32(bv);
	stm->dictionary = parser_create_dictionary(px);

	bitval_read_rectangle(bv, &def->size);
	bitval_sync(bv);
	def->rate = bitval_read_uint16(bv);
	def->nframe = bitval_read_uint16(bv);
	def->tagbeg = (uintptr_t)bitval_cursor(bv);
}

static void
parser_finish_stream(struct parser *px, struct stream *stm) {
	parser_delete_dictionary(px, stm->dictionary);
	// TODO resource ?
}

enum fill_type {
	FillStyleSolid,
	FillStyleLinearGradient			= 0x10,
	FillStyleRadialGradient			= 0x12,
	FillStyleFocalRadialGradient		= 0x13,
	FillStyleRepeatingBitmap		= 0x40,
	FillStyleClippedBitmap			= 0x41,
	FillStyleNonSmoothedRepeatingBitmap	= 0x42,
	FillStyleNonSmoothedClippedBitmap	= 0x43,
};

static enum color_type
fill2color(enum fill_type type) {
	switch (type) {
	case FillStyleSolid:
		return ColorTypeSolid;
	case FillStyleLinearGradient:
		return ColorTypeLinearGradient;
	case FillStyleRadialGradient:
		return ColorTypeRadialGradient;
	default:
		assert(!"unsupported fill palette");
		return -1;
	}
}

struct style {
	uint16_t width;
};

struct palette {
	struct palette *next;
	size_t ncolor;
	union color *colors[];
};

struct graph {
	struct texture *texture;
	struct palette *lineset;
	struct palette *fillset;
};

struct state {
	struct palette **fillptr;
	struct palette **lineptr;
	struct texture **texture;
	enum swftag tag;
	const struct transform *txform;
	void (*read_rgba8)(struct bitval *bv, struct rgba8 *c);
	union color * (*get_fillcolor)(struct state *st, struct render *rd, size_t idx, enum color_type type);
	union color * (*get_linecolor)(struct state *st, struct render *rd, size_t idx, enum color_type type);
};

static inline void
graph_init(struct graph *gh) {
	gh->texture = NULL;
	gh->fillset = NULL;
	gh->lineset = NULL;
}

static void
style_fini(struct palette *st, struct parser *px, struct render *rd) {
	while (st != NULL) {
		struct palette *ne = st->next;
		size_t i, n;
		for (i=1, n=st->ncolor; i<n; i++) {
			render_dealloc_color(rd, st->colors[i]);
		}
		parser_dealloc(px, st, __FILE__, __LINE__);
		st = ne;
	}
}

static void
graph_fini(struct graph *gh, struct parser *px, struct render *rd) {
	render_delete_texture(rd, gh->texture);
	style_fini(gh->fillset, px, rd);
	style_fini(gh->lineset, px, rd);
	graph_init(gh);
}

static void
state_new_fills(struct state *st, struct parser *px, size_t nfill) {
	size_t n = nfill+1;
	struct palette *pa = *(st->fillptr);
	if (pa == NULL) {
		size_t size = sizeof(struct palette) + n*sizeof(union color *);
		pa = parser_malloc(px, size, __FILE__, __LINE__);
		pa->next = NULL;
		pa->ncolor = n;
		pa->colors[0] = NULL;
	}
	st->fillptr = &pa->next;
}

static struct style *
state_new_lines(struct state *st, struct parser *px, size_t nline) {
	size_t n = nline+1;
	struct palette *pa = *(st->lineptr);
	if (pa == NULL) {
		size_t size = sizeof(struct palette) + n*sizeof(union color *) + n*sizeof(struct style);
		pa = parser_malloc(px, size, __FILE__, __LINE__);
		pa->next = NULL;
		pa->ncolor = n;
		pa->colors[0] = NULL;
	}
	st->lineptr = &pa->next;
	return (struct style *)&pa->colors[n];
}

static inline union color **
_state_get_fillcolors(struct state *st) {
	struct palette *pa = (struct palette *)st->fillptr;
	return pa->colors;
}

static inline union color **
_state_get_linecolors(struct state *st) {
	struct palette *pa = (struct palette *)st->lineptr;
	return pa->colors;
}

static inline struct style *
_state_get_linestyles(struct state *st) {
	struct palette *pa = (struct palette *)st->lineptr;
	return (struct style *)&pa->colors[pa->ncolor];
}

static inline union color *
state_index_fillcolor(struct state *st, size_t idx) {
	union color **cos = _state_get_fillcolors(st);
	return cos[idx];
}

static union color *
state_index_linecolor(struct state *st, size_t idx) {
	union color **cos = _state_get_linecolors(st);
	return cos[idx];
}

static struct style *
state_index_linestyle(struct state *st, size_t idx) {
	struct style *los = _state_get_linestyles(st);
	return &los[idx];
}

static union color *
state_new_fillcolor(struct state *st, struct render *rd, size_t idx, enum color_type type) {
	union color **cos = _state_get_fillcolors(st);
	cos[idx] = render_malloc_color(rd, type);
	return cos[idx];
}

static union color *
state_get_fillcolor(struct state *st, struct render *rd, size_t idx, enum color_type type) {
	(void)rd; (void)type;
	union color **cos = _state_get_fillcolors(st);
	return cos[idx];
}

static union color *
state_new_linecolor(struct state *st, struct render *rd, size_t idx, enum color_type type) {
	union color **cos = _state_get_linecolors(st);
	cos[idx] = render_malloc_color(rd, type);
	return cos[idx];
}

static union color *
state_get_linecolor(struct state *st, struct render *rd, size_t idx, enum color_type type) {
	(void)rd; (void)type;
	union color **cos = _state_get_linecolors(st);
	return cos[idx];
}

static inline void
bitval_read_raw_rgba8_without_alpha(struct bitval *bv, struct rgba8 *c) {
	c->r = (uint8_t)bitval_read_uint8(bv);
	c->g = (uint8_t)bitval_read_uint8(bv);
	c->b = (uint8_t)bitval_read_uint8(bv);
	c->a = 255;
}

static inline void
bitval_read_raw_rgba8(struct bitval *bv, struct rgba8 *c) {
	c->r = (uint8_t)bitval_read_uint8(bv);
	c->g = (uint8_t)bitval_read_uint8(bv);
	c->b = (uint8_t)bitval_read_uint8(bv);
	c->a = (uint8_t)bitval_read_uint8(bv);
}

static inline void
state_init_struct(struct state *st, struct graph *gh, const struct transform *tsm, enum swftag tag) {
	st->fillptr = &gh->fillset;
	st->lineptr = &gh->lineset;
	st->texture = &gh->texture;
	st->tag = tag;
	st->txform = tsm;
	st->read_rgba8 = bitval_read_raw_rgba8_without_alpha;
	if (tag >= SwftagDefineShape3) {
		st->read_rgba8 = bitval_read_raw_rgba8;
	}
	st->get_fillcolor = state_new_fillcolor;
	st->get_linecolor = state_new_linecolor;
}

static inline void
state_init_change(struct state *st, struct graph *gh, const struct transform *tsm, enum swftag tag) {
	st->fillptr = &gh->fillset;
	st->lineptr = &gh->lineset;
	st->texture = &gh->texture;
	st->tag = tag;
	st->txform = tsm;
	st->read_rgba8 = bitval_read_raw_rgba8_without_alpha;
	if (tag >= SwftagDefineShape3) {
		st->read_rgba8 = bitval_read_raw_rgba8;
	}
	st->get_fillcolor = state_get_fillcolor;
	st->get_linecolor = state_get_linecolor;
}

// Read cxformed color.
static inline void
bitval_read_rgba8(struct bitval *bv, struct rgba8 *c, struct state *st) {
	st->read_rgba8(bv, c);
	cxform_transform_rgba8(&st->txform->cxform, c);
}

// XXX How to write back transparent info ?
static inline void
bitval_read_gradient(struct bitval *bv, struct gradient *gd, struct state *st) {
	bool transparent;
	struct rgba8 colori, colorz;
	intreg_t ratioi;
	intreg_t i, n;
	struct rgba8 *ramps = gd->ramps;
	intreg_t r, g, b, a;

	struct matrix mx;
	bitval_read_matrix(bv, &mx);
	bitval_sync(bv);

	matrix_concat(&gd->invmat, &st->txform->matrix);
	matrix_concat(&gd->invmat, &mx);

	n = bitval_read_uint8(bv);
	ratioi = bitval_read_uint8(bv);
	bitval_read_rgba8(bv, &colori, st);
	transparent = colori.a < 255;
	for (i=0; i<=ratioi; i++) {	// inclusive
		ramps[i] = colori;
	}
	r = colori.r<<16; g = colori.g<<16; b = colori.b<<16; a =colori.a<<16;
	for (i=1; i<n; i++) {
		intreg_t ratioz = bitval_read_uint8(bv);
		bitval_read_rgba8(bv, &colorz, st);
		transparent |= colorz.a < 255;
		if (ratioz > ratioi) {
			intreg_t ratiod = ratioz - ratioi;
			intreg_t dr, dg, db, da;
			dr = ((colorz.r<<16) - r)/ratiod;
			dg = ((colorz.g<<16) - g)/ratiod;
			db = ((colorz.b<<16) - b)/ratiod;
			da = ((colorz.a<<16) - a)/ratiod;
			for (ratioi++; ratioi < ratioz; ratioi++) {
				r += dr;
				g += dg;
				b += db;
				a += da;
				ramps[ratioi].r = (uint8_t)r;
				ramps[ratioi].g = (uint8_t)g;
				ramps[ratioi].b = (uint8_t)b;
				ramps[ratioi].a = (uint8_t)a;
			}
			colori = colorz;
		}
	}
	for (i=ratioi+1; i<256; i++) {
		ramps[i] = colori;
	}
}

static size_t
bitval_read_count(struct bitval *bv, struct state *st) {
	size_t n = bitval_read_uint8(bv);
	if (n == 0xFF && st->tag >= SwftagDefineShape2) {
		n = bitval_read_uint16(bv);
	}
	return n;
}

static void
parser_struct_palette(struct parser *px, struct render *rd, struct bitval *bv, struct state *st) {
	size_t i, n;
	n = bitval_read_count(bv, st);
	state_new_fills(st, px, n);
	for (i=1; i<=n; i++) {
		uintreg_t type = bitval_read_uint8(bv);
		union color *co = st->get_fillcolor(st, rd, i, fill2color(type));
		switch (type) {
		case FillStyleSolid:
			bitval_read_rgba8(bv, &co->solid, st);
			break;
		case FillStyleLinearGradient:
		case FillStyleRadialGradient:
		case FillStyleFocalRadialGradient:
			bitval_read_gradient(bv, &co->gradient, st);
			break;
		case FillStyleRepeatingBitmap:
		case FillStyleClippedBitmap:
		case FillStyleNonSmoothedRepeatingBitmap:
		case FillStyleNonSmoothedClippedBitmap:
			bitval_skip_bytes(bv, 2);
			bitval_skip_matrix(bv);
			bitval_sync(bv);
			break;
		default:
			break;
		}
	}
	n = bitval_read_count(bv, st);
	struct style *li = state_new_lines(st, px, n);
	for (i=1; i<=n; i++) {
		li[i].width = bitval_read_uint16(bv);
		union color *co = st->get_linecolor(st, rd, i, ColorTypeSolid);
		bitval_read_rgba8(bv, &co->solid, st);
	}
}

#define READ_NUM_BITS(bv, nfill, nline)		\
do {						\
	uintreg_t __n = bitval_read_uint8(bv);	\
	nline = __n & 0x0F;			\
	nfill = __n >> 4;			\
} while (0)

#define RecordTypeEdge		0x20
#define RecordEdgeLine		0x10

#define RecordStateMoveTo	0x01
#define RecordStateFillStyle0	0x02
#define RecordStateFillStyle1	0x04
#define RecordStateLineStyle	0x08
#define RecordStateNewStyles	0x10

#define RecordStateFillChange	(RecordStateFillStyle0|RecordStateFillStyle1)
#define RecordStateLineChange	(RecordStateMoveTo|RecordStateLineStyle)
#define RecordStatePathChange	(RecordStateMoveTo|RecordStateFillChange|RecordStateLineChange)

static void
parser_struct_texture(struct parser *px, struct render *rd, struct bitval *bv, struct state *st) {
	size_t nfillbits, nlinebits;
	size_t fill0index, fill1index, lineindex;
	struct point anchor0;
	anchor0.x = anchor0.y = 0;
	fill0index = fill1index = lineindex = 0;
	READ_NUM_BITS(bv, nfillbits, nlinebits);
	render_struct_texture(rd);
	for (;;) {
		uintreg_t flag = bitval_read_ubits(bv, 6);
		if (flag == 0) {
			render_close_stroke(rd);
			*(st->texture) = render_return_texture(rd);
			return;
		} else if ((flag & RecordTypeEdge) == 0) {
			if ((flag & RecordStateMoveTo)) {
				size_t n = bitval_read_ubits(bv, 5);
				anchor0.x = bitval_read_sbits(bv, n);
				anchor0.y = bitval_read_sbits(bv, n);
			}
			if ((flag & RecordStateFillStyle0)) {
				fill0index = bitval_read_ubits(bv, nfillbits);
			}
			if ((flag & RecordStateFillStyle1)) {
				fill1index = bitval_read_ubits(bv, nfillbits);
			}
			if ((flag & RecordStateLineStyle)) {
				lineindex = bitval_read_ubits(bv, nlinebits);
			}
			if ((flag & RecordStateNewStyles)) {
				parser_struct_palette(px, rd, bv, st);
				READ_NUM_BITS(bv, nfillbits, nlinebits);
			}
			if ((flag & RecordStateFillChange)) {
				union color *fill0 = state_index_fillcolor(st, fill0index);
				union color *fill1 = state_index_fillcolor(st, fill1index);
				render_set_fillcolor(rd, fill0, fill1);
			}
			if ((flag & RecordStateLineChange)) {
				render_close_stroke(rd);
				union color *co = state_index_linecolor(st, lineindex);
				if (co != NULL) {
					// XXX transform linewidth
					struct style *lo = state_index_linestyle(st, lineindex);
					render_set_linewidth(rd, lo->width);
					render_start_stroke(rd, co);
				}
			}
			if ((flag & RecordStatePathChange)) {
				// Start a new path.
				render_struct_texture(rd);
			} else if ((flag & RecordStateMoveTo)) {
				struct point anchor1 = anchor0;
				matrix_transform_point(&st->txform->matrix, &anchor1);
				render_move_to(rd, &anchor1);
			}
		} else {
			struct point anchor1;
			size_t n = (size_t)((flag & 0x0F) + 2);
			if ((flag & RecordEdgeLine) == 0) {
				struct point control;
				control.x = anchor0.x + bitval_read_sbits(bv, n);
				control.y = anchor0.y + bitval_read_sbits(bv, n);
				anchor1.x = anchor1.x + bitval_read_sbits(bv, n);
				anchor1.y = anchor1.y + bitval_read_sbits(bv, n);
				anchor0 = anchor1;
				matrix_transform_point(&st->txform->matrix, &control);
				matrix_transform_point(&st->txform->matrix, &anchor1);
				render_curve_to(rd, &control, &anchor1);
			} else {
				bool general, vert;
				if ((general = bitval_read_bit(bv))) {
					anchor1.x = anchor0.x + bitval_read_sbits(bv, n);
					anchor1.y = anchor0.y + bitval_read_sbits(bv, n);
				} else if ((vert = bitval_read_bit(bv))) {
					anchor1.x = anchor0.x;
					anchor1.y = anchor0.y + bitval_read_sbits(bv, n);
				} else {
					anchor1.x = anchor0.x + bitval_read_sbits(bv, n);
					anchor1.y = anchor0.y;
				}
				anchor0 = anchor1;
				matrix_transform_point(&st->txform->matrix, &anchor1);
				render_line_to(rd, &anchor1);
			}
		}
	}
}

static inline struct graph *
parser_malloc_graph(struct parser *px) {
	return parser_malloc(px, sizeof(struct graph), __FILE__, __LINE__);
}

static inline void
parser_dealloc_graph(struct parser *px, struct graph *gh) {
	parser_dealloc(px, gh, __FILE__, __LINE__);
}

static struct graph *
parser_struct_graph(struct parser *px, struct stream *stm, struct render *rd, const struct transform *tsm, uintptr_t chptr, struct graph *in) {
	(void)stm;
	struct graph gh;
	struct state st;
	const struct character *ch = (void *)chptr;
	if (in == NULL) {
		graph_init(&gh);
		state_init_struct(&st, &gh, tsm, ch->tag);
		in = parser_malloc_graph(px);
	} else {
		gh = *in;
		render_delete_texture(rd, gh.texture);
		gh.texture = NULL;
		state_init_change(&st, &gh, tsm, ch->tag);
	}
	struct bitval bv[1];
	bitval_init(bv, (void*)ch->data, (size_t)-1);
	parser_struct_palette(px, rd, bv, &st);
	parser_struct_texture(px, rd, bv, &st);
	*in = gh;
	return in;
}

static struct graph *
parser_change_graph(struct parser *px, struct stream *stm, struct render *rd, const struct transform *tsm, uintptr_t chptr, struct graph *gh) {
	(void)px; (void)stm; (void)rd; (void)tsm; (void)chptr; (void)gh;
	return NULL;
}

static void
parser_render_graph(struct parser *px, struct stream *stm, struct render *rd, struct graph *gh) {
	(void)px; (void)stm; (void)rd; (void)gh;
}

static void
parser_delete_graph(struct parser *px, struct stream *stm, struct render *rd, struct graph *gh) {
	(void)px; (void)stm; (void)rd; (void)gh;
}

void
parser_delete_default(struct parser *px) {
	(void)px;
}

struct pxface *
parser_create_default(struct memface *mem, struct logface *log, struct errface *err) {
	struct parser *px = mem->alloc(mem->ctx, sizeof(*px), __FILE__, __LINE__);
	px->interface.parser = px;
	px->interface.progress_frame = parser_progress_frame;
	px->interface.struct_graph = parser_struct_graph;
	px->interface.change_graph = parser_change_graph;
	px->interface.render_graph = parser_render_graph;
	px->interface.delete_graph = parser_delete_graph;
	px->interface.struct_stream = parser_struct_stream;
	px->interface.finish_stream = parser_finish_stream;
	px->interface.struct_sprite = parser_struct_sprite;
	px->interface.delete_parser = parser_delete_default;
	px->memctx = mem->ctx;
	px->malloc = mem->alloc;
	px->zalloc = mem->zalloc;
	px->dealloc = mem->dealloc;
	(void)log; (void)err;
	return &px->interface;
}
