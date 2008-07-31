#ifndef _LTT_FACILITY_TEST_H_
#define _LTT_FACILITY_TEST_H_


/* Facility activation at compile time. */
#ifdef CONFIG_LTT_FACILITY_TEST

/* Named types */


enum lttng_tasklet_priority {
	LTTNG_LOW,
	LTTNG_HIGH,
};

enum lttng_irq_mode {
	LTTNG_user,
	LTTNG_kernel,
};

struct lttng_mystruct2 {
	unsigned int irq_id;
	enum lttng_irq_mode mode;
	//struct lttng_mystruct teststr1;
};


size_t lttng_get_size_mystruct2(
		struct lttng_mystruct2 *obj)
{
	size_t size=0, locsize;
	
	locsize = sizeof(unsigned int);
	size += ltt_align(size, locsize) + locsize;
	
	locsize = sizeof(enum lttng_irq_mode);
	size += ltt_align(size, locsize) + locsize;

	BUG_ON(sizeof(struct lttng_mystruct2) != size);

	return sizeof(struct lttng_mystruct2);
}

size_t lttng_get_alignment_mystruct2(
		struct lttng_mystruct2 *obj)
{
	size_t align=0, localign;
	
	localign = sizeof(unsigned int);
	align = max(align, localign);
	
	localign = sizeof(enum lttng_irq_mode);
	align = max(align, localign);

	return align;
}

void lttng_write_mystruct2(void **to,
		void **from,
		size_t *len,
		struct lttng_mystruct2 *obj)
{
	size_t align, size;

	align = lttng_get_alignment_mystruct2(obj);
	size = lttng_get_size_mystruct2(obj);

	if(*len == 0) {
		*to += ltt_align((size_t)(*to), align);	/* align output */
	} else {
		*len += ltt_align((size_t)(*to+*len), align);	/* C alignment, ok to do a memcpy of it */
	}
	
	*len += size;
}



#define LTTNG_ARRAY_SIZE_mystruct_myarray 10
typedef uint64_t lttng_array_mystruct_myarray[LTTNG_ARRAY_SIZE_mystruct_myarray];

size_t lttng_get_size_array_mystruct_myarray(
		lttng_array_mystruct_myarray obj)
{
	size_t size=0, locsize;
	
	locsize = sizeof(uint64_t);
	/* ltt_align == 0 always*/
	//size += ltt_align(size, locsize) + (LTTNG_ARRAY_SIZE_mystruct_myarray * locsize);
	BUG_ON(ltt_align(size, locsize) != 0);
	size += LTTNG_ARRAY_SIZE_mystruct_myarray * locsize;

	BUG_ON(size != LTTNG_ARRAY_SIZE_mystruct_myarray * sizeof(uint64_t));

	return size;
}

size_t lttng_get_alignment_array_mystruct_myarray(
		lttng_array_mystruct_myarray obj)
{
	size_t align=0, localign;
	
	localign = sizeof(uint64_t);
	align = max(align, localign);

	return align;
}


void lttng_write_array_mystruct_myarray(void **to,
		void **from,
		size_t *len,
		lttng_array_mystruct_myarray obj)
{
	size_t align, size;
	
	align = lttng_get_alignment_array_mystruct_myarray(obj);
	size = lttng_get_size_array_mystruct_myarray(obj);

	if(*len == 0) {
		*to += ltt_align((size_t)(*to), align);	/* align output */
	} else {
		*len += ltt_align((size_t)(*to+*len), align);	/* C alignment, ok to do a memcpy of it */
	}
	
	*len += size;
}


typedef struct lttng_sequence_mystruct_mysequence lttng_sequence_mystruct_mysequence;
struct lttng_sequence_mystruct_mysequence {
	unsigned int len;
	double *array;
};


size_t lttng_get_size_sequence_mystruct_mysequence(
																 lttng_sequence_mystruct_mysequence *obj)
{
	size_t size=0, locsize;
	
	locsize = sizeof(unsigned int);
	size += ltt_align(size, locsize) + locsize;

	locsize = sizeof(double);
	size += ltt_align(size, locsize) + (obj->len * locsize);

	return size;
}

size_t lttng_get_alignment_sequence_mystruct_mysequence(
																 lttng_sequence_mystruct_mysequence *obj)
{
	size_t align=0, localign;
	
	localign = sizeof(unsigned int);
	align = max(align, localign);

	localign = sizeof(double);
	align = max(align, localign);

	return align;
}


void lttng_write_sequence_mystruct_mysequence(void **to,
		void **from,
		size_t *len,
		lttng_sequence_mystruct_mysequence *obj)
{
	size_t align, size;
	void *varfrom;
	size_t varlen=0;
	
	/* Flush pending memcpy */
	if(*len != 0) {
		memcpy(*to, *from, *len);
		*to += *len;
		*len = 0;
	}

	align = lttng_get_alignment_sequence_mystruct_mysequence(obj);
	//no need size = lttng_get_size_sequence_mystruct_mysequence(obj);

	/* Align output */
	*to += ltt_align((size_t)(*to), align);
	
	/* Copy members */
	*to += ltt_align((size_t)*to, sizeof(unsigned int));
	varfrom = &obj->len;
	varlen += sizeof(unsigned int);
	memcpy(*to, varfrom, varlen);
	*to += varlen;
	varlen = 0;

	*to += ltt_align((size_t)*to, sizeof(double));
	varfrom = obj->array;
	varlen += obj->len * sizeof(double);
	memcpy(*to, varfrom, varlen);
	*to += varlen;
	varlen = 0;

	/* Put source *from just after the C sequence */
	*from = obj+1;
}



union lttng_mystruct_myunion {
	double myfloat;
	unsigned long myulong;
};


size_t lttng_get_size_mystruct_myunion(
																 union lttng_mystruct_myunion *obj)
{
	size_t size=0, locsize;
	
	locsize = sizeof(double);
	size = max(size, locsize);

	locsize = sizeof(unsigned long);
	size = max(size, locsize);

	BUG_ON(size != sizeof(union lttng_mystruct_myunion));

	return size;
}


size_t lttng_get_alignment_mystruct_myunion(
																 union lttng_mystruct_myunion *obj)
{
	size_t align=0, localign;
	
	localign = sizeof(double);
	align = max(align, localign);

	localign = sizeof(unsigned long);
	align = max(align, localign);

	return align;
}


void lttng_write_mystruct_myunion(void **to,
		void **from,
		size_t *len,
		union lttng_mystruct_myunion *obj)
{
	size_t align, size;
	
	align = lttng_get_alignment_mystruct_myunion(obj);
	size = lttng_get_size_mystruct_myunion(obj);

	if(*len == 0) {
		*to += ltt_align((size_t)(*to), align);	/* align output */
	} else {
		*len += ltt_align((size_t)(*to+*len), align);	/* C alignment, ok to do a memcpy of it */
	}
	
	*len += size;
}


struct lttng_mystruct {
	unsigned int irq_id;
	enum lttng_irq_mode mode;
	struct lttng_mystruct2 teststr;
	lttng_array_mystruct_myarray myarray;
	lttng_sequence_mystruct_mysequence mysequence;
	union lttng_mystruct_myunion myunion;
};

size_t lttng_get_size_mystruct(
		struct lttng_mystruct *obj)
{
	size_t size=0, locsize, localign;
	
	locsize = sizeof(unsigned int);
	size += ltt_align(size, locsize) + locsize;
	
	locsize = sizeof(enum lttng_irq_mode);
	size += ltt_align(size, locsize) + locsize;

	localign = lttng_get_alignment_mystruct2(&obj->teststr);
	locsize = lttng_get_size_mystruct2(&obj->teststr);
	size += ltt_align(size, localign) + locsize;
	
	localign = lttng_get_alignment_array_mystruct_myarray(obj->myarray);
	locsize = lttng_get_size_array_mystruct_myarray(obj->myarray);
	size += ltt_align(size, localign) + locsize;
	
	localign = lttng_get_alignment_sequence_mystruct_mysequence(&obj->mysequence);
	locsize = lttng_get_size_sequence_mystruct_mysequence(&obj->mysequence);
	size += ltt_align(size, localign) + locsize;
	
	localign = lttng_get_alignment_mystruct_myunion(&obj->myunion);
	locsize = lttng_get_size_mystruct_myunion(&obj->myunion);
	size += ltt_align(size, localign) + locsize;

	return size;
}


size_t lttng_get_alignment_mystruct(
		struct lttng_mystruct *obj)
{
	size_t align=0, localign;
	
	localign = sizeof(unsigned int);
	align = max(align, localign);
	
	localign = sizeof(enum lttng_irq_mode);
	align = max(align, localign);

	localign = lttng_get_alignment_mystruct2(&obj->teststr);
	align = max(align, localign);
	
	localign = lttng_get_alignment_array_mystruct_myarray(obj->myarray);
	align = max(align, localign);
	
	localign = lttng_get_alignment_sequence_mystruct_mysequence(&obj->mysequence);
	align = max(align, localign);
	
	localign = lttng_get_alignment_mystruct_myunion(&obj->myunion);
	align = max(align, localign);

	return align;
}

void lttng_write_mystruct(void **to,
		void **from,
		size_t *len,
		struct lttng_mystruct *obj)
{
	size_t align, size;
	
	align = lttng_get_alignment_mystruct(obj);
	// no need : contains variable size fields.
	// locsize = lttng_get_size_mystruct(obj);

	if(*len == 0) {
		*to += ltt_align((size_t)(*to), align);	/* align output */
	} else {
		*len += ltt_align((size_t)(*to+*len), align);	/* C alignment, ok to do a memcpy of it */
	}
	
	/* Contains variable sized fields : must explode the structure */
	
	size = sizeof(unsigned int);
	*len += ltt_align((size_t)(*to+*len), size) + size;
	
	size = sizeof(enum lttng_irq_mode);
	*len += ltt_align((size_t)(*to+*len), size) + size;

	lttng_write_mystruct2(to, from, len, &obj->teststr);

	lttng_write_array_mystruct_myarray(to, from, len, obj->myarray);

	/* Variable length field */
	lttng_write_sequence_mystruct_mysequence(to, from, len, &obj->mysequence);
	
	lttng_write_mystruct_myunion(to, from, len, &obj->myunion);

	/* Don't forget to flush last write..... */
}




/* Event syscall_entry structures */

/* Event syscall_entry logging function */
static inline void trace_test_syscall_entry(
		unsigned int syscall_id,
		void * address)
#ifndef CONFIG_LTT
{
}
#else
{
}
#endif //CONFIG_LTT


/* Event syscall_exit structures */

/* Event syscall_exit logging function */
static inline void trace_test_syscall_exit(
		void)
#ifndef CONFIG_LTT
{
}
#else
{
}
#endif //CONFIG_LTT


/* Event trap_entry structures */

/* Event trap_entry logging function */
static inline void trace_test_trap_entry(
		unsigned int trap_id,
		void * address)
#ifndef CONFIG_LTT
{
}
#else
{
}
#endif //CONFIG_LTT


/* Event trap_exit structures */

/* Event trap_exit logging function */
static inline void trace_test_trap_exit(
		void)
#ifndef CONFIG_LTT
{
}
#else
{
}
#endif //CONFIG_LTT


/* Event soft_irq_entry structures */

/* Event soft_irq_entry logging function */
static inline void trace_test_soft_irq_entry(
		void * softirq_id)
#ifndef CONFIG_LTT
{
}
#else
{
}
#endif //CONFIG_LTT


/* Event soft_irq_exit structures */

/* Event soft_irq_exit logging function */
static inline void trace_test_soft_irq_exit(
		void * softirq_id)
#ifndef CONFIG_LTT
{
}
#else
{
}
#endif //CONFIG_LTT


/* Event tasklet_entry structures */

/* Event tasklet_entry logging function */
static inline void trace_test_tasklet_entry(
		enum lttng_tasklet_priority priority,
		void * address,
		unsigned long data)
#ifndef CONFIG_LTT
{
}
#else
{
}
#endif //CONFIG_LTT


/* Event tasklet_exit structures */

/* Event tasklet_exit logging function */
static inline void trace_test_tasklet_exit(
		enum lttng_tasklet_priority priority,
		void * address,
		unsigned long data)
#ifndef CONFIG_LTT
{
}
#else
{
}
#endif //CONFIG_LTT


/* Event irq_entry structures */

/* Event irq_entry logging function */
static inline void trace_test_irq_entry(
		unsigned int irq_id,
		enum lttng_irq_mode mode)
#ifndef CONFIG_LTT
{
}
#else
{
}
#endif //CONFIG_LTT


/* Event irq_exit structures */

/* Event irq_exit logging function */
static inline void trace_test_irq_exit(
		void)
#ifndef CONFIG_LTT
{
}
#else
{
}
#endif //CONFIG_LTT


/* Event big_array structures */
union lttng_test_big_array_myarray_b {
	void * c;
};

struct lttng_test_big_array_myarray {
	void * a;
	union lttng_test_big_array_myarray_b b;
};

#define LTTNG_ARRAY_SIZE_test_big_array_myarray 2
typedef struct lttng_test_big_array_myarray lttng_array_test_big_array_myarray[LTTNG_ARRAY_SIZE_test_big_array_myarray];

#define LTTNG_ARRAY_SIZE_test_big_array_myarray 10000
typedef lttng_array_test_big_array_myarray lttng_array_test_big_array_myarray[LTTNG_ARRAY_SIZE_test_big_array_myarray];


/* Event big_array logging function */
static inline void trace_test_big_array(
		lttng_array_test_big_array_myarray myarray)
#ifndef CONFIG_LTT
{
}
#else
{
}
#endif //CONFIG_LTT


#endif //CONFIG_LTT_FACILITY_TEST

#endif //_LTT_FACILITY_TEST_H_
