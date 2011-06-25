#include "report.h"
#include <stdio.h>
#include <assert.h>

static void
report_test(void) {
	printf("report_test, start.\n");

	int n;						assert(report_level() == ReportLevelDefault);
	n = report_error("%s\n", "SHOW ERROR.");	assert(n > 0);
	n = report_fixme("%s\n", "SHOW FIXME.");	assert(n > 0);
	n = report_report("%s\n", "SHOW INFO.");	assert(n > 0);
	n = report_warning("%s\n", "HIDE WARNING.");	assert(n == 0);
	n = report_debug("%s\n", "HIDE DEBUG.");	assert(n == 0);
	n = report_log("%s\n", "HIDE LOG.");		assert(n == 0);

	report_setup(ReportLevelDebug);			assert(report_level() == ReportLevelDebug);
	n = report_report("SHOW REPORT.\n");		assert(n > 0);
	n = report_debug("SHOW DEBUG.\n");		assert(n > 0);
	n = report_log("HIDE LOG.\n");			assert(n == 0);

	report_setup(ReportLevelError);			assert(report_level() == ReportLevelError);
	n = report_error("SHOW ERROR.\n");		assert(n > 0);
	n = report_fixme("HIDE FIXME.\n");		assert(n == 0);

	report_setup(ReportLevelMinimum);		assert(report_level() == ReportLevelMinimum);
	report_setup(ReportLevelMaximum + 2);		assert(report_level() == ReportLevelMaximum);
	n = report_log("SHOW ERROR.\n");		assert(n > 0);
	n = report_log("SHOW LOG.\n");			assert(n > 0);

	report_flush();

	printf("report_test, done.\n");
}

int
main(void) {
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	printf("Test report, start.\n");
	report_test();
	printf("Test report, done.\n");
	return 0;
}
