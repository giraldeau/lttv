/* test-time-probe.c
 *
 * Test time spent in a LTTng instrumentation probe.
 */


#define CONFIG_LTT_FACILITY_TESTS
#include "ltt-facility-tests.h"

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ltt-core.h>


/* Event logged : 4 bytes + 20 bytes header = 24 bytes. Let's use 1MB of
 * buffers. 1MB / 24bytes = 43690. So, if we write 20000 event, we should not
 * lose events. Check event lost count after tests. */

#define NR_LOOPS 20000

static int ltt_test_init(void)
{
	unsigned int i;
	cycles_t time1, time2, time;
	cycles_t max_time = 0, min_time = 18446744073709551615ULL; /* (2^64)-1 */
	cycles_t tot_time = 0;
	unsigned long flags;
	printk(KERN_ALERT "test init\n");
	
	local_irq_save(flags);
	for(i=0; i<NR_LOOPS; i++) {
		time1 = get_cycles();
		trace_tests_write_4bytes(5000);
		time2 = get_cycles();
		time = time2 - time1;
		max_time = max(max_time, time);
		min_time = min(min_time, time);
		tot_time += time;
	}
	local_irq_restore(flags);

	printk(KERN_ALERT "test results : time per probe\n");
	printk(KERN_ALERT "number of loops : %d\n", NR_LOOPS);
	printk(KERN_ALERT "total time : %llu\n", tot_time);
	printk(KERN_ALERT "min : %llu\n", min_time);
	printk(KERN_ALERT "max : %llu\n", max_time);

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

