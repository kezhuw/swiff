#ifndef __HELPER_H
#define __HELPER_H

union align {
	long long l;
	void *ptr;
	void (*fp)(void);
	double d;
	long double ld;
};

#define ALIGNMENT		sizeof(union align)
#define ALIGNMASK		(ALIGNMENT-1)

#define MAKEALIGN(n)		(ALIGNMENT * (((n) + ALIGNMASK)/ALIGNMENT))

#endif
