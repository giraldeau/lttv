#ifndef _LTT_FACILITY_USER_GENERIC_H_
#define _LTT_FACILITY_USER_GENERIC_H_

#include <sys/types.h>
#include <ltt/ltt-facility-id-user_generic.h>
#include <ltt/ltt-usertrace.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Named types */

/* Event string structures */
static inline void lttng_write_string_user_generic_string_data(
		char *buffer,
		size_t *to_base,
		size_t *to,
		const char **from,
		size_t *len,
		const char * obj)
{
	size_t size;
	size_t align;

	/* Flush pending memcpy */
	if (*len != 0) {
		if (buffer != NULL)
			memcpy(buffer+*to_base+*to, *from, *len);
	}
	*to += *len;
	*len = 0;

	align = sizeof(char);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	/* Contains variable sized fields : must explode the structure */

	size = strlen(obj) + 1; /* Include final NULL char. */
	if (buffer != NULL)
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
		const char * lttng_param_data);

#ifndef LTT_TRACE_FAST
static inline int trace_user_generic_string(
		const char * lttng_param_data)
#ifndef LTT_TRACE
{
}
#else
{
	int ret = 0;
	char *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	const char *real_from;
	const char **from = &real_from;
		/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = (const char*)lttng_param_data;
	lttng_write_string_user_generic_string_data(buffer, to_base, to, from, len, lttng_param_data);

	reserve_size = *to_base + *to + *len;
	{
		char stack_buffer[reserve_size];
		buffer = stack_buffer;

		*to_base = *to = *len = 0;

		*from = (const char*)lttng_param_data;
		lttng_write_string_user_generic_string_data(buffer, to_base, to, from, len, lttng_param_data);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		ret = ltt_trace_generic(ltt_facility_user_generic_B1865E44, event_user_generic_string, buffer, reserve_size, LTT_BLOCKING, 0);
	}

	return ret;

}
#endif //LTT_TRACE
#endif //!LTT_TRACE_FAST

#ifdef LTT_TRACE_FAST
static inline int trace_user_generic_string(
		const char * lttng_param_data)
#ifndef LTT_TRACE
{
}
#else
{
	unsigned int index;
	struct ltt_trace_info *trace = thread_trace_info;
	struct ltt_buf *ltt_buf;
	char *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	const char *real_from;
	const char **from = &real_from;
	uint64_t tsc;
		if (!trace) {
		ltt_thread_init();
		trace = thread_trace_info;
	}


	/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = (const char*)lttng_param_data;
	lttng_write_string_user_generic_string_data(buffer, to_base, to, from, len, lttng_param_data);

	reserve_size = *to_base + *to + *len;
	trace->nesting++;
	index = ltt_get_index_from_facility(ltt_facility_user_generic_B1865E44,
						event_user_generic_string);

	{
		ltt_buf = ltt_get_channel_from_index(trace, index);
				slot_size = 0;
		buffer = ltt_reserve_slot(trace, ltt_buf,
			reserve_size, &slot_size, &tsc);
		if (!buffer)
			goto end; /* buffer full */

		*to_base = *to = *len = 0;

		buffer = ltt_write_event_header(trace, ltt_buf, buffer,
			ltt_facility_user_generic_B1865E44, event_user_generic_string,
			reserve_size, tsc);
		*from = (const char*)lttng_param_data;
		lttng_write_string_user_generic_string_data(buffer, to_base, to, from, len, lttng_param_data);

		/* Flush pending memcpy */
		if (*len != 0) {
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

/* Event string_pointer structures */
static inline void lttng_write_string_user_generic_string_pointer_string(
		char *buffer,
		size_t *to_base,
		size_t *to,
		const char **from,
		size_t *len,
		const char * obj)
{
	size_t size;
	size_t align;

	/* Flush pending memcpy */
	if (*len != 0) {
		if (buffer != NULL)
			memcpy(buffer+*to_base+*to, *from, *len);
	}
	*to += *len;
	*len = 0;

	align = sizeof(char);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	/* Contains variable sized fields : must explode the structure */

	size = strlen(obj) + 1; /* Include final NULL char. */
	if (buffer != NULL)
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
		const void * lttng_param_pointer);

#ifndef LTT_TRACE_FAST
static inline int trace_user_generic_string_pointer(
		const char * lttng_param_string,
		const void * lttng_param_pointer)
#ifndef LTT_TRACE
{
}
#else
{
	int ret = 0;
	char *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	size_t align;
	const char *real_from;
	const char **from = &real_from;
		/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = (const char*)lttng_param_string;
	lttng_write_string_user_generic_string_pointer_string(buffer, to_base, to, from, len, lttng_param_string);

	*from = (const char*)&lttng_param_pointer;
	align = sizeof(const void *);

	if (*len == 0) {
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

		*from = (const char*)lttng_param_string;
		lttng_write_string_user_generic_string_pointer_string(buffer, to_base, to, from, len, lttng_param_string);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_pointer;
		align = sizeof(const void *);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(const void *);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		ret = ltt_trace_generic(ltt_facility_user_generic_B1865E44, event_user_generic_string_pointer, buffer, reserve_size, LTT_BLOCKING, 0);
	}

	return ret;

}
#endif //LTT_TRACE
#endif //!LTT_TRACE_FAST

#ifdef LTT_TRACE_FAST
static inline int trace_user_generic_string_pointer(
		const char * lttng_param_string,
		const void * lttng_param_pointer)
#ifndef LTT_TRACE
{
}
#else
{
	unsigned int index;
	struct ltt_trace_info *trace = thread_trace_info;
	struct ltt_buf *ltt_buf;
	char *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	size_t align;
	const char *real_from;
	const char **from = &real_from;
	uint64_t tsc;
		if (!trace) {
		ltt_thread_init();
		trace = thread_trace_info;
	}


	/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = (const char*)lttng_param_string;
	lttng_write_string_user_generic_string_pointer_string(buffer, to_base, to, from, len, lttng_param_string);

	*from = (const char*)&lttng_param_pointer;
	align = sizeof(const void *);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(const void *);

	reserve_size = *to_base + *to + *len;
	trace->nesting++;
	index = ltt_get_index_from_facility(ltt_facility_user_generic_B1865E44,
						event_user_generic_string_pointer);

	{
		ltt_buf = ltt_get_channel_from_index(trace, index);
				slot_size = 0;
		buffer = ltt_reserve_slot(trace, ltt_buf,
			reserve_size, &slot_size, &tsc);
		if (!buffer)
			goto end; /* buffer full */

		*to_base = *to = *len = 0;

		buffer = ltt_write_event_header(trace, ltt_buf, buffer,
			ltt_facility_user_generic_B1865E44, event_user_generic_string_pointer,
			reserve_size, tsc);
		*from = (const char*)lttng_param_string;
		lttng_write_string_user_generic_string_pointer_string(buffer, to_base, to, from, len, lttng_param_string);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_pointer;
		align = sizeof(const void *);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(const void *);

		/* Flush pending memcpy */
		if (*len != 0) {
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

/* Event slow_printf structures */
static inline void lttng_write_string_user_generic_slow_printf_string(
		char *buffer,
		size_t *to_base,
		size_t *to,
		const char **from,
		size_t *len,
		const char * obj)
{
	size_t size;
	size_t align;

	/* Flush pending memcpy */
	if (*len != 0) {
		if (buffer != NULL)
			memcpy(buffer+*to_base+*to, *from, *len);
	}
	*to += *len;
	*len = 0;

	align = sizeof(char);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	/* Contains variable sized fields : must explode the structure */

	size = strlen(obj) + 1; /* Include final NULL char. */
	if (buffer != NULL)
		memcpy(buffer+*to_base+*to, obj, size);
	*to += size;

	/* Realign the *to_base on arch size, set *to to 0 */
	*to += ltt_align(*to, sizeof(void *));
	*to_base = *to_base+*to;
	*to = 0;

	/* Put source *from just after the C string */
	*from += size;
}


/* Event slow_printf logging function */
static inline int trace_user_generic_slow_printf_param_buffer(
		char *buffer,
		size_t reserve_size);

#ifndef LTT_TRACE_FAST
static inline int trace_user_generic_slow_printf_param_buffer(
		char *buffer,
		size_t reserve_size)
#ifndef LTT_TRACE
{
}
#else
{
	int ret = 0;
	{
		ret = ltt_trace_generic(ltt_facility_user_generic_B1865E44, event_user_generic_slow_printf, buffer, reserve_size, LTT_BLOCKING, 0);
	}

	return ret;

}
#endif //LTT_TRACE
#endif //!LTT_TRACE_FAST

#ifdef LTT_TRACE_FAST
static inline int trace_user_generic_slow_printf(
		const char * lttng_param_string)
#ifndef LTT_TRACE
{
}
#else
{
	unsigned int index;
	struct ltt_trace_info *trace = thread_trace_info;
	struct ltt_buf *ltt_buf;
	char *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	const char *real_from;
	const char **from = &real_from;
	uint64_t tsc;
		if (!trace) {
		ltt_thread_init();
		trace = thread_trace_info;
	}


	/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = (const char*)lttng_param_string;
	lttng_write_string_user_generic_slow_printf_string(buffer, to_base, to, from, len, lttng_param_string);

	reserve_size = *to_base + *to + *len;
	trace->nesting++;
	index = ltt_get_index_from_facility(ltt_facility_user_generic_B1865E44,
						event_user_generic_slow_printf);

	{
		ltt_buf = ltt_get_channel_from_index(trace, index);
				slot_size = 0;
		buffer = ltt_reserve_slot(trace, ltt_buf,
			reserve_size, &slot_size, &tsc);
		if (!buffer)
			goto end; /* buffer full */

		*to_base = *to = *len = 0;

		buffer = ltt_write_event_header(trace, ltt_buf, buffer,
			ltt_facility_user_generic_B1865E44, event_user_generic_slow_printf,
			reserve_size, tsc);
		*from = (const char*)lttng_param_string;
		lttng_write_string_user_generic_slow_printf_string(buffer, to_base, to, from, len, lttng_param_string);

		/* Flush pending memcpy */
		if (*len != 0) {
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

/* Event function_entry structures */

/* Event function_entry logging function */
static inline __attribute__((no_instrument_function)) int trace_user_generic_function_entry(
		const void * lttng_param_this_fn,
		const void * lttng_param_call_site);

#ifndef LTT_TRACE_FAST
static inline int trace_user_generic_function_entry(
		const void * lttng_param_this_fn,
		const void * lttng_param_call_site)
#ifndef LTT_TRACE
{
}
#else
{
	int ret = 0;
	char *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	size_t align;
	const char *real_from;
	const char **from = &real_from;
		/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = (const char*)&lttng_param_this_fn;
	align = sizeof(const void *);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(const void *);

	*from = (const char*)&lttng_param_call_site;
	align = sizeof(const void *);

	if (*len == 0) {
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

		*from = (const char*)&lttng_param_this_fn;
		align = sizeof(const void *);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(const void *);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_call_site;
		align = sizeof(const void *);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(const void *);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		ret = ltt_trace_generic(ltt_facility_user_generic_B1865E44, event_user_generic_function_entry, buffer, reserve_size, LTT_BLOCKING, 0);
	}

	return ret;

}
#endif //LTT_TRACE
#endif //!LTT_TRACE_FAST

#ifdef LTT_TRACE_FAST
static inline __attribute__((no_instrument_function)) int trace_user_generic_function_entry(
		const void * lttng_param_this_fn,
		const void * lttng_param_call_site)
#ifndef LTT_TRACE
{
}
#else
{
	unsigned int index;
	struct ltt_trace_info *trace = thread_trace_info;
	struct ltt_buf *ltt_buf;
	char *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	size_t align;
	const char *real_from;
	const char **from = &real_from;
	uint64_t tsc;
		if (!trace) {
		ltt_thread_init();
		trace = thread_trace_info;
	}


	/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = (const char*)&lttng_param_this_fn;
	align = sizeof(const void *);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(const void *);

	*from = (const char*)&lttng_param_call_site;
	align = sizeof(const void *);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(const void *);

	reserve_size = *to_base + *to + *len;
	trace->nesting++;
	index = ltt_get_index_from_facility(ltt_facility_user_generic_B1865E44,
						event_user_generic_function_entry);

	{
		ltt_buf = ltt_get_channel_from_index(trace, index);
				slot_size = 0;
		buffer = ltt_reserve_slot(trace, ltt_buf,
			reserve_size, &slot_size, &tsc);
		if (!buffer)
			goto end; /* buffer full */

		*to_base = *to = *len = 0;

		buffer = ltt_write_event_header(trace, ltt_buf, buffer,
			ltt_facility_user_generic_B1865E44, event_user_generic_function_entry,
			reserve_size, tsc);
		*from = (const char*)&lttng_param_this_fn;
		align = sizeof(const void *);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(const void *);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_call_site;
		align = sizeof(const void *);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(const void *);

		/* Flush pending memcpy */
		if (*len != 0) {
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

/* Event function_exit structures */

/* Event function_exit logging function */
static inline __attribute__((no_instrument_function)) int trace_user_generic_function_exit(
		const void * lttng_param_this_fn,
		const void * lttng_param_call_site);

#ifndef LTT_TRACE_FAST
static inline int trace_user_generic_function_exit(
		const void * lttng_param_this_fn,
		const void * lttng_param_call_site)
#ifndef LTT_TRACE
{
}
#else
{
	int ret = 0;
	char *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	size_t align;
	const char *real_from;
	const char **from = &real_from;
		/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = (const char*)&lttng_param_this_fn;
	align = sizeof(const void *);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(const void *);

	*from = (const char*)&lttng_param_call_site;
	align = sizeof(const void *);

	if (*len == 0) {
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

		*from = (const char*)&lttng_param_this_fn;
		align = sizeof(const void *);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(const void *);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_call_site;
		align = sizeof(const void *);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(const void *);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		ret = ltt_trace_generic(ltt_facility_user_generic_B1865E44, event_user_generic_function_exit, buffer, reserve_size, LTT_BLOCKING, 0);
	}

	return ret;

}
#endif //LTT_TRACE
#endif //!LTT_TRACE_FAST

#ifdef LTT_TRACE_FAST
static inline __attribute__((no_instrument_function)) int trace_user_generic_function_exit(
		const void * lttng_param_this_fn,
		const void * lttng_param_call_site)
#ifndef LTT_TRACE
{
}
#else
{
	unsigned int index;
	struct ltt_trace_info *trace = thread_trace_info;
	struct ltt_buf *ltt_buf;
	char *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	size_t align;
	const char *real_from;
	const char **from = &real_from;
	uint64_t tsc;
		if (!trace) {
		ltt_thread_init();
		trace = thread_trace_info;
	}


	/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = (const char*)&lttng_param_this_fn;
	align = sizeof(const void *);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(const void *);

	*from = (const char*)&lttng_param_call_site;
	align = sizeof(const void *);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(const void *);

	reserve_size = *to_base + *to + *len;
	trace->nesting++;
	index = ltt_get_index_from_facility(ltt_facility_user_generic_B1865E44,
						event_user_generic_function_exit);

	{
		ltt_buf = ltt_get_channel_from_index(trace, index);
				slot_size = 0;
		buffer = ltt_reserve_slot(trace, ltt_buf,
			reserve_size, &slot_size, &tsc);
		if (!buffer)
			goto end; /* buffer full */

		*to_base = *to = *len = 0;

		buffer = ltt_write_event_header(trace, ltt_buf, buffer,
			ltt_facility_user_generic_B1865E44, event_user_generic_function_exit,
			reserve_size, tsc);
		*from = (const char*)&lttng_param_this_fn;
		align = sizeof(const void *);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(const void *);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_call_site;
		align = sizeof(const void *);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(const void *);

		/* Flush pending memcpy */
		if (*len != 0) {
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

/* Event thread_brand structures */
static inline void lttng_write_string_user_generic_thread_brand_name(
		char *buffer,
		size_t *to_base,
		size_t *to,
		const char **from,
		size_t *len,
		const char * obj)
{
	size_t size;
	size_t align;

	/* Flush pending memcpy */
	if (*len != 0) {
		if (buffer != NULL)
			memcpy(buffer+*to_base+*to, *from, *len);
	}
	*to += *len;
	*len = 0;

	align = sizeof(char);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	/* Contains variable sized fields : must explode the structure */

	size = strlen(obj) + 1; /* Include final NULL char. */
	if (buffer != NULL)
		memcpy(buffer+*to_base+*to, obj, size);
	*to += size;

	/* Realign the *to_base on arch size, set *to to 0 */
	*to += ltt_align(*to, sizeof(void *));
	*to_base = *to_base+*to;
	*to = 0;

	/* Put source *from just after the C string */
	*from += size;
}


/* Event thread_brand logging function */
static inline int trace_user_generic_thread_brand(
		const char * lttng_param_name);

#ifndef LTT_TRACE_FAST
static inline int trace_user_generic_thread_brand(
		const char * lttng_param_name)
#ifndef LTT_TRACE
{
}
#else
{
	int ret = 0;
	char *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	const char *real_from;
	const char **from = &real_from;
		/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = (const char*)lttng_param_name;
	lttng_write_string_user_generic_thread_brand_name(buffer, to_base, to, from, len, lttng_param_name);

	reserve_size = *to_base + *to + *len;
	{
		char stack_buffer[reserve_size];
		buffer = stack_buffer;

		*to_base = *to = *len = 0;

		*from = (const char*)lttng_param_name;
		lttng_write_string_user_generic_thread_brand_name(buffer, to_base, to, from, len, lttng_param_name);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		ret = ltt_trace_generic(ltt_facility_user_generic_B1865E44, event_user_generic_thread_brand, buffer, reserve_size, LTT_BLOCKING, 1);
	}

	return ret;

}
#endif //LTT_TRACE
#endif //!LTT_TRACE_FAST

#ifdef LTT_TRACE_FAST
static inline int trace_user_generic_thread_brand(
		const char * lttng_param_name)
#ifndef LTT_TRACE
{
}
#else
{
	unsigned int index;
	struct ltt_trace_info *trace = thread_trace_info;
	struct ltt_buf *ltt_buf;
	char *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	const char *real_from;
	const char **from = &real_from;
	uint64_t tsc;
		if (!trace) {
		ltt_thread_init();
		trace = thread_trace_info;
	}


	/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = (const char*)lttng_param_name;
	lttng_write_string_user_generic_thread_brand_name(buffer, to_base, to, from, len, lttng_param_name);

	reserve_size = *to_base + *to + *len;
	trace->nesting++;
	index = ltt_get_index_from_facility(ltt_facility_user_generic_B1865E44,
						event_user_generic_thread_brand);

	{
		ltt_buf = ltt_get_channel_from_index(trace, index);
				slot_size = 0;
		buffer = ltt_reserve_slot(trace, ltt_buf,
			reserve_size, &slot_size, &tsc);
		if (!buffer)
			goto end; /* buffer full */

		*to_base = *to = *len = 0;

		buffer = ltt_write_event_header(trace, ltt_buf, buffer,
			ltt_facility_user_generic_B1865E44, event_user_generic_thread_brand,
			reserve_size, tsc);
		*from = (const char*)lttng_param_name;
		lttng_write_string_user_generic_thread_brand_name(buffer, to_base, to, from, len, lttng_param_name);

		/* Flush pending memcpy */
		if (*len != 0) {
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

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif //_LTT_FACILITY_USER_GENERIC_H_
