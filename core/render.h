#ifndef __CORE_RENDER_H
#define __CORE_RENDER_H

#include <base/intreg.h>

struct memface;
struct logface;
struct errface;
struct render;

struct render *render_create(struct memface *mc, struct logface *lc, struct errface *ec);
void render_delete(struct render *rd);

struct cache;
void render_struct_cache(struct render *rd);
struct cache *render_finish_cache(struct render *rd);
void render_commit_cache(struct render *rd, struct cache *ca);
void render_delete_cache(struct render *rd, struct cache *ca);

union color;
void render_set_fillcolor(struct render *rd, union color *fill0, union color *fill1);
void render_set_linewidth(struct render *rd, uintreg_t width);

void render_start_stroke(struct render *rd);
void render_close_stroke(struct render *rd);

struct point;
void render_move_to(struct render *rd, struct point *pt);
void render_line_to(struct render *rd, struct point *pt);
void render_curve_to(struct render *rd, struct point *control, struct point *anchor1);
#endif
