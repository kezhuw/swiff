#ifndef __BASE_STRUCT_H
#define __BASE_STRUCT_H

#include <stdint.h>

struct rgba8 {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

struct rgba16 {
	uint16_t r;
	uint16_t g;
	uint16_t b;
	uint16_t a;
};
#endif
