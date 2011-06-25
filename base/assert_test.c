#include "assert.h"
#include "except.h"
#include "report.h"
#include <stdio.h>
#include <assert.h>

static void
assert_test(void) {
	printf("assert_test, start.\n");

	unsigned level = report_level();
	report_setup(ReportLevelLog);

	BEGIN_EXCEPT {
		assert_truth(6 > 0);
	}
	CATCH_EXCEPT(ExceptionAssert) {
		assert(0);
	}
	ENDUP_EXCEPT;

	BEGIN_EXCEPT {
		assert_truth(0 > 6);
	}
	CATCH_EXCEPT(ExceptionAssert) {
		except_report();
	}
	OTHER_EXCEPT {
		assert(0);
	}
	ENDUP_EXCEPT;

	BEGIN_EXCEPT {
		BEGIN_EXCEPT {
			assert_truth(0 > 20);
		}
		CATCH_EXCEPT(NULL) {
			assert(0);
		}
		ENDUP_EXCEPT;
	}
	CATCH_EXCEPT(ExceptionAssert) {
	}
	ENDUP_EXCEPT;

	report_setup(level);

	printf("assert_test, done.\n");
}

int
main(void) {
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	printf("Test assert, start.\n");
	assert_test();
	printf("Test assert, done.\n");
	return 0;
}
