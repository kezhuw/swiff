#ifndef __MEMORY_H
#define __MEMORY_H

#include "except.h"
#include <stddef.h>

extern const Exception_t ExceptionMemory;

// Convenient macros make they similar to standant malloc API.
//
// Same constrains to standand malloc API, except that when ptr to
// realloc/memory_dealloc is invalid, ExceptionMemory will be thrown.
#define memory_alloc(size)		((memory_alloc)((size), __func__, __FILE__, __LINE__))
#define memory_zalloc(size)		((memory_zalloc)((size), __func__, __FILE__, __LINE__))
#define memory_realloc(ptr, size)	((memory_realloc)((ptr), (size), __func__, __FILE__, __LINE__))
#define memory_dealloc(ptr)		((memory_dealloc)((ptr), __func__, __FILE__, __LINE__))

#define memory_report(ptr, msg)		((memory_report)(ptr, msg, __func__, __FILE__, __LINE__))

size_t memory_amount(void);
// Write current memory status to 'status' parameter.
int memory_status(char *status, size_t size);
int (memory_report)(const void *ptr, const char *msg, const char *func, const char *file, int line);

void *(memory_alloc)(size_t size, const char *func, const char *file, int line);
void *(memory_zalloc)(size_t size, const char *func, const char *file, int line);
void *(memory_realloc)(void *ptr, size_t size, const char *func, const char *file, int line);
void (memory_dealloc)(void *ptr, const char *func, const char *file, int line);

void memory_fini(void);

#endif
