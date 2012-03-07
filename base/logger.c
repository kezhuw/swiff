#include "logger.h"
#include "compat.h"
#include <stdarg.h>
#include <stdbool.h>

static FILE **LoggerStdout = &stdout;
static FILE **LoggerStderr = &stderr;

int
logger_error(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int n = vfprintf(*LoggerStderr, fmt, ap);
	va_end(ap);
	return n;
}

int
logger_print(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt); 
	int n = vfprintf(*LoggerStdout, fmt, ap);
	va_end(ap);
	return n;
}

void
logger_flush(void) {
	fflush(*LoggerStdout);
	fflush(*LoggerStderr);
}
