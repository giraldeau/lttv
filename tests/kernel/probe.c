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
#define DO_MARK1_FORMAT "%d"
asmlinkage void do_mark1(const char *format, int value)
{
	__mark_check_format(DO_MARK1_FORMAT, value);
	printk("value is %d\n", value);
}

#define DO_MARK2_FORMAT "%d %s"
asmlinkage void do_mark2(const char *format, int value, const char *string)
{
	__mark_check_format(DO_MARK2_FORMAT, value, string);
	printk("value is %d %s\n", value, string);
}

#define DO_MARK3_FORMAT "%d %s %s"
asmlinkage void do_mark3(const char *format, int value, const char *s1,
				const char *s2)
{
	__mark_check_format(DO_MARK3_FORMAT, value, s1, s2);
	printk("value is %d %s %s\n", value, s1, s2);
}

int init_module(void)
{
	int result;
	result = marker_set_probe("subsys_mark1", DO_MARK1_FORMAT,
			(marker_probe_func*)do_mark1);
	if(!result) goto end;
	result = marker_set_probe("subsys_mark2", DO_MARK2_FORMAT,
			(marker_probe_func*)do_mark2);
	if(!result) goto cleanup1;
	result = marker_set_probe("subsys_mark3", DO_MARK3_FORMAT,
			(marker_probe_func*)do_mark3);
	if(!result) goto cleanup2;

	return 0;

cleanup2:
	marker_remove_probe((marker_probe_func*)do_mark2);
cleanup1:
	marker_remove_probe((marker_probe_func*)do_mark1);
end:
	return -EPERM;
}

void cleanup_module(void)
{
	marker_remove_probe((marker_probe_func*)do_mark1);
	marker_remove_probe((marker_probe_func*)do_mark2);
	marker_remove_probe((marker_probe_func*)do_mark3);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Probe");

