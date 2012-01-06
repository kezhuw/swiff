#ifndef __HELPER_H
#define __HELPER_H

union align {
	long long l;
	void *ptr;
	void (*fp)(void);
	double d;
	long double ld;
};

#define ALIGNMENT		sizeof(union align)
#define ALIGNMASK		(ALIGNMENT-1)

#define MAKEALIGN(n)		(ALIGNMENT * (((n) + ALIGNMASK)/ALIGNMENT))


typedef void * (*MemfaceAllocFunc_t)(void *ctx, size_t size, const char *file, int line);
typedef void * (*MemfaceReallocFunc_t)(void *ctx, void *ptr, size_t size, const char *file, int line);
typedef void (*MemfaceDeallocFunc_t)(void *ctx, void *ptr, const char *file, int line);
// All implementations of memface must implement alloc/dealloc.
struct memface {
	void *ctx;
	MemfaceAllocFunc_t alloc;
	MemfaceAllocFunc_t zalloc;
	MemfaceReallocFunc_t realloc;
	MemfaceDeallocFunc_t dealloc;
};

typedef int (*ErrfaceErrFunc_t)(void *ctx, const char *fmt, ...);
struct errface {
	void *ctx;
	ErrfaceErrFunc_t err;
};

typedef int (*LogfaceLogFunc_t)(void *ctx, const char *fmt, ...);
struct logface {
	void *ctx;
	LogfaceLogFunc_t log;
};

#endif
