#ifndef __EXCEPT_H
#define __EXCEPT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * Code example:
 *
 * const Exception_t ExceptionExample = except_define("Example exception");
 *
 *	BEGIN_EXCEPT {
 *		do_something();
 *		except_throw(ExceptionExample);
 *	}
 *	CATCH_EXCEPT(ExceptionMemory) {
 *		memory_except_handler();
 *	}
 *	CATCH_EXCEPT(ExceptionExample) {
 *		example_except_handler();
 *	}
 *	...
 *	...
 *	...
 *	OTHER_EXCEPT {
 *		except_report();
 *		generic_handler();
 *	}
 *	FINAL_EXCEPT {
 *		finalize();
 *	}
 *	ENDUP_EXCEPT;
 */

#define BEGIN_EXCEPT					\
do {							\
	volatile enum except_state state;		\
	int64_t except_data[(exceptenv_size()+sizeof(int64_t)-1)/sizeof(int64_t)];	\
	struct exceptenv *ee =				\
		(struct exceptenv *)except_data;	\
							\
	exceptenv_enque(ee);				\
							\
	if ((state = exceptenv_setup(ee)) == ExceptNone) {

#define CATCH_EXCEPT(ex)				\
		if (state == ExceptNone)		\
			exceptenv_deque(ee);		\
	} else if (except_catch(ex)) {			\
		state = ExceptHandled;

#define OTHER_EXCEPT					\
		if (state == ExceptNone)		\
			exceptenv_deque(ee);		\
	} else {					\
		state = ExceptHandled;

#define FINAL_EXCEPT					\
		if (state == ExceptNone)		\
			exceptenv_deque(ee);		\
	} {						\
		if (state == ExceptNone)		\
			state = ExceptFinalized;

#define ENDUP_EXCEPT					\
		if (state == ExceptNone)		\
			exceptenv_deque(ee);		\
	}						\
							\
	if (state == ExceptRaised)			\
		except_raise();				\
} while (0)

enum except_state {
	ExceptNone = 0,
	ExceptRaised,
	ExceptHandled,
	ExceptFinalized
};

typedef const struct exception {
	const char *desc;
} *Exception_t;

static inline const char *
except_string(Exception_t ex) {
	return ex->desc;
}

void except_throw(Exception_t ex, const char *spec, const char *func, const char *file, int line);
bool except_catch(Exception_t ex, const char *func, const char *file, int line);
void except_raise(const char *func, const char *file, int line);
int except_report(const char *func, const char *file, int line);

#define except_throw(ex, spec)	((except_throw)((ex), (spec), __func__, __FILE__, __LINE__))
#define except_catch(ex)	((except_catch)((ex), __func__, __FILE__, __LINE__))
#define except_raise()		((except_raise)(__func__, __FILE__, __LINE__))
#define except_report()		((except_report)(__func__, __FILE__, __LINE__))
#define except_define(desc)	(&(const struct exception) {desc})

struct exceptenv;

size_t exceptenv_size(void);
extern enum except_state (*exceptenv_setup)(struct exceptenv *ee);
void exceptenv_enque(struct exceptenv *ee);
void exceptenv_deque(struct exceptenv *ee);

#endif
