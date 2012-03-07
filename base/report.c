#include "logger.h"
#include "report.h"
#include "compat.h"
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

static unsigned Level = ReportLevelDefault;

/* ORDER REPORT LEVEL */
static const char *Names[] = {
	"ERROR",
	"FIXME",
	"REPORT",
	"WARNING",
	"DEBUG",
	"LOG"
};

void
report_setup(unsigned lvl) {
	if (lvl > ReportLevelMaximum) {
		lvl = ReportLevelMaximum;
	}
	Level = lvl;
}

unsigned
report_level(void) {
	return Level;
}

int
report_print(unsigned lvl, const char *func, const char *file, int line, const char *fmt, ...) {
	if (lvl > Level) {
		return 0;
	}

	int (*print)(const char *fmt, ...) = logger_print;
	if (lvl == ReportLevelError) {
		print = logger_error;
	}

	static char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	return print("%s:{Print at function %s file %s line %d.} %s",
			Names[lvl], func, file, line, buffer);
}

void
report_flush(void) {
	logger_flush();
}
