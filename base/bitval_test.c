#include "bitval.h"
#include "except.h"
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

static const Exception_t ExceptionParser = except_define("Parser error");

static void
bitval_throw_error(const char *err) {
	except_throw(ExceptionParser, err);
}

// bitval_test_integer formula:
// 	bitval_read_bit(b);			results[0]
// 	bitval_read_sbits(b, 5);		results[1]
// 	bitval_sync(b);
//
// 	bitval_read_bigendian_ubyte2(b);	results[2]
//	bitval_read_sbits(b, 5);		results[3]
//	bitval_read_ubits(b, 7);		results[4]
//	bitval_sync(b);
//
//	n = bitval_read_ubits(b, 4);		results[5]
//	bitval_read_sbits(b, n);		results[6]
//	bitval_read_sbits(b, n);		results[7]
//	bitval_sync(b);
//
//	bitval_read_byte4(b);			results[8]
//	bitval_read_sbits(b, 24);		results[9]
//	bitval_read_ubits(b, 30);		results[10]
//	bitval_sync(b);
//
//	bitval_read_byte1(b);			results[11]
//	bitval_read_byte2(b);			results[12]
//	bitval_read_byte3(b);			results[13]
//	bitval_read_byte4(b);			results[14]
//
//	bitval_read_ubyte1(b);			results[15]
//	bitval_read_ubyte2(b);			results[16]
//	bitval_read_ubyte3(b);			results[17]
//	bitval_read_ubyte4(b);			results[18]
//
//	bitval_read_bigendian_byte1(b);		results[19]
//	bitval_read_bigendian_byte2(b);		results[20]
//	bitval_read_bigendian_byte3(b);		results[21]
//	bitval_read_bigendian_byte4(b);		results[22]
//
//	bitval_read_bigendian_ubyte1(b);	results[23]
//	bitval_read_bigendian_ubyte2(b);	results[24]
//	bitval_read_bigendian_ubyte3(b);	results[25]
//	bitval_read_bigendian_ubyte4(b);	results[26]

static uint8_t g_integer_data[] = {0x45, 0x58, 0xC7, 0xD5, 0x9E, 0xB3, 0x77, 0x8D, 0x9E, 0x2A, 0xB5, 0x47, 0x38, 0x99, 0xBB, 0x44, 0x77, 0x66, 0xBF, 0xA7, 0x80, 0x80, 0x40, 0x7F, 0x08, 0x9F, 0x44, 0x55, 0x66, 0x77, 0x78, 0x64, 0x7E, 0x37, 0x5C, 0x3B, 0xA5, 0xD5, 0x2B, 0xC2, 0x4C, 0x77, 0x32, 0xE5, 0x28, 0xC3, 0x92, 0xB4, 0xC3, 0xD2, 0xE5, 0x53, 0xC2, 0xB7, 0x4B, 0x92, 0x37, 0xA3, 0xB5, 0xC3};
static uint32_t g_integer_data_results[] = {0x00, 0xFFFFFFF1, 0x58C7, 0xFFFFFFFA, 0x59, 0x0B, 0x01BB, 0xFFFFFE36, 0x3847B52A, 0xFF99BB44, 0x1DD9AFE9, 0xFFFFFF80, 0x4080, 0xFF9F087F, 0x77665544, 0x78, 0x7E64, 0x3B5C37, 0xC22BD5A5, 0x4C, 0x7732, 0xFFE528C3, 0x92B4C3D2, 0xE5, 0x53C2, 0xB74B92, 0x37A3B5C3};

static uint8_t g_integer_data1[] = {0x34, 0x78, 0x96, 0xE3, 0x9D, 0x7A, 0x94, 0xBC, 0x25, 0x4C, 0xCC, 0x42, 0x69, 0x23, 0xB8, 0x54, 0xC5, 0x20, 0x32, 0x8C, 0xC2, 0x92, 0x3C, 0x47, 0x91, 0x5F, 0x28, 0x84, 0x2A, 0xA5, 0xC3, 0xD2, 0xB4, 0xF6, 0x6F, 0xA0, 0x72, 0x61, 0xF0, 0xC3, 0xD5, 0x4B, 0xA2, 0x24, 0xC2, 0xB7, 0xA9, 0x9C, 0x80, 0x8E, 0x9A, 0xE0, 0xC3, 0x28, 0xC2, 0x20, 0x00, 0xA4, 0x40};
static uint32_t g_integer_data1_results[] = {0x00, 0x0D, 0x7896, 0xFFFFFFFC, 0x39, 0x07, 0xFFFFFFD4, 0xFFFFFFD2, 0x42CC4C25, 0x6923B8, 0x1531480C, 0xFFFFFF8C, 0xFFFF92C2, 0xFF91473C, 0x2A84285F, 0xA5, 0xD2C3, 0x6FF6B4, 0xF06172A0, 0xFFFFFFC3, 0xFFFFD54B, 0xFFA224C2, 0xB7A99C80, 0x8E, 0x9AE0, 0xC328C2, 0x2000A440};

static void
bitval_test_integer(const char *name, const uint8_t *data, size_t size, uint32_t results[]) {
	int64_t bdata[(bitval_size()+sizeof(int64_t)-1)/sizeof(int64_t)];
	uint32_t val, n;

	printf("bitval_test_integer[%s], start.\n", name);

	struct bitval *b = (struct bitval *)bdata;
	bitval_init(b, (ubyte1_t *)data, size, bitval_throw_error);assert(bitval_ensure_bytes(b, size));
	val = (uint32_t)bitval_peek_bit(b);		assert(val == results[0]);
	val = (uint32_t)bitval_read_bit(b);		assert(val == results[0]);
	val = (uint32_t)bitval_peek_sbits(b, 5);	assert(val == results[1]);
	val = (uint32_t)bitval_read_sbits(b, 5);	assert(val == results[1]);
	bitval_sync(b);					assert(bitval_synced(b));

	val = bitval_peek_bigendian_ubyte2(b);		assert(val == results[2]);
	val = bitval_read_bigendian_ubyte2(b);		assert(val == results[2]);
	val = (uint32_t)bitval_peek_sbits(b, 5);	assert(val == results[3]);
	val = (uint32_t)bitval_read_sbits(b, 5);	assert(val == results[3]);
	val = bitval_peek_ubits(b, 7);			assert(val == results[4]);
	val = bitval_read_ubits(b, 7);			assert(val == results[4]);
	bitval_sync(b);					assert(bitval_synced(b));

	val = bitval_peek_ubits(b, 4);			assert(val == results[5]);
	val = bitval_read_ubits(b, 4);			assert(val == results[5]);
	n = val;
	val = (uint32_t)bitval_peek_sbits(b, n);	assert(val == results[6]);
	val = (uint32_t)bitval_read_sbits(b, n);	assert(val == results[6]);
	val = (uint32_t)bitval_peek_sbits(b, n);	assert(val == results[7]);
	val = (uint32_t)bitval_read_sbits(b, n);	assert(val == results[7]);
	bitval_sync(b);					assert(bitval_synced(b));

	val = (uint32_t)bitval_peek_byte4(b);		assert(val == results[8]);
	val = (uint32_t)bitval_read_byte4(b);		assert(val == results[8]);
	val = (uint32_t)bitval_peek_sbits(b, 24);	assert(val == results[9]);
	val = (uint32_t)bitval_read_sbits(b, 24);	assert(val == results[9]);
	val = bitval_peek_ubits(b, 30);			assert(val == results[10]);
	val = bitval_read_ubits(b, 30);			assert(val == results[10]);
	bitval_sync(b);					assert(bitval_synced(b));

	val = (uint32_t)bitval_peek_byte1(b);		assert(val == results[11]);
	val = (uint32_t)bitval_read_byte1(b);		assert(val == results[11]);
	val = (uint32_t)bitval_peek_byte2(b);		assert(val == results[12]);
	val = (uint32_t)bitval_read_byte2(b);		assert(val == results[12]);
	val = (uint32_t)bitval_peek_byte3(b);		assert(val == results[13]);
	val = (uint32_t)bitval_read_byte3(b);		assert(val == results[13]);
	val = (uint32_t)bitval_peek_byte4(b);		assert(val == results[14]);
	val = (uint32_t)bitval_read_byte4(b);		assert(val == results[14]);

	val = bitval_peek_ubyte1(b);			assert(val == results[15]);
	val = bitval_read_ubyte1(b);			assert(val == results[15]);
	val = bitval_peek_ubyte2(b);			assert(val == results[16]);
	val = bitval_read_ubyte2(b);			assert(val == results[16]);
	val = bitval_peek_ubyte3(b);			assert(val == results[17]);
	val = bitval_read_ubyte3(b);			assert(val == results[17]);
	val = bitval_peek_ubyte4(b);			assert(val == results[18]);
	val = bitval_read_ubyte4(b);			assert(val == results[18]);

	val = (uint32_t)bitval_peek_bigendian_byte1(b);	assert(val == results[19]);
	val = (uint32_t)bitval_read_bigendian_byte1(b);	assert(val == results[19]);
	val = (uint32_t)bitval_peek_bigendian_byte2(b);	assert(val == results[20]);
	val = (uint32_t)bitval_read_bigendian_byte2(b);	assert(val == results[20]);
	val = (uint32_t)bitval_peek_bigendian_byte3(b);	assert(val == results[21]);
	val = (uint32_t)bitval_read_bigendian_byte3(b);	assert(val == results[21]);
	val = (uint32_t)bitval_peek_bigendian_byte4(b);	assert(val == results[22]);
	val = (uint32_t)bitval_read_bigendian_byte4(b);	assert(val == results[22]);

	val = bitval_peek_bigendian_ubyte1(b);		assert(val == results[23]);
	val = bitval_read_bigendian_ubyte1(b);		assert(val == results[23]);
	val = bitval_peek_bigendian_ubyte2(b);		assert(val == results[24]);
	val = bitval_read_bigendian_ubyte2(b);		assert(val == results[24]);
	val = bitval_peek_bigendian_ubyte3(b);		assert(val == results[25]);
	val = bitval_read_bigendian_ubyte3(b);		assert(val == results[25]);
	val = bitval_peek_bigendian_ubyte4(b);		assert(val == results[26]);
	val = bitval_read_bigendian_ubyte4(b);		assert(val == results[26]);

	printf("bitval_test_integer[%s], done.\n", name);
}

// bitval_test_string formula:
//	bitval_read_string()		results[0]
//	bitval_read_string()		results[1]
//	bitval_read_string()		results[2]

#define STRING(str)		str "\0"
static const char g_string_data[] = STRING("first string") STRING("second string") STRING("this is third string");
static const char *g_string_data_results[] = {"first string", "second string", "this is third string"};

static void
bitval_test_string(const char *name, const char *data, size_t size, const char *results[]) {
	printf("bitval_test_string[%s], start.\n", name);

	struct bitval *b = malloc(bitval_size());
	struct bitval *b1 = malloc(bitval_size());

	bitval_init(b, (ubyte1_t *)data, size, bitval_throw_error);
	bitval_copy(b1, b);

	const char *str;
	size_t len;
	str = bitval_peek_string(b, &len);	assert(str != NULL); assert(strlen(str) == len); assert(strcmp(str, results[0]) == 0);
	str = bitval_read_string(b, &len);	assert(str != NULL); assert(strlen(str) == len); assert(strcmp(str, results[0]) == 0);
	bitval_skip_string(b1);			assert(bitval_getptr(b) == bitval_getptr(b1));

	str = bitval_peek_string(b1, &len);	assert(str != NULL); assert(strlen(str) == len); assert(strcmp(str, results[1]) == 0);
	str = bitval_read_string(b1, &len);	assert(str != NULL); assert(strlen(str) == len); assert(strcmp(str, results[1]) == 0);
	bitval_skip_string(b);			assert(bitval_getptr(b) == bitval_getptr(b1));

	str = bitval_peek_string(b, &len);	assert(str != NULL);assert(strlen(str) == len); assert(strcmp(str, results[2]) == 0);
	str = bitval_read_string(b, &len);	assert(str != NULL);assert(strlen(str) == len); assert(strcmp(str, results[2]) == 0);
	bitval_skip_string(b1);			assert(bitval_getptr(b) == bitval_getptr(b1));

	free(b1);
	free(b);

	printf("bitval_test_string[%s], done.\n", name);
}

static void
bitval_test_error(const char *name, const void *data, size_t size) {
	printf("bitval_test_error[%s], start.\n", name);
	struct bitval *b = malloc(bitval_size());

	bitval_init(b, (ubyte1_t *)data, size, bitval_throw_error);

	BEGIN_EXCEPT {
		bitval_skip_bytes(b, (size_t)-1);
	}
	CATCH_EXCEPT(ExceptionParser) {
		except_report();
	}
	OTHER_EXCEPT {
		assert(0);
	}
	ENDUP_EXCEPT;

	BEGIN_EXCEPT {
		bitval_read_sbits(b, sizeof(ubyte4_t)*CHAR_BIT + 5);
	}
	CATCH_EXCEPT(ExceptionParser) {
		except_report();
	}
	OTHER_EXCEPT {
		assert(0);
	}
	ENDUP_EXCEPT;

	bitval_read_bit(b);
	BEGIN_EXCEPT {
		bitval_skip_bytes(b, 1);
	}
	CATCH_EXCEPT(ExceptionParser) {
		except_report();
	}
	OTHER_EXCEPT {
		assert(0);
	}
	ENDUP_EXCEPT;

	free(b);
	printf("bitval_test_error[%s], done.\n", name);
}

int
main(void) {
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	printf("Test bitval, start.\n");
	bitval_test_integer("integer_data", g_integer_data, sizeof(g_integer_data), g_integer_data_results);
	bitval_test_integer("integer_data1", g_integer_data1, sizeof(g_integer_data1), g_integer_data1_results);
	bitval_test_string("string_data", g_string_data, sizeof(g_string_data), g_string_data_results);
	bitval_test_error("integer_data", g_integer_data, sizeof(g_integer_data));
	printf("Test bitval, done.\n");
	return 0;
}
