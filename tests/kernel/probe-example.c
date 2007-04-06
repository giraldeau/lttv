/* probe-example.c
 *
 * Connects a two functions to marker call sites.
 *
 * (C) Copyright 2007 Mathieu Desnoyers <mathieu.desnoyers@polymtl.ca>
 *
 * This file is released under the GPLv2.
 * See the file COPYING for more details.
 */

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/marker.h>
#include <asm/atomic.h>

#define NUM_PROBES (sizeof(probe_array) / sizeof(struct probe_data))

struct probe_data {
	const char *name;
	const char *format;
	marker_probe_func *probe_func;
};

void probe_subsystem_event(const struct __mark_marker_c *mdata,
		const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned int value;
	const char *mystr;
	int task_size, task_alignment;
	struct task_struct *task;

	/* Assign args */
	va_start(ap, format);
	value = va_arg(ap, typeof(value));
	mystr = va_arg(ap, typeof(mystr));
	task_size = va_arg(ap, typeof(task_size));
	task_alignment = va_arg(ap, typeof(task_alignment));
	task = va_arg(ap, typeof(task));

	/* Call printk */
	printk("Value %u, string %s, current ptr %p\n", value, mystr, current);

	/* or count, check rights, serialize data in a buffer */

	va_end(ap);
}

atomic_t eventb_count = ATOMIC_INIT(0);

void probe_subsystem_eventb(const struct __mark_marker_c *mdata,
	const char *format, ...)
{
	/* Increment counter */
	atomic_inc(&eventb_count);
}

static struct probe_data probe_array[] =
{
	{	.name = "subsystem_event",
		.format = "%d %s %*.*r",
		.probe_func = probe_subsystem_event },
	{	.name = "subsystem_eventb",
		.format = MARK_NOARGS,
		.probe_func = probe_subsystem_eventb },
};

static int __init probe_init(void)
{
	int result;
	uint8_t eID;

	for (eID = 0; eID < NUM_PROBES; eID++) {
		result = marker_set_probe(probe_array[eID].name,
				probe_array[eID].format,
				probe_array[eID].probe_func, &probe_array[eID]);
		if (!result)
			printk(KERN_INFO "Unable to register probe %s\n",
				probe_array[eID].name);
	}
	return 0;
}

static void __exit probe_fini(void)
{
	uint8_t eID;

	for (eID = 0; eID < NUM_PROBES; eID++) {
		marker_remove_probe(probe_array[eID].name);
	}
	synchronize_sched();	/* Wait for probes to finish */
	printk("Number of event b : %u\n", atomic_read(&eventb_count));
}

module_init(probe_init);
module_exit(probe_fini);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("SUBSYSTEM Probe");
