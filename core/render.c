#include "render.h"
#include <base/slab.h>
#include <base/helper.h>

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

#define COLOR_OFFSET		offsetof(struct active_color, ac_color)
#define COLOR2ACTIVE(co)	((struct active_color *)(((char*)co) - COLOR_OFFSET))

struct render {
	struct memory *memctx;
	MemfaceAllocFunc_t malloc;
	MemfaceDeallocFunc_t dealloc;
	struct slab *active_color_slabs[ColorTypeNumber];
};

static inline void *
render_malloc(struct render *rd, size_t size, const char *file, int line) {
	return rd->malloc(rd->memctx, size, file, line);
}

static inline void
render_dealloc(struct render *rd, void *ptr, const char *file, int line) {
	rd->dealloc(rd->memctx, ptr, file, line);
}

#define SolidColorSize		(sizeof(struct active_color)+sizeof(struct rgba8))
#define GradientColorSize	(sizeof(struct active_color)+sizeof(struct rgba8))

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
