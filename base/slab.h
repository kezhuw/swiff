#ifndef __SLAB_POOL_H
#define __SLAB_POOL_H
#include <stddef.h>
#include "helper.h"
struct slab;
struct slab_pool;

// Assert(nitem > 0).
struct slab_pool *slab_pool_create(struct memface *mc, size_t nitem);

// Free slab_pool and delete all alive slabs allocated from it.
void slab_pool_delete(struct slab_pool *so);

// Assert(isize >= void_pointer_size).
struct slab *slab_pool_alloc(struct slab_pool *so, size_t nitem, size_t isize);

// Free all memory allocated from this slab and dealloc it.
void slab_pool_dealloc(struct slab_pool *so, struct slab *sa);

void *slab_alloc(struct slab *sa);
void slab_dealloc(struct slab *sa, void *ptr);

struct slab *slab_create(struct memface *mc, size_t nitem, size_t isize);

// Free slab and all memory allocated from it.
void slab_delete(struct slab *sa);
#endif
