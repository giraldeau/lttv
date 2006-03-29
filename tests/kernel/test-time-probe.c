/* test-time-probe.c
 *
 * Test time spent in a LTTng instrumentation probe.
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/ltt-core.h>

static int ltt_test_init(void)
{
	printk(KERN_ALERT "test init\n");

	return 0;
}

static void ltt_test_exit(void)
{
	printk(KERN_ALERT "test exit\n");
}

module_init(ltt_test_init)
module_exit(ltt_test_exit)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Linux Trace Toolkit Test");

