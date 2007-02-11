
/* LTTng user-space "fast" tracing header
 *
 * Copyright 2006 Mathieu Desnoyers
 *
 */

#ifndef _LTT_USERTRACE_FAST_H
#define _LTT_USERTRACE_FAST_H

#ifdef LTT_TRACE
#ifdef LTT_TRACE_FAST

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <syscall.h>
#include <semaphore.h>
#include <signal.h>

#include <ltt/ltt-facility-id-user_generic.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef	LTT_N_SUBBUFS
#define LTT_N_SUBBUFS 2
#endif //LTT_N_SUBBUFS

#ifndef	LTT_SUBBUF_SIZE_PROCESS
#define LTT_SUBBUF_SIZE_PROCESS 1048576
#endif //LTT_BUF_SIZE_CPU

#define LTT_BUF_SIZE_PROCESS (LTT_SUBBUF_SIZE_PROCESS * LTT_N_SUBBUFS)

#ifndef LTT_USERTRACE_ROOT
#define LTT_USERTRACE_ROOT "/tmp/ltt-usertrace"
#endif //LTT_USERTRACE_ROOT


/* Buffer offset macros */

#define BUFFER_OFFSET(offset, buf) (offset & (buf->alloc_size-1))
#define SUBBUF_OFFSET(offset, buf) (offset & (buf->subbuf_size-1))
#define SUBBUF_ALIGN(offset, buf) \
  (((offset) + buf->subbuf_size) & (~(buf->subbuf_size-1)))
#define SUBBUF_TRUNC(offset, buf) \
  ((offset) & (~(buf->subbuf_size-1)))
#define SUBBUF_INDEX(offset, buf) \
  (BUFFER_OFFSET(offset,buf)/buf->subbuf_size)


#define LTT_TRACER_MAGIC_NUMBER		 0x00D6B7ED
#define LTT_TRACER_VERSION_MAJOR		0
#define LTT_TRACER_VERSION_MINOR		7

#ifndef atomic_cmpxchg
#define atomic_cmpxchg(v, old, new) ((int)cmpxchg(&((v)->counter), old, new))
#endif //atomic_cmpxchg

struct ltt_trace_header {
	uint32_t				magic_number;
	uint32_t				arch_type;
	uint32_t				arch_variant;
	uint32_t				float_word_order;	 /* Only useful for user space traces */
	uint8_t 				arch_size;
	//uint32_t				system_type;
	uint8_t					major_version;
	uint8_t					minor_version;
	uint8_t					flight_recorder;
	uint8_t					has_heartbeat;
	uint8_t					has_alignment;	/* Event header alignment */
	uint32_t				freq_scale;
	uint64_t				start_freq;
	uint64_t				start_tsc;
	uint64_t				start_monotonic;
  uint64_t        start_time_sec;
  uint64_t        start_time_usec;
} __attribute((packed));


struct ltt_block_start_header {
	struct { 
		uint64_t								cycle_count;
		uint64_t								freq; /* khz */
	} begin;
	struct { 
		uint64_t								cycle_count;
		uint64_t								freq; /* khz */
	} end;
	uint32_t								lost_size;	/* Size unused at the end of the buffer */
	uint32_t								buf_size;		/* The size of this sub-buffer */
	struct ltt_trace_header	trace;
} __attribute((packed));



struct ltt_buf {
	void 			*start;
	atomic_t	offset;
	atomic_t	consumed;
	atomic_t	reserve_count[LTT_N_SUBBUFS];
	atomic_t	commit_count[LTT_N_SUBBUFS];

	atomic_t	events_lost;
	atomic_t	corrupted_subbuffers;
	sem_t	writer_sem;	/* semaphore on which the writer waits */
	unsigned int	alloc_size;
	unsigned int	subbuf_size;
};

struct ltt_trace_info {
	int init;
	int filter;
	pid_t daemon_id;
	int nesting;
	struct {
		struct ltt_buf process;
		char process_buf[LTT_BUF_SIZE_PROCESS] __attribute__ ((aligned (8)));
	} channel;
};


struct ltt_event_header_nohb {
  uint64_t      timestamp;
  unsigned char facility_id;
  unsigned char event_id;
  uint16_t      event_size;
} __attribute((packed));

extern __thread struct ltt_trace_info *thread_trace_info;

void ltt_thread_init(void);

void __attribute__((no_instrument_function))
	ltt_usertrace_fast_buffer_switch(void);

/* Get the offset of the channel in the ltt_trace_struct */
#define GET_CHANNEL_INDEX(chan) \
  (unsigned int)&((struct ltt_trace_info*)NULL)->channel.chan

/* ltt_get_index_from_facility
 *
 * Get channel index from facility and event id.
 * 
 * @fID : facility ID
 * @eID : event number
 *
 * Get the channel index into which events must be written for the given
 * facility and event number. We get this structure offset as soon as possible
 * and remember it so we pass through this logic only once per trace call (not
 * for every trace).
 */
static inline unsigned int __attribute__((no_instrument_function))
		ltt_get_index_from_facility(ltt_facility_t fID,
																uint8_t eID)
{
	return GET_CHANNEL_INDEX(process);
}


static inline struct ltt_buf * __attribute__((no_instrument_function))
	ltt_get_channel_from_index(
		struct ltt_trace_info *trace, unsigned int index)
{
	return (struct ltt_buf *)((void*)trace+index);
}


/*
 * ltt_get_header_size
 *
 * Calculate alignment offset for arch size void*. This is the
 * alignment offset of the event header.
 *
 * Important note :
 * The event header must be a size multiple of the void* size. This is necessary
 * to be able to calculate statically the alignment offset of the variable
 * length data fields that follows. The total offset calculated here :
 *
 *   Alignment of header struct on arch size
 * + sizeof(header struct)
 * + padding added to end of struct to align on arch size.
 * */
static inline unsigned char __attribute__((no_instrument_function))
														ltt_get_header_size(struct ltt_trace_info *trace,
                                                void *address,
                                                size_t *before_hdr_pad,
                                                size_t *after_hdr_pad,
                                                size_t *header_size)
{
  unsigned int padding;
  unsigned int header;

  header = sizeof(struct ltt_event_header_nohb);

  /* Padding before the header. Calculated dynamically */
  *before_hdr_pad = ltt_align((unsigned long)address, header);
  padding = *before_hdr_pad;

  /* Padding after header, considering header aligned on ltt_align.
   * Calculated statically if header size if known. */
  *after_hdr_pad = ltt_align(header, sizeof(void*));
  padding += *after_hdr_pad;

  *header_size = header;

  return header+padding;
}


/* ltt_write_event_header
 *
 * Writes the event header to the pointer.
 *
 * @channel : pointer to the channel structure
 * @ptr : buffer pointer
 * @fID : facility ID
 * @eID : event ID
 * @event_size : size of the event, excluding the event header.
 * @offset : offset of the beginning of the header, for alignment.
 * 					 Calculated by ltt_get_event_header_size.
 * @tsc : time stamp counter.
 */
static inline void __attribute__((no_instrument_function))
	ltt_write_event_header(
		struct ltt_trace_info *trace, struct ltt_buf *buf,
		void *ptr, ltt_facility_t fID, uint32_t eID, size_t event_size,
		size_t offset, uint64_t tsc)
{
	struct ltt_event_header_nohb *nohb;
	
	event_size = min(event_size, 0xFFFFU);
	nohb = (struct ltt_event_header_nohb *)(ptr+offset);
	nohb->timestamp = (uint64_t)tsc;
	nohb->facility_id = fID;
	nohb->event_id = eID;
	nohb->event_size = (uint16_t)event_size;
}



static inline uint64_t __attribute__((no_instrument_function))
ltt_get_timestamp()
{
	return get_cycles();
}

static inline unsigned int __attribute__((no_instrument_function))
ltt_subbuf_header_len(struct ltt_buf *buf)
{
	return sizeof(struct ltt_block_start_header);
}



static inline void __attribute__((no_instrument_function))
ltt_write_trace_header(struct ltt_trace_header *header)
{
	header->magic_number = LTT_TRACER_MAGIC_NUMBER;
	header->major_version = LTT_TRACER_VERSION_MAJOR;
	header->minor_version = LTT_TRACER_VERSION_MINOR;
	header->float_word_order = 0;	//FIXME
	header->arch_type = 0; //FIXME LTT_ARCH_TYPE;
	header->arch_size = sizeof(void*);
	header->arch_variant = 0; //FIXME LTT_ARCH_VARIANT;
	header->flight_recorder = 0;
	header->has_heartbeat = 0;

#ifndef LTT_PACK
	header->has_alignment = sizeof(void*);
#else
	header->has_alignment = 0;
#endif
	
	//FIXME
	header->freq_scale = 0;
	header->start_freq = 0;
	header->start_tsc = 0;
	header->start_monotonic = 0;
	header->start_time_sec = 0;
	header->start_time_usec = 0;
}


static inline void __attribute__((no_instrument_function))
ltt_buffer_begin_callback(struct ltt_buf *buf,
		      uint64_t tsc, unsigned int subbuf_idx)
{
	struct ltt_block_start_header *header = 
					(struct ltt_block_start_header*)
						(buf->start + (subbuf_idx*buf->subbuf_size));
	
	header->begin.cycle_count = tsc;
	header->begin.freq = 0; //ltt_frequency();

	header->lost_size = 0xFFFFFFFF; // for debugging...
	
	header->buf_size = buf->subbuf_size;
	
	ltt_write_trace_header(&header->trace);

}



static inline void __attribute__((no_instrument_function))
ltt_buffer_end_callback(struct ltt_buf *buf,
		      uint64_t tsc, unsigned int offset, unsigned int subbuf_idx)
{
	struct ltt_block_start_header *header = 
						(struct ltt_block_start_header*)
								(buf->start + (subbuf_idx*buf->subbuf_size));
  /* offset is assumed to never be 0 here : never deliver a completely
   * empty subbuffer. */
  /* The lost size is between 0 and subbuf_size-1 */
	header->lost_size = SUBBUF_OFFSET((buf->subbuf_size - offset),
																		buf);
	header->end.cycle_count = tsc;
	header->end.freq = 0; //ltt_frequency();
}


static inline void __attribute__((no_instrument_function))
ltt_deliver_callback(struct ltt_buf *buf,
    unsigned subbuf_idx,
    void *subbuf)
{
	ltt_usertrace_fast_buffer_switch();
}


/* ltt_reserve_slot
 *
 * Atomic slot reservation in a LTTng buffer. It will take care of
 * sub-buffer switching.
 *
 * Parameters:
 *
 * @trace : the trace structure to log to.
 * @buf : the buffer to reserve space into.
 * @data_size : size of the variable length data to log.
 * @slot_size : pointer to total size of the slot (out)
 * @tsc : pointer to the tsc at the slot reservation (out)
 * @before_hdr_pad : dynamic padding before the event header.
 * @after_hdr_pad : dynamic padding after the event header.
 *
 * Return : NULL if not enough space, else returns the pointer
 * 					to the beginning of the reserved slot. */
static inline void * __attribute__((no_instrument_function)) ltt_reserve_slot(
															struct ltt_trace_info *trace,
															struct ltt_buf *ltt_buf,
															unsigned int data_size,
															size_t *slot_size,
															uint64_t *tsc,
															size_t *before_hdr_pad,
															size_t *after_hdr_pad,
															size_t *header_size)
{
	int offset_begin, offset_end, offset_old;
	//int has_switch;
	int begin_switch, end_switch_current, end_switch_old;
	int reserve_commit_diff = 0;
	unsigned int size;
	int consumed_old, consumed_new;
	int commit_count, reserve_count;
	int ret;
	sigset_t oldset, set;
	int signals_disabled = 0;

	do {
		offset_old = atomic_read(&ltt_buf->offset);
		offset_begin = offset_old;
		//has_switch = 0;
		begin_switch = 0;
		end_switch_current = 0;
		end_switch_old = 0;
		*tsc = ltt_get_timestamp();
		if(*tsc == 0) {
			/* Error in getting the timestamp, event lost */
			atomic_inc(&ltt_buf->events_lost);
			return NULL;
		}

		if(SUBBUF_OFFSET(offset_begin, ltt_buf) == 0) {
			begin_switch = 1; /* For offset_begin */
		} else {
			size = ltt_get_header_size(trace, ltt_buf->start + offset_begin,
																 before_hdr_pad, after_hdr_pad, header_size)
						 + data_size;

			if((SUBBUF_OFFSET(offset_begin, ltt_buf)+size)>ltt_buf->subbuf_size) {
				//has_switch = 1;
				end_switch_old = 1;	/* For offset_old */
				begin_switch = 1;	/* For offset_begin */
			}
		}

		if(begin_switch) {
			if(end_switch_old) {
				offset_begin = SUBBUF_ALIGN(offset_begin, ltt_buf);
			}
			offset_begin = offset_begin + ltt_subbuf_header_len(ltt_buf);
			/* Test new buffer integrity */
			reserve_commit_diff =
				atomic_read(&ltt_buf->reserve_count[SUBBUF_INDEX(offset_begin,
																												 ltt_buf)])
				- atomic_read(&ltt_buf->commit_count[SUBBUF_INDEX(offset_begin,
																						 ltt_buf)]);

			if(reserve_commit_diff == 0) {
				/* Next buffer not corrupted. */
				//if((SUBBUF_TRUNC(offset_begin, ltt_buf) 
				//				- SUBBUF_TRUNC(atomic_read(&ltt_buf->consumed), ltt_buf))
				//					>= ltt_buf->alloc_size) {
				{
					/* sem_wait is not signal safe. Disable signals around it.
					 * Signals are kept disabled to make sure we win the cmpxchg. */

					/* Disable signals */
					if(!signals_disabled) {
						ret = sigfillset(&set);
						if(ret) perror("LTT Error in sigfillset\n"); 
		
						ret = pthread_sigmask(SIG_BLOCK, &set, &oldset);
						if(ret) perror("LTT Error in pthread_sigmask\n");
						signals_disabled = 1;
					}
					sem_wait(&ltt_buf->writer_sem);

				}
					/* go on with the write */

				//} else {
				//	/* next buffer not corrupted, we are either in overwrite mode or
				//	 * the buffer is not full. It's safe to write in this new subbuffer.*/
				//}
			} else {
				/* Next subbuffer corrupted. Force pushing reader even in normal
				 * mode. It's safe to write in this new subbuffer. */
				/* No sem_post is required because we fall through without doing a
				 * sem_wait. */
			}
			size = ltt_get_header_size(trace, ltt_buf->start + offset_begin,
					before_hdr_pad, after_hdr_pad, header_size) + data_size;
			if((SUBBUF_OFFSET(offset_begin,ltt_buf)+size)>ltt_buf->subbuf_size) {
				/* Event too big for subbuffers, report error, don't complete 
				 * the sub-buffer switch. */
				atomic_inc(&ltt_buf->events_lost);
				if(reserve_commit_diff == 0) {
					ret = pthread_sigmask(SIG_SETMASK, &oldset, NULL);
					if(ret) perror("LTT Error in pthread_sigmask\n");
				}
				return NULL;
			} else {
				/* We just made a successful buffer switch and the event fits in the
				 * new subbuffer. Let's write. */
			}
		} else {
			/* Event fits in the current buffer and we are not on a switch boundary.
			 * It's safe to write */
		}
		offset_end = offset_begin + size;

		if((SUBBUF_OFFSET(offset_end, ltt_buf))	== 0) {
			/* The offset_end will fall at the very beginning of the next subbuffer.
			 */
			end_switch_current = 1;	/* For offset_begin */
		}

	} while(atomic_cmpxchg(&ltt_buf->offset, offset_old, offset_end)
							!= offset_old);

	/* Push the reader if necessary */
	do {
		consumed_old = atomic_read(&ltt_buf->consumed);
		/* If buffer is in overwrite mode, push the reader consumed count if
			 the write position has reached it and we are not at the first
			 iteration (don't push the reader farther than the writer). 
			 This operation can be done concurrently by many writers in the
			 same buffer, the writer being at the fartest write position sub-buffer
			 index in the buffer being the one which will win this loop. */
		/* If the buffer is not in overwrite mode, pushing the reader only
			 happen if a sub-buffer is corrupted */
		if((SUBBUF_TRUNC(offset_end-1, ltt_buf) 
					- SUBBUF_TRUNC(consumed_old, ltt_buf)) 
							>= ltt_buf->alloc_size)
			consumed_new = SUBBUF_ALIGN(consumed_old, ltt_buf);
		else {
			consumed_new = consumed_old;
			break;
		}
	} while(atomic_cmpxchg(&ltt_buf->consumed, consumed_old, consumed_new)
						!= consumed_old);

	if(consumed_old != consumed_new) {
		/* Reader pushed : we are the winner of the push, we can therefore
			 reequilibrate reserve and commit. Atomic increment of the commit
			 count permits other writers to play around with this variable
			 before us. We keep track of corrupted_subbuffers even in overwrite mode :
			 we never want to write over a non completely committed sub-buffer : 
			 possible causes : the buffer size is too low compared to the unordered
			 data input, or there is a writer who died between the reserve and the
			 commit. */
		if(reserve_commit_diff) {
			/* We have to alter the sub-buffer commit count : a sub-buffer is
				 corrupted. We do not deliver it. */
			atomic_add(reserve_commit_diff,
								&ltt_buf->commit_count[SUBBUF_INDEX(offset_begin, ltt_buf)]);
			atomic_inc(&ltt_buf->corrupted_subbuffers);
		}
	}


	if(end_switch_old) {
		/* old subbuffer */
		/* Concurrency safe because we are the last and only thread to alter this
			 sub-buffer. As long as it is not delivered and read, no other thread can
			 alter the offset, alter the reserve_count or call the
			 client_buffer_end_callback on this sub-buffer.
			 The only remaining threads could be the ones with pending commits. They
			 will have to do the deliver themself.
			 Not concurrency safe in overwrite mode. We detect corrupted subbuffers 
			 with commit and reserve counts. We keep a corrupted sub-buffers count
			 and push the readers across these sub-buffers.
			 Not concurrency safe if a writer is stalled in a subbuffer and
			 another writer switches in, finding out it's corrupted. The result will
			 be than the old (uncommited) subbuffer will be declared corrupted, and
			 that the new subbuffer will be declared corrupted too because of the
			 commit count adjustment.
			 Note : offset_old should never be 0 here.*/
		ltt_buffer_end_callback(ltt_buf, *tsc, offset_old, 
														SUBBUF_INDEX((offset_old-1), ltt_buf));
		/* Setting this reserve_count will allow the sub-buffer to be delivered by
			 the last committer. */
		reserve_count = 
						 atomic_add_return((SUBBUF_OFFSET((offset_old-1), ltt_buf)+1),
						 &ltt_buf->reserve_count[SUBBUF_INDEX((offset_old-1), ltt_buf)]);
		if(reserve_count 
					== atomic_read(&ltt_buf->commit_count[SUBBUF_INDEX((offset_old-1),
																															ltt_buf)])) {
			ltt_deliver_callback(ltt_buf, SUBBUF_INDEX((offset_old-1), ltt_buf),
														 NULL);
		}
	}

	if(begin_switch) {
		/* Enable signals : this is what guaranteed that same reserve which did the
		 * sem_wait does in fact win the cmpxchg for the offset. We only call
		 * these system calls on buffer boundaries because of their performance
		 * cost. */
		if(signals_disabled) {
			ret = pthread_sigmask(SIG_SETMASK, &oldset, NULL);
			if(ret) perror("LTT Error in pthread_sigmask\n");
		}
		/* New sub-buffer */
		/* This code can be executed unordered : writers may already have written
			 to the sub-buffer before this code gets executed, caution. */
		/* The commit makes sure that this code is executed before the deliver
			 of this sub-buffer */
		ltt_buffer_begin_callback(ltt_buf, *tsc, SUBBUF_INDEX(offset_begin, ltt_buf));
		commit_count = atomic_add_return(ltt_subbuf_header_len(ltt_buf),
							 &ltt_buf->commit_count[SUBBUF_INDEX(offset_begin, ltt_buf)]);
		/* Check if the written buffer has to be delivered */
		if(commit_count
					== atomic_read(&ltt_buf->reserve_count[SUBBUF_INDEX(offset_begin,
																															ltt_buf)])) {
			ltt_deliver_callback(ltt_buf, SUBBUF_INDEX(offset_begin, ltt_buf), NULL);
		}
	}

	if(end_switch_current) {
		/* current subbuffer */
		/* Concurrency safe because we are the last and only thread to alter this
			 sub-buffer. As long as it is not delivered and read, no other thread can
			 alter the offset, alter the reserve_count or call the
			 client_buffer_end_callback on this sub-buffer.
			 The only remaining threads could be the ones with pending commits. They
			 will have to do the deliver themself.
			 Not concurrency safe in overwrite mode. We detect corrupted subbuffers 
			 with commit and reserve counts. We keep a corrupted sub-buffers count
			 and push the readers across these sub-buffers.
			 Not concurrency safe if a writer is stalled in a subbuffer and
			 another writer switches in, finding out it's corrupted. The result will
			 be than the old (uncommited) subbuffer will be declared corrupted, and
			 that the new subbuffer will be declared corrupted too because of the
			 commit count adjustment. */
		ltt_buffer_end_callback(ltt_buf, *tsc, offset_end,
														SUBBUF_INDEX((offset_end-1), ltt_buf));
		/* Setting this reserve_count will allow the sub-buffer to be delivered by
			 the last committer. */
		reserve_count = 
      atomic_add_return((SUBBUF_OFFSET((offset_end-1), ltt_buf)+1),
			&ltt_buf->reserve_count[SUBBUF_INDEX((offset_end-1), ltt_buf)]);
		if(reserve_count 
					== atomic_read(&ltt_buf->commit_count[SUBBUF_INDEX((offset_end-1),
																															ltt_buf)])) {
			ltt_deliver_callback(ltt_buf, SUBBUF_INDEX((offset_end-1), ltt_buf), NULL);
		}
	}

	*slot_size = size;

	//BUG_ON(*slot_size != (data_size + *before_hdr_pad + *after_hdr_pad + *header_size));
	//BUG_ON(*slot_size != (offset_end - offset_begin));
	
	return ltt_buf->start + BUFFER_OFFSET(offset_begin, ltt_buf);
}
	
	
/* ltt_commit_slot
 *
 * Atomic unordered slot commit. Increments the commit count in the
 * specified sub-buffer, and delivers it if necessary.
 *
 * Parameters:
 *
 * @buf : the buffer to commit to.
 * @reserved : address of the beginnig of the reserved slot.
 * @slot_size : size of the reserved slot.
 *
 */
static inline void __attribute__((no_instrument_function)) ltt_commit_slot(
															struct ltt_buf *ltt_buf,
															void *reserved,
															unsigned int slot_size)
{
	unsigned int offset_begin = reserved - ltt_buf->start;
	int commit_count;

	commit_count = atomic_add_return(slot_size,
													&ltt_buf->commit_count[SUBBUF_INDEX(offset_begin,
																															ltt_buf)]);
	
	/* Check if all commits have been done */
	if(commit_count	==
	atomic_read(&ltt_buf->reserve_count[SUBBUF_INDEX(offset_begin, ltt_buf)])) {
		ltt_deliver_callback(ltt_buf, SUBBUF_INDEX(offset_begin, ltt_buf), NULL);
	}
}

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif //LTT_TRACE_FAST
#endif //LTT_TRACE
#endif //_LTT_USERTRACE_FAST_H
