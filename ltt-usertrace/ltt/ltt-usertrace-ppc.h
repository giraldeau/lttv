/*
 * Copyright (C) 1999 Cort Dougan <cort@cs.nmt.edu>
 */
#ifndef __LTT_USERTRACE_PPC_H
#define __LTT_USERTRACE_PPC_H

#ifdef __powerpc64__
#include <ltt/atomic-ppc64.h>
#include <ltt/system-ppc64.h>
#else
#include <ltt/ppc_asm-ppc.h>
#include <ltt/atomic-ppc.h>
#include <ltt/system-ppc.h>
#include <ltt/timex-ppc.h>
#endif


#endif /* __LTT_USERTRACE_PPC_H */
