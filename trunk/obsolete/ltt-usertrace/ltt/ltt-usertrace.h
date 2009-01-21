/*****************************************************************************
 * ltt-usertrace.h
 *
 * LTT userspace tracing header
 *
 * Mathieu Desnoyers, March 2006
 */

#ifndef _LTT_USERTRACE_H
#define _LTT_USERTRACE_H

#include <errno.h>
#include <syscall.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif


#define inline inline __attribute__((always_inline))

#if defined(__powerpc__) || defined(__powerpc64__)
#ifdef __powerpc64__
#include <ltt/atomic-ppc64.h>
#include <ltt/system-ppc64.h>
#include <asm/timex.h>
#else
#include <ltt/ppc_asm-ppc.h>
#include <ltt/atomic-ppc.h>
#include <ltt/system-ppc.h>
#include <ltt/timex-ppc.h>
#endif
#elif defined(__x86_64__)
#include <ltt/kernelutils-x86_64.h>
#elif defined(__i386__)
#include <ltt/kernelutils-i386.h>
#elif defined(__arm__)
#include <ltt/kernelutils-arm.h>
#elif defined(__SH4__)
#include <ltt/kernelutils-sh.h>
#else
#error "Unsupported architecture"
#endif

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#ifdef i386
#define __NR_ltt_trace_generic	328
#define __NR_ltt_register_generic	329
#undef NR_syscalls
#define NR_syscalls 330
#endif

#ifdef __x86_64__
#define __NR_ltt_trace_generic	286
#define __NR_ltt_register_generic	287
#undef NR_syscalls
#define NR_syscalls 288
#endif

#ifdef __powerpc__
#define __NR_ltt_trace_generic	309
#define __NR_ltt_register_generic	310
#undef NR_syscalls
#define NR_syscalls 311
#endif

#ifdef __powerpc64__
#define __NR_ltt_trace_generic	309
#define __NR_ltt_register_generic	310
#undef NR_syscalls
#define NR_syscalls 311
#endif

#ifdef __arm__
#define __NR_ltt_trace_generic	352
#define __NR_ltt_register_generic	353
#undef NR_syscalls
#define NR_syscalls 354
#endif

//FIXME : setup for MIPS

#ifndef _LIBC
// Put in bits/syscall.h
#define SYS_ltt_trace_generic	__NR_ltt_trace_generic
#define SYS_ltt_register_generic	__NR_ltt_register_generic
#endif

#define FACNAME_LEN 32

/* LTT userspace tracing is non blocking by default when buffers are full */
#ifndef LTT_BLOCKING
#define LTT_BLOCKING 0
#endif //LTT_BLOCKING

typedef unsigned int ltt_facility_t;

struct user_facility_info {
  char name[FACNAME_LEN];
  uint32_t num_events;
  uint32_t alignment;
  uint32_t checksum;
  uint32_t int_size;
  uint32_t long_size;
  uint32_t pointer_size;
  uint32_t size_t_size;
};
#if 0
static inline __attribute__((no_instrument_function)) 
_syscall5(int, ltt_trace_generic, unsigned int, facility_id,
	unsigned int, event_id, void *, data, size_t, data_size, int, blocking)
static inline __attribute__((no_instrument_function))
_syscall2(int, ltt_register_generic, unsigned int *, facility_id,
	const struct user_facility_info *, info)
#endif //0

#define ltt_register_generic(...)  syscall(__NR_ltt_register_generic, __VA_ARGS__)
#define ltt_trace_generic(...)  syscall(__NR_ltt_trace_generic, __VA_ARGS__)

static inline unsigned int __attribute__((no_instrument_function)) 
	ltt_align(size_t align_drift, size_t size_of_type);

#ifndef LTT_PACK
/* Calculate the offset needed to align the type */
static inline unsigned int
	ltt_align(size_t align_drift, size_t size_of_type)
{
  size_t alignment = min(sizeof(void*), size_of_type);

  return ((alignment - align_drift) & (alignment-1));
}
#define LTT_ALIGN
#else
static inline unsigned int ltt_align(size_t align_drift, size_t size_of_type)
{
  return 0;
}
#define LTT_ALIGN __attribute__((packed))
#endif //LTT_PACK

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#ifdef LTT_TRACE_FAST
#include <ltt/ltt-usertrace-fast.h>
#endif //LTT_TRACE_FAST

#endif //_LTT_USERTRACE_H
