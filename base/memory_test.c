#include "memory.h"
#include "except.h"
#include <stdio.h>
#include <assert.h>

static const Exception_t ExceptionOther = except_define("Other exception");

#define STR(i)	#i
#define XSTR(i)	STR(i)
void
memory_test(void) {
	printf("memory_test, start.\n");

	void *ptr = memory_alloc(__LINE__);	memory_report(ptr, "Memory info(" XSTR(__LINE__) " bytes)");
	ptr = memory_realloc(ptr, __LINE__);	memory_report(ptr, "Memory info(" XSTR(__LINE__) " bytes)");	assert(memory_amount() == __LINE__);
	memory_dealloc(ptr);

	memory_alloc(0x20); memory_zalloc(0x200); memory_alloc(0x02);	assert(memory_amount() == 0x222);

	size_t n = 0x33;
	ptr = memory_zalloc(n);
	for (size_t i=0; i<n; i++) {
		assert(((char*)ptr)[i] == 0);
	}

	memory_fini();

	ptr = (void *)0x12;
	BEGIN_EXCEPT {
		memory_dealloc(ptr);
	}
	CATCH_EXCEPT(ExceptionOther) {
		assert(0);
	}
	CATCH_EXCEPT(ExceptionMemory) {
		except_report();
	}
	ENDUP_EXCEPT;

	printf("memory_test, done.\n");
}

int
main(void) {
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	printf("Test memory, start.\n");
	memory_test();
	printf("Test memory, done.\n");
	return 0;
}
