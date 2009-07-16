/*****************************************************************************
 * kernelutils-arm.h
 *
 * This file holds the code needed by LTT usertrace that comes from the
 * kernel headers.  Since including kernel headers is not recommended in
 * userspace programs/libraries, we rewrote implementations HIGHLY INSPIRED
 * (i.e. copied/pasted) from the original kernel headers (2.6.18).
 *
 * Do not use these functions within signal handlers, as the architecture offers
 * no atomic operations. (Mathieu Desnoyers) It is safe to do multithreaded
 * tracing though, as the buffers are per thread.
 *
 * Deepak Saxena, October 2006
 */

#ifndef _KERNELUTILS_ARM_H
#define _KERNELUTILS_ARM_H

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { volatile int counter; } atomic_t;

#define atomic_read(v)	((v)->counter)

static inline int atomic_add_return(int i, atomic_t *v)
{
	unsigned long flags;
	int val;

	val = v->counter;
	v->counter = val += i;

	return val;
}

#define atomic_add(i, v)	(void) atomic_add_return(i, v)
#define atomic_inc(v)		(void) atomic_add_return(1, v)

static inline unsigned long cmpxchg(volatile void *ptr,
				    unsigned long old,
				    unsigned long new)
{
	unsigned long flags, prev;
	volatile unsigned long *p = ptr;

	if ((prev = *p) == old)
		*p = new;
	return(prev);
}

static inline unsigned long long get_cycles(void)
{
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);	
	return tp.tv_sec * 1000000000 + tp.tv_nsec;
}


#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif
