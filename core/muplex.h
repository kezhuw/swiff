#ifndef __CORE_MUPLEX_H
#define __CORE_MUPLEX_H

#include "common.h"
#include <stdint.h>

struct memface;
struct logface;
struct errface;
struct sprite;
struct graph;
struct render;
struct transform;
struct stream;
struct muplex;
struct sprite_define;
struct stream_define;

struct muface {
	struct muplex *muplex;
	struct stream * (*create_stream)(struct muplex *mux, const void *ud, enum stream_type ut, struct stream_define *inf);
	void (*delete_stream)(struct muplex *mux, struct stream *stm);
	void (*delete_muplex)(struct muplex *mux);
};

struct muface *muplex_create_default(struct memface *mc, struct logface *lc, struct errface *ec);

uintptr_t stream_progress_frame(struct stream *stm, struct sprite *si, uintptr_t tagpos);

void stream_struct_sprite(struct stream *stm, uintptr_t chptr, struct sprite_define *inf);

struct graph *stream_struct_shape(struct stream *stm, struct render *rd, const struct transform *tsm, uintptr_t chptr, struct graph *gh);
struct graph *stream_change_style(struct stream *stm, struct render *rd, const struct transform *tsm, uintptr_t chptr, struct graph *gh);

void stream_render_graph(struct stream *stm, struct render *rd, struct graph *gh);
void stream_delete_graph(struct stream *stm, struct render *rd, struct graph *gh);
#endif
