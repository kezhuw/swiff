#include "slab.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

struct stat {
	size_t peak;
	size_t size;
};

static struct stat memstat;

static void *
alloc(struct stat *mt, size_t size) {
	mt->size += size;
	if (mt->size > mt->peak) {
		mt->peak = mt->size;
	}
	char *ptr = malloc(size+sizeof(size_t));
	*((size_t *)ptr) = size;
	return ptr+sizeof(size_t);
}

static void
dealloc(struct stat *mt, void *p) {
	char *ptr = (char *)p - sizeof(size_t);
	size_t size = *((size_t *)ptr);
	if (mt->size < size) {
		fprintf(stderr, "free %zu, but only size %zu peak %zu.\n", size, mt->size, mt->peak);
		abort();
	}
	mt->size -= size;
	free(ptr);
}

static struct memface memory = {.ctx = &memstat, .alloc = (MemfaceAllocFunc_t)alloc, .dealloc = (MemfaceDeallocFunc_t)dealloc};

struct custom {
	size_t isize;
	size_t nitem;
	size_t times;
	bool dealloc;
};

static struct custom customs[] = {
	{2*sizeof(void *), 10, 131, false},
	{sizeof(struct custom), 30, 283, true},
	{sizeof(struct memface), 21, 393, true},
	{2*sizeof(struct memface), 17, 441, false},
	{sizeof(struct memface) + sizeof(struct custom), 47, 333, false},
	{31, 19, 7, false},
	{17, 77, 11, true},
	{43, 21, 123, false},
	{88, 33, 34, true},
	{0, 0, 0, false}
};

static void
slab_alloc_times(struct slab *sa, size_t n) {
	size_t i;
	for (i=0; i<n; i++) {
		void *ptr = slab_alloc(sa);
		if ((i%5 == 0) || (i%13 == 0)) {
			slab_dealloc(sa, ptr);
		}
	}
}

static void
slab_alloc_custom(struct custom *cm) {
	size_t i;
	for (i=0; cm[i].isize != 0; i++) {
		struct slab *sa = slab_create(&memory, cm[i].nitem, cm[i].isize);
		slab_alloc_times(sa, cm[i].times);
		slab_delete(sa);
	}
}

static void
slab_pool_alloc_custom(struct slab_pool *so, struct custom *cm) {
	size_t i;
	for (i=0; cm[i].isize != 0; i++) {
		struct slab *sa = slab_pool_alloc(so, cm[i].nitem, cm[i].isize);
		slab_alloc_times(sa, cm[i].times);
		if (cm[i].dealloc) {
			slab_pool_dealloc(so, sa);
		}
	}
}

static void
slab_ensure_zerosize(const char *ident) {
	if (memstat.size != 0) {
		fprintf(stderr, "%s fail, leak %zu size memory, peak size %zu .\n", ident, memstat.size, memstat.peak);
		abort();
	}
}

static void
TestSlab(void) {
	slab_alloc_custom(customs);
	slab_ensure_zerosize(__func__);
}

static void
TestSlabPool(void) {
	struct slab_pool *so = slab_pool_create(&memory, 5);
	slab_pool_alloc_custom(so, customs);
	slab_pool_delete(so);
	slab_ensure_zerosize(__func__);
}

int
main(void) {
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	TestSlab();
	TestSlabPool();

	return 0;
}
