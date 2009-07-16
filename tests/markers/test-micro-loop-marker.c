/* test-micro-loop-marker.c
 *
 * Execute a marker in a loop
 */

#include <linux/marker.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <asm/ptrace.h>
#include <linux/timex.h>
#include <linux/string.h>

#define NR_LOOPS 100000


#define COPYLEN 4096
char test[COPYLEN];
char src[COPYLEN] = "aaaaaaaaaaaa";

struct proc_dir_entry *pentry = NULL;

static int my_open(struct inode *inode, struct file *file)
{
	unsigned int i;
	unsigned long flags;
	cycles_t time1, time2;

	local_irq_save(flags);
	time1 = get_cycles();
	for(i=0; i<NR_LOOPS; i++) {
		MARK(subsys_mark1, "%d %p", 1, NULL);
		//memcpy(test, src, COPYLEN);
	}
	time2 = get_cycles();
	local_irq_restore(flags);
	printk("time delta (cycles): %llu\n", time2-time1);

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

