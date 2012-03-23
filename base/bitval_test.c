#include "bitval.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

// bitval_test_integer formula:
// 	bitval_read_bit(b);			results[0]
// 	bitval_read_sbits(b, 5);		results[1]
// 	bitval_sync(b);
//
// 	bitval_read_bigendian_uint16(b);	results[2]
//	bitval_read_sbits(b, 5);		results[3]
//	bitval_read_ubits(b, 7);		results[4]
//	bitval_sync(b);
//
//	n = bitval_read_ubits(b, 4);		results[5]
//	bitval_read_sbits(b, n);		results[6]
//	bitval_read_sbits(b, n);		results[7]
//	bitval_sync(b);
//
//	bitval_read_int32(b);			results[8]
//	bitval_read_sbits(b, 24);		results[9]
//	bitval_read_ubits(b, 30);		results[10]
//	bitval_sync(b);
//
//	bitval_read_int8(b);			results[11]
//	bitval_read_int16(b);			results[12]
//	bitval_read_int16(b);			results[13]
//	bitval_read_int8(b);			results[14]
//	bitval_read_int32(b);			results[15]
//	bitval_read_uint8(b);			results[16]
//	bitval_read_uint16(b);			results[17]
//	bitval_read_uint8(b);			results[18]
//	bitval_read_uint16(b);			results[19]
//	bitval_read_uint32(b);			results[20]
//	bitval_read_bigendian_int8(b);		results[21]
//	bitval_read_bigendian_int16(b);		results[22]
//	bitval_read_bigendian_int16(b);		results[23]
//	bitval_read_bigendian_int8(b);		results[24]
//	bitval_read_bigendian_int32(b);		results[25]
//	bitval_read_bigendian_uint8(b);		results[26]
//	bitval_read_bigendian_uint16(b);	results[27]
//	bitval_read_bigendian_uint16(b);	results[28]
//	bitval_read_bigendian_uint8(b);		results[29]
//	bitval_read_bigendian_uint32(b);	results[30]

static uint8_t g_integer_data[] = {0x45, 0x58, 0xC7, 0xD5, 0x9E, 0xB3, 0x77, 0x8D, 0x9E, 0x2A, 0xB5, 0x47, 0x38, 0x99, 0xBB, 0x44, 0x77, 0x66, 0xBF, 0xA7, 0x80, 0x80, 0x40, 0x7F, 0x08, 0x9F, 0x44, 0x55, 0x66, 0x77, 0x78, 0x64, 0x7E, 0x37, 0x5C, 0x3B, 0xA5, 0xD5, 0x2B, 0xC2, 0x4C, 0x77, 0x32, 0xE5, 0x28, 0xC3, 0x92, 0xB4, 0xC3, 0xD2, 0xE5, 0x53, 0xC2, 0xB7, 0x4B, 0x92, 0x37, 0xA3, 0xB5, 0xC3};
static uint32_t g_integer_data_results[] = {0x00, 0xFFFFFFF1, 0x58C7, 0xFFFFFFFA, 0x59, 0x0B, 0x01BB, 0xFFFFFE36, 0x3847B52A, 0xFF99BB44, 0x1DD9AFE9, 0xFFFFFF80, 0x4080, 0x087F, 0xFFFFFF9F, 0x77665544, 0x78, 0x7E64, 0x37, 0x3B5C, 0xC22BD5A5, 0x4C, 0x7732, 0xFFFFE528, 0xFFFFFFC3, 0x92B4C3D2, 0xE5, 0x53C2, 0xB74B, 0x92, 0x37A3B5C3};

static uint8_t g_integer_data1[] = {0x34, 0x78, 0x96, 0xE3, 0x9D, 0x7A, 0x94, 0xBC, 0x25, 0x4C, 0xCC, 0x42, 0x69, 0x23, 0xB8, 0x54, 0xC5, 0x20, 0x32, 0x8C, 0xC2, 0x92, 0x3C, 0x47, 0x91, 0x5F, 0x28, 0x84, 0x2A, 0xA5, 0xC3, 0xD2, 0xB4, 0xF6, 0x6F, 0xA0, 0x72, 0x61, 0xF0, 0xC3, 0xD5, 0x4B, 0xA2, 0x24, 0xC2, 0xB7, 0xA9, 0x9C, 0x80, 0x8E, 0x9A, 0xE0, 0xC3, 0x28, 0xC2, 0x20, 0x00, 0xA4, 0x40};
static uint32_t g_integer_data1_results[] = {0x00, 0x0D, 0x7896, 0xFFFFFFFC, 0x39, 0x07, 0xFFFFFFD4, 0xFFFFFFD2, 0x42CC4C25, 0x6923B8, 0x1531480C, 0xFFFFFF8C, 0xFFFF92C2, 0x473C, 0xFFFFFF91, 0x2A84285F, 0xA5, 0xD2C3, 0xB4, 0x6FF6, 0xF06172A0, 0xFFFFFFC3, 0xFFFFD54B, 0xFFFFA224, 0xFFFFFFC2, 0xB7A99C80, 0x8E, 0x9AE0, 0xC328, 0xC2, 0x2000A440};

static void
integer_compare(const char *head, const char *info, uint32_t val, size_t idx, uint32_t cmp[]) {
	if (val != cmp[idx]) {
		fprintf(stderr, "%s %s: compare at index %zu, got 0x%" PRIX32 ", expect 0x%" PRIX32 ".\n", head, info, idx, val, cmp[idx]);
		abort();
	}
}

static void
bitval_test_integer(const char *name, const uint8_t *data, size_t size, uint32_t cmp[]) {
	static char head[1024];
	snprintf(head, sizeof(head), "bitval_test_integer[%s]", name);

	uint32_t val, n;
	bitval_t b;
	bitval_init(b, (byte_t*)data, size);
	assert(bitval_ensure_bytes(b, size));

	val = (uint32_t)bitval_peek_bit(b);
	integer_compare(head, "peek", val, 0, cmp);
	val = (uint32_t)bitval_read_bit(b);
	integer_compare(head, "read", val, 0, cmp);

	val = (uint32_t)bitval_peek_sbits(b, 5);
	integer_compare(head, "peek", val, 1, cmp);
	val = (uint32_t)bitval_read_sbits(b, 5);
	integer_compare(head, "read", val, 1, cmp);

	bitval_sync(b);
	assert(bitval_synced(b));

	val = bitval_peek_bigendian_uint16(b);
	integer_compare(head, "peek", val, 2, cmp);
	val = bitval_read_bigendian_uint16(b);
	integer_compare(head, "read", val, 2, cmp);

	val = (uint32_t)bitval_peek_sbits(b, 5);
	integer_compare(head, "peek", val, 3, cmp);
	val = (uint32_t)bitval_read_sbits(b, 5);
	integer_compare(head, "read", val, 3, cmp);

	val = bitval_peek_ubits(b, 7);
	integer_compare(head, "peek", val, 4, cmp);
	val = bitval_read_ubits(b, 7);
	integer_compare(head, "read", val, 4, cmp);
	bitval_sync(b);
	assert(bitval_synced(b));

	val = bitval_peek_ubits(b, 4);
	integer_compare(head, "peek", val, 5, cmp);
	val = bitval_read_ubits(b, 4);
	integer_compare(head, "read", val, 5, cmp);

	n = val;
	val = (uint32_t)bitval_peek_sbits(b, n);
	integer_compare(head, "peek", val, 6, cmp);
	val = (uint32_t)bitval_read_sbits(b, n);
	integer_compare(head, "read", val, 6, cmp);

	val = (uint32_t)bitval_peek_sbits(b, n);
	integer_compare(head, "peek", val, 7, cmp);
	val = (uint32_t)bitval_read_sbits(b, n);
	integer_compare(head, "read", val, 7, cmp);

	bitval_sync(b);
	assert(bitval_synced(b));

	val = (uint32_t)bitval_peek_int32(b);
	integer_compare(head, "peek", val, 8, cmp);
	val = (uint32_t)bitval_read_int32(b);
	integer_compare(head, "read", val, 8, cmp);

	val = (uint32_t)bitval_peek_sbits(b, 24);
	integer_compare(head, "peek", val, 9, cmp);
	val = (uint32_t)bitval_read_sbits(b, 24);
	integer_compare(head, "read", val, 9, cmp);

	val = bitval_peek_ubits(b, 30);
	integer_compare(head, "peek", val, 10, cmp);
	val = bitval_read_ubits(b, 30);
	integer_compare(head, "read", val, 10, cmp);
	bitval_sync(b);
	assert(bitval_synced(b));

	val = (uint32_t)bitval_peek_int8(b);
	integer_compare(head, "peek", val, 11, cmp);
	val = (uint32_t)bitval_read_int8(b);
	integer_compare(head, "read", val, 11, cmp);

	val = (uint32_t)bitval_peek_int16(b);
	integer_compare(head, "peek", val, 12, cmp);
	val = (uint32_t)bitval_read_int16(b);
	integer_compare(head, "read", val, 12, cmp);

	val = (uint32_t)bitval_peek_int16(b);
	integer_compare(head, "peek", val, 13, cmp);
	val = (uint32_t)bitval_read_int16(b);
	integer_compare(head, "read", val, 13, cmp);

	val = (uint32_t)bitval_peek_int8(b);
	integer_compare(head, "peek", val, 14, cmp);
	val = (uint32_t)bitval_read_int8(b);
	integer_compare(head, "read", val, 14, cmp);

	val = (uint32_t)bitval_peek_int32(b);
	integer_compare(head, "peek", val, 15, cmp);
	val = (uint32_t)bitval_read_int32(b);
	integer_compare(head, "read", val, 15, cmp);

	val = bitval_peek_uint8(b);
	integer_compare(head, "peek", val, 16, cmp);
	val = bitval_read_uint8(b);
	integer_compare(head, "read", val, 16, cmp);

	val = bitval_peek_uint16(b);
	integer_compare(head, "peek", val, 17, cmp);
	val = bitval_read_uint16(b);
	integer_compare(head, "read", val, 17, cmp);

	val = bitval_peek_uint8(b);
	integer_compare(head, "peek", val, 18, cmp);
	val = bitval_read_uint8(b);
	integer_compare(head, "read", val, 18, cmp);

	val = bitval_peek_uint16(b);
	integer_compare(head, "peek", val, 19, cmp);
	val = bitval_read_uint16(b);
	integer_compare(head, "read", val, 19, cmp);

	val = bitval_peek_uint32(b);
	integer_compare(head, "peek", val, 20, cmp);
	val = bitval_read_uint32(b);
	integer_compare(head, "read", val, 20, cmp);

	val = (uint32_t)bitval_peek_bigendian_int8(b);
	integer_compare(head, "peek", val, 21, cmp);
	val = (uint32_t)bitval_read_bigendian_int8(b);
	integer_compare(head, "read", val, 21, cmp);

	val = (uint32_t)bitval_peek_bigendian_int16(b);
	integer_compare(head, "peek", val, 22, cmp);
	val = (uint32_t)bitval_read_bigendian_int16(b);
	integer_compare(head, "read", val, 22, cmp);

	val = (uint32_t)bitval_peek_bigendian_int16(b);
	integer_compare(head, "peek", val, 23, cmp);
	val = (uint32_t)bitval_read_bigendian_int16(b);
	integer_compare(head, "read", val, 23, cmp);

	val = (uint32_t)bitval_peek_bigendian_int8(b);
	integer_compare(head, "peek", val, 24, cmp);
	val = (uint32_t)bitval_read_bigendian_int8(b);
	integer_compare(head, "read", val, 24, cmp);

	val = (uint32_t)bitval_peek_bigendian_int32(b);
	integer_compare(head, "peek", val, 25, cmp);
	val = (uint32_t)bitval_read_bigendian_int32(b);
	integer_compare(head, "read", val, 25, cmp);

	val = bitval_peek_bigendian_uint8(b);
	integer_compare(head, "peek", val, 26, cmp);
	val = bitval_read_bigendian_uint8(b);
	integer_compare(head, "read", val, 26, cmp);

	val = bitval_peek_bigendian_uint16(b);
	integer_compare(head, "peek", val, 27, cmp);
	val = bitval_read_bigendian_uint16(b);
	integer_compare(head, "read", val, 27, cmp);

	val = bitval_peek_bigendian_uint16(b);
	integer_compare(head, "peek", val, 28, cmp);
	val = bitval_read_bigendian_uint16(b);
	integer_compare(head, "read", val, 28, cmp);

	val = bitval_peek_bigendian_uint8(b);
	integer_compare(head, "peek", val, 29, cmp);
	val = bitval_read_bigendian_uint8(b);
	integer_compare(head, "read", val, 29, cmp);

	val = bitval_peek_bigendian_uint32(b);
	integer_compare(head, "peek", val, 30, cmp);
	val = bitval_read_bigendian_uint32(b);
	integer_compare(head, "read", val, 30, cmp);
}

// bitval_test_string formula:
//	bitval_read_string()		results[0]
//	bitval_read_string()		results[1]
//	bitval_read_string()		results[2]

#define STRING(str)		str "\0"
static const char g_string_data[] = STRING("first string") STRING("second string") STRING("this is third string");
static const char *g_string_data_results[] = {"first string", "second string", "this is third string"};

static void
string_compare(const char *head, const char *info, const char *str, size_t len, size_t idx, const char *cmp[]) {
	if (str == NULL) {
		str = "";
		goto error;
	}
	if (strlen(str) != len) {
		goto error;
	}
	if (strlen(cmp[idx]) != len) {
		goto error;
	}
	if (strcmp(str, cmp[idx]) != 0) {
		goto error;
	}
	return;
error:
	fprintf(stderr, "%s %s: compare at index %zu, got %s, len %zu, expect %s.\n", head, info, idx, str, len, cmp[idx]);
	abort();
}

static void
bitval_test_string(const char *name, const char *data, size_t size, const char *cmp[]) {
	static char head[1024];
	snprintf(head, sizeof(head), "bitval_test_string[%s]", name);

	bitval_t b, b1;
	bitval_init(b, (byte_t*)data, size);
	bitval_copy(b1, b);

	const char *str;
	size_t len;
	str = bitval_peek_string(b, &len);
	string_compare(head, "peek", str, len, 0, cmp);
	str = bitval_read_string(b, &len);
	string_compare(head, "read", str, len, 0, cmp);
	bitval_skip_string(b1);
	assert(bitval_cursor(b) == bitval_cursor(b1));

	str = bitval_peek_string(b1, &len);
	string_compare(head, "peek", str, len, 1, cmp);
	str = bitval_read_string(b1, &len);
	string_compare(head, "read", str, len, 1, cmp);
	bitval_skip_string(b);
	assert(bitval_cursor(b) == bitval_cursor(b1));

	str = bitval_peek_string(b, &len);
	string_compare(head, "peek", str, len, 2, cmp);
	str = bitval_read_string(b, &len);
	string_compare(head, "read", str, len, 2, cmp);
	bitval_skip_string(b1);
	assert(bitval_cursor(b) == bitval_cursor(b1));
}

int
main(void) {
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	bitval_test_integer("integer_data", g_integer_data, sizeof(g_integer_data), g_integer_data_results);
	bitval_test_integer("integer_data1", g_integer_data1, sizeof(g_integer_data1), g_integer_data1_results);
	bitval_test_string("string_data", g_string_data, sizeof(g_string_data), g_string_data_results);
	return 0;
}
