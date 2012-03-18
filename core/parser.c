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

static struct dictionary *
parser_create_dictionary(struct parser *px) {
	return px->zalloc(px->memctx, sizeof(struct dictionary), __FILE__, __LINE__);
}

static void
parser_delete_dictionary(struct parser *px, struct dictionary *dc) {
	// TODO What about character and user defined data ?
	px->dealloc(px->memctx, dc, __FILE__, __LINE__);
}

static struct character *
parser_alloc_character(struct parser *px) {
	return px->malloc(px->memctx, sizeof(struct character), __FILE__, __LINE__);
}

static void
parser_free_character(struct parser *px, struct character *ch) {
	px->dealloc(px->memctx, ch, __FILE__, __LINE__);
}

static struct rectangle *
parser_alloc_rectangle(struct parser *px, size_t num) {
	return px->malloc(px->memctx, sizeof(struct rectangle)*num, __FILE__, __LINE__);
}

static void
parser_free_rectangle(struct parser *px, struct rectangle *rt) {
	px->dealloc(px->memctx, rt, __FILE__, __LINE__);
}

static void
DefineShape(struct stream *stm, enum swftag tag, const uint8_t *pos, size_t len) {
	struct bitval bv[1];
	bitval_init(bv, pos, len);
	uintreg_t id = bitval_read_uint16(bv);
	struct character *ch = parser_alloc_character(stm->pxface->parser);
	struct rectangle *rt = parser_alloc_rectangle(stm->pxface->parser, 1);
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

struct chain {
	struct chain *next;
	size_t ncolor;
	union color *colors[];
};

struct style {
	struct chain firstfill;
	struct chain firstline;
	struct chain *fillchain;
	struct chain *linechain;
};

struct graph {
	struct cache *cache;
	struct style style[1];
};

static void
parser_struct_style(struct parser *px, struct stream *stm, struct render *rd, const struct transform *tsm, struct bitval *bv, struct style *st) {
	(void)px; (void)stm; (void)rd; (void)tsm; (void)bv; (void)st;
}

static struct graph *
parser_struct_graph(struct parser *px, struct stream *stm, struct render *rd, const struct transform *tsm, uintptr_t chptr, struct graph *gh) {
	(void)px; (void)stm; (void)rd; (void)tsm; (void)chptr; (void)gh;
	return NULL;
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
