#include "logger.h"
#include <stdio.h>
#include <stddef.h>

static void
logger_test(void) {
	printf("logger_test, start.\n");
	logger_print("%s(%d): %s: %s\n", __FILE__, __LINE__, __func__, __DATE__ " " __TIME__ " " "message in stdout.");
	logger_error("%s(%d): %s: %s\n", __FILE__, __LINE__, __func__, __DATE__ " " __TIME__ " " "message in stderr.");
	printf("logger_test, done.\n");
}

int
main(void) {
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	printf("Test logger, start.\n");
	logger_test();
	printf("Test logger, done.\n");
	return 0;
}
