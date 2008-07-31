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
void do_mark1(const char *format, ...)
{
	va_list ap;
	int value;

	va_start(ap, format);
	value = va_arg(ap, int);
	printk("value is %d\n", value);
	
	va_end(ap);
}

void do_mark2(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vprintk(format, ap);
	va_end(ap);
	printk("\n");
}

#define DO_MARK3_FORMAT "%d %s %s"
void do_mark3(const char *format, ...)
{
	va_list ap;
	int value;
	const char *s1, *s2;
	
	va_start(ap, format);
	value = va_arg(ap, int);
	s1 = va_arg(ap, const char*);
	s2 = va_arg(ap, const char *);

	printk("value is %d %s %s\n",
		value, s1, s2);
	va_end(ap);
}

int init_module(void)
{
	int result;
	result = marker_set_probe("subsys_mark1", DO_MARK1_FORMAT,
			(marker_probe_func*)do_mark1);
	if(!result) goto end;
	result = marker_set_probe("subsys_mark2", NULL,
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
