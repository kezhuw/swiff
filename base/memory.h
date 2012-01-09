#ifndef __MEMORY_H
#define __MEMORY_H

#include "helper.h"
#include <stddef.h>
#include <stdbool.h>

typedef struct {
	struct memface _1[2];
	struct logface _2[1];
	struct errface _3[1];
	void *_[10];
} memory_t[1];

// Arguments mc, lc, ec must be valid memface, logface, errface, respectively.
// After init(), mm is a valid memface with all functions implemented.
struct memface *memory_init(memory_t mm, const struct memface *mc, const struct logface *lc, const struct errface *ec, const char *ident);
// Finalizes mm to initialized state.
// If there are memories alive when fini(), we dealloc() and log() it.
void memory_fini(memory_t mm);

size_t memory_amount(memory_t mm);
// Return ptr, if ptr was allocated by mm, otherwise NULL.
void *memory_reside(memory_t mm, const void *ptr);
// Log() memory utilization.
int memory_status(memory_t mm);
// Log() about ptr.
int memory_report(memory_t mm, const void *ptr);

void *memory_alloc(memory_t mm, size_t size, const char *file, int line);
void *memory_zalloc(memory_t mm, size_t size, const char *file, int line);
// If argument ptr to realloc/dealloc isn't returned by memface mm,
// err() this and then abort().
void *memory_realloc(memory_t mm, void *ptr, size_t size, const char *file, int line);
void memory_dealloc(memory_t mm, void *ptr, const char *file, int line);

#endif
