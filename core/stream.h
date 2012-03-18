#ifndef __CORE_STREAM_H
#define __CORE_STREAM_H

struct stream {
	int type;
	int version;
	struct dictionary *dictionary;
	struct pxface *pxface;
	uintptr_t userdef;
};

#endif
