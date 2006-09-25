/* test-mark.c
 *
 */

#include <linux/marker.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <asm/ptrace.h>

volatile int x=7;

struct proc_dir_entry *pentry = NULL;

static inline void test(struct pt_regs * regs)
{
	MARK(kernel_debug_test, "%d %ld %p", 2, regs->eip, regs);
}

static int my_open(struct inode *inode, struct file *file)
{
	unsigned int i;

	for(i=0; i<2; i++) {
		MARK(subsys_mark1, "%d", 1);
		x=i;
		barrier();
	}
	MARK(subsys_mark2, "%d %s", 2, "blah2");
	MARK(subsys_mark3, "%d %s %s", x, "blah3", "blah5");
	test(NULL);
	test(NULL);

	return -EPERM;
}


static struct file_operations my_operations = {
	.open = my_open,
};

int init_module(void)
{
	unsigned int i;

	pentry = create_proc_entry("testmark", 0444, NULL);
	if (pentry)
		pentry->proc_fops = &my_operations;

	marker_list_probe(NULL);

	return 0;
}

void cleanup_module(void)
{
	remove_proc_entry("testmark", NULL);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Marker Test");

