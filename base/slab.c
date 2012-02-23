#include <stddef.h>
#include <stdint.h>
#include "slab.h"
#include "multip.h"
#include "helper.h"

struct next {
	struct next *next;
};

union slot {
	union slot *next;
	void *ptr;
	void (*func)(void);
};

#define assert_isize(isize)	(assert((isize >= sizeof(union slot)) || !"item size too small"))
#define assert_nitem(nitem)	(assert((nitem > 0) || !"nitem must great than zero"))

typedef uintptr_t invar_t;

#define slab_invariant(sa)	((invar_t)((sa)->dealloc))

#define assert_invar()		assert((sizeof(union slot *) == sizeof(MemfaceDeallocFunc_t)) || !"sorry for architecture dependent")
struct slab {
	MemfaceDeallocFunc_t dealloc;
	void *memctx;
	MemfaceAllocFunc_t alloc;
	size_t blocksize;
	size_t chunksize;
	char *allocpos;
	char *sentinel;
	union slot *frees;
	struct next *ahead;
	struct next first;
};

static void
slab_init(struct slab *sa, size_t blocksize, size_t totalsize) {
	sa->blocksize = blocksize;
	sa->chunksize = totalsize + sizeof(struct next);
	sa->allocpos = (char *)(sa+1);
	sa->sentinel = sa->allocpos;
	sa->frees = NULL;
	sa->ahead = &sa->first;
	sa->first.next = NULL;
}

static void
slab_fini(struct slab *sa) {
	MemfaceDeallocFunc_t dealloc = sa->dealloc;
	void *memctx = sa->memctx;
	struct next *i = sa->ahead;
	while (i != &sa->first) {
		struct next *n = i->next;
		dealloc(memctx, i, __FILE__, __LINE__);
		i = n;
	}
}

static void
pool_fini(struct slab *so) {
	invar_t invar = slab_invariant(so);
	size_t chunksize = so->chunksize;
	struct next *n = so->ahead;
	char *revptr = so->allocpos;
	for (;;) {
		char *begptr = (char *)(n+1);
		while (revptr > begptr) {
			revptr -= sizeof(struct slab);
			struct slab *sa = (struct slab *)revptr;
			if (invar == slab_invariant(sa)) {
				slab_fini(sa);
			}
		}
		assert(revptr == begptr);
		if (n == &so->first) {
			break;
		}
		n = n->next;
		revptr = (char *)n + chunksize;
	}
	slab_fini(so);
}

struct slab *
slab_create(struct memface *mc, size_t nitem,  size_t isize) {
	assert_isize(isize);
	assert_nitem(nitem);
	size_t totalsize = isize*nitem;
	struct slab *sa = mc->alloc(mc->ctx, totalsize + sizeof(struct slab), __FILE__, __LINE__);
	slab_init(sa, isize, totalsize);
	sa->sentinel = (char *)(sa+1) + totalsize;
	sa->memctx = mc->ctx;
	sa->alloc = mc->alloc;
	sa->dealloc = mc->dealloc;
	return sa;
}

void
slab_delete(struct slab *sa) {
	slab_fini(sa);
	sa->dealloc(sa->memctx, sa, __FILE__, __LINE__);
}

void *
slab_alloc(struct slab *sa) {
	if (sa->frees != NULL) {
		union slot *ptr = sa->frees;
		sa->frees = ptr->next;
		return ptr;
	}
	if (sa->allocpos == sa->sentinel) {
		struct next *n = sa->alloc(sa->memctx, sa->chunksize, __FILE__, __LINE__);
		n->next = sa->ahead;
		sa->ahead = n;
		sa->allocpos = (char *)(n+1);
		sa->sentinel = (char *)n + sa->chunksize;
	}
	void *ptr = sa->allocpos;
	sa->allocpos += sa->blocksize;
	return ptr;
}

void
slab_dealloc(struct slab *sa, void *ptr) {
	union slot *p = ptr;
	p->next = sa->frees;
	sa->frees = p;
}

struct slab_pool *
slab_pool_create(struct memface *mc, size_t nitem) {
	assert_nitem(nitem);
	assert_invar();
	struct slab_pool *so = (struct slab_pool *)slab_create(mc, nitem, sizeof(struct slab));
	return so;
}

void
slab_pool_delete(struct slab_pool *so) {
	struct slab *po = (struct slab *)so;
	pool_fini(po);
	po->dealloc(po->memctx, po, __FILE__, __LINE__);
}

struct slab *
slab_pool_alloc(struct slab_pool *so, size_t nitem, size_t isize) {
	assert_isize(isize);
	assert_nitem(nitem);
	struct slab *sa = slab_alloc((struct slab *)so);
	slab_init(sa, isize, isize*nitem);

	struct slab *po = (struct slab *)so;
	sa->memctx = po->memctx;
	sa->alloc = po->alloc;
	sa->dealloc = po->dealloc;
	return sa;
}

void
slab_pool_dealloc(struct slab_pool *so, struct slab *sa) {
	slab_fini(sa);
	slab_dealloc((struct slab *)so, sa);
}
