/*
 * Copyright (C) 1999 Cort Dougan <cort@cs.nmt.edu>
 */
#ifndef __LTT_USERTRACE_PPC_H
#define __LTT_USERTRACE_PPC_H

#ifdef __powerpc64__
#include "ltt/atomic-ppc64.h"
#include "ltt/system-ppc64.h"
#else
#include "ltt/ppc_asm-ppc.h"
#include "ltt/atomic-ppc.h"
#include "ltt/system-ppc.h"
#endif

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
