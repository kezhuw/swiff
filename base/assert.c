#include "assert.h"
#include "except.h"

const Exception_t ExceptionAssert = except_define("Assertion failure");

void
assert_throw(const char *spec, const char *func, const char *file, int line) {
	(except_throw)(ExceptionAssert, spec, func, file, line);
}
