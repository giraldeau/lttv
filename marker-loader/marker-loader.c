/* marker-loader.c
 *
 * Marker Loader
 *
 * Loads a function at a marker call site.
 *
 * (C) Copyright 2006 Mathieu Desnoyers <mathieu.desnoyers@polymtl.ca>
 *
 * This file is released under the GPLv2.
 * See the file COPYING for more details.
 */

#include <linux/marker.h>
#include <linux/module.h>
#include <linux/kallsyms.h>

/* function to install */
void do_mark1(const char *format, int value)
{
	printk("value is %d\n", value);
}


#define PROBE_NAME subsys_mark1

#define CALL __mark_subsys_mark1_call
#define JUMP_OVER __mark_subsys_mark1_jump_over
#define JUMP_CALL __mark_subsys_mark1_jump_call
#define JUMP_INLINE __mark_subsys_mark1_jump_inline

static void *saved_over;

static void **target_mark_call;
static void **target_mark_jump_over;
static void **target_mark_jump_call;
static void **target_mark_jump_inline;

void show_symbol_pointers(void)
{
	printk("Marker loader : Loading symbols...\n");
	printk("  %s %p %p\n", __stringify(CALL), target_mark_call,
		target_mark_call?*target_mark_call:0x0);
	printk("  %s %p %p\n", __stringify(JUMP_OVER), target_mark_jump_over,
		target_mark_jump_over?*target_mark_jump_over:0x0);
	printk("  %s %p %p\n", __stringify(JUMP_CALL), target_mark_jump_call,
		target_mark_jump_call?*target_mark_jump_call:0x0);
	printk("  %s %p %p\n", __stringify(JUMP_INLINE), target_mark_jump_inline,
		target_mark_jump_inline?*target_mark_jump_inline:0x0);
}

int mark_install_hook(void)
{
	target_mark_call = (void**)kallsyms_lookup_name(__stringify(CALL));
	target_mark_jump_over = (void**)kallsyms_lookup_name(__stringify(JUMP_OVER));
	target_mark_jump_call = (void**)kallsyms_lookup_name(__stringify(JUMP_CALL));
	target_mark_jump_inline = (void**)kallsyms_lookup_name(__stringify(JUMP_INLINE));

	show_symbol_pointers();
	
	if(!(target_mark_call && target_mark_jump_over && target_mark_jump_call && 
		target_mark_jump_inline)) {
		printk("Target symbols missing in kallsyms.\n");
		return -EPERM;
	}
	
	printk("Installing hook\n");
	*target_mark_call = (void*)do_mark1;
	saved_over = *target_mark_jump_over;
	*target_mark_jump_over = *target_mark_jump_call;

	return 0;
}

int init_module(void)
{
	return mark_install_hook();
}

void cleanup_module(void)
{
	printk("Removing hook\n");
	*target_mark_jump_over = saved_over;
	*target_mark_call = __mark_empty_function;

	/* Wait for instrumentation functions to return before quitting */
	synchronize_sched();
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Marker Loader");

