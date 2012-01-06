#include "mscope.h"
#include "memory.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

static void
mscope_test_global(mscope_t mo) {
	printf("mscope_test_global(), start.\n");
	void *ptr;
	ptr = mscope_alloc_global(mo, 6666);		assert(ptr != NULL);
	ptr = mscope_alloc_global(mo, 1);		assert(ptr != NULL);
	ptr = mscope_alloc_global(mo, 7777);		assert(ptr != NULL);
	ptr = mscope_alloc_global(mo, 513);		assert(ptr != NULL);
	ptr = mscope_alloc_global(mo, 129);		assert(ptr != NULL);
	ptr = mscope_alloc_global(mo, 17);		assert(ptr != NULL);
	printf("mscope_test_global(), done.\n");
}

static void
mscope_test_local(mscope_t mo) {
	printf("mscope_test_local(), start.\n");
	printf("mscope_test_local: before mscope_enter_local().\n");
	void *left0 = mscope_alloc_local(mo, 333);
	void *mark = mscope_enter_local(mo);
	void *ptr;
	ptr = mscope_alloc_local(mo, 256);		assert(ptr != NULL);
	ptr = mscope_alloc_local(mo, 8172);		assert(ptr != NULL);
	ptr = mscope_alloc_global(mo, 5555);		assert(ptr != NULL);
	ptr = mscope_alloc_global(mo, 1111);		assert(ptr != NULL);
	ptr = mscope_alloc_global(mo, 6666);		assert(ptr != NULL);
	ptr = mscope_alloc_global(mo, 22);		assert(ptr != NULL);
	ptr = mscope_alloc_local(mo, 3);		assert(ptr != NULL);
	ptr = mscope_alloc_local(mo, 1034);		assert(ptr != NULL);
	ptr = mscope_alloc_local(mo, 4888);		assert(ptr != NULL);
	void *left1 = mscope_alloc_local(mo, 44);
	void *left2 = mscope_alloc_local(mo, 215);
	void *left0_backup = left0;
	void *left1_backup = left1;
	void *left2_backup = left2;
	mscope_leave_local(mo, mark, &ptr, &left0, &left1, &left2, NULL);
	assert(left0 == left0_backup);
	assert(left1 != left1_backup);
	assert(left2 != left2_backup);
	printf("mscope_test_local: after mscope_leave_local().\n");
	printf("mscope_test_local(), done.\n");
}

static void *
alloc(void *ctx, size_t size) {
	(void)ctx;
	return malloc(size);
}

static void
dealloc(void *ctx, void *ptr) {
	(void)ctx;
	free(ptr);
}

static void
mscope_test(void) {
	printf("mscope_test(), start.\n");
	mscope_t mo;
	mscope_init(mo, alloc, dealloc, NULL);

	printf("mscope_test: before mscope_enter_local().\n");
	void *mark = mscope_enter_local(mo);
	void *ptr;
	ptr = mscope_alloc_local(mo, 45);		assert(ptr != NULL);
	ptr = mscope_alloc_local(mo, 1024);		assert(ptr != NULL);
	mscope_test_local(mo);
	mscope_test_global(mo);
	mscope_leave_local(mo, mark, &mo, NULL);
	printf("mscope_test: after mscope_leave_local().\n");
	mark = mscope_enter_local(mo);
	mscope_leave_local(mo, mark, &mo, NULL);

	ptr = mscope_alloc_global(mo, 125);		assert(ptr != NULL);
	mscope_fini(mo);
	printf("mscope_test(), done.\n");
}

int
main(void) {
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	printf("Test mscope, start.\n");
	mscope_test();
	printf("Test mscope, done.\n");

	return 0;
}
