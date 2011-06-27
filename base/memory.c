#include "memory.h"
#include "except.h"
#include "report.h"
#include "multip.h"
#include <stddef.h>
#include <stdbool.h>

const Exception_t ExceptionMemory = except_define("Memory error");

#define mmstat_define(ptr, size, func, file, line)	(&(const struct mmstat) {ptr, size, func, file, line})
struct mmstat {
	const void *ptr;
	size_t size;
	const char *func;
	const char *file;
	int line;
};

static size_t Statn;
static struct mmstat *Stats;

static size_t MemorySize;
static size_t MstatsSize;

static inline bool
mmstat_expand(void) {
	if (((Statn+1) ^ Statn) > Statn) {
		void *data = realloc(Stats, sizeof(struct mmstat) * (2*Statn + 1));
		if (data == NULL) {
			return false;
		}
		Stats = data;
		MstatsSize = sizeof(struct mmstat) *(2*Statn + 1);
	}
	return true;
}

// *st valid until  mmstat_insert()/mmstat_delete().
static bool
mmstat_search(const void *ptr, struct mmstat **st) {
	assert(st != NULL);
	for (size_t i=0; i<Statn; i++) {
		if (Stats[i].ptr == ptr) {
			*st = &Stats[i];
			return true;
		}
	}
	return false;
}

static inline bool
mmstat_insert(const struct mmstat *st) {
	if (!mmstat_expand()) {
		return false;
	}
	Stats[Statn++] = *st;
	return true;
}

// Ensure *st is returned by mmstat_search(), and there is no mmstat_insert()
// or/and mmstat_delete() invokes between mmstat_update() and mmstat_search().
static inline void
mmstat_update(struct mmstat *st, const struct mmstat *ch) {
	assert(st >= Stats && st < Stats+Statn);
	*st = *ch;
}

static inline void
mmstat_delete(struct mmstat *st) {
	assert(Statn > 0);
	assert(st >= Stats && st < Stats+Statn);

	Statn--;
	memmove(st, st+1, (&Stats[Statn] - st) * sizeof(struct mmstat));
}

static inline void
mmstat_deinit(void (*dealloc)(void *)) {
	struct mmstat * restrict ss = Stats;
	for (size_t i=0; i<Statn; i++) {
		dealloc((void *)ss[i].ptr);
	}

	free(Stats);
	Statn = 0;
	Stats = NULL;
	MstatsSize = 0;
}

static inline size_t
mmstat_mmsize(const struct mmstat *st) {
	return st->size;
}

static inline size_t
mmstat_amount(void) {
	return MstatsSize;
}

size_t
memory_amount(void) {
#ifdef MEMORY_STATS_SUPPORT
	return MemorySize;
#endif
	return (size_t)-1;
}

static inline int
mmstat_report(const struct mmstat *st, const char *msg, const char *func, const char *file, int line) {
	return report_print(ReportLevelReport, func, file, line, "Memory:{%s: memory %p size %zu, allocated at function %s file %s line %d.}\n",
			msg, st->ptr, st->size, st->func, st->file, st->line);
}

int
(memory_report)(const void *ptr, const char *msg, const char *func, const char *file, int line) {
#ifdef MEMORY_STATS_SUPPORT
	struct mmstat *st;
	if (mmstat_search(ptr, &st)) {
		return mmstat_report(st, msg, func, file, line);
	}
	return 0;
#else
	return report_print(ReportLevelReport, func, file, line, "Memory:{%s: memory %p.}\n", msg, ptr);
#endif
}

int
memory_status(char *status, size_t size) {
	return snprintf(status, size, "Memory:{Memory status: %zu bytes allocated by clients, %zu bytes used by memory module itself.}", memory_amount(), mmstat_amount());
}

#undef memory_alloc
#undef memory_zalloc
#undef memory_realloc
#undef memory_dealloc

// C99 malloc(0)
// If the size of the space requested is zero, the behavior is implementation-defined:
// either a null pointer is returned, or the behavior is as if the size were some nonzero value,
// except that the returned pointer shall not be used to access no object.
void *
memory_alloc(size_t size, const char *func, const char *file, int line) {
	void *ptr = malloc(size);
#ifdef MEMORY_STATS_SUPPORT
	if (ptr != NULL) {
		if (!mmstat_insert(mmstat_define(ptr, size, func, file, line))) {
			free(ptr);
			ptr = NULL;
		}
		else {
			MemorySize += size;
		}
	}
#endif
	return ptr;
}

void *
memory_zalloc(size_t size, const char *func, const char *file, int line) {
	void *ptr = memory_alloc(size, file, func, line);
	if (ptr != NULL) {
		memset(ptr, 0, size);
	}
	return ptr;
}

// Invoke realloc() directly to reserve	implementation-defined behavior of realloc.
void *
memory_realloc(void *ptr, size_t size, const char *func, const char *file, int line) {
#ifdef MEMORY_STATS_SUPPORT
	struct mmstat *st;
	if (ptr != NULL && !mmstat_search(ptr, &st)) {
		(except_throw)(ExceptionMemory, "invalid memory address", func, file, line);
	}
#endif
	void *ret = realloc(ptr, size);
#ifdef MEMORY_STATS_SUPPORT
	if (ret == NULL) {
		if (size == 0 && ptr != NULL) {
			MemorySize -= mmstat_mmsize(st);
			mmstat_delete(st);
		}
	}
	else {
		const struct mmstat *re = mmstat_define(ret, size, func, file, line);
		if (ptr == NULL) {
			if (!mmstat_insert(re)) {
				free(ret);
				ret = NULL;
			}
			else {
				MemorySize += size;
			}
		}
		else {
			MemorySize += size;
			MemorySize -= mmstat_mmsize(st);
			mmstat_update(st, re);
		}
	}
#endif
	return ret;
}

void
memory_dealloc(void *ptr, const char *func, const char *file, int line) {
#ifdef MEMORY_STATS_SUPPORT
	if (ptr != NULL) {
		struct mmstat *st;
		if (!mmstat_search(ptr, &st)) {
			(except_throw)(ExceptionMemory, "invalid memory address", func, file, line);
			return;
		}
		MemorySize -= mmstat_mmsize(st);
		mmstat_delete(st);
	}
#endif
	free(ptr);
}

static void
memory_leak(void *ptr) {
	memory_report(ptr, "Memory leak");
	free(ptr);
}

void
memory_fini(void) {
#ifdef MEMORY_STATS_SUPPORT
	mmstat_deinit(memory_leak);
	MemorySize = 0;
#endif
}
