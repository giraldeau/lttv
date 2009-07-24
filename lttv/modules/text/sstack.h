#ifndef SSTACK_H
#define SSTACK_H

#define SSTACK_TYPE_PUSH  1
#define SSTACK_TYPE_POP   2
#define SSTACK_TYPE_EVENT 3

//#define SSTACK_PUSH_VAL(i) (()i->data_val)

/* An item of a struct sstack, that describes a stack operation */

struct sstack_item {
	/* state flags */
	unsigned char finished;
	unsigned char processable;
	unsigned char deletable;

	/* Type of operation: SSTACK_TYPE_PUSH, SSTACK_TYPE_POP or SSTACK_TYPE_EVENT */
	int data_type;
	/* private, application-dependant data */
	void *data_val;

	/* Function to call to delete data_val */
	void (*delete_data_val)(void *data_val);

	/* The index of the corresponding push (for a pop) or pop (for a push) */
	int pushpop;

	/* Does this item require that we wait for its pop to process it */
	int wait_pop;

	GArray *depends;
	GArray *rev_depends;
};

/* external debugging function to print the private data of an item */
extern void (*print_sstack_item_data)(struct sstack_item *);

/* An actual sstack */

struct sstack {
	GArray *array;

	/* Stack of the indexes of the pushes that have been done. An index is popped when the
	 * corresponding pop is added to the sstack. This enables us to find the index of the
	 * last push.
	 */
	GArray *pushes;

	/* Stack of 0's and 1's. 0: don't wait for pop to process the children
	 * 1: wait for pop to process its children
	 */
	GArray *wait_pop_stack;

	/* Next item we must try to process */
	int proc_index;

	void (*process_func)(void *arg, struct sstack_item *item);
	void *process_func_arg; /* the pointer passed as the "arg" argument of process_func */
};

struct sstack_item *sstack_new_item();

void sstack_add_item(struct sstack *stack, struct sstack_item *item);

struct sstack *sstack_new(void);
struct sstack_item *sstack_item_new(void);
struct sstack_item *sstack_item_new_push(unsigned char finished);
struct sstack_item *sstack_item_new_pop(void);
struct sstack_item *sstack_item_new_evt(void);

#endif /* SSTACK_H */
