#include <glib.h>
#include <stdio.h>

#include "sstack.h"

/* This is the implementation of sstack, a data structure that holds
 * operations done on a stack in order to play them on a stack a short
 * while later. They are played when dependencies are fulfilled. The
 * operations are held in a queue.
 *
 * Operations include PUSH of data, POP, as well as other special markers.
 *
 * Stack operations are defined by a struct sstack_item. Each struct
 * sstack_item has 3 flags:
 *   - finished
 *   - processable
 *   - deletable
 *
 * Finished is raised when all the dependencies of the operation are
 * fulfilled. POPs are always created finished because they have no
 * dependencies. PUSHes are marked finished when their corresponding
 * POP is added to the queueit is the PUSHes
 * that contain the and EVENTs are always created finished.
 *
 * Once an operation is finished
 */

void (*print_sstack_item_data)(struct sstack_item *);

/* Debugging function: print a queue item */

static void print_item(struct sstack_item *item)
{
	char *label;

	if(item->data_type == SSTACK_TYPE_PUSH) {
		label = "PUSH";
	}
	else if(item->data_type == SSTACK_TYPE_POP) {
		label = "POP";
	}
	else {
		label = "UNKNOWN";
	}

	printf("%-10s    %-2u%-2u%-2u", label, item->finished, item->processable, item->deletable);
	/* hack: call this external, application-dependant function to show the private data in the item */
	print_sstack_item_data(item);
}

/* Debugging function: print the queue as it is now */

void print_stack(struct sstack *stack)
{
	int i;

	printf("************************\n");
	printf("**     %-10s    F P D\n", "label");
	for(i=0; i<stack->array->len; i++) {
		struct sstack_item *item = g_array_index(stack->array, struct sstack_item *, i);

		printf("** %-3d- ", i);
		print_item(item);
	}
	printf("\n");
}

static void try_start_deleting(struct sstack *stack)
{
	int index = stack->array->len-1;

	while(index >= 0 && g_array_index(stack->array, struct sstack_item *, index)->deletable) {
		struct sstack_item *item = g_array_index(stack->array, struct sstack_item *, index);

		if(item->delete_data_val)
			item->delete_data_val(item->data_val);

		//g_array_free(item->depends, FALSE);
		//g_array_free(item->rev_depends, FALSE);
		g_free(item);

		g_array_remove_index(stack->array, index);
		index--;
	}

	if(stack->proc_index > stack->array->len)
		stack->proc_index = stack->array->len;
}

/* An item is deletable as soon as it is processed. However, all the items after it
 * in the list must be deleted before it can be deleted.
 *
 * After this function, try_start_deleting must be called.
 */

static void mark_deletable(struct sstack *stack, int index)
{
	struct sstack_item *item = g_array_index(stack->array, struct sstack_item *, index);

	item->deletable = 1;

//	if(index == stack->array->len - 1) {
//		start_deleting(stack);
//	}
}

#if 0
/* Called to process an index of the queue */

static void process(struct sstack *stack, int index)
{
	g_assert(stack->proc_index == index);

	//printf("sstack: starting to process\n");
	while(stack->proc_index < stack->array->len) {
		struct sstack_item *item = g_array_index(stack->array, struct sstack_item *, stack->proc_index);

		if(!item->processable)
			break;

		//printf("sstack: processing ");
		//print_item(item);

		if(stack->process_func) {
			stack->process_func(stack->process_func_arg, item);
		}
		else {
			printf("warning: no process func\n");
		}

		stack->proc_index++;

		mark_deletable(stack, stack->proc_index-1);
		if(item->pushpop >= 0 && item->pushpop < stack->proc_index-1) {
			mark_deletable(stack, item->pushpop);
		}
		try_start_deleting(stack);
	}
	//printf("sstack: stopping processing\n");
}

/* Called to mark an index of the queue as processable */

static void mark_processable(struct sstack *stack, int index)
{
	struct sstack_item *item = g_array_index(stack->array, struct sstack_item *, index);

	item->processable = 1;

	if(stack->proc_index <= stack->array->len && stack->proc_index == index) {
		process(stack, index);
	}
}

/* Called to check whether an index of the queue could be marked processable. If so,
 * mark it processable.
 *
 * To be processable, an item must:
 * - be finished
 * - have its push/pop dependency fulfilled
 * - have its other dependencies fulfilled
 */

static void try_mark_processable(struct sstack *stack, int index)
{
	int i;
	int all_finished = 1;

	struct sstack_item *item = g_array_index(stack->array, struct sstack_item *, index);

	//for(i=0; i<stack->array->len; i++) {
	//	if(!g_array_index(stack->array, struct sstack_item *, index)->finished) {
	//		all_finished = 0;
	//		break;
	//	}
	//}

	/* Theoritically, we should confirm that the push/pop dependency is
	 * finished, but in practice, it's not necessary. If we are a push, the
	 * corresponding pop is always finished. If we are a pop and we exist,
	 * the corresponding push is necessarily finished.
	 */

	//if(all_finished) {
	if(item->finished) {
		mark_processable(stack, index);
	}
	//}
}

/* Called to mark an index of the queue as finished */

static void mark_finished(struct sstack *stack, int index)
{
	struct sstack_item *item = g_array_index(stack->array, struct sstack_item *, index);

	item->finished = 1;

	try_mark_processable(stack, index);
}

#endif
/* --------------------- */

static void try_advance_processing(struct sstack *stack)
{
	//g_assert(stack->proc_index == index);

	//printf("sstack: starting to process\n");
	while(stack->proc_index < stack->array->len) {
		struct sstack_item *item = g_array_index(stack->array, struct sstack_item *, stack->proc_index);

		//if(!item->finished) {
		//	break;
		//}

		//printf("sstack: processing ");
		//print_item(item);

		if(stack->process_func) {
			stack->process_func(stack->process_func_arg, item);
		}
		else {
			printf("warning: no process func\n");
		}

		stack->proc_index++;

		if(item->data_type == SSTACK_TYPE_POP)
			mark_deletable(stack, stack->proc_index-1);
		if(item->pushpop >= 0 && item->pushpop < stack->proc_index-1) {
			mark_deletable(stack, item->pushpop);
		}
	}
	try_start_deleting(stack);
	//printf("sstack: stopping processing\n");
}


/* Add an item to the queue */

void sstack_add_item(struct sstack *stack, struct sstack_item *item)
{
	int index;

	g_array_append_val(stack->array, item);

	//printf("stack after adding\n");
	//print_stack(stack);

	index = stack->array->len-1;

	if(item->data_type == SSTACK_TYPE_PUSH) {
		int top_of_wait_pop_stack;

		if(stack->wait_pop_stack->len && g_array_index(stack->wait_pop_stack, int, stack->wait_pop_stack->len-1)) {
			/* if the preceding is a wait_for_pop (and there is a preceding), push a wait_for_pop */
			const int one=1;
			g_array_append_val(stack->wait_pop_stack, one);
		}
		else {
			/* otherwise, push what the item wants */
			g_array_append_val(stack->wait_pop_stack, item->wait_pop);
		}

		top_of_wait_pop_stack = g_array_index(stack->wait_pop_stack, int, stack->wait_pop_stack->len-1);

		g_array_append_val(stack->pushes, index);

		//printf("after pushing:\n");
		//print_stack(stack);
		if(top_of_wait_pop_stack == 0) {
			try_advance_processing(stack);
			/* ASSERT that we processed the whole sstack */
		}
		//printf("after processing:\n");
		//print_stack(stack);
	}
	else if(item->data_type == SSTACK_TYPE_POP) {
		item->finished = 1;

		if(stack->pushes->len > 0) {
			/* FIXME: confirm we are popping what we expected to pop */
			item->pushpop = g_array_index(stack->pushes, int, stack->pushes->len-1);
			g_array_index(stack->array, struct sstack_item *, item->pushpop)->pushpop = index;
			g_array_index(stack->array, struct sstack_item *, item->pushpop)->finished = 1;

			g_array_remove_index(stack->pushes, stack->pushes->len-1);
		}

		if(stack->wait_pop_stack->len > 0) {
			int top_of_wait_pop_stack;

			g_array_remove_index(stack->wait_pop_stack, stack->wait_pop_stack->len-1);

			if(stack->wait_pop_stack->len > 0) {
				top_of_wait_pop_stack = g_array_index(stack->wait_pop_stack, int, stack->wait_pop_stack->len-1);

				if(top_of_wait_pop_stack == 0)
					try_advance_processing(stack);
			}
			else {
				try_advance_processing(stack);
			}
		}
		else {
			try_advance_processing(stack);
		}
	}

	//printf("stack after processing\n");
	//print_stack(stack);
}

/* Force processing of all items */

void sstack_force_flush(struct sstack *stack)
{
	int i;

	for(i=0; i<stack->array->len; i++) {
		struct sstack_item *item = g_array_index(stack->array, struct sstack_item *, i);

		if(!item->finished) {
			item->finished = 1;
		}
	}

	try_advance_processing(stack);
}

/* Create a new sstack */

struct sstack *sstack_new(void)
{
	struct sstack *retval;

	retval = (struct sstack *) g_malloc(sizeof(struct sstack));

	retval->array = g_array_new(FALSE, FALSE, sizeof(struct sstack_item *));
	retval->pushes = g_array_new(FALSE, FALSE, sizeof(int));
	retval->wait_pop_stack = g_array_new(FALSE, FALSE, sizeof(int));
	retval->proc_index = 0;
	retval->process_func = NULL;

	return retval;
}

/* Create a new sstack_item. Normally not invoked directly. See other functions below. */

struct sstack_item *sstack_item_new(void)
{
	struct sstack_item *retval;

	retval = (struct sstack_item *) g_malloc(sizeof(struct sstack_item));
	retval->finished = 0;
	retval->processable = 0;
	retval->deletable = 0;
	retval->data_type = 0;
	retval->data_val = NULL;
	retval->delete_data_val = NULL;
	retval->pushpop = -1;
	retval->wait_pop = 0;
	//retval->depends = g_array_new(FALSE, FALSE, sizeof(int));
	//retval->rev_depends = g_array_new(FALSE, FALSE, sizeof(int));

	return retval;
}

/* Create a new sstack_item that will represent a PUSH operation */

struct sstack_item *sstack_item_new_push(unsigned char wait_pop)
{
	struct sstack_item *retval = sstack_item_new();

	retval->data_type = SSTACK_TYPE_PUSH;
	retval->wait_pop = wait_pop;

	return retval;
}

/* Create a new sstack_item that will represent a POP operation */

struct sstack_item *sstack_item_new_pop(void)
{
	struct sstack_item *retval = sstack_item_new();

	retval->data_type = SSTACK_TYPE_POP;
	retval->finished = 1;

	return retval;
}

/* Create a new sstack_item that will represent an EVENT operation */

struct sstack_item *sstack_item_new_event(void)
{
	struct sstack_item *retval = sstack_item_new();

	retval->data_type = SSTACK_TYPE_EVENT;
	retval->finished = 1;
	retval->processable = 1;
	retval->deletable = 1;

	return retval;
}
