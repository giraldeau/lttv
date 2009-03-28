/* test-lttng-tp.c
 *
 * Test lttng event write.
 */

#include <linux/jiffies.h>
#include <linux/compiler.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/math64.h>
#include <linux/proc_fs.h>
#include "tp-test.h"
#include <asm/timex.h>
#include <asm/system.h>

#define NR_LOOPS 20000

DEFINE_TRACE(kernel_test);

struct proc_dir_entry *pentry = NULL;

int test_val;

static void do_testbaseline(void)
{
	unsigned long flags;
	unsigned int i;
	cycles_t time1, time2, time;
	u32 rem;

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
	time = div_u64_rem(time, NR_LOOPS, &rem);
	printk(KERN_ALERT "-> baseline takes %llu cycles\n", time);
	printk(KERN_ALERT "test end\n");
}

static void do_test_tp(void)
{
	unsigned long flags;
	unsigned int i;
	cycles_t time1, time2, time;
	u32 rem;

	local_irq_save(flags);
	preempt_disable();
	time1 = get_cycles();
	for (i = 0; i < NR_LOOPS; i++) {
		trace_kernel_test((void *)999, (void *)10);
	}
	time2 = get_cycles();
	local_irq_restore(flags);
	preempt_enable();
	time = time2 - time1;

	printk(KERN_ALERT "test results: time for one probe\n");
	printk(KERN_ALERT "number of loops: %d\n", NR_LOOPS);
	printk(KERN_ALERT "total time: %llu\n", time);
	time = div_u64_rem(time, NR_LOOPS, &rem);
	printk(KERN_ALERT "-> one probe takes %llu cycles\n", time);
	printk(KERN_ALERT "test end\n");
}

static int my_open(struct inode *inode, struct file *file)
{
	do_testbaseline();
	do_test_tp();

	return -EPERM;
}

static const struct file_operations my_operations = {
	.open = my_open,
};

static int ltt_test_init(void)
{
	printk(KERN_ALERT "test init\n");
	pentry = create_proc_entry("testltt", 0444, NULL);
	if (pentry)
		pentry->proc_fops = &my_operations;
	return 0;
}

static void ltt_test_exit(void)
{
	printk(KERN_ALERT "test exit\n");
	remove_proc_entry("testltt", NULL);
}

module_init(ltt_test_init)
module_exit(ltt_test_exit)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("TP test");
