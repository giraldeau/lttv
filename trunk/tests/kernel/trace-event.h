#include <stdarg.h>

/* Maximum number of callbacks per marker */
#define LTT_NR_CALLBACKS	10

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


char *(*ltt_serialize_cb)(char *buffer, int *cb_args,
			const char *fmt, va_list args);


static int skip_atoi(const char **s)
{
	int i=0;

	while (isdigit(**s))
		i = i*10 + *((*s)++) - '0';
	return i;
}

/* Inspired from vsnprintf */
/* New types :
 * %r : serialized fixed length struct, union, array.
 * %v : serialized sequence
 * %k : callback
 */
static inline __attribute__((no_instrument_function))
char *ltt_serialize_data(char *buffer, int *cb_args,
			const char *fmt, va_list args)
{
	int len;
	const char *s;
	int elem_size;		/* Size of the integer for 'b' */
				/* Size of the data contained by 'r' */
	int elem_alignment;	/* Element alignment for 'r' */
	int qualifier;		/* 'h', 'l', or 'L' for integer fields */
				/* 'z' support added 23/7/1999 S.H.    */
				/* 'z' changed to 'Z' --davidm 1/25/99 */
				/* 't' added for ptrdiff_t */
	char *str;		/* Pointer to write to */
	ltt_serialize_cb cb;
	int cb_arg_nr = 0;

	str = buf;

	for (; *fmt ; ++fmt) {
		if (*fmt != '%') {
			/* Skip text */
			continue;
		}

		/* process flags : ignore standard print formats for now. */
		repeat:
			++fmt;		/* this also skips first '%' */
			switch (*fmt) {
				case '-':
				case '+':
				case ' ':
				case '#':
				case '0': goto repeat;
			}

		/* get element size */
		elem_size = -1;
		if (isdigit(*fmt))
			elem_size = skip_atoi(&fmt);
		else if (*fmt == '*') {
			++fmt;
			/* it's the next argument */
			elem_size = va_arg(args, int);
		}

		/* get the alignment */
		elem_alignment = -1;
		if (*fmt == '.') {
			++fmt;	
			if (isdigit(*fmt))
				elem_alignment = skip_atoi(&fmt);
			else if (*fmt == '*') {
				++fmt;
				/* it's the next argument */
				elem_alignment = va_arg(args, int);
			}
		}

		/* get the conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' ||
		    *fmt =='Z' || *fmt == 'z' || *fmt == 't') {
			qualifier = *fmt;
			++fmt;
			if (qualifier == 'l' && *fmt == 'l') {
				qualifier = 'L';
				++fmt;
			}
		}

		switch (*fmt) {
			case 'c':
				if (buffer)
					*str = (char) va_arg(args, int);
				str += sizeof(char);
				continue;

			case 's':
				s = va_arg(args, char *);
				if ((unsigned long)s < PAGE_SIZE)
					s = "<NULL>";
				if (buffer)
					strcpy(str, s);
				str += strlen(s);
				/* Following alignment for genevent
				 * compatibility */
				str += ltt_align(str, sizeof(void*));
				continue;

			case 'p':
				str += ltt_align(str, sizeof(void*));
				if (buffer)
					*(void**)str = va_arg(args, void *);
				continue;

			case 'r':
				/* For array, struct, union */
				if (elem_alignment < 0)
					elem_alignment = sizeof(void*);
				str += ltt_align(str, elem_alignment);
				if (elem_size > 0) {
					const char *src = va_arg(args,
								const char *);
					if (buffer)
						memcpy(str, src, elem_size);
					str += elem_size;
				}
				continue;

			case 'v':
				/* For sequence */
				str += ltt_align(str, sizeof(int));
				if (buffer)
					*(int*)str = elem_size;
				str += sizeof(int);
				if (elem_alignment > 0)
					str += ltt_align(str, elem_alignment);
				if (elem_size > 0) {
					const char *src = va_arg(args,
								const char *);
					if (buffer)
						memcpy(str, src, elem_size);
					str += elem_size;
				}
				/* Following alignment for genevent
				 * compatibility */
				str += ltt_align(str, sizeof(void*));
				continue;

			case 'k':
				/* For callback */
				cb = va_arg(args, ltt_serialize_cb);
				 /* The callback will take as many arguments
				  * as it needs from args. They won't be
				  * type verified. */
				if (cb_arg_nr < LTT_NR_CALLBACKS)
					str = cb(str, &cb_args[cb_arg_nr++],
						fmt, args);
				continue;

			case 'n':
				/* FIXME:
				* What does C99 say about the overflow case
				* here? */
				if (qualifier == 'l') {
					long * ip = va_arg(args, long *);
					*ip = (str - buf);
				} else if (qualifier == 'Z'
					|| qualifier == 'z') {
					size_t * ip = va_arg(args, size_t *);
					*ip = (str - buf);
				} else {
					int * ip = va_arg(args, int *);
					*ip = (str - buf);
				}
				continue;

			case '%':
				continue;

			case 'o':
			case 'X':
			case 'x':
			case 'd':
			case 'i':
			case 'u':
				break;

			default:
				if (!*fmt)
					--fmt;
				continue;
		}
		switch (qualifier) {
		case 'L':
			str += ltt_align(str, sizeof(long long));
			if (buffer)
				*(long long*)str = va_arg(args, long long);
			str += sizeof(long long);
			break;
		case 'l':
			str += ltt_align(str, sizeof(long));
			if (buffer)
				*(long*)str = va_arg(args, long);
			str += sizeof(long);
			break;
		case 'Z':
		case 'z':
			str += ltt_align(str, sizeof(size_t));
			if (buffer)
				*(size_t*)str = va_arg(args, size_t);
			str += sizeof(size_t);
			break;
		case 't':
			str += ltt_align(str, sizeof(ptrdiff_t));
			if (buffer)
				*(ptrdiff_t*)str = va_arg(args, ptrdiff_t);
			str += sizeof(ptrdiff_t);
			break;
		case 'h':
			str += ltt_align(str, sizeof(short));
			if (buffer)
				*(short*)str = (short) va_arg(args, int);
			str += sizeof(short);
			break;
		case 'b':
			if (elem_size > 0)
				str += ltt_align(str, elem_size);
			if (buffer)
				switch (elem_size) {
				case 1:
					*(int8_t*)str =
						(int8_t)va_arg(args, int);
					break;
				case 2:
					*(int16_t*)str =
						(int16_t)va_arg(args, int);
					break;
				case 4:
					*(int32_t*)str = va_arg(args, int32_t);
					break;
				case 8:
					*(int64_t*)str = va_arg(args, int64_t);
					break;
				}
			str += elem_size;
		default:
			str += ltt_align(str, sizeof(int));
			if (buffer)
				*(int*)str = va_arg(args, int);
			str += sizeof(int);
		}
	}
	return str;
}

/* Calculate data size */
/* Assume that the padding for alignment starts at a
 * sizeof(void *) address. */
static inline __attribute__((no_instrument_function))
size_t ltt_get_data_size(ltt_facility_t fID, uint8_t eID,
				int *cb_args,
				const char *fmt, va_list args)
{
	return (size_t)ltt_serialize_data(NULL, fmt, args);
}

static inline __attribute__((no_instrument_function))
void ltt_write_event_data(char *buffer,
				ltt_facility_t fID, uint8_t eID,
				int *cb_args,
				const char *fmt, va_list args)
{
	ltt_serialize_data(buffer, fmt, args);
}


__attribute__((no_instrument_function))
void _vtrace(ltt_facility_t fID, uint8_t eID, long flags,
		const char *fmt, va_list args)
{
	size_t data_size, slot_size;
	int channel_index;
	struct ltt_channel_struct *channel;
	struct ltt_trace_struct *trace, *dest_trace;
	void *transport_data;
	uint64_t tsc;
	char *buffer;
	va_list args_copy;
	int cb_args[LTT_NR_CALLBACKS];

	/* This test is useful for quickly exiting static tracing when no
	 * trace is active. */
	if (likely(ltt_traces.num_active_traces == 0
		&& !(flags & LTT_FLAG_FORCE)))
		return;

	preempt_disable();
	ltt_nesting[smp_processor_id()]++;

	if (unlikely(flags & LTT_FLAG_TRACE))
		dest_trace = va_arg(args, struct ltt_trace_struct *);
	if (unlikely(flags & LTT_FLAG_CHANNEL))
		channel_index = va_arg(args, int);
	else
		channel_index = ltt_get_channel_index(fID, eID);

	va_copy(args_copy, args);	/* Check : skip 2 st args if trace/ch */
	data_size = ltt_get_data_size(fID, eID, cb_args, fmt, args_copy);
	va_end(args_copy);

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
		va_copy(args_copy, args);
		ltt_write_event_data(buffer, fID, eID, cb_args, fmt, args_copy);
		va_end(args_copy);
		/* Out-of-order commit */
		ltt_commit_slot(channel, &transport_data, buffer, slot_size);
	}

	ltt_nesting[smp_processor_id()]--;
	preempt_enable();
}

__attribute__((no_instrument_function))
void _trace(ltt_facility_t fID, uint8_t eID, long flags, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	_vtrace(fID, eID, flags, fmt, args);
	va_end(args);
}

