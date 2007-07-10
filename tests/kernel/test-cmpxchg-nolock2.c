/* test-cmpxchg-nolock.c
 *
 * Compare local cmpxchg with irq disable / enable.
 */


#include <linux/jiffies.h>
#include <linux/compiler.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/calc64.h>
#include <asm/timex.h>
#include <asm/system.h>

#define NR_LOOPS 20000

int test_val;

static void do_test_cmpxchg(void)
{
	int ret;
	long flags;
	unsigned int i;
	cycles_t time1, time2, time;
	long rem;

	local_irq_save(flags);
	preempt_disable();
	time1 = get_cycles();
	for (i = 0; i < NR_LOOPS; i++) {
		ret = cmpxchg_local(&test_val, 0, 0);
	}
	time2 = get_cycles();
	local_irq_restore(flags);
	preempt_enable();
	time = time2 - time1;

	printk(KERN_ALERT "test results: time for non locked cmpxchg\n");
	printk(KERN_ALERT "number of loops: %d\n", NR_LOOPS);
	printk(KERN_ALERT "total time: %llu\n", time);
	time = div_long_long_rem(time, NR_LOOPS, &rem);
	printk(KERN_ALERT "-> non locked cmpxchg takes %llu cycles\n", time);
	printk(KERN_ALERT "test end\n");
}

/*
 * This test will have a higher standard deviation due to incoming interrupts.
 */
static void do_test_enable_int(void)
{
	long flags;
	unsigned int i;
	cycles_t time1, time2, time;
	long rem;

	local_irq_save(flags);
	preempt_disable();
	time1 = get_cycles();
	for (i = 0; i < NR_LOOPS; i++) {
		local_irq_restore(flags);
	}
	time2 = get_cycles();
	local_irq_restore(flags);
	preempt_enable();
	time = time2 - time1;

	printk(KERN_ALERT "test results: time for enabling interrupts (STI)\n");
	printk(KERN_ALERT "number of loops: %d\n", NR_LOOPS);
	printk(KERN_ALERT "total time: %llu\n", time);
	time = div_long_long_rem(time, NR_LOOPS, &rem);
	printk(KERN_ALERT "-> enabling interrupts (STI) takes %llu cycles\n",
					time);
	printk(KERN_ALERT "test end\n");
}

static void do_test_disable_int(void)
{
	unsigned long flags, flags2;
	unsigned int i;
	cycles_t time1, time2, time;
	long rem;

	local_irq_save(flags);
	preempt_disable();
	time1 = get_cycles();
	for ( i = 0; i < NR_LOOPS; i++) {
		local_irq_save(flags2);
	}
	time2 = get_cycles();
	local_irq_restore(flags);
	preempt_enable();
	time = time2 - time1;

	printk(KERN_ALERT "test results: time for disabling interrupts (CLI)\n");
	printk(KERN_ALERT "number of loops: %d\n", NR_LOOPS);
	printk(KERN_ALERT "total time: %llu\n", time);
	time = div_long_long_rem(time, NR_LOOPS, &rem);
	printk(KERN_ALERT "-> disabling interrupts (CLI) takes %llu cycles\n",
				time);
	printk(KERN_ALERT "test end\n");
}



static int ltt_test_init(void)
{
	printk(KERN_ALERT "test init\n");
	
	do_test_cmpxchg();
	do_test_enable_int();
	do_test_disable_int();
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
MODULE_DESCRIPTION("Cmpxchg vs int Test");

