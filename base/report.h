#ifndef __REPORT_H
#define __REPORT_H

// Macros as apis. Prototypes:
// 	int report_at(unsigned lvl, const char *fmt, ...);
//
// 	int report_error(const char *fmt, ...);
// 	int report_fixme(const char *fmt, ...);
// 	int report_report(const char *fmt, ...);
// 	int report_warning(const char *fmt, ...);
// 	int report_debug(const char *fmt, ...);
// 	int report_log(const char *fmt, ...);
#define report_at(lvl, ...)		report_print(lvl, __func__, __FILE__, __LINE__, __VA_ARGS__)

#define report_error(...)		report_at(ReportLevelError, __VA_ARGS__)
#define report_fixme(...)		report_at(ReportLevelFixme, __VA_ARGS__)
#define report_report(...)		report_at(ReportLevelReport, __VA_ARGS__)
#define report_warning(...)		report_at(ReportLevelWarning, __VA_ARGS__)
#define report_debug(...)		report_at(ReportLevelDebug, __VA_ARGS__)
#define report_log(...)			report_at(ReportLevelLog, __VA_ARGS__)

/* ORDER REPORT LEVEL */
enum report_level {
	ReportLevelError,
	ReportLevelFixme,
	ReportLevelReport,
	ReportLevelWarning,
	ReportLevelDebug,
	ReportLevelLog,

	ReportLevelMaximum = ReportLevelLog,
	ReportLevelMinimum = ReportLevelError,
	ReportLevelDefault = ReportLevelReport
};

// Messages print only when their levels are less than or equal to report_level().
// ReportLevelDefault is the default level.
unsigned report_level(void);

// Use 'unsigned' here to avoid enumerated type differences among multiple compilers.
// If lvl > ReportLevelMaximum, it acts as ReportLevelMaximum.
//
// Each enumerated type shall be compatible with char, a signed integer type,
// or an unsigned integer type. The choice of type is implementation-defined,
// but shall be capable of representing the values of all the members of the enumeration.
void report_setup(unsigned lvl);

int report_print(unsigned lvl, const char *func, const char *file, int line, const char *fmt, ...);
void report_flush(void);

#endif
