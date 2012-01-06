#include "memory.h"
#include "helper.h"
#include <stdio.h>
#include <assert.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

struct errenv {
	FILE *out;
	jmp_buf env;
};

static int
_err(void *ctx, const char *fmt, ...) {
	struct errenv *e = ctx;

	va_list args;
	va_start(args, fmt);
	vfprintf(e->out, fmt, args);
	va_end(args);

	longjmp(e->env, 1);
	return 0;
}

static int
_log(void *ctx, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int n = vfprintf(ctx, fmt, args);
	va_end(args);
	return n;
}

static void *
_alloc(void *ctx, size_t size) {
	(void)ctx;
	return malloc(size);
}

static void *
_zalloc(void *ctx, size_t size) {
	void *ptr = _alloc(ctx, size);
	if (ptr != NULL) {
		memset(ptr, 0, size);
	}
	return ptr;
}

static void *
_realloc(void *ctx, void *ptr, size_t size) {
	(void)ctx;
	return realloc(ptr, size);
}

static void
_dealloc(void *ctx, void *ptr) {
	(void)ctx;
	free(ptr);
}

static void
test_memory_error(void) {
	struct memface mc = {.alloc = (MemfaceAllocFunc_t)_alloc, .dealloc = (MemfaceDeallocFunc_t)_dealloc};
	struct logface lc = {.ctx = stdout, .log = _log};
	struct errenv e;
	e.out = stderr;
	struct errface ec = {.ctx = &e, .err = _err};

	memory_t mm;
	memory_init(mm, &mc, &lc, &ec);
	if (setjmp(e.env) == 0) {
		memory_dealloc(mm, (void *)12, __FILE__, __LINE__);
	} else {
		_log(stderr, "Caught dealloc error.\n");
	}
	if (setjmp(e.env) == 0) {
		int any_number = 33;
		memory_realloc(mm, (void *)12, any_number, __FILE__, __LINE__);
	} else {
		_log(stderr, "Caught realloc error.\n");
	}
	memory_fini(mm);
}

static void
test_memory_memctx(void) {
	struct memface mc = {.alloc = (MemfaceAllocFunc_t)_alloc, .dealloc = (MemfaceDeallocFunc_t)_dealloc};
	struct logface lc = {.ctx = stdout, .log = _log};
	struct errenv e;
	e.out = stderr;
	struct errface ec = {.ctx = &e, .err = _err};

	memory_t mm;
	struct memface *m = memory_init(mm, &mc, &lc, &ec);
	if (setjmp(e.env) == 0) {
		void *ptr = m->alloc(m->ctx, 0x20, __FILE__, __LINE__);
		assert(ptr != NULL);
		ptr = m->realloc(m->ctx, ptr, 0x50, __FILE__, __LINE__);
		assert(ptr != NULL);
		m->dealloc(m->ctx, ptr, __FILE__, __LINE__);

		ptr = m->zalloc(m->ctx, 0x30, __FILE__, __LINE__);
		assert(ptr != NULL);
		m->dealloc(m->ctx, ptr, __FILE__, __LINE__);
	} else {
		assert(0);
	}
	memory_fini(mm);
}

static void
test_memory_fini(void) {
	struct memface mc = {.alloc = (MemfaceAllocFunc_t)_alloc, .dealloc = (MemfaceDeallocFunc_t)_dealloc};
	struct logface lc = {.ctx = stdout, .log = _log};
	struct errenv e;
	e.out = stderr;
	struct errface ec = {.ctx = &e, .err = _err};

	memory_t mm;
	memory_init(mm, &mc, &lc, &ec);

	void *ptr;
	ptr = memory_alloc(mm, 11, __FILE__, __LINE__);
	ptr = memory_alloc(mm, 22, __FILE__, __LINE__);
	ptr = memory_alloc(mm, 33, __FILE__, __LINE__);
	memory_dealloc(mm, ptr, __FILE__, __LINE__);
	ptr = memory_alloc(mm, 44, __FILE__, __LINE__);
	ptr = memory_alloc(mm, 55, __FILE__, __LINE__);
	ptr = memory_alloc(mm, 66, __FILE__, __LINE__);
	ptr = memory_alloc(mm, 77, __FILE__, __LINE__);
	memory_dealloc(mm, ptr, __FILE__, __LINE__);
	ptr = memory_alloc(mm, 88, __FILE__, __LINE__);
	ptr = memory_alloc(mm, 99, __FILE__, __LINE__);
	ptr = memory_alloc(mm, 111, __FILE__, __LINE__);
	ptr = memory_alloc(mm, 222, __FILE__, __LINE__);
	ptr = memory_alloc(mm, 333, __FILE__, __LINE__);
	memory_dealloc(mm, ptr, __FILE__, __LINE__);
	ptr = memory_alloc(mm, 444, __FILE__, __LINE__);
	ptr = memory_alloc(mm, 555, __FILE__, __LINE__);
	ptr = memory_alloc(mm, 666, __FILE__, __LINE__);
	ptr = memory_alloc(mm, 777, __FILE__, __LINE__);
	ptr = memory_alloc(mm, 888, __FILE__, __LINE__);
	ptr = memory_alloc(mm, 999, __FILE__, __LINE__);
	ptr = memory_alloc(mm, 1111, __FILE__, __LINE__);
	memory_dealloc(mm, ptr, __FILE__, __LINE__);

	memory_fini(mm);
}

static void
test_memory_mixed(void) {
	struct memface mc = {.alloc = (MemfaceAllocFunc_t)_alloc, .dealloc = (MemfaceDeallocFunc_t)_dealloc};
	struct logface lc = {.ctx = stdout, .log = _log};
	struct errenv e;
	e.out = stderr;
	struct errface ec = {.ctx = &e, .err = _err};

	memory_t mm;
_restart:
	memory_init(mm, &mc, &lc, &ec);
	if (setjmp(e.env) == 0) {
		void *ptr0, *ptr1, *ptr2, *ptr3;
		memory_alloc(mm, 19, __FILE__, __LINE__);
		ptr0 = memory_zalloc(mm, 29, __FILE__, __LINE__);
		memory_alloc(mm, 57, __FILE__, __LINE__);
		memory_alloc(mm, 99, __FILE__, __LINE__);
		ptr1 = memory_zalloc(mm, 333, __FILE__, __LINE__);
		memory_zalloc(mm, 200, __FILE__, __LINE__);
		memory_alloc(mm, 1024, __FILE__, __LINE__);
		memory_alloc(mm, 7, __FILE__, __LINE__);
		ptr1 = memory_realloc(mm, ptr1, 458, __FILE__, __LINE__);
		ptr2 = memory_alloc(mm, 3, __FILE__, __LINE__);
		memory_alloc(mm, 2222, __FILE__, __LINE__);
		memory_zalloc(mm, 123, __FILE__, __LINE__);
		memory_alloc(mm, 89, __FILE__, __LINE__);
		ptr3 = memory_alloc(mm, 45, __FILE__, __LINE__);
		ptr1 = memory_realloc(mm, ptr1, 457, __FILE__, __LINE__);
		memory_dealloc(mm, ptr0, __FILE__, __LINE__);
		memory_alloc(mm, 444, __FILE__, __LINE__);
		memory_zalloc(mm, 21, __FILE__, __LINE__);
		memory_dealloc(mm, ptr3, __FILE__, __LINE__);
		memory_alloc(mm, 31, __FILE__, __LINE__);
		memory_dealloc(mm, ptr1, __FILE__, __LINE__);
		memory_dealloc(mm, ptr2, __FILE__, __LINE__);
	} else {
		assert(0);
	}
	memory_fini(mm);
	if (mc.realloc == NULL) {
		mc.realloc = (MemfaceReallocFunc_t)_realloc;
		goto _restart;
	}
	if (mc.zalloc == NULL) {
		mc.zalloc = (MemfaceAllocFunc_t)_zalloc;
		goto _restart;
	}
}

int
main(void) {
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	printf("Test memory, start.\n");
	test_memory_error();
	test_memory_memctx();
	test_memory_fini();
	test_memory_mixed();
	printf("Test memory, done.\n");
	return 0;
}
