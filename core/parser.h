#ifndef __CORE_PARSER_H
#define __CORE_PARSER_H

#include <stddef.h>
#include <stdint.h>

struct sprite;
struct stream;
struct render;
struct parser;
struct transform;
struct sprite_define;
struct graph;
struct memface;
struct logface;
struct errface;

struct pxface {
	struct parser *parser;

	uintptr_t (*progress_frame)(struct parser *px, struct stream *stm, struct sprite *si, uintptr_t tagpos);

	struct graph * (*struct_graph)(struct parser *px, struct stream *stm, struct render *rd, const struct transform *tsm, uintptr_t chptr, struct graph *gh);
	struct graph * (*change_graph)(struct parser *px, struct stream *stm, struct render *rd, const struct transform *tsm, uintptr_t chptr, struct graph *gh);

	void (*render_graph)(struct parser *px, struct stream *stm, struct render *rd, struct graph *gh);
	void (*delete_graph)(struct parser *px, struct stream *stm, struct render *rd, struct graph *gh);

	void (*struct_sprite)(struct parser *px, struct stream *stm, uintptr_t chptr, struct sprite_define *inf);
	void (*struct_stream)(struct parser *px, struct stream *stm, const void *ud, struct stream_define *inf);
	void (*finish_stream)(struct parser *px, struct stream *stm);
	void (*delete_parser)(struct parser *px);
};

struct pxface *parser_create_default(struct memface *mc, struct logface *lc, struct errface *ec);
#endif
