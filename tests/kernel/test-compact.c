/* test-time-probe.c
 *
 * Test time spent in a LTTng instrumentation probe.
 */


#include <ltt/ltt-facility-select-compact.h>
#include <ltt/ltt-facility-compact.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ltt-core.h>
#include <linux/timex.h>


/* Event logged : 4 bytes. Let's use 1MB of
 * buffers. 1MB / 4bytes = 262144 (plus heartbeats). So, if we write 20000
 * event, we should not lose events. Check event lost count after tests. */

#define NR_LOOPS 20000

static int ltt_test_init(void)
{
	unsigned int i;
	cycles_t time1, time2, time;
	cycles_t tot_time = 0;
	unsigned long flags;
	printk(KERN_ALERT "test init\n");
	
	local_irq_save(flags);
	time1 = get_cycles();
	for(i=0; i<NR_LOOPS; i++) {
		trace_compact_event_a();
		trace_compact_event_c(get_cycles());
		trace_compact_event_d(0xFF);
		trace_compact_event_e(0xFF, 0xFAFAFA);
	}
	time2 = get_cycles();
	time = time2 - time1;
	tot_time += time;
	local_irq_restore(flags);

	printk(KERN_ALERT "test results : time per probe\n");
	printk(KERN_ALERT "number of loops : %d\n", NR_LOOPS);
	printk(KERN_ALERT "total time : %llu\n", tot_time);

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

