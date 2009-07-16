/* test-mark.c
 *
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/timex.h>
#include <linux/marker.h>
#include <asm/ptrace.h>

	//asm volatile ("");
struct proc_dir_entry *pentry = NULL;

static inline void test(unsigned long arg)
{
	//asm volatile ("");
	trace_mark(test_marker, "arg1 %lu arg2 %p", arg, current);
}

static int my_open(struct inode *inode, struct file *file)
{
	unsigned int i;
	cycles_t cycles1, cycles2;
	unsigned long flags;

	local_irq_save(flags);
	rdtsc_barrier();
	cycles1 = get_cycles();
	rdtsc_barrier();
	for(i=0; i<20000; i++) {
		test(i);
	}
	rdtsc_barrier();
	cycles2 = get_cycles();
	rdtsc_barrier();
	local_irq_restore(flags);
	printk("cycles : %llu\n", cycles2-cycles1);
	return -EPERM;
}


static struct file_operations my_operations = {
	.open = my_open,
};

int init_module(void)
{
	pentry = create_proc_entry("testmark", 0444, NULL);
	if (pentry)
		pentry->proc_fops = &my_operations;

	return 0;
}

void cleanup_module(void)
{
	remove_proc_entry("testmark", NULL);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Marker Test");

