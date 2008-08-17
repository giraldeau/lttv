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
#include <linux/percpu.h>
#include <asm/ptrace.h>

#if (NR_CPUS > 64 && (BITS_PER_LONG == 32 || NR_CPUS > 32768))
#error "fair rwlock needs more bits per long to deal with that many CPUs"
#endif

/* Test duration, in seconds */
#define TEST_DURATION 600

#define THREAD_ROFFSET	1UL
#define THREAD_RMASK	((NR_CPUS - 1) * THREAD_ROFFSET)
#define SOFTIRQ_ROFFSET	(THREAD_RMASK + 1)
#define SOFTIRQ_RMASK	((NR_CPUS - 1) * SOFTIRQ_ROFFSET)
#define HARDIRQ_ROFFSET	((SOFTIRQ_RMASK | THREAD_RMASK) + 1)
#define HARDIRQ_RMASK	((NR_CPUS - 1) * HARDIRQ_ROFFSET)

#define SUBSCRIBERS_WOFFSET	\
	((HARDIRQ_RMASK | SOFTIRQ_RMASK | THREAD_RMASK) + 1)
#define SUBSCRIBERS_WMASK	\
	((NR_CPUS - 1) * SUBSCRIBERS_WOFFSET)
#define WRITER_MUTEX		\
	((SUBSCRIBERS_WMASK | HARDIRQ_RMASK | SOFTIRQ_RMASK | THREAD_RMASK) + 1)
#define SOFTIRQ_WMASK	(WRITER_MUTEX << 1)
#define SOFTIRQ_WOFFSET	SOFTIRQ_WMASK
#define HARDIRQ_WMASK	(SOFTIRQ_WMASK << 1)
#define HARDIRQ_WOFFSET	HARDIRQ_WMASK

#define NR_VARS 100
#define NR_WRITERS 3
#define NR_READERS 6
#define NR_INTERRUPT_READERS 2

/* Writer iteration delay, in ms. 0 for busy loop. */
#define WRITER_DELAY 1

static int var[NR_VARS];
static struct task_struct *reader_threads[NR_READERS];
static struct task_struct *writer_threads[NR_WRITERS];
static struct task_struct *interrupt_reader;

static struct fair_rwlock frwlock = {
	.value = ATOMIC_LONG_INIT(0),
};

struct proc_dir_entry *pentry = NULL;

static int reader_thread(void *data)
{
	int i;
	int prev, cur;
	unsigned long iter = 0;
	cycles_t time1, time2, delaymax = 0;

	printk("reader_thread/%lu runnning\n", (unsigned long)data);
	do {
		iter++;
		preempt_disable();	/* for get_cycles accuracy */
		time1 = get_cycles();
		fair_read_lock(&frwlock);
		time2 = get_cycles();
		delaymax = max(delaymax, time2 - time1);
		prev = var[0];
		for (i = 1; i < NR_VARS; i++) {
			cur = var[i];
			if (cur != prev)
				printk(KERN_ALERT
				"Unequal cur %d/prev %d at i %d, iter %lu "
				"in thread\n", cur, prev, i, iter);
		}
		fair_read_unlock(&frwlock);
		preempt_enable();	/* for get_cycles accuracy */
		//msleep(100);
	} while (!kthread_should_stop());
	printk("reader_thread/%lu iterations : %lu, "
		"max contention %llu cycles\n",
		(unsigned long)data, iter, delaymax);
	return 0;
}

DEFINE_PER_CPU(cycles_t, int_delaymax);

static void interrupt_reader_ipi(void *data)
{
	int i;
	int prev, cur;
	cycles_t time1, time2;
	cycles_t *delaymax;

	/*
	 * Skip the ipi caller, not in irq context.
	 */
	if (!in_irq())
		return;

	delaymax = &per_cpu(int_delaymax, smp_processor_id());
	time1 = get_cycles();
	fair_read_lock(&frwlock);
	time2 = get_cycles();
	*delaymax = max(*delaymax, time2 - time1);
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
	int i;

	do {
		iter++;
		on_each_cpu(interrupt_reader_ipi, NULL, 0);
		msleep(100);
	} while (!kthread_should_stop());
	printk("interrupt_reader_thread/%lu iterations : %lu\n",
			(unsigned long)data, iter);
	for_each_online_cpu(i) {
		printk("interrupt readers on CPU %i, "
			"max contention : %llu cycles\n",
			i, per_cpu(int_delaymax, i));
	}
	return 0;
}

static int writer_thread(void *data)
{
	int i;
	int new;
	unsigned long iter = 0;
	cycles_t time1, time2, delaymax = 0;

	printk("writer_thread/%lu runnning\n", (unsigned long)data);
	do {
		iter++;
		preempt_disable();	/* for get_cycles accuracy */
		time1 = get_cycles();
		fair_write_lock_irq(&frwlock);
		//fair_write_lock(&frwlock);
		time2 = get_cycles();
		delaymax = max(delaymax, time2 - time1);
		new = (int)get_cycles();
		for (i = 0; i < NR_VARS; i++) {
			var[i] = new;
		}
		//fair_write_unlock(&frwlock);
		fair_write_unlock_irq(&frwlock);
		preempt_enable();	/* for get_cycles accuracy */
		if (WRITER_DELAY > 0)
			msleep(WRITER_DELAY);
	} while (!kthread_should_stop());
	printk("writer_thread/%lu iterations : %lu, "
		"max contention %llu cycles\n",
		(unsigned long)data, iter, delaymax);
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

	for (i = 0; i < NR_WRITERS; i++) {
		kthread_stop(writer_threads[i]);
	}

	for (i = 0; i < NR_READERS; i++) {
		kthread_stop(reader_threads[i]);
	}

	kthread_stop(interrupt_reader);
}


static void perform_test(const char *name, void (*callback)(void))
{
	printk("%s\n", name);
	callback();
}

static int my_open(struct inode *inode, struct file *file)
{
	perform_test("fair-rwlock-create", fair_rwlock_create);
	ssleep(600);
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
	printk("SOFTIRQ_ROFFSET : %lX\n", SOFTIRQ_ROFFSET);
	printk("SOFTIRQ_RMASK : %lX\n", SOFTIRQ_RMASK);
	printk("HARDIRQ_ROFFSET : %lX\n", HARDIRQ_ROFFSET);
	printk("HARDIRQ_RMASK : %lX\n", HARDIRQ_RMASK);
	printk("SUBSCRIBERS_WOFFSET : %lX\n", SUBSCRIBERS_WOFFSET);
	printk("SUBSCRIBERS_WMASK : %lX\n", SUBSCRIBERS_WMASK);
	printk("WRITER_MUTEX : %lX\n", WRITER_MUTEX);
	printk("SOFTIRQ_WMASK : %lX\n", SOFTIRQ_WMASK);
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
