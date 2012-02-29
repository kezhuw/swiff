#ifndef __MEMORY_H
#define __MEMORY_H

#include <stddef.h>

struct memface;
struct logface;
struct errface;

// Memory is a full-fledged memface.
struct memory;

// Arguments mc, lc, ec must be valid memface, logface, errface, respectively.
void *memory_create(const struct memface *mc, const struct logface *lc, const struct errface *ec, const char *ident);

// Delete mm.
// If there are memories alive when delete(), dealloc() and log() it.
void memory_delete(void *mm);

// Amount of memory allocated by mm.
size_t memory_amount(const struct memory *mm);

// Return ptr, if ptr was allocated by mm, otherwise NULL.
void *memory_reside(const struct memory *mm, const void *ptr);

// Log() memory utilization.
int memory_status(const struct memory *mm);

// Log() about ptr.
int memory_report(const struct memory *mm, const void *ptr);

void *memory_alloc(struct memory *mm, size_t size, const char *file, int line);
void *memory_zalloc(struct memory *mm, size_t size, const char *file, int line);

// If argument ptr to realloc/dealloc isn't returned by mm,
// err() this and then abort().
void *memory_realloc(struct memory *mm, void *ptr, size_t size, const char *file, int line);
void memory_dealloc(struct memory *mm, void *ptr, const char *file, int line);

#endif
