/*
 * Copyright (C) 1999 Cort Dougan <cort@cs.nmt.edu>
 */
#ifndef __LTT_USERTRACE_PPC_H
#define __LTT_USERTRACE_PPC_H

static __inline__ unsigned long
xchg_u32(volatile void *p, unsigned long val)
{
	unsigned long prev;

	__asm__ __volatile__ ("\n\
1:	lwarx	%0,0,%2 \n"
"	stwcx.	%3,0,%2 \n\
	bne-	1b"
	: "=&r" (prev), "=m" (*(volatile unsigned long *)p)
	: "r" (p), "r" (val), "m" (*(volatile unsigned long *)p)
	: "cc", "memory");

	return prev;
}

/*
 * This function doesn't exist, so you'll get a linker error
 * if something tries to do an invalid xchg().
 */
extern void __xchg_called_with_bad_pointer(void);

#define xchg(ptr,x) ((__typeof__(*(ptr)))__xchg((unsigned long)(x),(ptr),sizeof(*(ptr))))
#define tas(ptr) (xchg((ptr),1))

static inline unsigned long __xchg(unsigned long x, volatile void *ptr, int size)
{
	switch (size) {
	case 4:
		return (unsigned long) xchg_u32(ptr, x);
#if 0	/* xchg_u64 doesn't exist on 32-bit PPC */
	case 8:
		return (unsigned long) xchg_u64(ptr, x);
#endif /* 0 */
	}
	__xchg_called_with_bad_pointer();
	return x;


}

extern inline void * xchg_ptr(void * m, void * val)
{
	return (void *) xchg_u32(m, (unsigned long) val);
}


#define __HAVE_ARCH_CMPXCHG	1

static __inline__ unsigned long
__cmpxchg_u32(volatile unsigned int *p, unsigned int old, unsigned int new)
{
	unsigned int prev;

	__asm__ __volatile__ ("\n\
1:	lwarx	%0,0,%2 \n\
	cmpw	0,%0,%3 \n\
	bne	2f \n"
"	stwcx.	%4,0,%2 \n\
	bne-	1b\n"
#ifdef CONFIG_SMP
"	sync\n"
#endif /* CONFIG_SMP */
"2:"
	: "=&r" (prev), "=m" (*p)
	: "r" (p), "r" (old), "r" (new), "m" (*p)
	: "cc", "memory");

	return prev;
}

/* This function doesn't exist, so you'll get a linker error
   if something tries to do an invalid cmpxchg().  */
extern void __cmpxchg_called_with_bad_pointer(void);

static __inline__ unsigned long
__cmpxchg(volatile void *ptr, unsigned long old, unsigned long new, int size)
{
	switch (size) {
	case 4:
		return __cmpxchg_u32(ptr, old, new);
#if 0	/* we don't have __cmpxchg_u64 on 32-bit PPC */
	case 8:
		return __cmpxchg_u64(ptr, old, new);
#endif /* 0 */
	}
	__cmpxchg_called_with_bad_pointer();
	return old;
}

#define cmpxchg(ptr,o,n)						 \
  ({									 \
     __typeof__(*(ptr)) _o_ = (o);					 \
     __typeof__(*(ptr)) _n_ = (n);					 \
     (__typeof__(*(ptr))) __cmpxchg((ptr), (unsigned long)_o_,		 \
				    (unsigned long)_n_, sizeof(*(ptr))); \
  })


#define CPU_FTR_601			0x00000100

#define CLOCK_TICK_RATE	1193180 /* Underlying HZ */

typedef uint64_t cycles_t;

/* On ppc64 this gets us the whole timebase; on ppc32 just the lower half */
static inline unsigned long get_tbl(void)
{
	unsigned long tbl;

//#if defined(CONFIG_403GCX)
//	asm volatile("mfspr %0, 0x3dd" : "=r" (tbl));
//#else
	asm volatile("mftb %0" : "=r" (tbl));
//#endif
	return tbl;
}

static inline unsigned int get_tbu(void)
{
	unsigned int tbu;

//#if defined(CONFIG_403GCX)
//	asm volatile("mfspr %0, 0x3dc" : "=r" (tbu));
//#else
	asm volatile("mftbu %0" : "=r" (tbu));
//#endif
	return tbu;
}


#ifdef __powerpc64__
static inline uint64_t get_tb(void)
{
	return mftb();
}
#else
static inline uint64_t get_tb(void)
{
	unsigned int tbhi, tblo, tbhi2;

	do {
		tbhi = get_tbu();
		tblo = get_tbl();
		tbhi2 = get_tbu();
	} while (tbhi != tbhi2);

	return ((uint64_t)tbhi << 32) | tblo;
}
#endif

static inline cycles_t get_cycles(void)
{
	return get_tb();
}


#endif /* __LTT_USERTRACE_PPC_H */
