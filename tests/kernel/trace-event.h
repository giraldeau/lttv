#include <stdarg.h>

/* LTT flags
 *
 * LTT_FLAG_TRACE : first arg contains trace to write into.
 * 		    (type : struct ltt_trace_struct *)
 * LTT_FLAG_CHANNEL : following arg contains channel index to write into.
 *                    (type : uint8_t)
 * LTT_FLAG_FORCE : Force write in disabled traces (internal ltt use)
 */

#define _LTT_FLAG_TRACE		0	
#define _LTT_FLAG_CHANNEL	1
#define _LTT_FLAG_FORCE		2

#define LTT_FLAG_TRACE		(1 << _LTT_FLAG_TRACE)
#define LTT_FLAG_CHANNEL	(1 << _LTT_FLAG_CHANNEL)
#define LTT_FLAG_FORCE		(1 << _LTT_FLAG_FORCE)

/* Calculate data size */
/* Assume that the padding for alignment starts at a
 * sizeof(void *) address. */
static inline __attribute__((no_instrument_function))
size_t ltt_get_data_size(ltt_facility_t fID, uint8_t eID,
				const char *fmt, va_list args)
{



}

static inline __attribute__((no_instrument_function))
size_t ltt_write_event_data(char *buffer,
				ltt_facility_t fID, uint8_t eID,
				const char *fmt, va_list args)
{



}


__attribute__((no_instrument_function))
void vtrace(ltt_facility_t fID, uint8_t eID, long flags, 
		const char *fmt, va_list args)
{
	size_t data_size, slot_size;
	uint8_t channel_index;
	struct ltt_channel_struct *channel;
	struct ltt_trace_struct *trace, *dest_trace;
	void *transport_data;
	uint64_t tsc;
	char *buffer;

	/* This test is useful for quickly exiting static tracing when no
	 * trace is active. */
	if (likely(ltt_traces.num_active_traces == 0 && !(flags & LTT_FLAG_FORCE)))
		return;

	data_size = ltt_get_data_size(fID, eID, fmt, args);
	preempt_disable();
	ltt_nesting[smp_processor_id()]++;

	if (unlikely(flags & LTT_FLAG_TRACE))
		dest_trace = va_arg(args, struct ltt_trace_struct *);
	if (unlikely(flags & LTT_FLAG_CHANNEL))
		channel_index = va_arg(args, uint8_t);
	else
		channel_index = ltt_get_channel_index(fID, eID);

	/* Iterate on each traces */
	list_for_each_entry_rcu(trace, &ltt_traces.head, list) {
		if (unlikely(!trace->active && !(flags & LTT_FLAG_FORCE)))
			continue;
		if (unlikely(flags & LTT_FLAG_TRACE && trace != dest_trace))
			continue;
		channel = ltt_get_channel_from_index(trace, channel_index);
		/* reserve space : header and data */
		buffer = ltt_reserve_slot(trace, channel, &transport_data,
						data_size, &slot_size, &tsc);
		if (unlikely(!buffer))
			continue; /* buffer full */
		/* Out-of-order write : header and data */
		buffer = ltt_write_event_header(trace, channel, buffer,
						fID, eID, data_size, tsc);
		ltt_write_event_data(buffer, fID, eID, fmt, args);
		/* Out-of-order commit */
		ltt_commit_slot(channel, &transport_data, buffer, slot_size);
	}

	ltt_nesting[smp_processor_id()]--;
	preempt_enable();
}

__attribute__((no_instrument_function))
void trace(ltt_facility_t fID, uint8_t eID, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vtrace(fmt, args);
	va_end(args);
}

