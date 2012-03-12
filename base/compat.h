#ifndef __COMPAT_H
#define __COMPAT_H

// Platform dependent header file.

// Support only full-os currently.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <assert.h>

#if defined(BYTE_ORDER) && defined(LITTLE_ENDIAN) && defined(BIG_ENDIAN)
#else
#define LITTLE_ENDIAN		0x1234
#define BIG_ENDIAN		0x3412
#define BYTE_ORDER		LITTLE_ENDIAN
#endif

#if defined(__ILP32__) || defined(__LP64__)
#else
#define __ILP32__
#endif

#endif
