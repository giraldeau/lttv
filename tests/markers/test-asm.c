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

#define NR_LOOPS 10000


#define COPYLEN 4096
char test[COPYLEN];
char src[COPYLEN] = "aaaaaaaaaaaa";

struct proc_dir_entry *pentry = NULL;

static int my_open(struct inode *inode, struct file *file)
{
	MARK(subsys_mark1, "%d %p", 1, NULL);

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

