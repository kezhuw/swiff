#ifndef __CORE_STREAM_H
#define __CORE_STREAM_H

#include <stdint.h>

struct pxface;
struct dictionary;

struct stream {
	int type;
	int version;
	struct dictionary *dictionary;
	struct pxface *pxface;
	uintptr_t resource;
	uintptr_t userdef;
};

#endif
