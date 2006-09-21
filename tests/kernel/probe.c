/* probe.c
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
asmlinkage void do_mark1(const char *format, int value)
{
	printk("value is %d\n", value);
}

int init_module(void)
{
	return marker_set_probe("subsys_mark1", (marker_probe_func*)do_mark1,
			MARKER_CALL);
}

void cleanup_module(void)
{
	marker_disable_probe("subsys_mark1", (marker_probe_func*)do_mark1,
		MARKER_CALL);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Probe");

