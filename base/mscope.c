#include "mscope.h"
#include "memory.h"
#include "helper.h"
#include <assert.h>
#include <stddef.h>
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

struct _mscope {
	struct arena *local;
	struct arena *global;
	struct arena *frees;
	void *memctx;
	MscopeAlloc_t alloc;
	MscopeDealloc_t dealloc;
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
_mscope_dealloc_arena(struct _mscope *_mo, struct arena *beg, struct arena *end) {
	void *memctx = _mo->memctx;
	MscopeDealloc_t dealloc = _mo->dealloc;
	while (beg != end) {
		struct arena *prev = beg->prev;
		dealloc(memctx, beg);
		beg = prev;
	}
}

static struct arena *
_mscope_acquire_arena(struct _mscope *_mo, size_t hint) {
	size_t size = hint+sizeof(union header);
	if (size <= ARENA_SIZE) {
		if (_mo->frees != NULL) {
			struct arena *aa = _mo->frees;
			_mo->frees = aa->prev;
			aa->avail = (uint8_t *)aa + sizeof(union header);
			return aa;
		}
		size = ARENA_SIZE;
	}
	struct arena *aa = _mo->alloc(_mo->memctx, size);
	aa->avail = (uint8_t *)aa + sizeof(union header);
	aa->limit = (uint8_t *)aa + size;
	return aa;
}

void
_mscope_release_arena(struct _mscope *_mo, struct arena *aa) {
	size_t size = (size_t)(aa->limit - (uint8_t *)aa);
	if (size > ARENA_SIZE) {
		_mo->dealloc(_mo->memctx, aa);
	} else {
		assert(size == ARENA_SIZE);
		aa->prev = _mo->frees;
		_mo->frees = aa;
	}
}

static void
_mscope_segment_block(struct _mscope *_mo, uint8_t *blk, size_t size) {
	size_t i = NSEGMENT;
	while (i-- && size>=segsizes[0]) {
		if (size >= segsizes[i]) {
			size = size-segsizes[i];
			struct block *ins = (struct block *)(blk+size);
			ins->next = _mo->segments[i];
			_mo->segments[i] = ins;
		}
	}
}

static void *
_mscope_alloc_block(struct _mscope *_mo, size_t size) {
	int k;
	struct block *blk = NULL;
	size_t i = NSEGMENT;
	while (i--) {
		if (size > segsizes[i])
			break;
		if (_mo->segments[i] != NULL) {
			blk = _mo->segments[i];
			k = i;
		}
	}
	if (blk != NULL) {
		assert(_mo->segments[k] == blk);
		assert(segsizes[k] >= size);
		_mo->segments[k] = blk->next;
		size_t left = segsizes[k]-size;
		_mscope_segment_block(_mo, (uint8_t *)blk, left);
		return (void *)((uint8_t *)blk+left);
	}
	return NULL;
}

static void *
_mscope_alloc_global(struct _mscope *_mo, size_t size) {
	void *ptr = _mscope_alloc_block(_mo, size);
	if (ptr == NULL) {
		if (_mo->global->avail+size > _mo->global->limit) {
			size_t rem = (size_t)(_mo->global->limit - _mo->global->avail);
			_mscope_segment_block(_mo, _mo->global->avail, rem);
			struct arena *aa = _mscope_acquire_arena(_mo, size);
			aa->prev = _mo->global;
			_mo->global = aa;
		}
		ptr = _mo->global->avail;
		_mo->global->avail += size;
	}
	return ptr;
}

static void *
_mscope_alloc_local(struct _mscope *_mo, size_t size) {
	size_t total = size+sizeof(union align);
	if (_mo->local->avail+total > _mo->local->limit) {
		struct arena *aa = _mscope_acquire_arena(_mo, total);
		aa->prev = _mo->local;
		_mo->local = aa;
	}
	uint8_t *avail = _mo->local->avail;
	_mo->local->avail += total;
	assert(_mo->local->avail <= _mo->local->limit);
	*((size_t *)avail) = size;
	return (avail+sizeof(union align));
}

static void
_mscope_inset_local(struct _mscope *_mo, struct arena *aa) {
	assert(_mo->local != _mo->dummy);
	aa->prev = _mo->local->prev;
	_mo->local->prev = aa;
}

static void
_mscope_leave_local(struct _mscope *_mo, void *mark, va_list args) {
	struct arena **itr = &_mo->local;
	struct arena *end = _mo->dummy;
	for (; *itr != end; itr = &(*itr)->prev) {
		if (inside_arena(*itr, mark)) {
			break;
		}
	}
	assert(reside_arena(*itr, mark));
	struct arena *save = *itr;
	*itr = end;

	va_list pptrs;
	va_copy(pptrs, args);
	for (void **pptr = va_arg(pptrs, void **); pptr != NULL; pptr = va_arg(pptrs, void **)) {
		if (inside_arena(save, *pptr) && !reside_arena(save, *pptr)) {
			size_t size = local_ptr2size(*pptr);
			void *ptr = _mscope_alloc_local(_mo, size);
			memcpy(ptr, *pptr, size);
			*pptr = ptr;
		}
	}
	va_end(pptrs);

	struct arena *cur = _mo->local;
	_mo->local = save;
	save->avail = mark;
	while (cur != end) {
		struct arena *prev = cur->prev;
		va_copy(pptrs, args);
		for (void **pptr = va_arg(pptrs, void **); pptr != NULL; pptr = va_arg(pptrs, void **)) {
			if (inside_arena(cur, *pptr)) {
				assert(reside_arena(cur, *pptr));
				if (cur->limit-(uint8_t *)cur > ARENA_SIZE) {
					// Arena used for only one allocation.
					_mscope_inset_local(_mo, cur);
					goto _next_arena;
				} else {
					size_t size = local_ptr2size(*pptr);
					void *ptr = _mscope_alloc_local(_mo, size);
					memcpy(ptr, *pptr, size);
					*pptr = ptr;
				}
			}
		}
		_mscope_release_arena(_mo, cur);
	_next_arena:
		va_end(pptrs);
		cur = prev;
	}
}

void
mscope_init(mscope_t mo, MscopeAlloc_t alloc, MscopeDealloc_t dealloc, void *memctx) {
	assert(mo != NULL && alloc != NULL);
	struct _mscope *_mo = (struct _mscope *)mo;
	_mo->memctx = memctx;
	_mo->alloc = alloc;
	_mo->dealloc = dealloc;
	_mo->dummy->prev = NULL;
	_mo->dummy->avail = _mo->dummy->limit = (uint8_t *)_mo->dummy + sizeof(union header);
	_mo->local = _mo->global = _mo->dummy;
	_mo->frees = NULL;
	size_t i=NSEGMENT;
	while (i--) {
		_mo->segments[i] = NULL;
	}
}

void
mscope_fini(mscope_t mo) {
	struct _mscope *_mo = (struct _mscope *)mo;
	_mscope_dealloc_arena(_mo, _mo->local, _mo->dummy);
	_mscope_dealloc_arena(_mo, _mo->global, _mo->dummy);
	_mscope_dealloc_arena(_mo, _mo->frees, NULL);
	_mo->local = _mo->global = _mo->dummy;
	_mo->frees = NULL;
}

void
mscope_collect(mscope_t mo) {
	struct _mscope *_mo = (struct _mscope *)mo;
	_mscope_dealloc_arena(_mo, _mo->frees, NULL);
	_mo->frees = NULL;
}

void *
mscope_enter_local(mscope_t mo) {
	struct _mscope *_mo = (struct _mscope *)mo;
	return _mo->local->avail;
}

void
mscope_leave_local(mscope_t mo, void *mark, ...) {
	va_list args;
	va_start(args, mark);
	_mscope_leave_local((struct _mscope *)mo, mark, args);
	va_end(args);
}

// alloc_local() records memory's size for leave_local().
void *
mscope_alloc_local(mscope_t mo, size_t size) {
	size = align_size(size);
	struct _mscope *_mo = (struct _mscope *)mo;
	return _mscope_alloc_local(_mo, size);
}
#include <stdio.h>
void *
mscope_alloc_global(mscope_t mo, size_t size) {
	printf("{alloc_global:{%zd}.\n", size);
	size = align_size(size);
	struct _mscope *_mo = (struct _mscope *)mo;
	void *ptr = _mscope_alloc_global(_mo, size);
	printf("}}}}}}.\n");
	return ptr;
}
