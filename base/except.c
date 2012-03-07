#include "except.h"
#include "report.h"
#include "compat.h"
#include <stddef.h>
#include <stdbool.h>

struct exceptenv {
	jmp_buf env;	// env must be the first member.
	struct exceptenv *prev;
	Exception_t excp;
	const char *spec;
	const char *func;
	const char *file;
	int line;
};

typedef enum except_state (*except_setup_t)(struct exceptenv *ee);

except_setup_t exceptenv_setup = (except_setup_t)setjmp;

static const struct exceptenv *ExceptFrame;
static struct exceptenv *ExceptStack;

size_t
exceptenv_size(void) {
	return sizeof(struct exceptenv);
}

static inline const struct exceptenv *
exceptenv_frame(void) {
	return ExceptFrame;
}

static inline void
exceptenv_throw(const struct exceptenv *ee) {
	ExceptFrame = ee;
}

static inline struct exceptenv *
exceptenv_stack(void) {
	return ExceptStack;
}

void
exceptenv_enque(struct exceptenv *ee) {
	assert(ee != NULL);
	ee->prev = ExceptStack;
	ExceptStack = ee;
}

void
exceptenv_deque(struct exceptenv *ee) {
	assert(ee == ExceptStack);
	ExceptStack = ExceptStack->prev;
}

#undef except_throw
#undef except_catch
#undef except_raise
#undef except_report

void
except_throw(const struct exception *ex, const char *spec, const char *func, const char *file, int line) {
	struct exceptenv *ee = exceptenv_stack();
	if (ee == NULL) {
		report_error("ExceptionUncaught:{%s: %s, threw at function %s file %s line %d.}\n",
				except_string(ex), spec, func, file, line);
		report_flush();
		abort();
		return;
	}
	exceptenv_deque(ee);
	exceptenv_throw(ee);

	report_log("ExceptionThrow:{%s: %s, threw at function %s file %s line %d.}\n",
			except_string(ex), spec, func, file, line);

	ee->excp = ex;
	ee->spec = spec;
	ee->func = func;
	ee->file = file;
	ee->line = line;
	longjmp(ee->env, ExceptRaised);
}

bool
except_catch(const struct exception *ex, const char *func, const char *file, int line) {
	const struct exceptenv *ee = exceptenv_frame();
	if (ee->excp == ex) {
		report_log("ExceptionCatch:{%s: %s, threw at function %s file %s line %d, caught at function %s file %s line %d.}\n",
				except_string(ex), ee->spec, ee->func, ee->file, ee->line, func, file, line);
		return true;
	}
	return false;
}

void
except_raise(const char *func, const char *file, int line) {
	const struct exceptenv *ee = exceptenv_frame();
	const struct exception *ex = ee->excp;
	report_log("ExceptionRaise:{%s: %s, threw at function %s file %s line %d, raised at function %s file %s line %d.}\n",
			except_string(ex), ee->spec, ee->func, ee->file, ee->line, func, file, line);
	except_throw(ee->excp, ee->spec, ee->func, ee->file, ee->line);
}

int
except_report(const char *func, const char *file, int line) {
	const struct exceptenv *ee = exceptenv_frame();
	const struct exception *ex = ee->excp;
	return report_report("ExceptionReport:{%s: %s, threw at function %s file %s line %d, reported at function %s file %s line %d.}\n",
			except_string(ex), ee->spec, ee->func, ee->file, ee->line, func, file, line);
}
