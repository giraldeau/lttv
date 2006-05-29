#ifndef _LTT_FACILITY_TESTS_H_
#define _LTT_FACILITY_TESTS_H_

#include <linux/types.h>
#include "ltt-facility-id-tests.h"
#include <linux/ltt-core.h>

/* Named types */

/* Event write_4bytes structures */

/* Event write_4bytes logging function */
static inline void trace_tests_write_4bytes(
		int lttng_param_data)
#if (!defined(CONFIG_LTT) || !defined(CONFIG_LTT_FACILITY_TESTS))
{
}
#else
{
	unsigned int index;
	struct ltt_channel_struct *channel;
	struct ltt_trace_struct *trace;
	struct rchan_buf *relayfs_buf;
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
	u64 tsc;
	size_t before_hdr_pad, after_hdr_pad, header_size;

	if(ltt_traces.num_active_traces == 0) return;

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
	preempt_disable();
	ltt_nesting[smp_processor_id()]++;
	index = ltt_get_index_from_facility(ltt_facility_tests_2F06D8DB,
						event_tests_write_4bytes);

	list_for_each_entry_rcu(trace, &ltt_traces.head, list) {
		if(!trace->active) continue;

		channel = ltt_get_channel_from_index(trace, index);
		relayfs_buf = channel->rchan->buf[smp_processor_id()];

		slot_size = 0;
		buffer = ltt_reserve_slot(trace, relayfs_buf,
			reserve_size, &slot_size, &tsc,
			&before_hdr_pad, &after_hdr_pad, &header_size);
		if(!buffer) continue; /* buffer full */

		*to_base = *to = *len = 0;

		ltt_write_event_header(trace, channel, buffer,
			ltt_facility_tests_2F06D8DB, event_tests_write_4bytes,
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

		ltt_commit_slot(relayfs_buf, buffer, slot_size);

	}

	ltt_nesting[smp_processor_id()]--;
	preempt_enable_no_resched();
}
#endif //(!defined(CONFIG_LTT) || !defined(CONFIG_LTT_FACILITY_TESTS))


/* Event write_string structures */
static inline void lttng_write_string_tests_write_string_data(
		void *buffer,
		size_t *to_base,
		size_t *to,
		const void **from,
		size_t *len,
		const char * obj)
{
	size_t size;
	size_t align;

	/* Flush pending memcpy */
	if(*len != 0) {
		if(buffer != NULL)
			memcpy(buffer+*to_base+*to, *from, *len);
	}
	*to += *len;
	*len = 0;

	align = sizeof(char);

	if(*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	/* Contains variable sized fields : must explode the structure */

	size = strlen(obj) + 1; /* Include final NULL char. */
	if(buffer != NULL)
		memcpy(buffer+*to_base+*to, obj, size);
	*to += size;

	/* Realign the *to_base on arch size, set *to to 0 */
	*to += ltt_align(*to, sizeof(void *));
	*to_base = *to_base+*to;
	*to = 0;

	/* Put source *from just after the C string */
	*from += size;
}


/* Event write_string logging function */
static inline void trace_tests_write_string(
		const char * lttng_param_data)
#if (!defined(CONFIG_LTT) || !defined(CONFIG_LTT_FACILITY_TESTS))
{
}
#else
{
	unsigned int index;
	struct ltt_channel_struct *channel;
	struct ltt_trace_struct *trace;
	struct rchan_buf *relayfs_buf;
	void *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	const void *real_from;
	const void **from = &real_from;
	u64 tsc;
	size_t before_hdr_pad, after_hdr_pad, header_size;

	if(ltt_traces.num_active_traces == 0) return;

	/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = lttng_param_data;
	lttng_write_string_tests_write_string_data(buffer, to_base, to, from, len, lttng_param_data);

	reserve_size = *to_base + *to + *len;
	preempt_disable();
	ltt_nesting[smp_processor_id()]++;
	index = ltt_get_index_from_facility(ltt_facility_tests_2F06D8DB,
						event_tests_write_string);

	list_for_each_entry_rcu(trace, &ltt_traces.head, list) {
		if(!trace->active) continue;

		channel = ltt_get_channel_from_index(trace, index);
		relayfs_buf = channel->rchan->buf[smp_processor_id()];

		slot_size = 0;
		buffer = ltt_reserve_slot(trace, relayfs_buf,
			reserve_size, &slot_size, &tsc,
			&before_hdr_pad, &after_hdr_pad, &header_size);
		if(!buffer) continue; /* buffer full */

		*to_base = *to = *len = 0;

		ltt_write_event_header(trace, channel, buffer,
			ltt_facility_tests_2F06D8DB, event_tests_write_string,
			reserve_size, before_hdr_pad, tsc);
		*to_base += before_hdr_pad + after_hdr_pad + header_size;

		*from = lttng_param_data;
		lttng_write_string_tests_write_string_data(buffer, to_base, to, from, len, lttng_param_data);

		/* Flush pending memcpy */
		if(*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		ltt_commit_slot(relayfs_buf, buffer, slot_size);

	}

	ltt_nesting[smp_processor_id()]--;
	preempt_enable_no_resched();
}
#endif //(!defined(CONFIG_LTT) || !defined(CONFIG_LTT_FACILITY_TESTS))


/* Event write_struct structures */
typedef struct lttng_sequence_tests_write_struct_data2_data5 lttng_sequence_tests_write_struct_data2_data5;
struct lttng_sequence_tests_write_struct_data2_data5 {
	unsigned int len;
	const int64_t *array;
};

struct lttng_tests_write_struct_data2 {
	const char * data3;
	int data4;
	lttng_sequence_tests_write_struct_data2_data5 data5;
	int data6;
} LTT_ALIGN;

static inline size_t lttng_get_alignment_sequence_tests_write_struct_data2_data5(
		lttng_sequence_tests_write_struct_data2_data5 *obj)
{
	size_t align=0, localign;
	localign = sizeof(unsigned int);
	align = max(align, localign);

	localign = sizeof(int64_t);
	align = max(align, localign);

	return align;
}

static inline size_t lttng_get_alignment_struct_tests_write_struct_data2(
		struct lttng_tests_write_struct_data2 *obj)
{
	size_t align=0, localign;
	localign = sizeof(char);
	align = max(align, localign);

	localign = sizeof(int);
	align = max(align, localign);

	localign = lttng_get_alignment_sequence_tests_write_struct_data2_data5(&obj->data5);
	align = max(align, localign);

	localign = sizeof(int);
	align = max(align, localign);

	return align;
}

static inline void lttng_write_string_tests_write_struct_data2_data3(
		void *buffer,
		size_t *to_base,
		size_t *to,
		const void **from,
		size_t *len,
		const char * obj)
{
	size_t size;
	size_t align;

	/* Flush pending memcpy */
	if(*len != 0) {
		if(buffer != NULL)
			memcpy(buffer+*to_base+*to, *from, *len);
	}
	*to += *len;
	*len = 0;

	align = sizeof(char);

	if(*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	/* Contains variable sized fields : must explode the structure */

	size = strlen(obj) + 1; /* Include final NULL char. */
	if(buffer != NULL)
		memcpy(buffer+*to_base+*to, obj, size);
	*to += size;

	/* Realign the *to_base on arch size, set *to to 0 */
	*to += ltt_align(*to, sizeof(void *));
	*to_base = *to_base+*to;
	*to = 0;

	/* Put source *from just after the C string */
	*from += size;
}

static inline void lttng_write_sequence_tests_write_struct_data2_data5(
		void *buffer,
		size_t *to_base,
		size_t *to,
		const void **from,
		size_t *len,
		lttng_sequence_tests_write_struct_data2_data5 *obj)
{
	size_t align;

	/* Flush pending memcpy */
	if(*len != 0) {
		if(buffer != NULL)
			memcpy(buffer+*to_base+*to, *from, *len);
	}
	*to += *len;
	*len = 0;

	align = lttng_get_alignment_sequence_tests_write_struct_data2_data5(obj);

	if(*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	/* Contains variable sized fields : must explode the structure */

	/* Copy members */
	align = sizeof(unsigned int);

	if(*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	if(buffer != NULL)
		memcpy(buffer+*to_base+*to, &obj->len, *len);
	*to += *len;
	*len = 0;

	align = sizeof(int64_t);

	if(*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(int64_t);

	*len = obj->len * (*len);
	if(buffer != NULL)
		memcpy(buffer+*to_base+*to, obj->array, *len);
	*to += *len;
	*len = 0;


	/* Realign the *to_base on arch size, set *to to 0 */
	*to += ltt_align(*to, sizeof(void *));
	*to_base = *to_base+*to;
	*to = 0;

	/* Put source *from just after the C sequence */
	*from = obj+1;
}

static inline void lttng_write_struct_tests_write_struct_data2(
		void *buffer,
		size_t *to_base,
		size_t *to,
		const void **from,
		size_t *len,
		struct lttng_tests_write_struct_data2 *obj)
{
	size_t align;

	align = lttng_get_alignment_struct_tests_write_struct_data2(obj);

	if(*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	/* Contains variable sized fields : must explode the structure */

	lttng_write_string_tests_write_struct_data2_data3(buffer, to_base, to, from, len, obj->data3);

	align = sizeof(int);

	if(*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(int);

	lttng_write_sequence_tests_write_struct_data2_data5(buffer, to_base, to, from, len, &obj->data5);
	align = sizeof(int);

	if(*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(int);

}


/* Event write_struct logging function */
static inline void trace_tests_write_struct(
		int lttng_param_data1,
		struct lttng_tests_write_struct_data2 * lttng_param_data2)
#if (!defined(CONFIG_LTT) || !defined(CONFIG_LTT_FACILITY_TESTS))
{
}
#else
{
	unsigned int index;
	struct ltt_channel_struct *channel;
	struct ltt_trace_struct *trace;
	struct rchan_buf *relayfs_buf;
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
	u64 tsc;
	size_t before_hdr_pad, after_hdr_pad, header_size;

	if(ltt_traces.num_active_traces == 0) return;

	/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = &lttng_param_data1;
	align = sizeof(int);

	if(*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(int);

	*from = lttng_param_data2;
	lttng_write_struct_tests_write_struct_data2(buffer, to_base, to, from, len, lttng_param_data2);
	reserve_size = *to_base + *to + *len;
	preempt_disable();
	ltt_nesting[smp_processor_id()]++;
	index = ltt_get_index_from_facility(ltt_facility_tests_2F06D8DB,
						event_tests_write_struct);

	list_for_each_entry_rcu(trace, &ltt_traces.head, list) {
		if(!trace->active) continue;

		channel = ltt_get_channel_from_index(trace, index);
		relayfs_buf = channel->rchan->buf[smp_processor_id()];

		slot_size = 0;
		buffer = ltt_reserve_slot(trace, relayfs_buf,
			reserve_size, &slot_size, &tsc,
			&before_hdr_pad, &after_hdr_pad, &header_size);
		if(!buffer) continue; /* buffer full */

		*to_base = *to = *len = 0;

		ltt_write_event_header(trace, channel, buffer,
			ltt_facility_tests_2F06D8DB, event_tests_write_struct,
			reserve_size, before_hdr_pad, tsc);
		*to_base += before_hdr_pad + after_hdr_pad + header_size;

		*from = &lttng_param_data1;
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

		*from = lttng_param_data2;
		lttng_write_struct_tests_write_struct_data2(buffer, to_base, to, from, len, lttng_param_data2);
		/* Flush pending memcpy */
		if(*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		ltt_commit_slot(relayfs_buf, buffer, slot_size);

	}

	ltt_nesting[smp_processor_id()]--;
	preempt_enable_no_resched();
}
#endif //(!defined(CONFIG_LTT) || !defined(CONFIG_LTT_FACILITY_TESTS))


#endif //_LTT_FACILITY_TESTS_H_
