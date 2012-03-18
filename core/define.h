#ifndef __CORE_DEFINE_H
#define __CORE_DEFINE_H

#include <base/geometry.h>
#include <base/intreg.h>
#include <stdint.h>

struct sprite_define {
	uintptr_t tagbeg;
	intreg_t nframe;
};

struct stream_define {
	struct rectangle size;
	uint16_t rate;
	uint16_t nframe;
	uintptr_t tagbeg;
};

#endif
