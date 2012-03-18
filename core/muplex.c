#include "muplex.h"
#include "stream.h"
#include "parser.h"
#include "common.h"
#include <base/helper.h>

#include <stdint.h>

struct muplex {
	struct muface interface;
	struct memctx *memctx;
	MemfaceAllocFunc_t malloc;
	MemfaceDeallocFunc_t free;
	struct pxface *default_parser;
};

static struct stream *
muplex_create_stream(struct muplex *mux, const void *ud, enum stream_type ut, struct stream_define *inf) {
	struct stream *stm = mux->malloc(mux->memctx, sizeof(*stm), __FILE__, __LINE__);
	stm->type = ut;
	stm->pxface = mux->default_parser;
	stm->pxface->struct_stream(stm->pxface->parser, stm, ud, inf);
	return stm;
}

static void
muplex_delete_stream(struct muplex *mux, struct stream *stm) {
	stm->pxface->finish_stream(stm->pxface->parser, stm);
	mux->free(mux->memctx, stm, __FILE__, __LINE__);
}

static void
muplex_delete_default(struct muplex *mux) {
	mux->default_parser->delete_parser(mux->default_parser->parser);
	mux->free(mux->memctx, mux, __FILE__, __LINE__);
}

struct muface *
muplex_create_default(struct memface *mc, struct logface *lc, struct errface *ec) {
	struct muplex *mux = mc->alloc(mc->ctx, sizeof(*mux), __FILE__, __LINE__);
	mux->interface.muplex = mux;
	mux->interface.create_stream = muplex_create_stream;
	mux->interface.delete_stream = muplex_delete_stream;
	mux->interface.delete_muplex = muplex_delete_default;
	mux->memctx = mc->ctx;
	mux->malloc = mc->alloc;
	mux->free = mc->dealloc;
	mux->default_parser = parser_create_default(mc, lc, ec);
	return &mux->interface;
}

uintptr_t
stream_progress_frame(struct stream *stm, struct sprite *si, uintptr_t tagpos) {
	return stm->pxface->progress_frame(stm->pxface->parser, stm, si, tagpos);
}

void
stream_struct_sprite(struct stream *stm, uintptr_t chptr, struct sprite_define *inf) {
	stm->pxface->struct_sprite(stm->pxface->parser, stm, chptr, inf);
}

struct graph *
stream_struct_graph(struct stream *stm, struct render *rd, const struct transform *tsm, uintptr_t chptr, struct graph *gh) {
	return stm->pxface->struct_graph(stm->pxface->parser, stm, rd, tsm, chptr, gh);
}

struct graph *
stream_change_graph(struct stream *stm, struct render *rd, const struct transform *tsm, uintptr_t chptr, struct graph *gh) {
	return stm->pxface->change_graph(stm->pxface->parser, stm, rd, tsm, chptr, gh);
}

void
stream_render_graph(struct stream *stm, struct render *rd, struct graph *gh) {
	stm->pxface->render_graph(stm->pxface->parser, stm, rd, gh);
}

void
stream_delete_graph(struct stream *stm, struct render *rd, struct graph *gh) {
	stm->pxface->delete_graph(stm->pxface->parser, stm, rd, gh);
}
