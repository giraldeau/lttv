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

size_t lttng_get_size_mystruct2(size_t *ret_align,
		struct lttng_mystruct2 *obj)
{
	size_t size=0, align=0, locsize, localign;
	
	locsize = sizeof(unsigned int);
	align = max(align, locsize);
	size += ltt_align(size, locsize) + locsize;
	
	locsize = sizeof(enum lttng_irq_mode);
	align = max(align, locsize);
	size += ltt_align(size, locsize) + locsize;

	*ret_align = align;
	
	return size;
}

void lttng_write_mystruct2(void **ws,
		void **we,
		struct lttng_mystruct2 *obj)
{
	size_t align=0, locsize, localign;

	locsize = lttng_get_size_mystruct2(&align, 
		obj);

	if(*ws == *we) {
		*we += ltt_align(*we, align);
		*ws = *we;
	} else {
		*we += ltt_align(*we, align);	/* C alignment, ok to do a memcpy of it */
	}
	
	BUG_ON(locsize != sizeof(struct lttng_mystruct2));
	*we += locsize;
}



#define LTTNG_ARRAY_SIZE_mystruct_myarray 10
typedef uint64_t lttng_array_mystruct_myarray[LTTNG_ARRAY_SIZE_mystruct_myarray];

size_t lttng_get_size_array_mystruct_myarray(size_t *ret_align,
		lttng_array_mystruct_myarray obj)
{
	size_t size=0, align=0, locsize, localign;
	
	locsize = sizeof(uint64_t);
	align = max(align, locsize);
	size += ltt_align(size, locsize) + (LTTNG_ARRAY_SIZE_mystruct_myarray * locsize);

	*ret_align = align;
	
	return size;
}

void lttng_write_array_mystruct_myarray(void **ws,
		void **we,
		struct lttng_mystruct2 *obj)
{
	size_t align=0, locsize, localign;
	
	locsize = lttng_get_size_array_mystruct_myarray(&align, 
		obj);

	if(*ws == *we) {
		*we += ltt_align(*we, align);
		*ws = *we;
	} else {
		*we += ltt_align(*we, align);	/* C alignment, ok to do a memcpy of it */
	}
	
	BUG_ON(locsize != LTTNG_ARRAY_SIZE_mystruct_myarray * sizeof(uint64_t));

	*we += locsize;
}


typedef struct lttng_sequence_mystruct_mysequence lttng_sequence_mystruct_mysequence;
struct lttng_sequence_mystruct_mysequence {
	unsigned int len;
	double *array;
};


size_t lttng_get_size_sequence_mystruct_mysequence(size_t *ret_align,
																 lttng_sequence_mystruct_mysequence *obj)
{
	size_t size=0, align=0, locsize, localign;
	
	locsize = sizeof(unsigned int);
	align = max(align, locsize);
	size += ltt_align(size, locsize) + locsize;

	locsize = sizeof(double);
	align = max(align, locsize);
	size += ltt_align(size, locsize) + (obj->len * locsize);

	*ret_align = align;

	return size;
}

void lttng_write_sequence_mystruct_mysequence(void **ws,
		void **we,
		lttng_sequence_mystruct_mysequence *obj)
{
	size_t align=0, locsize, localign;
	
	/* Flush pending memcpy */
	if(*ws != *we) {
		memcpy(*ws, obj, *we-*ws);
		*ws = *we;
	}
	
	lttng_get_size_sequence_mystruct_mysequence(&align, 
		obj);

	*we += ltt_align(*we, align);
	*ws = *we;
	
	*we += ltt_align(*we, sizeof(unsigned int));
	*ws = *we;
	*we += sizeof(unsigned int);
	memcpy(*ws, &obj->len, *we-*ws);
	*ws = *we;

	*we += ltt_align(*we, sizeof(double));
	*ws = *we;
	*we += obj->len * sizeof(double);
	memcpy(*ws, obj->array, *we-*ws);
	*ws = *we;
}



union lttng_mystruct_myunion {
	double myfloat;
	unsigned long myulong;
};

size_t lttng_get_size_mystruct_myunion(size_t *ret_align,
																 union lttng_mystruct_myunion *obj)
{
	size_t size=0, align=0, locsize, localign;
	
	locsize = sizeof(double);
	align = max(align, locsize);
	size = max(size, locsize);

	locsize = sizeof(unsigned long);
	align = max(align, locsize);
	size = max(size, locsize);

	*ret_align = align;

	return size;
}


void lttng_write_mystruct_myunion(void **ws,
		void **we,
		union lttng_mystruct_myunion *obj)
{
	size_t align=0, locsize, localign;
	
	locsize = lttng_get_size_mystruct_myunion(&align, 
		obj);

	if(*ws == *we) {
		*we += ltt_align(*we, align);
		*ws = *we;
	} else {
		*we += ltt_align(*we, align);	/* C alignment, ok to do a memcpy of it */
	}
	
	BUG_ON(locsize != sizeof(union lttng_mystruct_myunion));

	*we += locsize;
}


struct lttng_mystruct {
	unsigned int irq_id;
	enum lttng_irq_mode mode;
	struct lttng_mystruct2 teststr;
	lttng_array_mystruct_myarray myarray;
	lttng_sequence_mystruct_mysequence mysequence;
	union lttng_mystruct_myunion myunion;
};

size_t lttng_get_size_mystruct(size_t *ret_align,
		struct lttng_mystruct *obj)
{
	size_t size=0, align=0, locsize, localign;
	
	locsize = sizeof(unsigned int);
	align = max(align, locsize);
	size += ltt_align(size, locsize) + locsize;
	
	locsize = sizeof(enum lttng_irq_mode);
	align = max(align, locsize);
	size += ltt_align(size, locsize) + locsize;

	locsize = lttng_get_size_mystruct2(&localign,
			&obj->teststr);
	align = max(align, localign);
	size += ltt_align(size, localign) + locsize;
	
	locsize = lttng_get_size_array_mystruct_myarray(&localign,
			obj->myarray);
	align = max(align, localign);
	size += ltt_align(size, localign) + locsize;
	
	locsize = lttng_get_size_sequence_mystruct_mysequence(&localign,
			&obj->mysequence);
	align = max(align, localign);
	size += ltt_align(size, localign) + locsize;
	
	locsize = lttng_get_size_mystruct_myunion(&localign,
			&obj->myunion);
	align = max(align, localign);
	size += ltt_align(size, localign) + locsize;

	*ret_align = align;

	return size;
}

struct lttng_mystruct {
	unsigned int irq_id;
	enum lttng_irq_mode mode;
	struct lttng_mystruct2 teststr;
	lttng_array_mystruct_myarray myarray;
	lttng_sequence_mystruct_mysequence mysequence;
	union lttng_mystruct_myunion myunion;
};


void lttng_write_mystruct(void **ws,
		void **we,
		struct lttng_mystruct *obj)
{
	size_t align=0, locsize, localign;
	
	lttng_get_size_mystruct(&align, 
		obj);

	if(*ws == *we) {
		*we += ltt_align(*we, align);
		*ws = *we;
	} else {
		*we += ltt_align(*we, align);	/* C alignment, ok to do a memcpy of it */
	}
	
	locsize = sizeof(unsigned int);
	*we += ltt_align(*we, locsize) + locsize;
	
	locsize = sizeof(enum lttng_irq_mode);
	*we += ltt_align(*we, locsize) + locsize;

	lttng_write_mystruct2(ws, we, 
			&obj->teststr);

	lttng_write_array_mystruct_myarray(ws, we, 
			obj->myarray);

	/* Variable length field */
	lttng_write_sequence_mystruct_mysequence(ws, we, 
			&obj->mysequence);
	
	lttng_write_mystruct_myunion(ws, we, 
			obj->myunion);

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
