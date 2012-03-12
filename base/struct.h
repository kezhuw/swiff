#ifndef __BASE_STRUCT_H
#define __BASE_STRUCT_H

#include <stdint.h>

struct rgba8 {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha;
};

struct rgba16 {
	uint16_t red;
	uint16_t green;
	uint16_t blue;
	uint16_t alpha;
};
#endif
