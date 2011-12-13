#ifndef __SCOPE_H
#define __SCOPE_H

#include <stddef.h>

typedef struct {
	void *_[20];
} mscope_t[1];

typedef void *(*MscopeAlloc_t)(void *memctx, size_t size);
typedef void (*MscopeDealloc_t)(void *memctx, void *ptr);

// Memory returned from alloc() must be properly aligned.
void mscope_init(mscope_t mo, MscopeAlloc_t alloc, MscopeDealloc_t dealloc, void *memctx);
void mscope_fini(mscope_t mo);
// Reclaim cached memory.
void mscope_collect(mscope_t mo);

// Global memory lives until fini().
void *mscope_alloc_global(mscope_t mo, size_t size);
void *mscope_alloc_local(mscope_t mo, size_t size);

// Return value is the secend argument of leave_local.
void *mscope_enter_local(mscope_t mo);
// Variable argument list are null-terminated pointers pointing to return values
// from alloc_local(). The purpose of var-args is to provide the opportunity to
// access memory outside the scope where it been allocated. After leave_local(),
// these memories are still available, but with different addresses. It is valid
// to pass a pointer pointing to memory allocated by parent's scope or even
// other allocator.
//
// e.g.
// void *mark = mscope_enter_local(mo);
// void *ptr = mscope_alloc_local(mo, 33);
// ...
// mscope_leave_local(mo, mark, &ptr, NULL);
void mscope_leave_local(mscope_t mo, void *mark, ...);
#endif
