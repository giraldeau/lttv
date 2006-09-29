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


int value;
void *ptr;

/* function to install */
void do_mark1(const char *format, ...)
{
	unsigned int i = 0;
	va_list ap;
	int control = 0;

	va_start(ap, format);
	while(1) {
		if(control) {
			switch(format[i]) {
				case '\0' :
					return;
				case 'd' :
					value = va_arg(ap, int);
				case 'p' :
					ptr = va_arg(ap, void*);
			}
			control = 0;
		} else {
			switch(format[i]) {
				case '%' :
					control = 1;
					break;
				case '\0' :
					return;
				default:
					control = 0;
			}
		}
		i++;
	}
	va_end(ap);
}

int init_module(void)
{
	int result;
	result = marker_set_probe("subsys_mark1", NULL,
			do_mark1);
	if(!result) goto end;

	return 0;

end:
	marker_remove_probe(do_mark1);
	return -EPERM;
}

void cleanup_module(void)
{
	marker_remove_probe(do_mark1);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Probe");
