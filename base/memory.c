#include "memory.h"
#include "compat.h"
#include "intreg.h"
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

struct memory {
	struct memface iface[1];
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
_expand_meminf(struct memory *mm) {
	size_t n = mm->nmeminf;
	if (mm->imeminf == mm->nmeminf) {
		struct memface *mc = mm->mface;
		size_t newsize = (2*n + 1) * sizeof(struct meminf);
		void *data = NULL;
		if (mc->realloc == NULL) {
			data = mc->alloc(mc->ctx, newsize, __FILE__, __LINE__);
			if (data != NULL) {
				memcpy(data, mm->meminfs, n*sizeof(struct meminf));
				mc->dealloc(mc->ctx, mm->meminfs, __FILE__, __LINE__);
			}
		} else {
			data = mc->realloc(mc->ctx, mm->meminfs, newsize, __FILE__, __LINE__);
		}
		if (data == NULL) {
			return false;
		}
		mm->nmeminf = 2*n + 1;
		mm->meminfs = data;
	}
	return true;
}

// *mi valid until _insert_meminf()/_delete_meminf().
static bool
_search_meminf(const struct memory *mm, const void *ptr, struct meminf **mi) {
	assert(mi != NULL);
	for (size_t i=0, n=mm->imeminf; i<n; i++) {
		if (mm->meminfs[i].ptr == ptr) {
			*mi = &mm->meminfs[i];
			return true;
		}
	}
	return false;
}

static inline bool
_insert_meminf(struct memory *mm, const struct meminf *mi) {
	if (!_expand_meminf(mm)) {
		return false;
	}
	mm->meminfs[mm->imeminf++] = *mi;
	mm->memsize += mi->size;
	return true;
}

// Ensure *mi is returned by _search_meminf(), and there is no _insert_meminf()/
// _delete_meminf() between _update_meminf() and _search_meminf().
static inline void
_update_meminf(struct memory *mm, struct meminf *mi, const struct meminf *ch) {
	assert(mi >= mm->meminfs && mi < mm->meminfs+mm->imeminf);
	mm->memsize += ch->size;
	mm->memsize -= mi->size;
	*mi = *ch;
}

static inline void
_delete_meminf(struct memory *mm, struct meminf *mi) {
	assert(mm->imeminf > 0);
	assert(mm->memsize >= mi->size);
	assert(mi >= mm->meminfs && mi < mm->meminfs+mm->imeminf);

	mm->memsize -= mi->size;
	mm->imeminf--;
	memmove(mi, mi+1, (&mm->meminfs[mm->imeminf] - mi) * sizeof(struct meminf));
}

// If fails, dealloc() ptr and return NULL.
void *
_insert_newptr(struct memory *mm, void *ptr, size_t size, const char *file, int line) {
	const struct meminf *mi = meminf_define(ptr, size, file, line);
	if (!_insert_meminf(mm, mi)) {
		struct memface *mc = mm->mface;
		mc->dealloc(mc->ctx, ptr, __FILE__, __LINE__);
		ptr = NULL;
	}
	return ptr;
}

size_t
memory_amount(const struct memory *mm) {
	return mm->memsize;
}

void *
memory_reside(const struct memory *mm, const void *ptr) {
	struct meminf *mi;
	return (_search_meminf(mm, ptr, &mi) ? (void *)ptr : NULL);
}

int
memory_report(const struct memory *mm, const void *ptr) {
	const struct logface *lc = mm->lface;
	struct meminf *mi;
	if (_search_meminf(mm, ptr, &mi)) {
		return lc->log(lc->ctx, "[%s:%s()] memory %p, size %zu, allocated at file %s line %d.\n", mm->ident, __func__, mi->ptr, mi->size, mi->file, mi->line);
	} else {
		return lc->log(lc->ctx, "[%s:%s()] invalid memory address %p.\n", mm->ident, __func__, ptr);
	}
}

int
memory_status(const struct memory *mm) {
	const struct logface *lc = mm->lface;
	return lc->log(lc->ctx, "[%s:%s()] %zu bytes allocated by clients, %zu bytes used by memory module.\n", mm->ident, __func__, mm->memsize, mm->nmeminf * sizeof(struct meminf));
}

void *
memory_alloc(struct memory *mm, size_t size, const char *file, int line) {
	struct memface *mc = mm->mface;
	void *ptr = mc->alloc(mc->ctx, size, __FILE__, __LINE__);
	if (ptr != NULL) {
		ptr = _insert_newptr(mm, ptr, size, file, line);
	}
	return ptr;
}

void *
memory_zalloc(struct memory *mm, size_t size, const char *file, int line) {
	struct memface *mc = mm->mface;
	if (mc->zalloc != NULL) {
		void *ptr = mc->zalloc(mc->ctx, size, __FILE__, __LINE__);
		if (ptr != NULL) {
			ptr = _insert_newptr(mm, ptr, size, file, line);
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
memory_realloc(struct memory *mm, void *ptr, size_t size, const char *file, int line) {
	if (ptr == NULL) {
		return memory_alloc(mm, size, file, line);
	}
	struct meminf *mi;
	if (!_search_meminf(mm, ptr, &mi)) {
		struct errface *ec = mm->eface;
		ec->err(ec->ctx, "[%s:%s()] invalid memory address %p, reallocated at file %s line %d.\n", mm->ident, __func__, ptr, file, line);
		abort();
		return NULL;
	}
	struct memface *mc = mm->mface;
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
			_update_meminf(mm, mi, re);
		}
		return ret;
	}

	void *ret = mc->realloc(mc, ptr, size, __FILE__, __LINE__);
	if (ret == NULL) {
		if (size == 0) {
			_delete_meminf(mm, mi);
		}
	} else {
		const struct meminf *re = meminf_define(ret, size, file, line);
		_update_meminf(mm, mi, re);
	}
	return ret;
}

void
memory_dealloc(struct memory *mm, void *ptr, const char *file, int line) {
	if (ptr != NULL) {
		struct meminf *mi;
		if (!_search_meminf(mm, ptr, &mi)) {
			struct errface *ec = mm->eface;
			ec->err(ec->ctx, "[%s:%s()] invalid memory address %p, deallocated at file %s line %d.\n", mm->ident, __func__, ptr, file, line);
			abort();
			return;
		}
		_delete_meminf(mm, mi);
	}
	struct memface *mc = mm->mface;
	mc->dealloc(mc, ptr, __FILE__, __LINE__);
}

void *
memory_create(const struct memface *mc, const struct logface *lc, const struct errface *ec, const char *ident) {
	assert(mc != NULL);
	assert(lc != NULL);
	assert(ec != NULL);
	assert(mc->alloc != NULL && mc->dealloc != NULL);
	assert(lc->log != NULL);
	assert(ec->err != NULL);
	assert(ident != NULL);
	struct memory *mm = mc->alloc(mc->ctx, sizeof(*mm), __FILE__, __LINE__);
	if (mm != NULL) {
		mm->mface[0] = *mc;
		mm->lface[0] = *lc;
		mm->eface[0] = *ec;
		mm->memsize = 0;
		mm->imeminf = mm->nmeminf = 0;
		mm->meminfs = NULL;
		mm->ident = ident;
		struct memface *mc = mm->iface;
		mc->ctx = mm;
		mc->alloc = (MemfaceAllocFunc_t)memory_alloc;
		mc->zalloc = (MemfaceAllocFunc_t)memory_zalloc;
		mc->realloc = (MemfaceReallocFunc_t)memory_realloc;
		mc->dealloc = (MemfaceDeallocFunc_t)memory_dealloc;
	}
	return mm;
}

void
memory_delete(void *_m) {
	struct memory *mm = _m;
	struct memface *mc = mm->mface;
	struct logface *lc = mm->lface;
	MemfaceDeallocFunc_t dealloc = mc->dealloc;
	for (size_t i=0; i<mm->imeminf; i++) {
		struct meminf *mi = mm->meminfs+i;
		dealloc(mc->ctx, (void *)mi->ptr, __FILE__, __LINE__);
		lc->log(lc->ctx, "[%s:%s()] non-deallocted memory %p, size %zu, allocated at file %s line %d.\n", mm->ident, __func__, mi->ptr, mi->size, mi->file, mi->line);
	}
	dealloc(mc->ctx, mm->meminfs, __FILE__, __LINE__);
	mm->memsize = 0;
	mm->imeminf = mm->nmeminf = 0;
	mm->meminfs = NULL;
}
