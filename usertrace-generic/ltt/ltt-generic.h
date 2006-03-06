/*****************************************************************************
 * ltt-generic.h
 *
 * LTT generic userspace tracing header
 *
 * Mathieu Desnoyers, March 2006
 */

#ifndef _LTT_GENERIC_H
#define _LTT_GENERIC_H

#include <errno.h>
#include <syscall.h>
#include <linux/unistd.h>
#include <asm/atomic.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

//Put in asm-i486/unistd.h
#define __NR_ltt_trace_generic	294
#define __NR_ltt_register_generic	295

#undef NR_syscalls
#define NR_syscalls 296

//FIXME : setup for ARM
//FIXME : setup for MIPS

#ifndef _LIBC
// Put in bits/syscall.h
#define SYS_ltt_trace_generic	__NR_ltt_trace_generic
#define SYS_ltt_register_generic	__NR_ltt_register_generic
#endif

#define FACNAME_LEN 32

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

static inline _syscall4(int, ltt_trace_generic, unsigned int, facility_id,
		unsigned int, event_id, void *, data, size_t, data_size)
static inline _syscall2(int, ltt_register_generic, unsigned int *, facility_id, const struct user_facility_info *, info)

#ifndef LTT_PACK
/* Calculate the offset needed to align the type */
static inline unsigned int ltt_align(size_t align_drift,
                                     size_t size_of_type)
{
  size_t alignment = min(sizeof(void*), size_of_type);

  return ((alignment - align_drift) & (alignment-1));
}
#else
static inline unsigned int ltt_align(size_t align_drift,
                                     size_t size_of_type)
{
  return 0;
}
#endif //LTT_PACK

#endif //_LTT_GENERIC_H


