/* test-time-probe.c
 *
 * Test printk effect on timer interrupt.
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/ltt-core.h>

#define NR_LOOPS 20000

static int ltt_test_init(void)
{
	unsigned int i;

	printk(KERN_ALERT "test init\n");
	
	for(i=0; i<NR_LOOPS; i++) {
		printk(KERN_ALERT "Flooding the console\n");
	}
	printk(KERN_ALERT "test end\n");
	
	return -EAGAIN; /* Fail will directly unload the module */
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

