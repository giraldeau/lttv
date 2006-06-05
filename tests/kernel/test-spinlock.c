/* test-cmpxchg.c
 *
 * Test time spent in a LTTng instrumentation probe.
 */


#include <linux/config.h>
#include <linux/jiffies.h>
#include <linux/compiler.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/spinlock.h>

#define NR_LOOPS 20000

volatile int test_val = 100;

spinlock_t lock = SPIN_LOCK_UNLOCKED;

static inline void do_test(void)
{
	unsigned long flags;
	
	spin_lock_irqsave(&lock, flags);
	test_val += 1;
	spin_unlock_irqrestore(&lock, flags);
}

//void (*fct)(void) = do_test;

static int ltt_test_init(void)
{
	unsigned int i;
	cycles_t time1, time2, time;
	cycles_t max_time = 0, min_time = 18446744073709551615ULL; /* (2^64)-1 */
	cycles_t tot_time = 0;
	unsigned long flags;
	printk(KERN_ALERT "test init\n");
	
	local_irq_save(flags);
	time1 = get_cycles();
	for(i=0; i<NR_LOOPS; i++) {
		//for(int j=0; j<10; j++) {
		do_test();
		//}
		//max_time = max(max_time, time);
		//min_time = min(min_time, time);
		//printk("val : %d\n", test_val);
	}
	time2 = get_cycles();
	local_irq_restore(flags);
	time = time2 - time1;
	tot_time += time;

	printk(KERN_ALERT "test results : time for spinlock irqsave\n");
	printk(KERN_ALERT "number of loops : %d\n", NR_LOOPS);
	printk(KERN_ALERT "total time : %llu\n", tot_time);
	//printk(KERN_ALERT "min : %llu\n", min_time);
	//printk(KERN_ALERT "max : %llu\n", max_time);

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

