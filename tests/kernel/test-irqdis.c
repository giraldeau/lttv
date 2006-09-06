/* test-time-probe.c
 *
 * Test multiple kmallocs.
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

static int ltt_test_init(void)
{
	unsigned long flags;

	printk(KERN_ALERT "test init\n");
	
	local_irq_save(flags);
	msleep(1000);
	local_irq_restore(flags);
	return -1;
}

static void ltt_test_exit(void)
{
	printk(KERN_ALERT "test end\n");
}

module_init(ltt_test_init)
module_exit(ltt_test_exit)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Linux Trace Toolkit Test");

