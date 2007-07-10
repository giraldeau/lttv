/* test-time-probe.c
 *
 * Test time spent in a LTTng instrumentation probe.
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/ltt-tracer.h>
#include <linux/timex.h>
#include <linux/marker.h>
#include <linux/proc_fs.h>

struct proc_dir_entry *pentry_test = NULL;

/* Event logged : 4 bytes. Let's use 1MB of
 * buffers. 1MB / 4bytes = 262144 (plus heartbeats). So, if we write 20000
 * event, we should not lose events. Check event lost count after tests. */

#define NR_LOOPS 20000

static int my_open(struct inode *inode, struct file *file)
{
	unsigned int i;
	cycles_t time1, time2, time;
	cycles_t tot_time = 0;
	unsigned long flags;
	
	printk(KERN_ALERT "test begin\n");
	local_irq_save(flags);
	time1 = get_cycles();
	for(i=0; i<NR_LOOPS; i++) {
		_MARK(_MF_DEFAULT | ltt_flag_mask(LTT_FLAG_COMPACT),
			compact_event_a, MARK_NOARGS);
		_MARK(_MF_DEFAULT | ltt_flag_mask(LTT_FLAG_COMPACT),
			compact_event_b, "%4b", 0xFFFF);
		_MARK(_MF_DEFAULT | ltt_flag_mask(LTT_FLAG_COMPACT),
			compact_event_c, "%8b", 0xFFFFFFFFULL);
		_MARK(_MF_DEFAULT | ltt_flag_mask(LTT_FLAG_COMPACT)
			| ltt_flag_mask(LTT_FLAG_COMPACT_DATA),
			compact_event_d, MARK_NOARGS, 0xFFFF);
		_MARK(_MF_DEFAULT | ltt_flag_mask(LTT_FLAG_COMPACT)
			| ltt_flag_mask(LTT_FLAG_COMPACT_DATA),
			compact_event_e, "%8b", 0xFFFF, 0xFFFFFFFFULL);
	}
	time2 = get_cycles();
	time = time2 - time1;
	tot_time += time;
	local_irq_restore(flags);

	printk(KERN_ALERT "test results : time per probe\n");
	printk(KERN_ALERT "number of loops : %d\n", NR_LOOPS);
	printk(KERN_ALERT "total time : %llu\n", tot_time);

	printk(KERN_ALERT "test end\n");
	return -EPERM;
}

static struct file_operations mark_ops = {
	.open = my_open,
};

static int ltt_test_init(void)
{
	printk(KERN_ALERT "test init\n");
	pentry_test = create_proc_entry("test-compact", 0444, NULL);
	if (pentry_test)
		pentry_test->proc_fops = &mark_ops;
	else
		return -EPERM;
	return 0;
}

static void ltt_test_exit(void)
{
	printk(KERN_ALERT "test exit\n");
	remove_proc_entry("test-compact", NULL);
}

module_init(ltt_test_init)
module_exit(ltt_test_exit)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Linux Trace Toolkit Test");

