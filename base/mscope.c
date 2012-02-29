#include "mscope.h"
#include "helper.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

// Greatly inspired from:
// 	More Memory Management in <<C Interfaces and Implementations>>.
// 	http://blog.csdn.net/xushiweizh/article/category/265099/

struct arena {
	struct arena *prev;
	uint8_t *avail;
	uint8_t *limit;
};

struct block {
	struct block *next;
};

#define ARENA_SIZE	4040

// segsizes[NSEGMENT-1] greater than (ARENA_SIZE-sizeof(union header))/2
// _mscope_segment_block uses this fact to simplify segmentation.
static const size_t segsizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048};
#define NSEGMENT	(sizeof(segsizes)/sizeof(segsizes[0]))

struct mscope {
	struct arena *local;
	struct arena *global;
	struct arena *frees;
	void *memctx;
	MemfaceAllocFunc_t alloc;
	MemfaceDeallocFunc_t dealloc;
	struct arena dummy[1];
	struct block *segments[NSEGMENT];
};

union header {
	struct arena arena;
	union align align;
};

static inline size_t
align_size(size_t size) {
	return MAKEALIGN(size);
}

static inline size_t
local_ptr2size(void *ptr) {
	size_t *size = (size_t *)((uint8_t *)ptr - sizeof(union align));
	return *size;
}

static inline bool
inside_arena(struct arena *aa, uint8_t *ptr) {
	return ((uint8_t *)aa <= ptr && ptr <= aa->limit);
}

static inline bool
reside_arena(struct arena *aa, uint8_t *ptr) {
	return ((uint8_t *)aa <= ptr && ptr <= aa->avail);
}

static void
_mscope_dealloc_arena(struct mscope *mo, struct arena *beg, struct arena *end) {
	void *memctx = mo->memctx;
	MemfaceDeallocFunc_t dealloc = mo->dealloc;
	while (beg != end) {
		struct arena *prev = beg->prev;
		dealloc(memctx, beg, __FILE__, __LINE__);
		beg = prev;
	}
}

static struct arena *
_mscope_acquire_arena(struct mscope *mo, size_t hint) {
	size_t size = hint+sizeof(union header);
	if (size <= ARENA_SIZE) {
		if (mo->frees != NULL) {
			struct arena *aa = mo->frees;
			mo->frees = aa->prev;
			aa->avail = (uint8_t *)aa + sizeof(union header);
			return aa;
		}
		size = ARENA_SIZE;
	}
	struct arena *aa = mo->alloc(mo->memctx, size, __FILE__, __LINE__);
	aa->avail = (uint8_t *)aa + sizeof(union header);
	aa->limit = (uint8_t *)aa + size;
	return aa;
}

void
_mscope_release_arena(struct mscope *mo, struct arena *aa) {
	size_t size = (size_t)(aa->limit - (uint8_t *)aa);
	if (size > ARENA_SIZE) {
		mo->dealloc(mo->memctx, aa, __FILE__, __LINE__);
	} else {
		assert(size == ARENA_SIZE);
		aa->prev = mo->frees;
		mo->frees = aa;
	}
}

static void
_mscope_segment_block(struct mscope *mo, uint8_t *blk, size_t size) {
	size_t i = NSEGMENT;
	while (i-- && size>=segsizes[0]) {
		if (size >= segsizes[i]) {
			size = size-segsizes[i];
			struct block *ins = (struct block *)(blk+size);
			ins->next = mo->segments[i];
			mo->segments[i] = ins;
		}
	}
}

static void *
_mscope_alloc_block(struct mscope *mo, size_t size) {
	int k;
	struct block *blk = NULL;
	size_t i = NSEGMENT;
	while (i--) {
		if (size > segsizes[i])
			break;
		if (mo->segments[i] != NULL) {
			blk = mo->segments[i];
			k = i;
		}
	}
	if (blk != NULL) {
		assert(mo->segments[k] == blk);
		assert(segsizes[k] >= size);
		mo->segments[k] = blk->next;
		size_t left = segsizes[k]-size;
		_mscope_segment_block(mo, (uint8_t *)blk, left);
		return (void *)((uint8_t *)blk+left);
	}
	return NULL;
}

static void *
_mscope_alloc_global(struct mscope *mo, size_t size) {
	void *ptr = _mscope_alloc_block(mo, size);
	if (ptr == NULL) {
		if (mo->global->avail+size > mo->global->limit) {
			size_t rem = (size_t)(mo->global->limit - mo->global->avail);
			_mscope_segment_block(mo, mo->global->avail, rem);
			struct arena *aa = _mscope_acquire_arena(mo, size);
			aa->prev = mo->global;
			mo->global = aa;
		}
		ptr = mo->global->avail;
		mo->global->avail += size;
	}
	return ptr;
}

// alloc_local() records memory's size for leave_local().
static void *
_mscope_alloc_local(struct mscope *mo, size_t size) {
	size_t total = size+sizeof(union align);
	if (mo->local->avail+total > mo->local->limit) {
		struct arena *aa = _mscope_acquire_arena(mo, total);
		aa->prev = mo->local;
		mo->local = aa;
	}
	uint8_t *avail = mo->local->avail;
	mo->local->avail += total;
	assert(mo->local->avail <= mo->local->limit);
	*((size_t *)avail) = size;
	return (avail+sizeof(union align));
}

static void
_mscope_inset_local(struct mscope *mo, struct arena *aa) {
	assert(mo->local != mo->dummy);
	aa->prev = mo->local->prev;
	mo->local->prev = aa;
}

static void
_mscope_leave_local(struct mscope *mo, void *sign, va_list args) {
	struct arena **itr = &mo->local;
	struct arena *end = mo->dummy;
	for (; *itr != end; itr = &(*itr)->prev) {
		if (inside_arena(*itr, sign)) {
			break;
		}
	}
	assert(reside_arena(*itr, sign));
	struct arena *save = *itr;
	*itr = end;

	va_list pptrs;
	va_copy(pptrs, args);
	for (void **pptr = va_arg(pptrs, void **); pptr != NULL; pptr = va_arg(pptrs, void **)) {
		if (inside_arena(save, *pptr) && !reside_arena(save, *pptr)) {
			size_t size = local_ptr2size(*pptr);
			void *ptr = _mscope_alloc_local(mo, size);
			memcpy(ptr, *pptr, size);
			*pptr = ptr;
		}
	}
	va_end(pptrs);

	struct arena *cur = mo->local;
	mo->local = save;
	save->avail = sign;
	while (cur != end) {
		struct arena *prev = cur->prev;
		va_copy(pptrs, args);
		for (void **pptr = va_arg(pptrs, void **); pptr != NULL; pptr = va_arg(pptrs, void **)) {
			if (inside_arena(cur, *pptr)) {
				assert(reside_arena(cur, *pptr));
				if (cur->limit-(uint8_t *)cur > ARENA_SIZE) {
					// Arena used for only one allocation.
					_mscope_inset_local(mo, cur);
					goto _next_arena;
				} else {
					size_t size = local_ptr2size(*pptr);
					void *ptr = _mscope_alloc_local(mo, size);
					memcpy(ptr, *pptr, size);
					*pptr = ptr;
				}
			}
		}
		_mscope_release_arena(mo, cur);
	_next_arena:
		va_end(pptrs);
		cur = prev;
	}
}

struct mscope *
mscope_create(const struct memface *mc) {
	assert(mc != NULL);
	assert(mc->alloc != NULL);
	assert(mc->dealloc != NULL);
	struct mscope *mo = mc->alloc(mc->ctx, sizeof(*mo), __FILE__, __LINE__);
	if (mo != NULL) {
		mo->memctx = mc->ctx;
		mo->alloc = mc->alloc;
		mo->dealloc = mc->dealloc;
		mo->dummy->prev = NULL;
		mo->dummy->avail = mo->dummy->limit = (uint8_t *)mo->dummy + sizeof(union header);
		mo->local = mo->global = mo->dummy;
		mo->frees = NULL;
		size_t i=NSEGMENT;
		while (i--) {
			mo->segments[i] = NULL;
		}
	}
	return mo;
}

void
mscope_delete(struct mscope *mo) {
	assert(mo != NULL);
	_mscope_dealloc_arena(mo, mo->local, mo->dummy);
	_mscope_dealloc_arena(mo, mo->global, mo->dummy);
	_mscope_dealloc_arena(mo, mo->frees, NULL);
	mo->local = mo->global = mo->dummy;
	mo->frees = NULL;
	mo->dealloc(mo->memctx, mo, __FILE__, __LINE__);
}

void
mscope_collect(struct mscope * mo) {
	assert(mo != NULL);
	_mscope_dealloc_arena(mo, mo->frees, NULL);
	mo->frees = NULL;
}

void *
mscope_enter_local(struct mscope * mo) {
	assert(mo != NULL);
	return mo->local->avail;
}

void
mscope_leave_local(struct mscope * mo, void *sign, ...) {
	assert(mo != NULL);
	va_list args;
	va_start(args, sign);
	_mscope_leave_local(mo, sign, args);
	va_end(args);
}

void *
mscope_alloc_local(struct mscope * mo, size_t size) {
	assert(mo != NULL);
	size = align_size(size);
	return _mscope_alloc_local(mo, size);
}

void *
mscope_alloc_global(struct mscope * mo, size_t size) {
	assert(mo != NULL);
	size = align_size(size);
	void *ptr = _mscope_alloc_global(mo, size);
	return ptr;
}
