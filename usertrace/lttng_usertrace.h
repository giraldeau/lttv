
/* LTTng user-space tracing header
 *
 * Copyright 2006 Mathieu Desnoyers
 *
 */

#ifndef _LTTNG_USERTRACE_H
#define _LTTNG_USERTRACE_H

#include <errno.h>
#include <syscall.h>

//Put in asm-i486/unistd.h
#define __NR_ltt_update	294
#define __NR_ltt_switch	295

#undef NR_syscalls
#define NR_syscalls 296

#ifndef _LIBC
// Put in bits/syscall.h
#define SYS_ltt_update	__NR_ltt_update
#define SYS_ltt_switch	__NR_ltt_switch
#endif

void __lttng_sig_trace_handler(int signo);

/* Call this at the beginning of a new thread, except for the main() */
void lttng_thread_init(void);

static inline _syscall1(int, ltt_switch, unsigned long, addr)
static inline _syscall4(int, ltt_update, unsigned long *, cpu_addr, unsigned long *, fac_addr, int *, active, int *, filter)

#endif //_LTTNG_USERTRACE_H