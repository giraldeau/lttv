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
#else
#error "Unsupported architecture"
#endif

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#ifdef i386
#define __NR_ltt_trace_generic	320
#define __NR_ltt_register_generic	321
#undef NR_syscalls
#define NR_syscalls 322
#endif

#ifdef __x86_64__
#define __NR_ltt_trace_generic	281
#define __NR_ltt_register_generic	281
#undef NR_syscalls
#define NR_syscalls 282
#endif

#ifdef __powerpc__
#define __NR_ltt_trace_generic	302
#define __NR_ltt_register_generic	303
#undef NR_syscalls
#define NR_syscalls 304
#endif

#ifdef __powerpc64__
#define __NR_ltt_trace_generic	302
#define __NR_ltt_register_generic	303
#undef NR_syscalls
#define NR_syscalls 304
#endif

#ifdef __arm__
#define __NR_ltt_trace_generic	347
#define __NR_ltt_register_generic	348
#undef NR_syscalls
#define NR_syscalls 349
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
  unsigned int num_events;
  size_t alignment;
  uint32_t checksum;
  size_t int_size;
  size_t long_size;
  size_t pointer_size;
  size_t size_t_size;
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
