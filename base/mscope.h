#ifndef __MSCOPE_H
#define __MSCOPE_H

#include <stddef.h>
#include "helper.h"

struct mscope;

// mc is a standard memface.
struct mscope *mscope_create(const struct memface *mc);

// All functions below, assert(mo != NULL).

// Free all memory allocated from mo, and delete mo.
void mscope_delete(struct mscope *mo);

// Reclaim cached memory.
void mscope_collect(struct mscope *mo);

// Global memory lives until delete(mo).
void *mscope_alloc_global(struct mscope *mo, size_t size);

// Enter a local scope, return value is this scope's signature.
void *mscope_enter_local(struct mscope *mo);

// Alloc memory from local scope.
void *mscope_alloc_local(struct mscope *mo, size_t size);

// Variable argument list are null-terminated pointers pointing to return values
// from alloc_local(). The purpose of var-args is to provide the opportunity to
// access memory outside the scope where it been allocated. After leave_local(),
// these memories are still available, but with different addresses. It is valid
// to pass a pointer pointing to memory allocated by parent's scope or even
// other allocator.
//
// e.g.
// void *sign = mscope_enter_local(mo);
// void *ptr = mscope_alloc_local(mo, 33);
// ...
// mscope_leave_local(mo, sign, &ptr, NULL);
void mscope_leave_local(struct mscope *mo, void *sign, ...);
#endif
