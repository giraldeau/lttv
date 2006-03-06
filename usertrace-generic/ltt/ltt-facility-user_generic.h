#ifndef _LTT_FACILITY_USER_GENERIC_H_
#define _LTT_FACILITY_USER_GENERIC_H_

#include <sys/types.h>
#include <ltt/ltt-facility-id-user_generic.h>
#include <ltt/ltt-generic.h>

/* Named types */

/* Event string structures */
static inline void lttng_write_string_user_generic_string_data(
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


/* Event string logging function */
static inline int trace_user_generic_string(
		const char * lttng_param_data)
#ifndef LTT_TRACE
{
}
#else
{
	void *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	int ret = 0;
	const void *real_from;
	const void **from = &real_from;
		/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = lttng_param_data;
	lttng_write_string_user_generic_string_data(buffer, to_base, to, from, len, lttng_param_data);

	reserve_size = *to_base + *to + *len;
	{
		char stack_buffer[reserve_size];
		buffer = stack_buffer;

		*to_base = *to = *len = 0;

		*from = lttng_param_data;
		lttng_write_string_user_generic_string_data(buffer, to_base, to, from, len, lttng_param_data);

		/* Flush pending memcpy */
		if(*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		ret = ltt_trace_generic(ltt_facility_user_generic_411B0F83, event_user_generic_string, stack_buffer, sizeof(stack_buffer), LTT_BLOCKING);
	}

	return ret;

}
#endif //LTT_TRACE

/* Event string_pointer structures */
static inline void lttng_write_string_user_generic_string_pointer_string(
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


/* Event string_pointer logging function */
static inline int trace_user_generic_string_pointer(
		const char * lttng_param_string,
		const void * lttng_param_pointer)
#ifndef LTT_TRACE
{
}
#else
{
	void *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	int ret = 0;
	size_t align;
	const void *real_from;
	const void **from = &real_from;
		/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = lttng_param_string;
	lttng_write_string_user_generic_string_pointer_string(buffer, to_base, to, from, len, lttng_param_string);

	*from = &lttng_param_pointer;
	align = sizeof(const void *);

	if(*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(const void *);

	reserve_size = *to_base + *to + *len;
	{
		char stack_buffer[reserve_size];
		buffer = stack_buffer;

		*to_base = *to = *len = 0;

		*from = lttng_param_string;
		lttng_write_string_user_generic_string_pointer_string(buffer, to_base, to, from, len, lttng_param_string);

		/* Flush pending memcpy */
		if(*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = &lttng_param_pointer;
		align = sizeof(const void *);

		if(*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(const void *);

		/* Flush pending memcpy */
		if(*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		ret = ltt_trace_generic(ltt_facility_user_generic_411B0F83, event_user_generic_string_pointer, stack_buffer, sizeof(stack_buffer), LTT_BLOCKING);
	}

	return ret;

}
#endif //LTT_TRACE

#endif //_LTT_FACILITY_USER_GENERIC_H_
