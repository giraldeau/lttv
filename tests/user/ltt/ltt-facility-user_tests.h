#ifndef _LTT_FACILITY_USER_TESTS_H_
#define _LTT_FACILITY_USER_TESTS_H_

#include <sys/types.h>
#include <ltt/ltt-facility-id-user_tests.h>
#include <ltt/ltt-usertrace.h>

/* Named types */

/* Event write_4bytes structures */

/* Event write_4bytes logging function */
#ifndef LTT_TRACE_FAST
static inline int trace_user_tests_write_4bytes(
		int lttng_param_data)
#ifndef LTT_TRACE
{
}
#else
{
	int ret = 0;
	void *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	size_t align;
	const void *real_from;
	const void **from = &real_from;
		/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = &lttng_param_data;
	align = sizeof(int);

	if(*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(int);

	reserve_size = *to_base + *to + *len;
	{
		char stack_buffer[reserve_size];
		buffer = stack_buffer;

		*to_base = *to = *len = 0;

		*from = &lttng_param_data;
		align = sizeof(int);

		if(*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(int);

		/* Flush pending memcpy */
		if(*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		ret = ltt_trace_generic(ltt_facility_user_tests_CDD24456, event_user_tests_write_4bytes, buffer, reserve_size, LTT_BLOCKING);
	}

	return ret;

}
#endif //LTT_TRACE
#endif //!LTT_TRACE_FAST

#ifdef LTT_TRACE_FAST
static inline int trace_user_tests_write_4bytes(
		int lttng_param_data)
#ifndef LTT_TRACE
{
}
#else
{
	unsigned int index;
	struct ltt_trace_info *trace = thread_trace_info;
	struct ltt_buf *ltt_buf;
	void *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	size_t align;
	const void *real_from;
	const void **from = &real_from;
	uint64_t tsc;
	size_t before_hdr_pad, after_hdr_pad, header_size;

	if(!trace) {
		ltt_thread_init();
		trace = thread_trace_info;
	}


	/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = &lttng_param_data;
	align = sizeof(int);

	if(*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(int);

	reserve_size = *to_base + *to + *len;
	trace->nesting++;
	index = ltt_get_index_from_facility(ltt_facility_user_tests_CDD24456,
						event_user_tests_write_4bytes);

	{
		ltt_buf = ltt_get_channel_from_index(trace, index);
				slot_size = 0;
		buffer = ltt_reserve_slot(trace, ltt_buf,
			reserve_size, &slot_size, &tsc,
			&before_hdr_pad, &after_hdr_pad, &header_size);
		if(!buffer) goto end; /* buffer full */

		*to_base = *to = *len = 0;

		ltt_write_event_header(trace, ltt_buf, buffer,
			ltt_facility_user_tests_CDD24456, event_user_tests_write_4bytes,
			reserve_size, before_hdr_pad, tsc);
		*to_base += before_hdr_pad + after_hdr_pad + header_size;

		*from = &lttng_param_data;
		align = sizeof(int);

		if(*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(int);

		/* Flush pending memcpy */
		if(*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		ltt_commit_slot(ltt_buf, buffer, slot_size);

}

end:
	trace->nesting--;
}
#endif //LTT_TRACE
#endif //LTT_TRACE_FAST

#endif //_LTT_FACILITY_USER_TESTS_H_
