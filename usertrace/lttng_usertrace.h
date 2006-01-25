
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

static inline _syscall3(int, ltt_update, unsigned long, addr, int *, active, int *, filter)

#endif //_LTTNG_USERTRACE_H
