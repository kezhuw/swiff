#include "except.h"
#include "report.h"
#include <stdio.h>
#include <assert.h>

static const Exception_t ExceptionTest = except_define("Test failure");

static void
except_test(void) {
	printf("except_test, start.\n");

	unsigned level = report_level();
	report_setup(ReportLevelLog);

	int o = 1;
	BEGIN_EXCEPT {
	}
	ENDUP_EXCEPT;
	assert(o == 1);


	o = 1;
	BEGIN_EXCEPT {
		o = 2;
	}
	ENDUP_EXCEPT;
	assert(o == 2);


	o = 1;
	BEGIN_EXCEPT {
		o = 2;
	}
	FINAL_EXCEPT {
		assert(o == 2);
		o = 3;
	}
	ENDUP_EXCEPT;
	assert(o == 3);


	o = 1;
	BEGIN_EXCEPT {
		o = 2;
	}
	CATCH_EXCEPT(ExceptionTest) {
		assert(0);
		o = 3;
	}
	ENDUP_EXCEPT;
	assert(o == 2);


	o = 1;
	BEGIN_EXCEPT {
		o = 2;
	}
	CATCH_EXCEPT(NULL) {
		assert(0);
		o = 3;
	}
	CATCH_EXCEPT(ExceptionTest) {
		assert(0);
		o = 4;
	}
	ENDUP_EXCEPT;
	assert(o == 2);


	o = 1;
	BEGIN_EXCEPT {
		o = 2;
	}
	CATCH_EXCEPT(ExceptionTest) {
		assert(0);
		o = 3;
	}
	FINAL_EXCEPT {
		assert(o == 2);
		o = 4;
	}
	ENDUP_EXCEPT;
	assert(o == 4);


	o = 1;
	BEGIN_EXCEPT {
		o = 2;
	}
	OTHER_EXCEPT {
		assert(0);
		o = 3;
	}
	ENDUP_EXCEPT;
	assert(o == 2);


	o = 1;
	BEGIN_EXCEPT {
		o = 2;
	}
	OTHER_EXCEPT {
		assert(0);
		o = 3;
	}
	FINAL_EXCEPT {
		assert(o == 2);
		o = 4;
	}
	ENDUP_EXCEPT;
	assert(o == 4);


	o = 1;
	BEGIN_EXCEPT {
		o = 2;
	}
	CATCH_EXCEPT(ExceptionTest) {
		assert(0);
		o = 3;
	}
	OTHER_EXCEPT {
		assert(0);
		o = 4;
	}
	ENDUP_EXCEPT;
	assert(o == 2);


	o = 1;
	BEGIN_EXCEPT {
		o = 2;
	}
	CATCH_EXCEPT(ExceptionTest) {
		assert(0);
		o = 3;
	}
	OTHER_EXCEPT {
		assert(0);
		o = 4;
	}
	FINAL_EXCEPT {
		assert(o == 2);
		o = 5;
	}
	ENDUP_EXCEPT;
	assert(o == 5);


	o = 1;
	BEGIN_EXCEPT {
		o = 2;
		except_throw(ExceptionTest, "panic");
	}
	CATCH_EXCEPT(ExceptionTest) {
		assert(o == 2);
		o = 3;
	}
	FINAL_EXCEPT {
		assert(o == 3);
		o = 4;
	}
	ENDUP_EXCEPT;
	assert(o == 4);



	o = 1;
	BEGIN_EXCEPT {
		o = 2;
		except_throw(ExceptionTest, "panic");
	}
	OTHER_EXCEPT {
		assert(o == 2);
		o = 3;
	}
	FINAL_EXCEPT {
		assert(o == 3);
		o = 4;
	}
	ENDUP_EXCEPT;
	assert(o == 4);


	o = 1;
	BEGIN_EXCEPT {
		o = 2;
		except_throw(ExceptionTest, "panic");
	}
	CATCH_EXCEPT(NULL) {
		assert(0);
		o = 3;
	}
	CATCH_EXCEPT(ExceptionTest) {
		assert(o == 2);
		o = 4;
	}
	OTHER_EXCEPT {
		assert(0);
		o = 5;
	}
	FINAL_EXCEPT {
		assert(o == 4);
		o = 6;
	}
	ENDUP_EXCEPT;
	assert(o == 6);


	o = 1;
	BEGIN_EXCEPT {
		BEGIN_EXCEPT {
			o = 2;
			except_throw(ExceptionTest, "panic");
		}
		FINAL_EXCEPT {
			assert(o == 2);
			o = 3;
		}
		ENDUP_EXCEPT;
	}
	CATCH_EXCEPT(ExceptionTest) {
		assert(o == 3);
		o = 4;
	}
	ENDUP_EXCEPT;
	assert(o == 4);

	report_setup(level);

	printf("except_test, done.\n");
}

int
main(void) {
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	printf("Test except, start.\n");
	except_test();
	printf("Test except, done.\n");
	return 0;
}
