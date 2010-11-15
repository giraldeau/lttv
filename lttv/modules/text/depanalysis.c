/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2008 Pierre-Marc Fournier
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lttv/lttv.h>
#include <lttv/option.h>
#include <lttv/module.h>
#include <lttv/hook.h>
#include <lttv/attribute.h>
#include <lttv/iattribute.h>
#include <lttv/stats.h>
#include <lttv/filter.h>
#include <lttv/print.h>
#include <ltt/ltt.h>
#include <ltt/event.h>
#include <ltt/trace.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <glib.h>
#include <stdlib.h>

#include "sstack.h"

static LttvHooks
  *before_traceset,
  *after_traceset,
//  *before_trace,
  *event_hook;

static int depanalysis_range_pid = -1;
static int depanalysis_range_pid_searching = -1;
static int depanalysis_use_time=0;
static int depanalysis_event_limit = -1;
static int a_print_simple_summary = 0;
static LttTime depanalysis_time1, depanalysis_time2;
static char *arg_t1_str,*arg_t2_str;
static int statedump_finished = 0;


struct llev_state_info_irq {
	int irq;
};

struct llev_state_info_softirq {
	int softirq;
};

struct llev_state_info_syscall {
	int syscall_id;

	int substate;

	void *private;
};

struct llev_state_info_syscall__open {
	GQuark filename;
};

struct llev_state_info_syscall__read {
	GQuark filename;
};

struct llev_state_info_syscall__poll {
	GQuark filename;
};

struct llev_state_info_preempted {
	int prev_state;
};

struct hlev_state_info_blocked {
	int syscall_id;
	unsigned char trap; /* flag */
	int substate;

	/* Garray of pointers to struct process_state that reflect the
         * low-level state stack when respectively entering and exiting the blocked
         * state.
         */
	GArray *llev_state_entry;
	GArray *llev_state_exit;

	int pid_exit; /* FIXME: it's not pretty to have this here; find this info elsewhere */
	LttTime time_woken;

	void *private;
};

struct hlev_state_info_blocked__open {
	GQuark filename;
};

struct hlev_state_info_blocked__read {
	GQuark filename;
};

struct hlev_state_info_blocked__poll {
	GQuark filename;
};

struct hlev_state_info_interrupted_irq {
	int irq;
};

struct hlev_state_info_interrupted_softirq {
	int softirq;
};

struct summary_tree_node {
	char *name;
	GHashTable *children;
	LttTime duration;
	GArray *episodes;
	int id_for_episodes;
};

struct state_info {
	char name[40];
	int size_priv;
	char *tree_path[6];
};

struct state_info llev_state_infos[] = {
	{ "UNKNOWN", 0, { NULL } },
	{ "RUNNING", 0, { NULL } },
	{ "SYSCALL", sizeof(struct llev_state_info_syscall), { NULL } },
	{ "IRQ", sizeof(struct llev_state_info_irq), { NULL } },
	{ "SOFTIRQ", sizeof(struct llev_state_info_softirq), { NULL } },
	{ "TRAP", 0, { NULL } },
	{ "PREEMPTED", sizeof(struct llev_state_info_preempted), { NULL } },
};

struct state_info hlev_state_infos[] = {
	{ "UNKNOWN", 0, { "Total", "Unknown", NULL } },
	{ "RUNNING", 0, { "Total", "Working", NULL } },
	{ "BLOCKED", sizeof(struct hlev_state_info_blocked), { "Total", "Blocked", NULL } },
	{ "INTERRUPTED_IRQ", sizeof(struct hlev_state_info_interrupted_irq), { "Total", "Interrupted", "IRQ", NULL } },
	{ "INTERRUPTED_SOFTIRQ", sizeof(struct hlev_state_info_interrupted_softirq), { "Total", "Interrupted", "SoftIRQ", NULL } },
	{ "INTERRUPTED_CPU", 0, { "Total", "Interrupted", "Preempted", NULL } },
	{ "INTERRUPTED_POST_BLOCK", 0, { "Total", "Interrupted", "Waiting schedule after blocking", NULL } },
};

enum llev_state {
	LLEV_UNKNOWN=0,
	LLEV_RUNNING,
	LLEV_SYSCALL,
	LLEV_IRQ,
	LLEV_SOFTIRQ,
	LLEV_TRAP,
	LLEV_PREEMPTED,
};

enum llev_syscall_substate {
	LLEV_SYSCALL__UNDEFINED,
	LLEV_SYSCALL__OPEN,
	LLEV_SYSCALL__READ,
	LLEV_SYSCALL__POLL,
};

enum hlev_event {
	HLEV_EVENT_TRY_WAKEUP=0,
};

enum hlev_state {
	HLEV_UNKNOWN=0,
	HLEV_RUNNING,
	HLEV_BLOCKED,
	HLEV_INTERRUPTED_IRQ,
	HLEV_INTERRUPTED_SOFTIRQ,
	HLEV_INTERRUPTED_CPU,
	HLEV_INTERRUPTED_POST_BLOCK,
};

enum hlev_state_blocked {
	HLEV_BLOCKED__UNDEFINED,
	HLEV_BLOCKED__OPEN,
	HLEV_BLOCKED__READ,
	HLEV_BLOCKED__POLL,
};

struct sstack_event {
	int event_type;
	void *private;
};

struct try_wakeup_event {
	int pid; /* this sould be more precise avec pid may be reused */
	LttTime time;
	struct process *waker;
};

struct process_state {
	int bstate;
	int cause_type;
	void *private;

	LttTime time_begin;
	LttTime time_end;
};

struct process_with_state {
	struct process *process;
	struct process_state state;
};

#define PROCESS_STATE_STACK_SIZE 10
struct process {
	int pid;
	GQuark name;
	int parent;

	struct sstack *stack;
	struct process_state *llev_state_stack[PROCESS_STATE_STACK_SIZE];
	int stack_current;
	struct process_state *hlev_state;
	GArray *hlev_history;
};

static inline void *old_process_state_private_data(struct process *p)
{
	return p->llev_state_stack[p->stack_current]->private;
}

static inline struct process_state *process_find_state(struct process *p, enum llev_state st)
{
	int i;

	for(i=p->stack->array->len-1; i>=0; i--) {
		struct sstack_item *item = g_array_index(p->stack->array, struct sstack_item *, i);

		struct process_with_state *pwstate = item->data_val;
		if(pwstate->state.bstate == st) {
			return &pwstate->state;
		}
	}

	return NULL;
}

static int find_pos_in_stack(enum llev_state lls, struct process *p)
{
	int i;
	for(i=p->stack_current; i>=0; i--) {
		if(p->llev_state_stack[i]->bstate == lls)
			return i;
	}

	return -1;
}

static struct process_state *find_in_stack(enum llev_state lls, struct process *p)
{
	int result;

	result = find_pos_in_stack(lls, p);

	if(result >= 0)
		return p->llev_state_stack[result];
	else
		return NULL;

}

/* called back from sstack on deletion of a data_val which is
 * a struct process_with_state
 */

static void delete_data_val(struct process_with_state *pwstate)
{
	// FIXME: Free this also
	//g_free(pwstate->state.private);

	// FIXME: this is really ugly. Don't free the pwstate if the state is LLEV_RUNNING.
	// LLEV_RUNNING is a special case that's being processed and deleted immediately after
	// being inserted on the sstack, to prevent state begin accumulated because it couldn't
	// be processed before the end of the trace. If we free the state, we get invalid memory
	// reads when looking at it on the state_stack.
	//if(pwstate->state.bstate != LLEV_RUNNING)
	//	g_free(pwstate);
}

inline void print_time(LttTime t)
{
	//printf("%lu.%lu", t.tv_sec, t.tv_nsec);
	double f;
	f = (double)t.tv_sec + ((double)t.tv_nsec)/1000000000.0;
	printf("%.9f", f);
}

static struct sstack_item *prepare_push_item(struct process *p, enum llev_state st, LttTime t)
{
	struct process_with_state *pwstate = g_malloc(sizeof(struct process_with_state));
	struct sstack_item *item;

	int wait_for_pop = 0;

	if(st == LLEV_SYSCALL) {
		/* We need to push LLEV_SYSCALL as wait_for_pop because it depends on some of
		 * its children. If we don't do this, it's going to get processed immediately
		 * by the sstack and we might miss some details about it that will come later.
		 */
		wait_for_pop = 1;
	}

	item = sstack_item_new_push(wait_for_pop);

	//printf("pushing in context of %d\n", p->pid);

	pwstate->process = p;
	pwstate->state.bstate = st;
	pwstate->state.time_begin = t;
	pwstate->state.private = g_malloc(llev_state_infos[st].size_priv);

	item->data_val = pwstate;
	item->delete_data_val = (void (*)(void*))delete_data_val;

	return item;
}

static void *item_private(struct sstack_item *item)
{
	struct process_with_state *pwstate = item->data_val;
	return pwstate->state.private;
}

static void commit_item(struct process *p, struct sstack_item *item)
{
	sstack_add_item(p->stack, item);
}

static void old_process_push_llev_state(struct process *p, struct process_state *pstate)
{
	if(++p->stack_current >= PROCESS_STATE_STACK_SIZE) {
		fprintf(stderr, "depanalysis: internal process stack overflow\n");
		abort();
	}

	p->llev_state_stack[p->stack_current] = pstate;
}

static void live_complete_process_push_llev_state(struct process *p, enum llev_state st, LttTime t)
{
	struct process_state *pstate = g_malloc(sizeof(struct process_state));

	pstate->bstate = st;
	pstate->time_begin = t;
	pstate->private = g_malloc(llev_state_infos[st].size_priv);

	old_process_push_llev_state(p, pstate);
}

static void prepare_pop_item_commit_nocheck(struct process *p, enum llev_state st, LttTime t)
{
	struct process_with_state *pwstate;
	struct sstack_item *item = sstack_item_new_pop();

	int push_idx;

	if(p->stack->pushes->len > 0)
		push_idx = g_array_index(p->stack->pushes, int, p->stack->pushes->len-1);
	else
		push_idx = -1;

	if(push_idx >= 0) {
		pwstate = g_array_index(p->stack->array, struct sstack_item *, push_idx)->data_val;
		pwstate->process = p;
		pwstate->state.time_end = t;
		item->data_val = pwstate;
		/* don't set delete_data_val because we use the same pwstate as push, and we don't want to free it twice */
	}
	else {

		pwstate = g_malloc(sizeof(struct process_with_state));
		pwstate->process = p;
		item->data_val = pwstate;
		pwstate->state.time_end = t;
		pwstate->state.bstate = st;
	}

	sstack_add_item(p->stack, item);

}

static void prepare_pop_item_commit(struct process *p, enum llev_state st, LttTime t)
{
	struct process_with_state *pwstate;
	struct sstack_item *item = sstack_item_new_pop();

	int push_idx;
	       
	if(p->stack->pushes->len > 0)
		push_idx = g_array_index(p->stack->pushes, int, p->stack->pushes->len-1);
	else
		push_idx = -1;

	if(push_idx >= 0) {
		/* FIXME: ugly workaround for kernel bug that generates two kernel_arch_syscall_exit on fork.
		 * The bug only occurs upon creation of new processes. But these processes always have
		 * a LLEV_RUNNING at index 0. */
		if(push_idx >= p->stack->array->len)
			return;

		pwstate = g_array_index(p->stack->array, struct sstack_item *, push_idx)->data_val;

		if(pwstate->state.bstate != st) {
			/* FIXME: ugly workaround for kernel bug that generates two kernel_arch_syscall_exit on fork */
			if(st != LLEV_SYSCALL) {
				printf("bad pop! at ");
				print_time(t);
				printf("\n");
				print_stack(p->stack);
				abort();
			}
			else {
				/* case where we have a double syscall_exit */
				return;
			}
		}
	}

	prepare_pop_item_commit_nocheck(p, st, t);
}


static int try_pop_blocked_llev_preempted(struct process *p, LttTime t)
{
	int push_idx;
	struct process_with_state *pwstate;

	if(p->stack->pushes->len > 0)
		push_idx = g_array_index(p->stack->pushes, int, p->stack->pushes->len-1);
	else
		push_idx = -1;

	if(push_idx >= 0) {
		pwstate = g_array_index(p->stack->array, struct sstack_item *, push_idx)->data_val;

		if(!(pwstate->state.bstate == LLEV_PREEMPTED && ((struct llev_state_info_preempted *)pwstate->state.private)->prev_state > 0)) {
			//printf("double try wake up\n");
			return 0;
		}
	}

	prepare_pop_item_commit_nocheck(p, LLEV_PREEMPTED, t);
	return 1;
}

static void old_process_pop_llev_state(struct process *p, struct process_state *pstate)
{
	/* Ensure we are really popping the current state */
	/* FIXME: pstate->bstate is uninitialized? */
	// Commenting because it does not work. The way things work now, this check cannot work.
	//if(p->llev_state_stack[p->stack_current]->bstate != LLEV_UNKNOWN && p->llev_state_stack[p->stack_current]->bstate != pstate->bstate) {
	//	printf("ERROR! bad pop!\n");
	//	abort();
	//}

	/* Actually change the that position */
	if(p->stack_current >= 0)
		p->stack_current--;

	/* If stack empty, we must put something in it */
	if(p->stack_current == -1) {
		if(pstate->bstate == LLEV_SYSCALL) {
			//process_push_llev_state(p, LLEV_RUNNING, pstate->time_end);
			live_complete_process_push_llev_state(p, LLEV_RUNNING, pstate->time_end);
		}
		else {
			live_complete_process_push_llev_state(p, LLEV_UNKNOWN, pstate->time_end);
		}
	}
}

static GHashTable *process_hash_table;
static GHashTable *syscall_table;
static GHashTable *irq_table;
static GHashTable *softirq_table;

/* Insert the hooks before and after each trace and tracefile, and for each
   event. Print a global header. */

static FILE *a_file;

static GString *a_string;

static gboolean write_traceset_header(void *hook_data, void *call_data)
{
  LttvTracesetContext *tc = (LttvTracesetContext *)call_data;

  g_info("Traceset header");

  /* Print the trace set header */
  g_info(a_file,"Trace set contains %d traces\n\n",
      lttv_traceset_number(tc->ts));

  return FALSE;
}

GArray *oldstyle_stack_to_garray(struct process_state **oldstyle_stack, int current)
{
	GArray *retval;
	int i;

	retval = g_array_new(FALSE, FALSE, sizeof(struct process_state *));

	for(i=0; i<current; i++) {
		g_array_append_val(retval, oldstyle_stack[i]);
	}

	return retval;
}

static void update_hlev_state(struct process *p, LttTime t)
{
	int i;

	enum hlev_state new_hlev;

	for(i=p->stack_current; i>=0; i--) {
		enum llev_state st;
		st = p->llev_state_stack[i]->bstate;

		if(st == LLEV_RUNNING || st == LLEV_TRAP || st == LLEV_SYSCALL) {
			new_hlev = HLEV_RUNNING;
			break;
		}
		else if(st == LLEV_IRQ) {
			new_hlev = HLEV_INTERRUPTED_IRQ;
			break;
		}
		else if(st == LLEV_SOFTIRQ) {
			new_hlev = HLEV_INTERRUPTED_SOFTIRQ;
			break;
		}
		else if(st == LLEV_PREEMPTED) {
			int prev_state = ((struct llev_state_info_preempted *) old_process_state_private_data(p))->prev_state;

			if(prev_state == 0) {
				new_hlev = HLEV_INTERRUPTED_CPU;
			}
			else if(prev_state == -1) {
				new_hlev = HLEV_INTERRUPTED_POST_BLOCK;
			}
			else {
				new_hlev = HLEV_BLOCKED;
			}
			break;
		}
		else if(st == LLEV_UNKNOWN) {
			new_hlev = HLEV_UNKNOWN;
			break;
		}
		else {
			abort();
		}
	}

	/* If no state change, do nothing */
	if(p->hlev_state != NULL && new_hlev == p->hlev_state->bstate) {
		return;
	}

	p->hlev_state->time_end = t;
	/* This check is here because we initially put HLEV_UNKNOWN as hlev state, but in the case
	 * of processes newly created, it is immediately replaced by HLEV_BLOCKED. In order to avoid
	 * having a UNKNOWN state of duration 0 in the summary, we don't add it. This isn't as elegant
	 * as it ought to be.
	 */
	if(ltt_time_compare(p->hlev_state->time_begin, p->hlev_state->time_end) != 0)
		g_array_append_val(p->hlev_history, p->hlev_state);
	p->hlev_state = g_malloc(sizeof(struct process_state));
	p->hlev_state->bstate = new_hlev;
	p->hlev_state->time_begin = t;
	p->hlev_state->private = g_malloc(hlev_state_infos[new_hlev].size_priv);

	//printf("depanalysis: now at hlev state %s\n", hlev_state_infos[new_hlev].name);

	/* Set private data */
	switch(p->hlev_state->bstate) {
		case HLEV_UNKNOWN:
			break;
		case HLEV_RUNNING:
			break;
		case HLEV_BLOCKED: {
			struct hlev_state_info_blocked *hlev_blocked_private = p->hlev_state->private;
			//struct process_state *ps = find_in_stack(LLEV_SYSCALL, p);
			int syscall_pos = find_pos_in_stack(LLEV_SYSCALL, p);
			int trap_pos = find_pos_in_stack(LLEV_TRAP, p);

			/* init vals */
			hlev_blocked_private->syscall_id = 1;
			hlev_blocked_private->trap = 0;
			hlev_blocked_private->substate = HLEV_BLOCKED__UNDEFINED;
			hlev_blocked_private->private = NULL;
			hlev_blocked_private->llev_state_entry = oldstyle_stack_to_garray(p->llev_state_stack, p->stack_current);
			hlev_blocked_private->llev_state_exit = NULL;

			//g_assert(syscall_pos >= 0 || trap_pos >= 0);

			if(trap_pos > syscall_pos) {
				hlev_blocked_private->trap = 1;
			}

			/* initial value, may be changed below */
			hlev_blocked_private->substate = HLEV_BLOCKED__UNDEFINED;

			if(syscall_pos >= 0) {
				struct process_state *ps = p->llev_state_stack[syscall_pos];
				struct llev_state_info_syscall *llev_syscall_private = (struct llev_state_info_syscall *) ps->private;
				hlev_blocked_private->syscall_id = llev_syscall_private->syscall_id;

				if(llev_syscall_private->substate == LLEV_SYSCALL__OPEN) {
					struct llev_state_info_syscall__open *llev_syscall_open_private;
					struct hlev_state_info_blocked__open *hlev_blocked_open_private;
					llev_syscall_open_private = llev_syscall_private->private;
					hlev_blocked_private->substate = HLEV_BLOCKED__OPEN;
					hlev_blocked_open_private = g_malloc(sizeof(struct hlev_state_info_blocked__open));
					hlev_blocked_private->private = hlev_blocked_open_private;
					hlev_blocked_open_private->filename = llev_syscall_open_private->filename;

					//printf("depanalysis: blocked in an open!\n");
				}
				else if(llev_syscall_private->substate == LLEV_SYSCALL__READ) {
					struct llev_state_info_syscall__read *llev_syscall_read_private;
					struct hlev_state_info_blocked__read *hlev_blocked_read_private;
					llev_syscall_read_private = llev_syscall_private->private;
					hlev_blocked_private->substate = HLEV_BLOCKED__READ;
					hlev_blocked_read_private = g_malloc(sizeof(struct hlev_state_info_blocked__read));
					hlev_blocked_private->private = hlev_blocked_read_private;
					hlev_blocked_read_private->filename = llev_syscall_read_private->filename;

					//printf("depanalysis: blocked in a read!\n");
				}
				else if(llev_syscall_private->substate == LLEV_SYSCALL__POLL) {
					struct llev_state_info_syscall__poll *llev_syscall_poll_private;
					struct hlev_state_info_blocked__poll *hlev_blocked_poll_private;
					llev_syscall_poll_private = llev_syscall_private->private;
					hlev_blocked_private->substate = HLEV_BLOCKED__POLL;
					hlev_blocked_poll_private = g_malloc(sizeof(struct hlev_state_info_blocked__poll));
					hlev_blocked_private->private = hlev_blocked_poll_private;
					hlev_blocked_poll_private->filename = llev_syscall_poll_private->filename;

					//printf("depanalysis: blocked in a read!\n");
				}
			}
			else {
				hlev_blocked_private->syscall_id = -1;
			}

			break;
		}
		case HLEV_INTERRUPTED_IRQ: {
			struct hlev_state_info_interrupted_irq *sinfo = p->hlev_state->private;
			struct process_state *ps = find_in_stack(LLEV_IRQ, p);
			if(ps == NULL)
				abort();
			else
				sinfo->irq = ((struct llev_state_info_irq *) ps->private)->irq;
			break;
		}
		case HLEV_INTERRUPTED_SOFTIRQ: {
			struct hlev_state_info_interrupted_softirq *sinfo = p->hlev_state->private;
			struct process_state *ps = find_in_stack(LLEV_SOFTIRQ, p);
			if(ps == NULL)
				abort();
			else
				sinfo->softirq = ((struct llev_state_info_softirq *) ps->private)->softirq;
			break;
		}
		default:
			break;
	};
}

static gint compare_summary_tree_node_times(gconstpointer a, gconstpointer b)
{
	struct summary_tree_node *n1 = (struct summary_tree_node *) a;
	struct summary_tree_node *n2 = (struct summary_tree_node *) b;

	return ltt_time_compare(n2->duration, n1->duration);
}

/* Print an item of the simple summary tree, and recurse, printing its children.
 *
 * If depth == -1, this is the root: we don't print a label, we only recurse into
 * the children.
 */

static void print_summary_item(struct summary_tree_node *node, int depth)
{
	GList *vals;

	if(depth >= 0) {
		printf("\t%*s (", strlen(node->name)+2*depth, node->name);
		print_time(node->duration);
		printf(") <%d>\n", node->id_for_episodes);
	}

	if(!node->children)
		return;

	vals = g_hash_table_get_values(node->children);

	/* sort the values */
	vals = g_list_sort(vals, compare_summary_tree_node_times);

	while(vals) {
		print_summary_item((struct summary_tree_node *)vals->data, depth+1);
		vals = vals->next;
	}

	/* we must free the list returned by g_hash_table_get_values() */
	g_list_free(vals);
}

static inline void print_irq(int irq)
{
	printf("IRQ %d [%s]", irq, g_quark_to_string(g_hash_table_lookup(irq_table, &irq)));
}

static inline void print_softirq(int softirq)
{
	printf("SoftIRQ %d [%s]", softirq, g_quark_to_string(g_hash_table_lookup(softirq_table, &softirq)));
}

static inline void print_pid(int pid)
{
	struct process *event_process_info = g_hash_table_lookup(process_hash_table, &pid);

	char *pname;

	if(event_process_info == NULL)
		pname = "?";
	else
		pname = g_quark_to_string(event_process_info->name);
	printf("%d [%s]", pid, pname);
}

static void modify_path_with_private(GArray *path, struct process_state *pstate)
{
	//GString tmps = g_string_new("");
	char *tmps;

	// FIXME: fix this leak
	switch(pstate->bstate) {
		case HLEV_INTERRUPTED_IRQ:
			asprintf(&tmps, "IRQ %d [%s]", ((struct hlev_state_info_interrupted_irq *)pstate->private)->irq, g_quark_to_string(g_hash_table_lookup(irq_table, &((struct hlev_state_info_interrupted_irq *)pstate->private)->irq)));
			g_array_append_val(path, tmps);
			break;
		case HLEV_INTERRUPTED_SOFTIRQ:
			asprintf(&tmps, "SoftIRQ %d [%s]", ((struct hlev_state_info_interrupted_softirq *)pstate->private)->softirq, g_quark_to_string(g_hash_table_lookup(softirq_table, &((struct hlev_state_info_interrupted_softirq *)pstate->private)->softirq)));
			g_array_append_val(path, tmps);
			break;
		case HLEV_BLOCKED: {
			struct hlev_state_info_blocked *hlev_blocked_private = (struct hlev_state_info_blocked *)pstate->private;

			if(hlev_blocked_private->trap) {
				char *ptr = "Trap";
				g_array_append_val(path, ptr);
			}
			
			if(hlev_blocked_private->syscall_id == -1) {
				char *ptr = "Userspace";
				g_array_append_val(path, ptr);
			}
			else {
				asprintf(&tmps, "Syscall %d [%s]", hlev_blocked_private->syscall_id, g_quark_to_string(g_hash_table_lookup(syscall_table, &hlev_blocked_private->syscall_id)));
				g_array_append_val(path, tmps);
			}

			if(((struct hlev_state_info_blocked *)pstate->private)->substate == HLEV_BLOCKED__OPEN) {
				char *str = g_quark_to_string(((struct hlev_state_info_blocked__open *)((struct hlev_state_info_blocked *)pstate->private)->private)->filename);
				g_array_append_val(path, str);
			}
			else if(((struct hlev_state_info_blocked *)pstate->private)->substate == HLEV_BLOCKED__READ) {
				char *str;
				asprintf(&str, "%s", g_quark_to_string(((struct hlev_state_info_blocked__read *)((struct hlev_state_info_blocked *)pstate->private)->private)->filename));
				g_array_append_val(path, str);
				/* FIXME: this must be freed at some point */
				//free(str);
			}
			else if(((struct hlev_state_info_blocked *)pstate->private)->substate == HLEV_BLOCKED__POLL) {
				char *str;
				asprintf(&str, "%s", g_quark_to_string(((struct hlev_state_info_blocked__poll *)((struct hlev_state_info_blocked *)pstate->private)->private)->filename));
				g_array_append_val(path, str);
				/* FIXME: this must be freed at some point */
				//free(str);
			}
			break;
		}
	};
}

void print_stack_garray_horizontal(GArray *stack)
{
	/* FIXME: this function doesn't work if we delete the states as we process them because we
	 * try to read those states here to print the low level stack.
	 */
	int i;

	for(i=0; i<stack->len; i++) {
		struct process_state *pstate = g_array_index(stack, struct process_state *, i);
		printf("%s", llev_state_infos[pstate->bstate].name);

		if(pstate->bstate == LLEV_SYSCALL) {
			struct llev_state_info_syscall *llev_syscall_private = pstate->private;
			printf(" %d [%s]", llev_syscall_private->syscall_id, g_quark_to_string(g_hash_table_lookup(syscall_table, &llev_syscall_private->syscall_id)));
		}

		printf(", ");
		
	}
}

static int dicho_search_state_ending_after(struct process *p, LttTime t)
{
	int under = 0;
	int over = p->hlev_history->len-1;
	struct process_state *pstate;
	int result;

	if(over < 1)
		return -1;

	/* If the last element is smaller or equal than the time we are searching for,
	 * no match
	 */
	pstate = g_array_index(p->hlev_history, struct process_state *, over);
	if(ltt_time_compare(pstate->time_end, t) <= 0) {
		return -1;
	}
	/* no need to check for the equal case */

	pstate = g_array_index(p->hlev_history, struct process_state *, under);
	result = ltt_time_compare(pstate->time_end, t);
	if(result >= 1) {
		/* trivial match at the first element if it is greater or equal
		 * than the time we want
		 */
		return under;
	}

	while(1) {
		int dicho;

		dicho = (under+over)/2;
		pstate = g_array_index(p->hlev_history, struct process_state *, dicho);
		result = ltt_time_compare(pstate->time_end, t);

		if(result == -1) {
			under = dicho;
		}
		else if(result == 1) {
			over = dicho;
		}
		else {
			/* exact match */
			return dicho+1;
		}

		if(over-under == 1) {
			/* we have converged */
			return over;
		}
	}

}

/* FIXME: this shouldn't be based on pids in case of reuse
 * FIXME: should add a list of processes used to avoid loops
 */

static struct process_state *find_state_ending_after(int pid, LttTime t)
{
	struct process *p;
	int result;


	p = g_hash_table_lookup(process_hash_table, &pid);
	if(!p)
		return NULL;

	result = dicho_search_state_ending_after(p, t);

	if(result == -1)
		return NULL;
	else
		return g_array_index(p->hlev_history, struct process_state *, result);
}

static void print_indent(int offset)
{
	if (offset > 2) {
		int i;

		printf("%*s", 8, "");
		for (i = 3; i < offset; i++) {
			printf("|");
			printf("%*s", 4, "");
		}
	} else
		printf("%*s", 4*offset, "");
}

static void print_delay_pid(int pid, LttTime t1, LttTime t2, int offset)
{
	struct process *p;
	int i;

	p = g_hash_table_lookup(process_hash_table, &pid);
	if(!p)
		return;

	i = dicho_search_state_ending_after(p, t1);
	for(; i<p->hlev_history->len; i++) {
		struct process_state *pstate = g_array_index(p->hlev_history, struct process_state *, i);
		if(ltt_time_compare(pstate->time_end, t2) > 0)
			break;
			
		if(pstate->bstate == HLEV_BLOCKED) {
			struct hlev_state_info_blocked *state_private_blocked;
			state_private_blocked = pstate->private;
			struct process_state *state_unblocked;

			print_indent(offset);
			printf("--> Blocked in ");
			print_stack_garray_horizontal(state_private_blocked->llev_state_entry);

			printf("(times: ");
			print_time(pstate->time_begin);
			printf("-");
			print_time(pstate->time_end);

			printf(", dur: %f)\n", 1e-9*ltt_time_to_double(ltt_time_sub(pstate->time_end, pstate->time_begin)));

			state_unblocked = find_state_ending_after(state_private_blocked->pid_exit, state_private_blocked->time_woken);
			if(state_unblocked) {
				if(state_unblocked->bstate == HLEV_INTERRUPTED_IRQ) {
					struct hlev_state_info_interrupted_irq *priv = state_unblocked->private;
					/* if in irq or softirq, we don't care what the waking process was doing because they are asynchroneous events */
					print_indent(offset);
					printf("--- Woken up by an IRQ: ");
					print_irq(priv->irq);
					printf("\n");
				}
				else if(state_unblocked->bstate == HLEV_INTERRUPTED_SOFTIRQ) {
					struct hlev_state_info_interrupted_softirq *priv = state_unblocked->private;
					print_indent(offset);
					printf("--- Woken up by a SoftIRQ: ");
					print_softirq(priv->softirq);
					printf("\n");
				}
				else {
					LttTime t1prime=t1;
					LttTime t2prime=t2;

					if(ltt_time_compare(t1prime, pstate->time_begin) < 0)
						t1prime = pstate->time_begin;
					if(ltt_time_compare(t2prime, pstate->time_end) > 0)
						t2prime = pstate->time_end;

					print_delay_pid(state_private_blocked->pid_exit, t1prime, t2prime, offset+1);
					print_indent(offset);
					printf("--- Woken up in context of ");
					print_pid(state_private_blocked->pid_exit);
					if(state_private_blocked->llev_state_exit) {
						print_stack_garray_horizontal(state_private_blocked->llev_state_exit);
					}
					else {
					}
					printf(" in high-level state %s", hlev_state_infos[state_unblocked->bstate].name);
					printf("\n");
				}
			}
			else {
				print_indent(offset);
				printf("Weird... cannot find in what state the waker (%d) was\n", state_private_blocked->pid_exit);
			}


			//print_delay_pid(state_private_blocked->pid_exit, pstate->time_start, pstate->time_end);
			//printf("\t\t Woken up in context of %d: ", state_private_blocked->pid_exit);
			//if(state_private_blocked->llev_state_exit) {
			//	print_stack_garray_horizontal(state_private_blocked->llev_state_exit);
			//	printf("here3 (%d)\n", state_private_blocked->llev_state_exit->len);
			//}
			//else
			//	printf("the private_blocked %p had a null exit stack\n", state_private_blocked);
			//printf("\n");
		}
	}
}

static void print_range_critical_path(int process, LttTime t1, LttTime t2)
{
	printf("Critical path for requested range:\n");
	printf("Final process is %d\n", process);
	print_delay_pid(process, t1, t2, 2);
}

/*
 * output legend example:
 *
 *           --> Blocked in RUNNING, SYSCALL NNN [syscall_name]
 *           |      ---> Blocked in RUNNING, SYSCALL NNN [syscall_name]
 *           |      |        --> Blocked in RUNNING, SYSCALL  [syscall_name]
 *           |      |        --- Woken up by an IRQ: IRQ 0 [timer]
 *           |      ---  Woken up in context of PID [appname] in high-level state RUNNING
 *           --- Woken up in context of PID [appname] in high-level state RUNNING
 */

static void print_process_critical_path_summary()
{
	struct process *pinfo;
	GList *pinfos;
	int i,j;

	pinfos = g_hash_table_get_values(process_hash_table);
	if(pinfos == NULL) {
		fprintf(stderr, "error: no process found\n");
		return;
	}

	printf("Process Critical Path Summary:\n");

	for(;;) {
		struct summary_tree_node base_node = { children: NULL };

		struct process_state *hlev_state_cur;

		pinfo = (struct process *)pinfos->data;
		if (depanalysis_range_pid_searching != -1 && pinfo->pid != depanalysis_range_pid_searching)
			goto next_iter;
		printf("\tProcess %d [%s]\n", pinfo->pid, g_quark_to_string(pinfo->name));

		if(pinfo->hlev_history->len < 1)
			goto next_iter;

		print_delay_pid(pinfo->pid, g_array_index(pinfo->hlev_history, struct process_state *, 0)->time_begin, g_array_index(pinfo->hlev_history, struct process_state *, pinfo->hlev_history->len - 1)->time_end, 2);

		next_iter:

		if(pinfos->next)
			pinfos = pinfos->next;
		else
			break;
	}
}

gint compare_states_length(gconstpointer a, gconstpointer b)
{
	struct process_state **s1 = (struct process_state **)a;
	struct process_state **s2 = (struct process_state **)b;
	gint val;

	val = ltt_time_compare(ltt_time_sub((*s2)->time_end, (*s2)->time_begin), ltt_time_sub((*s1)->time_end, (*s1)->time_begin));
	return val;
}

static void print_simple_summary(void)
{
	struct process *pinfo;
	GList *pinfos;
	GList *pinfos_first;
	int i,j;
	int id_for_episodes = 0;

	if (!a_print_simple_summary)
		return;

	/* we save all the nodes here to print the episodes table quickly */
	GArray *all_nodes = g_array_new(FALSE, FALSE, sizeof(struct summary_tree_node *));

	pinfos_first = g_hash_table_get_values(process_hash_table);
	if(pinfos_first == NULL) {
		fprintf(stderr, "error: no processes found\n");
		return;
	}
	pinfos = pinfos_first;

	printf("Simple summary:\n");

	/* For each process */
	for(;;) {
		struct summary_tree_node base_node = { children: NULL, name: "Root" };

		struct process_state *hlev_state_cur;

		pinfo = (struct process *)pinfos->data;
		printf("\tProcess %d [%s]\n", pinfo->pid, g_quark_to_string(pinfo->name));

		/* For each state in the process history */
		for(i=0; i<pinfo->hlev_history->len; i++) {
			struct process_state *pstate = g_array_index(pinfo->hlev_history, struct process_state *, i);
			struct summary_tree_node *node_cur = &base_node;
			GArray *tree_path_garray;

			/* Modify the path based on private data */
			tree_path_garray = g_array_new(FALSE, FALSE, sizeof(char *));
			{
				int count=0;
				char **tree_path_cur2 = hlev_state_infos[pstate->bstate].tree_path;
				while(*tree_path_cur2) {
					count++;
					tree_path_cur2++;
				}
				g_array_append_vals(tree_path_garray, hlev_state_infos[pstate->bstate].tree_path, count);
			}
			modify_path_with_private(tree_path_garray, pstate);
			
			/* Walk the path, adding the nodes to the summary */
			for(j=0; j<tree_path_garray->len; j++) {
				struct summary_tree_node *newnode;
				GQuark componentquark;

				/* Have a path component we must follow */
				if(!node_cur->children) {
					/* must create the hash table for the children */
					node_cur->children = g_hash_table_new(g_int_hash, g_int_equal);
				}
				
				/* try to get the node for the next component */
				componentquark = g_quark_from_string(g_array_index(tree_path_garray, char *, j));
				newnode = g_hash_table_lookup(node_cur->children, &componentquark);
				if(newnode == NULL) {
					newnode = g_malloc(sizeof(struct summary_tree_node));
					newnode->children = NULL;
					newnode->name = g_array_index(tree_path_garray, char *, j);
					newnode->duration = ltt_time_zero;
					newnode->id_for_episodes = id_for_episodes++;
					newnode->episodes = g_array_new(FALSE, FALSE, sizeof(struct process_state *));
					g_hash_table_insert(node_cur->children, &componentquark, newnode);

					g_array_append_val(all_nodes, newnode);
				}
				node_cur = newnode;

				node_cur->duration = ltt_time_add(node_cur->duration, ltt_time_sub(pstate->time_end, pstate->time_begin));
				g_array_append_val(node_cur->episodes, pstate);
			}
		}

		/* print the summary */
		print_summary_item(&base_node, -1);

		printf("\n");

		if(pinfos->next)
			pinfos = pinfos->next;
		else
			break;
	}

	printf("\n");

	printf("Episode list\n");
	pinfos = pinfos_first;

	/* For all the nodes of the Simple summary tree */
	for(i=0; i<all_nodes->len; i++) {
		struct summary_tree_node *node = (struct summary_tree_node *)g_array_index(all_nodes, struct summary_tree_node *, i);

		/* Sort the episodes from longest to shortest */
		g_array_sort(node->episodes, compare_states_length);

		printf("\tNode id: <%d>\n", node->id_for_episodes);
		/* For each episode of the node */
		for(j=0; j<node->episodes->len; j++) {
			struct process_state *st = g_array_index(node->episodes, struct process_state *, j);

			printf("\t\t");
			print_time(st->time_begin);
			printf("-");
			print_time(st->time_end);
			printf(" (%f)\n", 1e-9*ltt_time_to_double(ltt_time_sub(st->time_end,st->time_begin)));
		}
	}
}

static void print_simple_summary_pid_range(int pid, LttTime t1, LttTime t2)
{
	struct process *pinfo;
	int i,j;
	int id_for_episodes = 0;

	/* we save all the nodes here to print the episodes table quickly */
	GArray *all_nodes = g_array_new(FALSE, FALSE, sizeof(struct summary_tree_node *));

	pinfo = g_hash_table_lookup(process_hash_table, &pid);

	{
		struct summary_tree_node base_node = { children: NULL, name: "Root" };

		struct process_state *hlev_state_cur;

		printf("\tProcess %d [%s]\n", pinfo->pid, g_quark_to_string(pinfo->name));

		/* For each state in the process history */
		for(i=0; i<pinfo->hlev_history->len; i++) {
			struct process_state *pstate = g_array_index(pinfo->hlev_history, struct process_state *, i);
			struct summary_tree_node *node_cur = &base_node;
			GArray *tree_path_garray;

			if(ltt_time_compare(pstate->time_end, t1) < 0)
				continue;

			if(ltt_time_compare(pstate->time_end, t2) > 0)
				break;

			/* Modify the path based on private data */
			tree_path_garray = g_array_new(FALSE, FALSE, sizeof(char *));
			{
				int count=0;
				char **tree_path_cur2 = hlev_state_infos[pstate->bstate].tree_path;
				while(*tree_path_cur2) {
					count++;
					tree_path_cur2++;
				}
				g_array_append_vals(tree_path_garray, hlev_state_infos[pstate->bstate].tree_path, count);
			}
			modify_path_with_private(tree_path_garray, pstate);
			
			/* Walk the path, adding the nodes to the summary */
			for(j=0; j<tree_path_garray->len; j++) {
				struct summary_tree_node *newnode;
				GQuark componentquark;

				/* Have a path component we must follow */
				if(!node_cur->children) {
					/* must create the hash table for the children */
					node_cur->children = g_hash_table_new(g_int_hash, g_int_equal);
				}
				
				/* try to get the node for the next component */
				componentquark = g_quark_from_string(g_array_index(tree_path_garray, char *, j));
				newnode = g_hash_table_lookup(node_cur->children, &componentquark);
				if(newnode == NULL) {
					newnode = g_malloc(sizeof(struct summary_tree_node));
					newnode->children = NULL;
					newnode->name = g_array_index(tree_path_garray, char *, j);
					newnode->duration = ltt_time_zero;
					newnode->id_for_episodes = id_for_episodes++;
					newnode->episodes = g_array_new(FALSE, FALSE, sizeof(struct process_state *));
					g_hash_table_insert(node_cur->children, &componentquark, newnode);

					g_array_append_val(all_nodes, newnode);
				}
				node_cur = newnode;

				node_cur->duration = ltt_time_add(node_cur->duration, ltt_time_sub(pstate->time_end, pstate->time_begin));
				g_array_append_val(node_cur->episodes, pstate);
			}
		}

		/* print the summary */
		print_summary_item(&base_node, -1);

		printf("\n");
	}

	printf("\n");

	printf("Episode list\n");

	/* For all the nodes of the Simple summary tree */
	for(i=0; i<all_nodes->len; i++) {
		struct summary_tree_node *node = (struct summary_tree_node *)g_array_index(all_nodes, struct summary_tree_node *, i);

		/* Sort the episodes from longest to shortest */
		g_array_sort(node->episodes, compare_states_length);

		printf("\tNode id: <%d>\n", node->id_for_episodes);
		/* For each episode of the node */
		for(j=0; j<node->episodes->len; j++) {
			struct process_state *st = g_array_index(node->episodes, struct process_state *, j);

			printf("\t\t");
			print_time(st->time_begin);
			printf("-");
			print_time(st->time_end);
			printf(" (%f)\n", 1e-9*ltt_time_to_double(ltt_time_sub(st->time_end,st->time_begin)));
		}
	}
}

static void flush_process_sstacks(void)
{
	GList *pinfos;
	
	pinfos = g_hash_table_get_values(process_hash_table);
	while(pinfos) {
		struct process *pinfo = (struct process *)pinfos->data;

		sstack_force_flush(pinfo->stack);

		pinfos = pinfos->next;
	}

	g_list_free(pinfos);
}

struct family_item {
	int pid;
	LttTime creation;
};

void print_range_reports(int pid, LttTime t1, LttTime t2)
{
	GArray *family = g_array_new(FALSE, FALSE, sizeof(struct family_item));
	int i;

	/* reconstruct the parental sequence */
	for(;;) {
		struct process *pinfo;
		struct family_item fi;
		LttTime cur_beg;

		pinfo = g_hash_table_lookup(process_hash_table, &pid);
		if(pinfo == NULL)
			abort();

		fi.pid = pid;
		cur_beg = g_array_index(pinfo->hlev_history, struct process_state *, 0)->time_begin;
		fi.creation = cur_beg;
		g_array_append_val(family, fi);

		if(ltt_time_compare(cur_beg, t1) == -1) {
			/* current pid starts before the interesting time */
			break;
		}
		if(pinfo->parent == -1) {
			printf("unable to go back, we don't know the parent of %d\n", fi.pid);
			abort();
		}
		/* else, we go on */
		pid = pinfo->parent;

	}

	printf("Simple summary for range:\n");
	for(i=family->len-1; i>=0; i--) {
		LttTime iter_t1, iter_t2;
		int iter_pid = g_array_index(family, struct family_item, i).pid;

		if(i == family->len-1)
			iter_t1 = t1;
		else
			iter_t1 = g_array_index(family, struct family_item, i).creation;

		if(i == 0)
			iter_t2 = t2;
		else
			iter_t2 = g_array_index(family, struct family_item, i-1).creation;

		printf("This section of summary concerns pid %d between ");
		print_time(iter_t1);
		printf(" and ");
		print_time(iter_t2);
		printf(".\n");
		print_simple_summary_pid_range(iter_pid, iter_t1, iter_t2);
	}
	print_range_critical_path(depanalysis_range_pid, t1, t2);
}

static gboolean write_traceset_footer(void *hook_data, void *call_data)
{
	LttvTracesetContext *tc = (LttvTracesetContext *)call_data;

	g_info("TextDump traceset footer");

	g_info(a_file,"End trace set\n\n");

//	if(LTTV_IS_TRACESET_STATS(tc)) {
//		lttv_stats_sum_traceset((LttvTracesetStats *)tc, ltt_time_infinite);
//		print_stats(a_file, (LttvTracesetStats *)tc);
//	}

	/* After processing all the events, we need to flush the sstacks
         * because some unfinished states may remain in them. We want them
         * event though there are incomplete.
         */
	flush_process_sstacks();

	/* print the reports */
	print_simple_summary();
	print_process_critical_path_summary();
	if(depanalysis_use_time == 3) {
		printf("depanalysis_use_time = %d\n", depanalysis_use_time);
		if(depanalysis_range_pid == -1 && depanalysis_range_pid_searching >= 0)
			depanalysis_range_pid = depanalysis_range_pid_searching;

		if(depanalysis_range_pid >= 0) {
			print_range_reports(depanalysis_range_pid, depanalysis_time1, depanalysis_time2);
		}
		else
			printf("range critical path: could not find the end of the range\n");
	}

  return FALSE;
}

#if 0
static gboolean write_trace_header(void *hook_data, void *call_data)
{
  LttvTraceContext *tc = (LttvTraceContext *)call_data;
#if 0 //FIXME
  LttSystemDescription *system = ltt_trace_system_description(tc->t);

  fprintf(a_file,"  Trace from %s in %s\n%s\n\n",
	  ltt_trace_system_description_node_name(system),
	  ltt_trace_system_description_domain_name(system),
	  ltt_trace_system_description_description(system));
#endif //0
  return FALSE;
}
#endif


static int write_event_content(void *hook_data, void *call_data)
{
  gboolean result;

//  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;

  LttEvent *e;

  guint cpu = tfs->cpu;
  LttvTraceState *ts = (LttvTraceState*)tfc->t_context;
  LttvProcessState *process = ts->running_process[cpu];

  e = ltt_tracefile_get_event(tfc->tf);

  lttv_event_to_string(e, a_string, TRUE, 1, tfs);

//  if(a_state) {
    g_string_append_printf(a_string, " %s ",
        g_quark_to_string(process->state->s));
//  }

  g_string_append_printf(a_string,"\n");

  fputs(a_string->str, a_file);
  return FALSE;
}

static int field_get_value_int(struct LttEvent *e, struct marker_info *info, GQuark f)
{
	struct marker_field *marker_field;
	int found=0;

	for_each_marker_field(marker_field, info) {
		if (marker_field->name == f) {
			found = 1;
			break;
		}
	}
	g_assert(found);
	return ltt_event_get_long_unsigned(e, marker_field);
}

static char *field_get_value_string(struct LttEvent *e, struct marker_info *info, GQuark f)
{
	struct marker_field *marker_field;
	int found=0;

	for_each_marker_field(marker_field, info) {
		if (marker_field->name == f) {
			found = 1;
			break;
		}
	}
	g_assert(found);
	return ltt_event_get_string(e, marker_field);
}

void process_delayed_stack_action(struct process *pinfo, struct sstack_item *item)
{
	//printf("processing delayed stack action on pid %d at ", pinfo->pid);
	//if(((struct process_with_state *) item->data_val)->state.time_begin.tv_nsec == 987799696)
	//	printf("HERE!!!\n");
	//print_time(((struct process_with_state *) item->data_val)->state.time_begin);
	//printf("\n");
	//printf("stack before:\n");
	//print_stack(pinfo->stack);

	if(item->data_type == SSTACK_TYPE_PUSH) {
		struct process_with_state *pwstate = item->data_val;
		//printf("pushing\n");
		old_process_push_llev_state(pinfo, &pwstate->state);
		update_hlev_state(pinfo, pwstate->state.time_begin);
	}
	else if(item->data_type == SSTACK_TYPE_POP) {
		struct process_with_state *pwstate = item->data_val;
		//printf("popping\n");
		old_process_pop_llev_state(pinfo, &pwstate->state);
		update_hlev_state(pinfo, pwstate->state.time_end);
	}
	else if(item->data_type == SSTACK_TYPE_EVENT) {
		struct sstack_event *se = item->data_val;
		if(se->event_type == HLEV_EVENT_TRY_WAKEUP) {
			/* FIXME: should change hlev event from BLOCKED to INTERRUPTED CPU  when receiving TRY_WAKEUP */
			struct try_wakeup_event *twe = se->private;

			/* FIXME: maybe do some more rigorous checking here */
			if(pinfo->hlev_state->bstate == HLEV_BLOCKED) {
				struct hlev_state_info_blocked *hlev_blocked_private = pinfo->hlev_state->private;

				hlev_blocked_private->pid_exit = twe->pid;
				hlev_blocked_private->time_woken = twe->time;
				hlev_blocked_private->llev_state_exit = oldstyle_stack_to_garray(twe->waker->llev_state_stack, twe->waker->stack_current);
				//printf("set a non null exit stack on %p, and stack size is %d\n", hlev_blocked_private, hlev_blocked_private->llev_state_exit->len);

				/*
				if(p->stack_current >= 0 && p->llev_state_stack[p->stack_current]->bstate == LLEV_PREEMPTED) {
					old_process_pop_llev_state(pinfo, p->llev_state_stack[p->stack_current]);
					update_hlev_state(pinfo
					old_process_push_llev_state
				}*/

			}
		}
	}

	//printf("stack after:\n");
	//print_stack(pinfo->stack);
}

static struct process *get_or_init_process_info(struct LttEvent *e, GQuark name, int pid, int *new)
{
	gconstpointer val;

	val = g_hash_table_lookup(process_hash_table, &pid);
	if(val == NULL) {
		struct process *pinfo;
		int i;

		/* Initialize new pinfo for newly discovered process */
		pinfo = g_malloc(sizeof(struct process));
		pinfo->pid = pid;
		pinfo->parent = -1; /* unknown parent */
		pinfo->hlev_history = g_array_new(FALSE, FALSE, sizeof(struct process_state *));
		pinfo->stack = sstack_new();
		pinfo->stack_current=-1;
		pinfo->stack->process_func = process_delayed_stack_action;
		pinfo->stack->process_func_arg = pinfo;
		for(i=0; i<PROCESS_STATE_STACK_SIZE; i++) {
			pinfo->llev_state_stack[i] = g_malloc(sizeof(struct process_state));
		}

		pinfo->hlev_state = g_malloc(sizeof(struct process_state));
		pinfo->hlev_state->bstate = HLEV_UNKNOWN;
		pinfo->hlev_state->time_begin = e->event_time;
		pinfo->hlev_state->private = NULL;

		/* set the name */
		pinfo->name = name;

		g_hash_table_insert(process_hash_table, &pinfo->pid, pinfo);
		if(new)
			*new = 1;
		return pinfo;
	}
	else {
		if(new)
			*new = 0;
		return val;
	 
	}
}

static int differentiate_swappers(int pid, LttEvent *e)
{
	if(pid == 0)
		return pid+e->tracefile->cpu_num+2000000;
	else
		return pid;
}

static int process_event(void *hook_data, void *call_data)
{
	LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
	LttvTracefileState *tfs = (LttvTracefileState *)call_data;
	LttEvent *e;
	struct marker_info *info;

	/* Extract data from event structures and state */
	guint cpu = tfs->cpu;
	LttvTraceState *ts = (LttvTraceState*)tfc->t_context;
	LttvProcessState *process = ts->running_process[cpu];
	LttTrace *trace = ts->parent.t;
	struct process *pinfo;

	e = ltt_tracefile_get_event(tfs->parent.tf);

	info = marker_get_info_from_id(tfc->tf->mdata, e->event_id);

	//if(depanalysis_use_time && (ltt_time_compare(e->timestamp, arg_t1) == -1 || ltt_time_compare(e->timestamp, arg_t2) == 1)) {
	//	return;
	//}
	/* Set the pid for the dependency analysis at each event, until we are passed the range. */
	if(depanalysis_use_time == 3) {
		if(ltt_time_compare(e->event_time, depanalysis_time2) <= 0) {
			depanalysis_range_pid = process->pid;
		}
		else {
			/* Should stop processing and print results */
		}
	}

	/* Code to limit the event count */
	if(depanalysis_event_limit > 0) {
		depanalysis_event_limit--;
	}
	else if(depanalysis_event_limit == 0) {
		write_traceset_footer(hook_data, call_data);
		printf("exit due to event limit reached\n");
		exit(0);
	}

	/* write event like textDump for now, for debugging purposes */
	//write_event_content(hook_data, call_data);

	if(tfc->tf->name == LTT_CHANNEL_SYSCALL_STATE && info->name == LTT_EVENT_SYS_CALL_TABLE) {
		GQuark q;
		int *pint = g_malloc(sizeof(int));

		*pint = field_get_value_int(e, info, LTT_FIELD_ID);
		q = g_quark_from_string(field_get_value_string(e, info, LTT_FIELD_SYMBOL));
		g_hash_table_insert(syscall_table, pint, q);
	}
	else if(tfc->tf->name == LTT_CHANNEL_IRQ_STATE && info->name == LTT_EVENT_LIST_INTERRUPT) {
		GQuark q;
		int *pint = g_malloc(sizeof(int));

		*pint = field_get_value_int(e, info, LTT_FIELD_IRQ_ID);
		q = g_quark_from_string(field_get_value_string(e, info, LTT_FIELD_ACTION));
		g_hash_table_insert(irq_table, pint, q);
	}
	else if(tfc->tf->name == LTT_CHANNEL_SOFTIRQ_STATE && info->name == LTT_EVENT_SOFTIRQ_VEC) {
		GQuark q;
		int *pint = g_malloc(sizeof(int));

		*pint = field_get_value_int(e, info, LTT_FIELD_ID);
		q = g_quark_from_string(field_get_value_string(e, info, LTT_FIELD_SYMBOL));
		g_hash_table_insert(softirq_table, pint, q);
	}


	/* Only look at events after the statedump is finished.
	 * Before that, the pids in the LttvProcessState are not reliable
	 */
	if(statedump_finished == 0) {
		if(tfc->tf->name == LTT_CHANNEL_GLOBAL_STATE && info->name == LTT_EVENT_STATEDUMP_END)
			statedump_finished = 1;
		else
			return FALSE;

	}

	pinfo = get_or_init_process_info(e, process->name, differentiate_swappers(process->pid, e), NULL);

	/* the state machine
	 * Process the event in the context of each process
	 */

	if(tfc->tf->name == LTT_CHANNEL_KERNEL && info->name == LTT_EVENT_IRQ_ENTRY) {
		struct process *event_process_info = pinfo;
		struct sstack_item *item;

		item = prepare_push_item(event_process_info, LLEV_IRQ, e->event_time);
		((struct llev_state_info_irq *) item_private(item))->irq = field_get_value_int(e, info, LTT_FIELD_IRQ_ID);
		commit_item(event_process_info, item);
	}
	else if(tfc->tf->name == LTT_CHANNEL_KERNEL && info->name == LTT_EVENT_IRQ_EXIT) {
		struct process *event_process_info = pinfo;

		prepare_pop_item_commit(event_process_info, LLEV_IRQ, e->event_time);
	}
	else if(tfc->tf->name == LTT_CHANNEL_KERNEL && info->name == LTT_EVENT_SCHED_SCHEDULE) {
		int next_pid = field_get_value_int(e, info, LTT_FIELD_NEXT_PID);
		int prev_pid = field_get_value_int(e, info, LTT_FIELD_PREV_PID);
		if(next_pid != 0) {
			struct process *event_process_info = get_or_init_process_info(e, process->name, differentiate_swappers(next_pid, e), NULL);
			prepare_pop_item_commit(event_process_info, LLEV_PREEMPTED, e->event_time);
		}
		if(prev_pid != 0) {
			struct sstack_item *item;
			struct process *event_process_info = get_or_init_process_info(e, process->name, differentiate_swappers(prev_pid, e), NULL);

			item = prepare_push_item(event_process_info, LLEV_PREEMPTED, e->event_time);
			((struct llev_state_info_preempted *) item_private(item))->prev_state = field_get_value_int(e, info, LTT_FIELD_PREV_STATE);
			commit_item(event_process_info, item);
		}
	}
	else if(tfc->tf->name == LTT_CHANNEL_KERNEL && info->name == LTT_EVENT_TRAP_ENTRY) {
		struct process *event_process_info = pinfo;
		struct sstack_item *item;

		item = prepare_push_item(event_process_info, LLEV_TRAP, e->event_time);
		commit_item(event_process_info, item);
	}
	else if(tfc->tf->name == LTT_CHANNEL_KERNEL && info->name == LTT_EVENT_TRAP_EXIT) {
		struct process *event_process_info = pinfo;

		prepare_pop_item_commit(event_process_info, LLEV_TRAP, e->event_time);
	}
	else if(tfc->tf->name == LTT_CHANNEL_KERNEL && info->name == LTT_EVENT_SYSCALL_ENTRY) {
		struct process *event_process_info = pinfo;
		struct sstack_item *item;

		item = prepare_push_item(event_process_info, LLEV_SYSCALL, e->event_time);
		((struct llev_state_info_syscall *) item_private(item))->syscall_id = field_get_value_int(e, info, LTT_FIELD_SYSCALL_ID);
		((struct llev_state_info_syscall *) item_private(item))->substate = LLEV_SYSCALL__UNDEFINED;
		commit_item(event_process_info, item);
	}
	else if(tfc->tf->name == LTT_CHANNEL_KERNEL && info->name == LTT_EVENT_SYSCALL_EXIT) {
		struct process *event_process_info = pinfo;

		prepare_pop_item_commit(event_process_info, LLEV_SYSCALL, e->event_time);
	}
	else if(tfc->tf->name == LTT_CHANNEL_KERNEL && info->name == LTT_EVENT_SOFT_IRQ_ENTRY) {
		struct process *event_process_info = pinfo;
		struct sstack_item *item;

		item = prepare_push_item(event_process_info, LLEV_SOFTIRQ, e->event_time);
		((struct llev_state_info_softirq *) item_private(item))->softirq = field_get_value_int(e, info, LTT_FIELD_SOFT_IRQ_ID);
		commit_item(event_process_info, item);
	}
	else if(tfc->tf->name == LTT_CHANNEL_KERNEL && info->name == LTT_EVENT_SOFT_IRQ_EXIT) {
		struct process *event_process_info = pinfo;

		prepare_pop_item_commit(event_process_info, LLEV_SOFTIRQ, e->event_time);
	}
	else if(tfc->tf->name == LTT_CHANNEL_KERNEL && info->name == LTT_EVENT_PROCESS_FORK) {
		int pid = differentiate_swappers(field_get_value_int(e, info, LTT_FIELD_CHILD_PID), e);
		struct process *event_process_info = get_or_init_process_info(e, process->name, differentiate_swappers(field_get_value_int(e, info, LTT_FIELD_CHILD_PID), e), NULL);
		struct sstack_item *item;

		event_process_info->parent = process->pid;
		//printf("At ");
		//print_time(e->event_time);
		//printf(", fork in process %d (%s), creating child %d\n", differentiate_swappers(process->pid, e), g_quark_to_string(process->name), pid);

		item = prepare_push_item(event_process_info, LLEV_RUNNING, e->event_time);
		commit_item(event_process_info, item);
		item = prepare_push_item(event_process_info, LLEV_SYSCALL, e->event_time);
		/* FIXME: this sets fork() as syscall, it's pretty inelegant */
		((struct llev_state_info_syscall *) item_private(item))->syscall_id = 57;
		((struct llev_state_info_syscall *) item_private(item))->substate = LLEV_SYSCALL__UNDEFINED;
		commit_item(event_process_info, item);

		item = prepare_push_item(event_process_info, LLEV_PREEMPTED, e->event_time);
		/* Consider fork as BLOCKED */
		((struct llev_state_info_preempted *) item_private(item))->prev_state = 1;
		commit_item(event_process_info, item);

		//printf("process %d now has a stack of height %d\n", differentiate_swappers(process->pid, e), get_or_init_process_info(e, process->name, differentiate_swappers(process->pid, cpu), NULL)->stack_current-1);

	}
	else if(tfc->tf->name == LTT_CHANNEL_FS && info->name == LTT_EVENT_EXEC) {
		struct process *event_process_info = pinfo;

		guint cpu = tfs->cpu;
		LttvProcessState *process_state = ts->running_process[cpu];
		event_process_info->name = process_state->name;
	}
	else if(tfc->tf->name == LTT_CHANNEL_FS && info->name == LTT_EVENT_OPEN) {
		struct process_state *pstate = process_find_state(pinfo, LLEV_SYSCALL);
		struct llev_state_info_syscall *llev_syscall_private;
		struct llev_state_info_syscall__open *llev_syscall_open_private;

		/* TODO: this is too easy */
		if(pstate == NULL)
			goto next_iter;

		llev_syscall_private = (struct llev_state_info_syscall *)pstate->private;

		//printf("depanalysis: found an open with state %d in pid %d\n", pstate->bstate, process->pid);
		if(pstate->bstate == LLEV_UNKNOWN)
			goto next_iter;

		g_assert(pstate->bstate == LLEV_SYSCALL);
		g_assert(llev_syscall_private->substate == LLEV_SYSCALL__UNDEFINED);

		llev_syscall_private->substate = LLEV_SYSCALL__OPEN;
		//printf("setting substate LLEV_SYSCALL__OPEN on syscall_private %p\n", llev_syscall_private);
		llev_syscall_private->private = g_malloc(sizeof(struct llev_state_info_syscall__open));
		llev_syscall_open_private = llev_syscall_private->private;

		llev_syscall_open_private->filename = g_quark_from_string(field_get_value_string(e, info, LTT_FIELD_FILENAME));
		
	}
	else if(tfc->tf->name == LTT_CHANNEL_FS && info->name == LTT_EVENT_READ) {
		struct process_state *pstate = process_find_state(pinfo, LLEV_SYSCALL);
		struct llev_state_info_syscall *llev_syscall_private;
		struct llev_state_info_syscall__read *llev_syscall_read_private;
		GQuark pfileq;
		int fd;

		/* TODO: this is too easy */
		if(pstate == NULL)
			goto next_iter;

		llev_syscall_private = (struct llev_state_info_syscall *)pstate->private;

		//printf("depanalysis: found an read with state %d in pid %d\n", pstate->bstate, process->pid);
		if(pstate->bstate == LLEV_UNKNOWN)
			goto next_iter;

		g_assert(pstate->bstate == LLEV_SYSCALL);
		g_assert(llev_syscall_private->substate == LLEV_SYSCALL__UNDEFINED);

		llev_syscall_private->substate = LLEV_SYSCALL__READ;
		//printf("setting substate LLEV_SYSCALL__READ on syscall_private %p\n", llev_syscall_private);
		llev_syscall_private->private = g_malloc(sizeof(struct llev_state_info_syscall__read));
		llev_syscall_read_private = llev_syscall_private->private;

		fd = field_get_value_int(e, info, LTT_FIELD_FD);
		pfileq = g_hash_table_lookup(process->fds, fd);
		if(pfileq) {
			llev_syscall_read_private->filename = pfileq;
		}
		else {
			char *tmp;
			asprintf(&tmp, "Unknown filename, fd %d", fd);
			llev_syscall_read_private->filename = g_quark_from_string(tmp);
			free(tmp);
		}
	}
	else if(tfc->tf->name == LTT_CHANNEL_FS && info->name == LTT_EVENT_POLL_EVENT) {
		struct process_state *pstate = process_find_state(pinfo, LLEV_SYSCALL);
		struct llev_state_info_syscall *llev_syscall_private;
		struct llev_state_info_syscall__poll *llev_syscall_poll_private;
		GQuark pfileq;
		int fd;

		/* TODO: this is too easy */
		if(pstate == NULL)
			goto next_iter;

		llev_syscall_private = (struct llev_state_info_syscall *)pstate->private;

		//printf("depanalysis: found an poll with state %d in pid %d\n", pstate->bstate, process->pid);
		if(pstate->bstate == LLEV_UNKNOWN)
			goto next_iter;

		/* poll doesn't have a single event that gives the syscall args. instead, there can be an arbitrary
		 * number of fs_pollfd or fd_poll_event events
		 * We use the fd_poll_event event, which occurs for each fd that had activity causing a return of the poll()
		 * For now we only use the first.
		 * We should do something about this. FIXME
		 */
		if(llev_syscall_private->substate == LLEV_SYSCALL__POLL)
			goto next_iter;

		g_assert(pstate->bstate == LLEV_SYSCALL);
		g_assert(llev_syscall_private->substate == LLEV_SYSCALL__UNDEFINED);

		llev_syscall_private->substate = LLEV_SYSCALL__POLL;
		//printf("setting substate LLEV_SYSCALL__POLL on syscall_private %p\n", llev_syscall_private);
		llev_syscall_private->private = g_malloc(sizeof(struct llev_state_info_syscall__poll));
		llev_syscall_poll_private = llev_syscall_private->private;

		fd = field_get_value_int(e, info, LTT_FIELD_FD);
		pfileq = g_hash_table_lookup(process->fds, fd);
		if(pfileq) {
			llev_syscall_poll_private->filename = pfileq;
		}
		else {
			char *tmp;
			asprintf(&tmp, "Unknown filename, fd %d", fd);
			llev_syscall_poll_private->filename = g_quark_from_string(tmp);
			free(tmp);
		}
	}
	else if(tfc->tf->name == LTT_CHANNEL_KERNEL && info->name == LTT_EVENT_SCHED_TRY_WAKEUP) {
		struct sstack_event *se = g_malloc(sizeof(struct sstack_event));
		struct try_wakeup_event *twe = g_malloc(sizeof(struct try_wakeup_event));
		struct sstack_item *item = sstack_item_new_event();
		int target = field_get_value_int(e, info, LTT_FIELD_PID);
		struct process *target_pinfo;
		int result;

		se->event_type = HLEV_EVENT_TRY_WAKEUP;
		se->private = twe;
		//printf("pushing try wake up event in context of %d\n", pinfo->pid);

		twe->pid = differentiate_swappers(process->pid, e);
		twe->time = e->event_time;
		twe->waker = pinfo;

		/* FIXME: the target could not yet have an entry in the hash table, we would then lose data */
		target_pinfo = g_hash_table_lookup(process_hash_table, &target);
		if(!target_pinfo)
			goto next_iter;

		item->data_val = se;
		item->delete_data_val = (void (*)(void *))delete_data_val;

		sstack_add_item(target_pinfo->stack, item);

		/* Now pop the blocked schedule out of the target */
		result = try_pop_blocked_llev_preempted(target_pinfo, e->event_time);

		if(result) {
			struct sstack_item *item;
			struct process *event_process_info = target_pinfo;

			item = prepare_push_item(event_process_info, LLEV_PREEMPTED, e->event_time);
			((struct llev_state_info_preempted *) item_private(item))->prev_state = -1; /* special value meaning post-block sched out */
			commit_item(event_process_info, item);
		}

	}

	next_iter:
	skip_state_machine:
	return FALSE;
}

void print_sstack_private(struct sstack_item *item)
{
	struct process_with_state *pwstate = item->data_val;

	if(pwstate && item->data_type == SSTACK_TYPE_PUSH)
		printf("\tstate: %s", llev_state_infos[pwstate->state.bstate].name);

	printf(" (");
	print_time(pwstate->state.time_begin);
	printf("-");
	print_time(pwstate->state.time_end);
	printf("\n");
		
}

static LttTime ltt_time_from_string(const char *str)
{
	LttTime retval;

	char *decdot = strchr(str, '.');

	if(decdot) {
		*decdot = '\0';
		retval.tv_nsec = atol(decdot+1);
	}
	else {
		retval.tv_nsec = 0;
	}

	retval.tv_sec = atol(str);

	return retval;
}

static void arg_t1(void *hook_data)
{
	printf("arg_t1\n");
	depanalysis_use_time |= 1;
	depanalysis_time1 = ltt_time_from_string(arg_t1_str);
}

static void arg_t2(void *hook_data)
{
	depanalysis_use_time |= 2;
	depanalysis_time2 = ltt_time_from_string(arg_t2_str);
}

static void arg_pid(void *hook_data)
{
}

static void arg_limit(void *hook_data)
{
}

static void arg_sum(void *hook_data)
{
}

static void init()
{
  gboolean result;

  print_sstack_item_data = print_sstack_private;

  LttvAttributeValue value;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  a_file = stdout;

  lttv_option_add("dep-time-start", 0, "dependency analysis time of analysis start", "time",
      LTTV_OPT_STRING, &arg_t1_str, arg_t1, NULL);
  lttv_option_add("dep-time-end", 0, "dependency analysis time of analysis end", "time",
      LTTV_OPT_STRING, &arg_t2_str, arg_t2, NULL);
  lttv_option_add("dep-pid", 0, "dependency analysis pid", "pid",
      LTTV_OPT_INT, &depanalysis_range_pid_searching, arg_pid, NULL);
  lttv_option_add("limit-events", 0, "dependency limit event count", "count",
      LTTV_OPT_INT, &depanalysis_event_limit, arg_limit, NULL);
  lttv_option_add("print-summary", 0, "print simple summary", "sum",
      LTTV_OPT_INT, &a_print_simple_summary, arg_sum, NULL);

  process_hash_table = g_hash_table_new(g_int_hash, g_int_equal);
  syscall_table = g_hash_table_new(g_int_hash, g_int_equal);
  irq_table = g_hash_table_new(g_int_hash, g_int_equal);
  softirq_table = g_hash_table_new(g_int_hash, g_int_equal);

  a_string = g_string_new("");

  result = lttv_iattribute_find_by_path(attributes, "hooks/event",
      LTTV_POINTER, &value);
  g_assert(result);
  event_hook = *(value.v_pointer);
  g_assert(event_hook);
  lttv_hooks_add(event_hook, process_event, NULL, LTTV_PRIO_DEFAULT);

//  result = lttv_iattribute_find_by_path(attributes, "hooks/trace/before",
//      LTTV_POINTER, &value);
//  g_assert(result);
//  before_trace = *(value.v_pointer);
//  g_assert(before_trace);
//  lttv_hooks_add(before_trace, write_trace_header, NULL, LTTV_PRIO_DEFAULT);
//
  result = lttv_iattribute_find_by_path(attributes, "hooks/traceset/before",
      LTTV_POINTER, &value);
  g_assert(result);
  before_traceset = *(value.v_pointer);
  g_assert(before_traceset);
  lttv_hooks_add(before_traceset, write_traceset_header, NULL,
      LTTV_PRIO_DEFAULT);

  result = lttv_iattribute_find_by_path(attributes, "hooks/traceset/after",
      LTTV_POINTER, &value);
  g_assert(result);
  after_traceset = *(value.v_pointer);
  g_assert(after_traceset);
  lttv_hooks_add(after_traceset, write_traceset_footer, NULL,
      LTTV_PRIO_DEFAULT);
}

static void destroy()
{
  lttv_option_remove("dep-time-start");
  lttv_option_remove("dep-time-end");
  lttv_option_remove("dep-pid");
  lttv_option_remove("limit-events");
  lttv_option_remove("print-summary");

  g_hash_table_destroy(process_hash_table);
  g_hash_table_destroy(syscall_table);
  g_hash_table_destroy(irq_table);
  g_hash_table_destroy(softirq_table);

  g_string_free(a_string, TRUE);

  lttv_hooks_remove_data(event_hook, write_event_content, NULL);
//  lttv_hooks_remove_data(before_trace, write_trace_header, NULL);
  lttv_hooks_remove_data(before_traceset, write_traceset_header, NULL);
  lttv_hooks_remove_data(after_traceset, write_traceset_footer, NULL);
}

LTTV_MODULE("depanalysis", "Dependency analysis test", \
	    "Produce a dependency analysis of a trace", \
	    init, destroy, "stats", "batchAnalysis", "option", "print")

