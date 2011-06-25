#ifndef __ASSERT_H
#define __ASSERT_H

#include "except.h"

extern const Exception_t ExceptionAssert;

#undef assert_truth
#define assert_truth(e)		((void)((e) ? (0) : (assert_throw(#e, __func__, __FILE__, __LINE__), 0)))

void assert_throw(const char *spec, const char *func, const char *file, int line);

#endif
