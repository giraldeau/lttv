/* test-cmpxchg-nolock.c
 *
 * Compare local cmpxchg with irq disable / enable.
 */


#include <linux/jiffies.h>
#include <linux/compiler.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/calc64.h>
#include <linux/spinlock.h>
#include <linux/seqlock.h>
#include <asm/timex.h>
#include <asm/system.h>

#define NR_LOOPS 20000

int test_val;

static void do_testbaseline(void)
{
	long flags;
	unsigned int i;
	cycles_t time1, time2, time;
	long rem;

	local_irq_save(flags);
	preempt_disable();
	time1 = get_cycles();
	for (i = 0; i < NR_LOOPS; i++) {
		asm volatile ("");
	}
	time2 = get_cycles();
	local_irq_restore(flags);
	preempt_enable();
	time = time2 - time1;

	printk(KERN_ALERT "test results: time for baseline\n");
	printk(KERN_ALERT "number of loops: %d\n", NR_LOOPS);
	printk(KERN_ALERT "total time: %llu\n", time);
	time = div_long_long_rem(time, NR_LOOPS, &rem);
	printk(KERN_ALERT "-> baseline takes %llu cycles\n", time);
	printk(KERN_ALERT "test end\n");
}

static void do_test_spinlock(void)
{
	static DEFINE_SPINLOCK(mylock);
	long flags;
	unsigned int i;
	cycles_t time1, time2, time;
	long rem;

	preempt_disable();
	spin_lock_irqsave(&mylock, flags);
	time1 = get_cycles();
	for (i = 0; i < NR_LOOPS; i++) {
		spin_unlock(&mylock);
		spin_lock(&mylock);
	}
	time2 = get_cycles();
	spin_unlock_irqrestore(&mylock, flags);
	preempt_enable();
	time = time2 - time1;

	printk(KERN_ALERT "test results: time for spinlock\n");
	printk(KERN_ALERT "number of loops: %d\n", NR_LOOPS);
	printk(KERN_ALERT "total time: %llu\n", time);
	time = div_long_long_rem(time, NR_LOOPS, &rem);
	printk(KERN_ALERT "-> spinlock takes %llu cycles\n", time);
	printk(KERN_ALERT "test end\n");
}

static void do_test_seqlock(void)
{
	static seqlock_t test_lock;
	unsigned long seq;
	long flags;
	unsigned int i;
	cycles_t time1, time2, time;
	long rem;

	local_irq_save(flags);
	preempt_disable();
	time1 = get_cycles();
	for (i = 0; i < NR_LOOPS; i++) {
		do {
			seq = read_seqbegin(&test_lock);
		} while (read_seqretry(&test_lock, seq));
	}
	time2 = get_cycles();
	preempt_enable();
	time = time2 - time1;
	local_irq_restore(flags);

	printk(KERN_ALERT "test results: time for seqlock\n");
	printk(KERN_ALERT "number of loops: %d\n", NR_LOOPS);
	printk(KERN_ALERT "total time: %llu\n", time);
	time = div_long_long_rem(time, NR_LOOPS, &rem);
	printk(KERN_ALERT "-> seqlock takes %llu cycles\n", time);
	printk(KERN_ALERT "test end\n");
}

/*
 * This test will have a higher standard deviation due to incoming interrupts.
 */
static void do_test_preempt(void)
{
	long flags;
	unsigned int i;
	cycles_t time1, time2, time;
	long rem;

	local_irq_save(flags);
	preempt_disable();
	time1 = get_cycles();
	for (i = 0; i < NR_LOOPS; i++) {
		preempt_disable();
		preempt_enable();
	}
	time2 = get_cycles();
	preempt_enable();
	time = time2 - time1;
	local_irq_restore(flags);

	printk(KERN_ALERT "test results: time for preempt disable/enable pairs\n");
	printk(KERN_ALERT "number of loops: %d\n", NR_LOOPS);
	printk(KERN_ALERT "total time: %llu\n", time);
	time = div_long_long_rem(time, NR_LOOPS, &rem);
	printk(KERN_ALERT "-> preempt disable/enable pair takes %llu cycles\n",
					time);
	printk(KERN_ALERT "test end\n");
}

static int ltt_test_init(void)
{
	printk(KERN_ALERT "test init\n");
	
	do_testbaseline();
	do_test_spinlock();
	do_test_seqlock();
	do_test_preempt();
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

