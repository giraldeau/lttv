
#define __KERNEL__

#include <assert.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <linux/compiler.h>

#define min(a,b) (((a)<(b))?a:b)
#define max(a,b) (((a)>(b))?a:b)
#define BUG_ON(a) assert(!(a))

// Useful outside __KERNEL__. Not used here because inline is already redefined.
#define force_inline inline __attribute__((always_inline))

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

#if 0
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
#endif //0

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

static inline void lttng_write_mystruct2(
		void *buffer,
		size_t *to_base,
		size_t *to,
		void **from,
		size_t *len,
		struct lttng_mystruct2 *obj)
{
	size_t align, size;

	align = lttng_get_alignment_mystruct2(obj);
	//size = lttng_get_size_mystruct2(obj);
	size = sizeof(struct lttng_mystruct2);

	if(*len == 0) {
		*to += ltt_align(*to, align);	/* align output */
	} else {
		*len += ltt_align(*to+*len, align);	/* C alignment, ok to do a memcpy of it */
	}
	
	*len += size;
}



#define LTTNG_ARRAY_SIZE_mystruct_myarray 10
typedef uint64_t lttng_array_mystruct_myarray[LTTNG_ARRAY_SIZE_mystruct_myarray];

#if 0
static inline size_t lttng_get_size_array_mystruct_myarray(
		lttng_array_mystruct_myarray obj)
{
	size_t size=0, locsize;
	
	locsize = sizeof(uint64_t);
	/* ltt_align == 0 always*/
	//size += ltt_align(size, locsize) + (LTTNG_ARRAY_SIZE_mystruct_myarray * locsize);
	BUG_ON(ltt_align(size, locsize) != 0);
	size += LTTNG_ARRAY_SIZE_mystruct_myarray * locsize;

	BUG_ON(sizeof(lttng_array_mystruct_myarray) != size);

	return sizeof(lttng_array_mystruct_myarray);
}
#endif //0

static inline size_t lttng_get_alignment_array_mystruct_myarray(
		lttng_array_mystruct_myarray obj)
{
	size_t align=0, localign;
	
	localign = sizeof(uint64_t);
	align = max(align, localign);

	return align;
}


static inline void lttng_write_array_mystruct_myarray(
		void *buffer,
		size_t *to_base,
		size_t *to,
		void **from,
		size_t *len,
		lttng_array_mystruct_myarray obj)
{
	size_t align, size;
	
	align = lttng_get_alignment_array_mystruct_myarray(obj);
	//size = lttng_get_size_array_mystruct_myarray(obj);
	size = sizeof(lttng_array_mystruct_myarray);

	if(*len == 0) {
		*to += ltt_align(*to, align);	/* align output */
	} else {
		*len += ltt_align(*to+*len, align);	/* C alignment, ok to do a memcpy of it */
	}
	
	*len += size;
#if 0
	/* For varlen child : let the child align itself. */
	for(unsigned int i=0; i<LTTNG_ARRAY_SIZE_mystruct_myarray; i++) {
		lttng_write_child(buffer, to_base, to, from, len, obj[i]);
	}
#endif //0

}


typedef struct lttng_sequence_mystruct_mysequence lttng_sequence_mystruct_mysequence;
struct lttng_sequence_mystruct_mysequence {
	unsigned int len;
	double *array;
};

#if 0
static inline size_t lttng_get_size_sequence_mystruct_mysequence(
																 lttng_sequence_mystruct_mysequence *obj)
{
	size_t size=0, locsize;
	
	locsize = sizeof(unsigned int);
	size += ltt_align(size, locsize) + locsize;

	locsize = sizeof(double);
	size += ltt_align(size, locsize) + (obj->len * locsize);

	/* Realign on arch size */
	locsize = sizeof(void *);
	size += ltt_align(size, locsize);

	return size;
}
#endif //0

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


static inline void lttng_write_sequence_mystruct_mysequence(
		void *buffer,
		size_t *to_base,
		size_t *to,
		void **from,
		size_t *len,
		lttng_sequence_mystruct_mysequence *obj)
{
	size_t align;
	size_t size;
	
	/* Flush pending memcpy */
	if(*len != 0) {
		if(buffer != NULL)
			memcpy(buffer+*to_base+*to, *from, *len);
	}
	*to += *len;
	*len = 0;

	align = lttng_get_alignment_sequence_mystruct_mysequence(obj);
	//no need size = lttng_get_size_sequence_mystruct_mysequence(obj);

	/* Align output */
	*to += ltt_align(*to, align);	/* *len = 0 in this function */
	
	/* Copy members */
	size = sizeof(unsigned int);
	*to += ltt_align(*to, size);
	if(buffer != NULL)
		memcpy(buffer+*to_base+*to, &obj->len, size);
	*to += size;

	size =  sizeof(double);
	*to += ltt_align(*to, size);
	size = obj->len * sizeof(double);
	if(buffer != NULL)
		memcpy(buffer+*to_base+*to, obj->array, size);
	*to += size;
#if 0
	/* If varlen child : let child align itself */
	for(unsigned int i=0; i<obj->len; i++) {
		lttng_write_child(buffer, to_base, to, from, len, obj->array[i]);
	}
#endif //0
	

	/* Realign the *to_base on arch size, set *to to 0 */
	*to = ltt_align(*to, sizeof(void *));
	*to_base = *to_base+*to;
	*to = 0;

	/* Put source *from just after the C sequence */
	*from = obj+1;
}



union lttng_mystruct_myunion {
	double myfloat;
	unsigned long myulong;
};

#if 0
static inline size_t lttng_get_size_mystruct_myunion(
																 union lttng_mystruct_myunion *obj)
{
	size_t size=0, locsize;
	
	locsize = sizeof(double);
	size = max(size, locsize);

	locsize = sizeof(unsigned long);
	size = max(size, locsize);

	BUG_ON(size != sizeof(union lttng_mystruct_myunion));

	return sizeof(union lttng_mystruct_myunion);
}
#endif //0

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


static inline void lttng_write_mystruct_myunion(
		void *buffer,
		size_t *to_base,
		size_t *to,
		void **from,
		size_t *len,
		union lttng_mystruct_myunion *obj)
{
	size_t align, size;
	
	align = lttng_get_alignment_mystruct_myunion(obj);
	//size = lttng_get_size_mystruct_myunion(obj);
	size = sizeof(union lttng_mystruct_myunion);

	if(*len == 0) {
		*to += ltt_align(*to, align);	/* align output */
	} else {
		*len += ltt_align(*to+*len, align);	/* C alignment, ok to do a memcpy of it */
	}
	
	*len += size;

	/* Assert : no varlen child. */
}


struct lttng_mystruct {
	unsigned int irq_id;
	enum lttng_irq_mode mode;
	struct lttng_mystruct2 teststr;
	lttng_array_mystruct_myarray myarray;
	lttng_sequence_mystruct_mysequence mysequence;
	union lttng_mystruct_myunion myunion;
};

#if 0
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
#endif //0

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
		void *buffer,
		size_t *to_base,
		size_t *to,
		void **from,
		size_t *len,
		struct lttng_mystruct *obj)
{
	size_t align, size;
	
	align = lttng_get_alignment_mystruct(obj);
	// no need : contains variable size fields.
	// locsize = lttng_get_size_mystruct(obj);

	if(*len == 0) {
		*to += ltt_align(*to, align);	/* align output */
	} else {
		*len += ltt_align(*to+*len, align);	/* C alignment, ok to do a memcpy of it */
	}
	
	/* Contains variable sized fields : must explode the structure */
	
	size = sizeof(unsigned int);
	size += ltt_align(*to+*len, size) + size;
	*len += size;
	
	size = sizeof(enum lttng_irq_mode);
	size += ltt_align(*to+*len, size) + size;
	*len += size;

	lttng_write_mystruct2(buffer, to_base, to, from, len, &obj->teststr);

	lttng_write_array_mystruct_myarray(buffer, to_base, to, from, len, obj->myarray);

	/* Variable length field */
	lttng_write_sequence_mystruct_mysequence(buffer, to_base, to, from, len, &obj->mysequence);
	/* After this previous write, we are sure that *to is 0, *len is 0 and 
	 * *to_base is aligned on the architecture size : to rest of alignment will
	 * be calculated statically. */

	lttng_write_mystruct_myunion(buffer, to_base, to, from, len, &obj->myunion);

	/* Don't forget to flush last write..... */
}






//void main()
void test()
{
	struct lttng_mystruct test;
	test.mysequence.len = 20;
	test.mysequence.array = malloc(20);

	//size_t size = lttng_get_size_mystruct(&test);
	//size_t align = lttng_get_alignment_mystruct(&test);
	//
	size_t to_base = 0;	/* the buffer is allocated on arch_size alignment */
	size_t to = 0;
	void *from = &test;
	size_t len = 0;

	/* Get size */
	lttng_write_mystruct(NULL, &to_base, &to, &from, &len, &test);
	/* Size = to_base + to + len */
	
	void *buffer = malloc(to_base + to + len);
	to_base = 0;	/* the buffer is allocated on arch_size alignment */
	to = 0;
	from = &test;
	len = 0;

	lttng_write_mystruct(buffer, &to_base, &to, &from, &len, &test);
	/* Final flush */
	/* Flush pending memcpy */
	if(len != 0) {
		// Assert buffer != NULL */
		memcpy(buffer+to_base+to, from, len);
		to += len;
		from += len;
		len = 0;
	}
	
	free(test.mysequence.array);
	free(buffer);
}
