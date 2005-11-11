
#include <assert.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>

#define min(a,b) (((a)<(b))?a:b)
#define max(a,b) (((a)>(b))?a:b)
#define BUG_ON(a) assert(!(a))

/* Calculate the offset needed to align the type */
static inline unsigned int ltt_align(size_t align_drift,
																		 size_t size_of_type)
{
	size_t alignment = min(sizeof(void*), size_of_type);

	return ((alignment - align_drift) & (alignment-1));
}


/* TEMPLATE */

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


static inline size_t lttng_get_size_mystruct2(
		struct lttng_mystruct2 * obj)
{
	size_t size=0, locsize;
	
	locsize = sizeof(unsigned int);
	size += ltt_align(size, locsize) + locsize;
	
	locsize = sizeof(enum lttng_irq_mode);
	size += ltt_align(size, locsize) + locsize;

	BUG_ON(sizeof(struct lttng_mystruct2) != size);

	return sizeof(struct lttng_mystruct2);
}

static inline size_t lttng_get_alignment_mystruct2(
		struct lttng_mystruct2 *obj)
{
	size_t align=0, localign;
	
	localign = sizeof(unsigned int);
	align = max(align, localign);
	
	localign = sizeof(enum lttng_irq_mode);
	align = max(align, localign);

	return align;
}

static inline size_t lttng_write_mystruct2(
		void *buf,
		size_t *to,
		void **from,
		size_t *len,
		struct lttng_mystruct2 *obj)
{
	size_t align, size, varalign;

	align = lttng_get_alignment_mystruct2(obj);
	size = lttng_get_size_mystruct2(obj);

	if(*len == 0) {
		varalign = ltt_align(*to, align);	/* align output */
		*to += varalign;
	} else {
		varalign = ltt_align(*to+*len, align);	/* C alignment, ok to do a memcpy of it */
		*len += varalign;
	}
	
	*len += size;

	return varalign+size;
}



#define LTTNG_ARRAY_SIZE_mystruct_myarray 10
typedef uint64_t lttng_array_mystruct_myarray[LTTNG_ARRAY_SIZE_mystruct_myarray];

static inline size_t lttng_get_size_array_mystruct_myarray(
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

static inline size_t lttng_get_alignment_array_mystruct_myarray(
		lttng_array_mystruct_myarray obj)
{
	size_t align=0, localign;
	
	localign = sizeof(uint64_t);
	align = max(align, localign);

	return align;
}


static inline size_t lttng_write_array_mystruct_myarray(
		void *buf,
		size_t *to,
		void **from,
		size_t *len,
		lttng_array_mystruct_myarray obj)
{
	size_t align, size, varalign;
	
	align = lttng_get_alignment_array_mystruct_myarray(obj);
	size = lttng_get_size_array_mystruct_myarray(obj);

	if(*len == 0) {
		varalign = ltt_align(*to, align);	/* align output */
		*to += varalign;
	} else {
		varalign = ltt_align(*to+*len, align);	/* C alignment, ok to do a memcpy of it */
		*len += varalign;
	}
	
	*len += size;

	return varalign + size; /* offset in the from */
}


typedef struct lttng_sequence_mystruct_mysequence lttng_sequence_mystruct_mysequence;
struct lttng_sequence_mystruct_mysequence {
	unsigned int len;
	double *array;
};


static inline size_t lttng_get_size_sequence_mystruct_mysequence(
																 lttng_sequence_mystruct_mysequence *obj)
{
	size_t size=0, locsize;
	
	locsize = sizeof(unsigned int);
	size += ltt_align(size, locsize) + locsize;

	locsize = sizeof(double);
	size += ltt_align(size, locsize) + (obj->len * locsize);

	return size;
}

static inline size_t lttng_get_alignment_sequence_mystruct_mysequence(
																 lttng_sequence_mystruct_mysequence *obj)
{
	size_t align=0, localign;
	
	localign = sizeof(unsigned int);
	align = max(align, localign);

	localign = sizeof(double);
	align = max(align, localign);

	return align;
}


static inline size_t lttng_write_sequence_mystruct_mysequence(
		void *buf,
		size_t *to,
		void **from,
		size_t *len,
		lttng_sequence_mystruct_mysequence *obj)
{
	size_t align;
	size_t size=0;
	void *varfrom;
	size_t varlen=0;
	size_t varalign=0;
	
	/* Flush pending memcpy */
	if(*len != 0) {
		memcpy(buf+*to, *from, *len);
		*to += *len;
		*len = 0;
	}

	align = lttng_get_alignment_sequence_mystruct_mysequence(obj);
	//no need size = lttng_get_size_sequence_mystruct_mysequence(obj);

	/* Align output */
	varalign = ltt_align((size_t)(*to), align);
	size += varalign;
	
	/* Copy members */
	varalign = ltt_align(size, sizeof(unsigned int));
	size += varalign;
	varfrom = &obj->len;
	varlen += sizeof(unsigned int);
	memcpy(buf+*to+size, varfrom, varlen);
	size += varlen;
	varlen = 0;

	varalign = ltt_align(size, sizeof(double));
	size += varalign;
	varfrom = obj->array;
	varlen += obj->len * sizeof(double);
	memcpy(buf+*to+size, varfrom, varlen);
	size += varlen;
	varlen = 0;

	*to += size;

	/* Put source *from just after the C sequence */
	*from = obj+1;

	return size;
}



union lttng_mystruct_myunion {
	double myfloat;
	unsigned long myulong;
};


static inline size_t lttng_get_size_mystruct_myunion(
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


static inline size_t lttng_get_alignment_mystruct_myunion(
																 union lttng_mystruct_myunion *obj)
{
	size_t align=0, localign;
	
	localign = sizeof(double);
	align = max(align, localign);

	localign = sizeof(unsigned long);
	align = max(align, localign);

	return align;
}


static inline size_t lttng_write_mystruct_myunion(
		void *buf,
		size_t *to,
		void **from,
		size_t *len,
		union lttng_mystruct_myunion *obj)
{
	size_t align, size, varalign;
	
	align = lttng_get_alignment_mystruct_myunion(obj);
	size = lttng_get_size_mystruct_myunion(obj);

	if(*len == 0) {
		varalign = ltt_align(*to, align);	/* align output */
		*to += varalign;
	} else {
		varalign = ltt_align(*to+*len, align);	/* C alignment, ok to do a memcpy of it */
		*len += varalign;
	}
	
	*len += size;

	return varalign + size;
}


struct lttng_mystruct {
	unsigned int irq_id;
	enum lttng_irq_mode mode;
	struct lttng_mystruct2 teststr;
	lttng_array_mystruct_myarray myarray;
	lttng_sequence_mystruct_mysequence mysequence;
	union lttng_mystruct_myunion myunion;
};

static inline size_t lttng_get_size_mystruct(
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


static inline size_t lttng_get_alignment_mystruct(
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

static inline void lttng_write_mystruct(
		void *buf,
		size_t *to,
		void **from,
		size_t *len,
		struct lttng_mystruct *obj)
{
	size_t align, size=0, locsize;
	
	align = lttng_get_alignment_mystruct(obj);
	// no need : contains variable size fields.
	// locsize = lttng_get_size_mystruct(obj);

	if(*len == 0) {
		*to += ltt_align(*to, align);	/* align output */
	} else {
		*len += ltt_align(*to+*len, align);	/* C alignment, ok to do a memcpy of it */
	}
	
	/* Contains variable sized fields : must explode the structure */
	
	locsize = sizeof(unsigned int);
	locsize += ltt_align(size, locsize) + locsize;
	*len += locsize;
	size += locsize;
	
	locsize = sizeof(enum lttng_irq_mode);
	locsize += ltt_align(size, locsize) + locsize;
	*len += locsize;
	size += locsize;

	size += lttng_write_mystruct2(buf, to, from, len, &obj->teststr);

	size += lttng_write_array_mystruct_myarray(buf, to, from, len, obj->myarray);

	/* Variable length field */
	size += lttng_write_sequence_mystruct_mysequence(buf, to, from, len, &obj->mysequence);
	
	size += lttng_write_mystruct_myunion(buf, to, from, len, &obj->myunion);

	/* Don't forget to flush last write..... */
}






//void main()
void test()
{
	struct lttng_mystruct test;
	test.mysequence.len = 20;
	test.mysequence.array = malloc(20);

	size_t size = lttng_get_size_mystruct(&test);
	size_t align = lttng_get_alignment_mystruct(&test);

	/* the buffer is allocated on arch_size alignment */
	void *buf = malloc(align + size);
	size_t to = 0;
	void *from = &test;
	size_t len = 0;

	lttng_write_mystruct(buf, &to, &from, &len, &test);
	/* Final flush */
	/* Flush pending memcpy */
	if(len != 0) {
		memcpy(buf+to, from, len);
		to += len;
		from += len;
		len = 0;
	}
	
	free(test.mysequence.array);
	free(buf);
}
