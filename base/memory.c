#include "memory.h"
#include "multip.h"
#include "helper.h"
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#define meminf_define(ptr, size, file, line)		(&(const struct meminf) {ptr, size, file, line})
struct meminf {
	const void *ptr;
	size_t size;
	const char *file;
	int line;
};

struct _memory {
	struct memface iface;
	struct memface mface[1];
	struct logface lface[1];
	struct errface eface[1];
	size_t memsize;
	size_t imeminf;
	size_t nmeminf;
	struct meminf *meminfs;
	const char *ident;
};

static inline bool
_expand_meminf(struct _memory *_mm) {
	size_t n = _mm->nmeminf;
	if (_mm->imeminf == _mm->nmeminf) {
		struct memface *mc = _mm->mface;
		size_t newsize = (2*n + 1) * sizeof(struct meminf);
		void *data = NULL;
		if (mc->realloc == NULL) {
			data = mc->alloc(mc->ctx, newsize, __FILE__, __LINE__);
			if (data != NULL) {
				memcpy(data, _mm->meminfs, n*sizeof(struct meminf));
				mc->dealloc(mc->ctx, _mm->meminfs, __FILE__, __LINE__);
			}
		} else {
			data = mc->realloc(mc->ctx, _mm->meminfs, newsize, __FILE__, __LINE__);
		}
		if (data == NULL) {
			return false;
		}
		_mm->nmeminf = 2*n + 1;
		_mm->meminfs = data;
	}
	return true;
}

// *mi valid until _insert_meminf()/_delete_meminf().
static bool
_search_meminf(struct _memory *_mm, const void *ptr, struct meminf **mi) {
	assert(mi != NULL);
	for (size_t i=0, n=_mm->imeminf; i<n; i++) {
		if (_mm->meminfs[i].ptr == ptr) {
			*mi = &_mm->meminfs[i];
			return true;
		}
	}
	return false;
}

static inline bool
_insert_meminf(struct _memory *_mm, const struct meminf *mi) {
	if (!_expand_meminf(_mm)) {
		return false;
	}
	_mm->meminfs[_mm->imeminf++] = *mi;
	_mm->memsize += mi->size;
	return true;
}

// Ensure *mi is returned by _search_meminf(), and there is no _insert_meminf()/
// _delete_meminf() between _update_meminf() and _search_meminf().
static inline void
_update_meminf(struct _memory *_mm, struct meminf *mi, const struct meminf *ch) {
	assert(mi >= _mm->meminfs && mi < _mm->meminfs+_mm->imeminf);
	_mm->memsize += ch->size;
	_mm->memsize -= mi->size;
	*mi = *ch;
}

static inline void
_delete_meminf(struct _memory *_mm, struct meminf *mi) {
	assert(_mm->imeminf > 0);
	assert(_mm->memsize >= mi->size);
	assert(mi >= _mm->meminfs && mi < _mm->meminfs+_mm->imeminf);

	_mm->memsize -= mi->size;
	_mm->imeminf--;
	memmove(mi, mi+1, (&_mm->meminfs[_mm->imeminf] - mi) * sizeof(struct meminf));
}

// If fails, dealloc() ptr and return NULL.
void *
_insert_newptr(struct _memory *_mm, void *ptr, size_t size, const char *file, int line) {
	const struct meminf *mi = meminf_define(ptr, size, file, line);
	if (!_insert_meminf(_mm, mi)) {
		struct memface *mc = _mm->mface;
		mc->dealloc(mc->ctx, ptr, __FILE__, __LINE__);
		ptr = NULL;
	}
	return ptr;
}

size_t
memory_amount(memory_t mm) {
	struct _memory *_mm = (struct _memory *)mm;
	return _mm->memsize;
}

void *
memory_reside(memory_t mm, const void *ptr) {
	struct _memory *_mm = (struct _memory *)mm;
	struct meminf *mi;
	return (_search_meminf(_mm, ptr, &mi) ? (void *)ptr : NULL);
}

int
memory_report(memory_t mm, const void *ptr) {
	struct _memory *_mm = (struct _memory *)mm;
	struct logface *lc = _mm->lface;
	struct meminf *mi;
	if (_search_meminf(_mm, ptr, &mi)) {
		return lc->log(lc->ctx, "[%s:%s()] memory %p, size %zu, allocated at file %s line %d.\n", _mm->ident, __func__, mi->ptr, mi->size, mi->file, mi->line);
	} else {
		return lc->log(lc->ctx, "[%s:%s()] invalid memory address %p.\n", _mm->ident, __func__, ptr);
	}
}

int
memory_status(memory_t mm) {
	struct _memory *_mm = (struct _memory *)mm;
	struct logface *lc = _mm->lface;
	return lc->log(lc->ctx, "[%s:%s()] %zu bytes allocated by clients, %zu bytes used by memory module.\n", _mm->ident, __func__, _mm->memsize, _mm->nmeminf * sizeof(struct meminf));
}

void *
memory_alloc(memory_t mm, size_t size, const char *file, int line) {
	struct _memory *_mm = (struct _memory *)mm;
	struct memface *mc = _mm->mface;
	void *ptr = mc->alloc(mc->ctx, size, __FILE__, __LINE__);
	if (ptr != NULL) {
		ptr = _insert_newptr(_mm, ptr, size, file, line);
	}
	return ptr;
}

void *
memory_zalloc(memory_t mm, size_t size, const char *file, int line) {
	struct _memory *_mm = (struct _memory *)mm;
	struct memface *mc = _mm->mface;
	if (mc->zalloc != NULL) {
		void *ptr = mc->zalloc(mc->ctx, size, __FILE__, __LINE__);
		if (ptr != NULL) {
			ptr = _insert_newptr(_mm, ptr, size, file, line);
		}
		return ptr;
	} else {
		void *ptr = memory_alloc(mm, size, file, line);
		if (ptr != NULL) {
			memset(ptr, 0, size);
		}
		return ptr;
	}
}

void *
memory_realloc(memory_t mm, void *ptr, size_t size, const char *file, int line) {
	if (ptr == NULL) {
		return memory_alloc(mm, size, file, line);
	}
	struct _memory *_mm = (struct _memory *)mm;
	struct meminf *mi;
	if (!_search_meminf(_mm, ptr, &mi)) {
		struct errface *ec = _mm->eface;
		ec->err(ec->ctx, "[%s:%s()] invalid memory address %p, reallocated at file %s line %d.\n", _mm->ident, __func__, ptr, file, line);
		abort();
		return NULL;
	}
	struct memface *mc = _mm->mface;
	if (mc->realloc == NULL) {
		void *ret = ptr;
		size_t oldsize = mi->size;
		if (size > oldsize || size < oldsize/2) {
			ret = memory_alloc(mm, size, file, line);
			if (ret != NULL) {
				size_t minsize = oldsize<size ? oldsize : size;
				memcpy(ret, ptr, minsize);
				memory_dealloc(mm, ptr, file, line);
			}
		} else {
			// Uses oldsize here.
			const struct meminf *re = meminf_define(ptr, oldsize, file, line);
			_update_meminf(_mm, mi, re);
		}
		return ret;
	}

	void *ret = mc->realloc(mc, ptr, size, __FILE__, __LINE__);
	if (ret == NULL) {
		if (size == 0) {
			_delete_meminf(_mm, mi);
		}
	} else {
		const struct meminf *re = meminf_define(ret, size, file, line);
		_update_meminf(_mm, mi, re);
	}
	return ret;
}

void
memory_dealloc(memory_t mm, void *ptr, const char *file, int line) {
	struct _memory *_mm = (struct _memory *)mm;
	if (ptr != NULL) {
		struct meminf *mi;
		if (!_search_meminf(_mm, ptr, &mi)) {
			struct errface *ec = _mm->eface;
			ec->err(ec->ctx, "[%s:%s()] invalid memory address %p, deallocated at file %s line %d.\n", _mm->ident, __func__, ptr, file, line);
			abort();
			return;
		}
		_delete_meminf(_mm, mi);
	}
	struct memface *mc = _mm->mface;
	mc->dealloc(mc, ptr, __FILE__, __LINE__);
}

struct memface *
memory_init(memory_t mm, const struct memface *mc, const struct logface *lc, const struct errface *ec, const char *ident) {
	assert(mc != NULL);
	assert(lc != NULL);
	assert(ec != NULL);
	assert(mc->alloc != NULL && mc->dealloc != NULL);
	assert(lc->log != NULL);
	assert(ec->err != NULL);
	assert(ident != NULL);
	struct _memory *_mm = (struct _memory *)mm;
	_mm->mface[0] = *mc;
	_mm->lface[0] = *lc;
	_mm->eface[0] = *ec;
	_mm->memsize = 0;
	_mm->imeminf = _mm->nmeminf = 0;
	_mm->meminfs = NULL;
	_mm->ident = ident;
	struct memface *m = &_mm->iface;
	m->ctx = mm;
	m->alloc = (MemfaceAllocFunc_t)memory_alloc;
	m->zalloc = (MemfaceAllocFunc_t)memory_zalloc;
	m->realloc = (MemfaceReallocFunc_t)memory_realloc;
	m->dealloc = (MemfaceDeallocFunc_t)memory_dealloc;
	return m;
}

void
memory_fini(memory_t mm) {
	struct _memory *_mm = (struct _memory *)mm;
	struct memface *mc = _mm->mface;
	struct logface *lc = _mm->lface;
	MemfaceDeallocFunc_t dealloc = mc->dealloc;
	for (size_t i=0; i<_mm->imeminf; i++) {
		struct meminf *mi = _mm->meminfs+i;
		dealloc(mc->ctx, (void *)mi->ptr, __FILE__, __LINE__);
		lc->log(lc->ctx, "[%s:%s()] non-deallocted memory %p, size %zu, allocated at file %s line %d.\n", _mm->ident, __func__, mi->ptr, mi->size, mi->file, mi->line);
	}
	dealloc(mc->ctx, _mm->meminfs, __FILE__, __LINE__);
	_mm->memsize = 0;
	_mm->imeminf = _mm->nmeminf = 0;
	_mm->meminfs = NULL;
}

