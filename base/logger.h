#ifndef __LOGGER_H
#define __LOGGER_H

int logger_error(const char *fmt, ...);
int logger_print(const char *fmt, ...);

void logger_flush(void);

#endif
