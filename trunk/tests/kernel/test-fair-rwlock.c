/* test-fair-rwlock.c
 *
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/timex.h>
#include <linux/fair-rwlock.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/hardirq.h>
#include <linux/module.h>
#include <asm/ptrace.h>

#if (NR_CPUS > 512 && (BITS_PER_LONG == 32 || NR_CPUS > 1048576))
#error "fair rwlock needs more bits per long to deal with that many CPUs"
#endif

#define THREAD_ROFFSET	1UL
#define THREAD_RMASK	((NR_CPUS - 1) * THREAD_ROFFSET)
#define SOFTIRQ_ROFFSET	(THREAD_RMASK + 1)
#define SOFTIRQ_RMASK	((NR_CPUS - 1) * SOFTIRQ_ROFFSET)
#define HARDIRQ_ROFFSET	((SOFTIRQ_RMASK | THREAD_RMASK) + 1)
#define HARDIRQ_RMASK	((NR_CPUS - 1) * HARDIRQ_ROFFSET)

#define THREAD_WMASK	((HARDIRQ_RMASK | SOFTIRQ_RMASK | THREAD_RMASK) + 1)
#define SOFTIRQ_WMASK	(THREAD_WMASK << 1)
#define HARDIRQ_WMASK	(SOFTIRQ_WMASK << 1)

#define NR_VARS 100
#define NR_WRITERS 3
#define NR_READERS 6
#define NR_INTERRUPT_READERS 2

static int var[NR_VARS];
static struct task_struct *reader_threads[NR_READERS];
static struct task_struct *writer_threads[NR_WRITERS];
static struct task_struct *interrupt_reader;

static struct fair_rwlock frwlock = {
	.value = ATOMIC_LONG_INIT(0),
	.wlock = __SPIN_LOCK_UNLOCKED(&frwlock.wlock),
};

struct proc_dir_entry *pentry = NULL;

static int reader_thread(void *data)
{
	int i;
	int prev, cur;
	unsigned long iter = 0;

	printk("reader_thread/%lu runnning\n", (unsigned long)data);
	do {
		iter++;
		fair_read_lock(&frwlock);
		prev = var[0];
		for (i = 1; i < NR_VARS; i++) {
			cur = var[i];
			if (cur != prev)
				printk(KERN_ALERT
				"Unequal cur %d/prev %d at i %d, iter %lu "
				"in thread\n", cur, prev, i, iter);
		}
		fair_read_unlock(&frwlock);
		//msleep(100);
	} while (!kthread_should_stop());
	printk("reader_thread/%lu iterations : %lu\n",
			(unsigned long)data, iter);
	return 0;
}

static void interrupt_reader_ipi(void *data)
{
	int i;
	int prev, cur;

	fair_read_lock(&frwlock);
	prev = var[0];
	for (i = 1; i < NR_VARS; i++) {
		cur = var[i];
		if (cur != prev)
			printk(KERN_ALERT
			"Unequal cur %d/prev %d at i %d in interrupt\n",
				cur, prev, i);
	}
	fair_read_unlock(&frwlock);
}

static int interrupt_reader_thread(void *data)
{
	unsigned long iter = 0;
	do {
		iter++;
		on_each_cpu(interrupt_reader_ipi, NULL, 0);
		msleep(100);
	} while (!kthread_should_stop());
	printk("interrupt_reader_thread/%lu iterations : %lu\n",
			(unsigned long)data, iter);
	return 0;
}

static int writer_thread(void *data)
{
	int i;
	int new;
	unsigned long iter = 0;

	printk("writer_thread/%lu runnning\n", (unsigned long)data);
	do {
		iter++;
		fair_write_lock_irq(&frwlock);
		//fair_write_lock(&frwlock);
		new = (int)get_cycles();
		for (i = 0; i < NR_VARS; i++) {
			var[i] = new;
		}
		//fair_write_unlock(&frwlock);
		fair_write_unlock_irq(&frwlock);
		//msleep(100);
	} while (!kthread_should_stop());
	printk("writer_thread/%lu iterations : %lu\n",
			(unsigned long)data, iter);
	return 0;
}

static void fair_rwlock_create(void)
{
	unsigned long i;

	for (i = 0; i < NR_READERS; i++) {
		printk("starting reader thread %lu\n", i);
		reader_threads[i] = kthread_run(reader_thread, (void *)i,
			"frwlock_reader");
		BUG_ON(!reader_threads[i]);
	}

	printk("starting interrupt reader %lu\n", i);
	interrupt_reader = kthread_run(interrupt_reader_thread, NULL,
		"frwlock_interrupt_reader");

	for (i = 0; i < NR_WRITERS; i++) {
		printk("starting writer thread %lu\n", i);
		writer_threads[i] = kthread_run(writer_thread, (void *)i,
			"frwlock_writer");
		BUG_ON(!writer_threads[i]);
	}
}

static void fair_rwlock_stop(void)
{
	unsigned long i;

	for (i = 0; i < NR_READERS; i++) {
		kthread_stop(reader_threads[i]);
	}

	//kthread_stop(interrupt_reader);

	for (i = 0; i < NR_WRITERS; i++) {
		kthread_stop(writer_threads[i]);
	}
}


static void perform_test(const char *name, void (*callback)(void))
{
	printk("%s\n", name);
	callback();
}

static int my_open(struct inode *inode, struct file *file)
{
	perform_test("fair-rwlock-create", fair_rwlock_create);
	ssleep(30);
	perform_test("fair-rwlock-stop", fair_rwlock_stop);

	return -EPERM;
}


static struct file_operations my_operations = {
	.open = my_open,
};

int init_module(void)
{
	pentry = create_proc_entry("testfrwlock", 0444, NULL);
	if (pentry)
		pentry->proc_fops = &my_operations;

	printk("NR_CPUS : %d\n", NR_CPUS);
	printk("THREAD_ROFFSET : %lX\n", THREAD_ROFFSET);
	printk("THREAD_RMASK : %lX\n", THREAD_RMASK);
	printk("THREAD_WMASK : %lX\n", THREAD_WMASK);
	printk("SOFTIRQ_ROFFSET : %lX\n", SOFTIRQ_ROFFSET);
	printk("SOFTIRQ_RMASK : %lX\n", SOFTIRQ_RMASK);
	printk("SOFTIRQ_WMASK : %lX\n", SOFTIRQ_WMASK);
	printk("HARDIRQ_ROFFSET : %lX\n", HARDIRQ_ROFFSET);
	printk("HARDIRQ_RMASK : %lX\n", HARDIRQ_RMASK);
	printk("HARDIRQ_WMASK : %lX\n", HARDIRQ_WMASK);

	return 0;
}

void cleanup_module(void)
{
	remove_proc_entry("testfrwlock", NULL);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Fair rwlock test");
