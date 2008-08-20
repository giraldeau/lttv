/* test-wbias-rwlock.c
 *
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/timex.h>
#include <linux/wbias-rwlock.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/hardirq.h>
#include <linux/module.h>
#include <linux/percpu.h>
#include <linux/spinlock.h>
#include <asm/ptrace.h>

/* Test with no contention duration, in seconds */
#define SINGLE_WRITER_TEST_DURATION 10
#define SINGLE_READER_TEST_DURATION 10
#define MULTIPLE_READERS_TEST_DURATION 10

/* Test duration, in seconds */
#define TEST_DURATION 60

#define NR_VARS 100
#define NR_WRITERS 2
#define NR_TRYLOCK_WRITERS 1
#define NR_PREADERS 2
#define NR_NPREADERS 2
#define NR_TRYLOCK_READERS 1

/*
 * 1 : test standard rwlock
 * 0 : test wbiasrwlock
 */
#define TEST_STD_RWLOCK 0

/*
 * 1 : test with thread and interrupt readers.
 * 0 : test only with thread readers.
 */
#define TEST_INTERRUPTS 1

#if (TEST_INTERRUPTS)
#define NR_INTERRUPT_READERS 1
#define NR_TRYLOCK_INTERRUPT_READERS 1
#else
#define NR_INTERRUPT_READERS 0
#define NR_TRYLOCK_INTERRUPT_READERS 0
#endif

/*
 * Writer iteration delay, in us. 0 for busy loop. Caution : writers can
 * starve readers.
 */
#define WRITER_DELAY 100
#define TRYLOCK_WRITER_DELAY 1000

/*
 * Number of iterations after which a trylock writer fails.
 * -1 for infinite loop.
 */
#define TRYLOCK_WRITERS_FAIL_ITER 100

/* Thread and interrupt reader delay, in ms */
#define THREAD_READER_DELAY 0	/* busy loop */
#define INTERRUPT_READER_DELAY 100

static int var[NR_VARS];
static struct task_struct *preader_threads[NR_PREADERS];
static struct task_struct *npreader_threads[NR_NPREADERS];
static struct task_struct *trylock_reader_threads[NR_TRYLOCK_READERS];
static struct task_struct *writer_threads[NR_WRITERS];
static struct task_struct *trylock_writer_threads[NR_TRYLOCK_WRITERS];
static struct task_struct *interrupt_reader[NR_INTERRUPT_READERS];
static struct task_struct *trylock_interrupt_reader[NR_TRYLOCK_INTERRUPT_READERS];

#if (TEST_STD_RWLOCK)

static DEFINE_RWLOCK(std_rw_lock);

#define wrap_read_lock()	read_lock(&std_rw_lock)
#define wrap_read_trylock()	read_trylock(&std_rw_lock)
#define wrap_read_unlock()	read_unlock(&std_rw_lock)

#define wrap_read_lock_inatomic()	read_lock(&std_rw_lock)
#define wrap_read_trylock_inatomic()	read_trylock(&std_rw_lock)
#define wrap_read_unlock_inatomic()	read_unlock(&std_rw_lock)

#define wrap_read_lock_irq()	read_lock(&std_rw_lock)
#define wrap_read_trylock_irq()	read_trylock(&std_rw_lock)
#define wrap_read_unlock_irq()	read_unlock(&std_rw_lock)

#if (TEST_INTERRUPTS)
#define wrap_write_lock()	write_lock_irq(&std_rw_lock)
#define wrap_write_unlock()	write_unlock_irq(&std_rw_lock)
#else
#define wrap_write_lock()	write_lock(&std_rw_lock)
#define wrap_write_unlock()	write_unlock(&std_rw_lock)
#endif

#else

static DEFINE_WBIAS_RWLOCK(wbiasrwlock);

#define wrap_read_lock()	wbias_read_lock(&wbiasrwlock)
#define wrap_read_trylock()	wbias_read_trylock(&wbiasrwlock)
#define wrap_read_unlock()	wbias_read_unlock(&wbiasrwlock)

#define wrap_read_lock_inatomic()	wbias_read_lock_inatomic(&wbiasrwlock)
#define wrap_read_trylock_inatomic()	\
		wbias_read_trylock_inatomic(&wbiasrwlock)
#define wrap_read_unlock_inatomic()	\
		wbias_read_unlock_inatomic(&wbiasrwlock)

#define wrap_read_lock_irq()	wbias_read_lock_irq(&wbiasrwlock)
#define wrap_read_trylock_irq()	wbias_read_trylock_irq(&wbiasrwlock)
#define wrap_read_unlock_irq()	wbias_read_unlock_irq(&wbiasrwlock)

#if (TEST_INTERRUPTS)
#define wrap_write_lock()	wbias_write_lock_irq(&wbiasrwlock)
#define wrap_write_unlock()	wbias_write_unlock_irq(&wbiasrwlock)
#else
#define wrap_write_lock()	wbias_write_lock(&wbiasrwlock)
#define wrap_write_unlock()	wbias_write_unlock(&wbiasrwlock)
#endif

#endif

static cycles_t cycles_calibration_min,
	cycles_calibration_avg,
	cycles_calibration_max;

static inline cycles_t calibrate_cycles(cycles_t cycles)
{
	return cycles - cycles_calibration_avg;
}

struct proc_dir_entry *pentry = NULL;

static int p_or_np_reader_thread(void *data, int preemptable)
{
	int i;
	int prev, cur;
	unsigned long iter = 0;
	cycles_t time1, time2, delay, delaymax = 0, delaymin = ULLONG_MAX,
		delayavg = 0;

	printk("reader_thread/%lu runnning\n", (unsigned long)data);
	do {
		iter++;
		if (!preemptable)
			preempt_disable();
		rdtsc_barrier();
		time1 = get_cycles();
		rdtsc_barrier();

		if (!preemptable)
			wrap_read_lock_inatomic();
		else
			wrap_read_lock();

		rdtsc_barrier();
		time2 = get_cycles();
		rdtsc_barrier();
		delay = time2 - time1;
		delaymax = max(delaymax, delay);
		delaymin = min(delaymin, delay);
		delayavg += delay;
		prev = var[0];
		for (i = 1; i < NR_VARS; i++) {
			cur = var[i];
			if (cur != prev)
				printk(KERN_ALERT
				"Unequal cur %d/prev %d at i %d, iter %lu "
				"in thread\n", cur, prev, i, iter);
		}

		if (!preemptable)
			wrap_read_unlock_inatomic();
		else
			wrap_read_unlock();
		if (!preemptable)
			preempt_enable();
		if (THREAD_READER_DELAY)
			msleep(THREAD_READER_DELAY);
	} while (!kthread_should_stop());
	if (!iter) {
		printk("reader_thread/%lu iterations : %lu",
			(unsigned long)data, iter);
	} else {
		delayavg /= iter;
		printk("reader_thread/%lu iterations : %lu, "
			"lock delay [min,avg,max] %llu,%llu,%llu cycles\n",
			(unsigned long)data, iter,
			calibrate_cycles(delaymin),
			calibrate_cycles(delayavg),
			calibrate_cycles(delaymax));
	}
	return 0;
}

static int preader_thread(void *data)
{
	return p_or_np_reader_thread(data, 1);
}

static int npreader_thread(void *data)
{
	return p_or_np_reader_thread(data, 0);
}

static int trylock_reader_thread(void *data)
{
	int i;
	int prev, cur;
	unsigned long iter = 0, success_iter = 0;

	printk("trylock_reader_thread/%lu runnning\n", (unsigned long)data);
	do {
		while (!wrap_read_trylock())
			iter++;
		success_iter++;
		prev = var[0];
		for (i = 1; i < NR_VARS; i++) {
			cur = var[i];
			if (cur != prev)
				printk(KERN_ALERT
				"Unequal cur %d/prev %d at i %d, iter %lu "
				"in thread\n", cur, prev, i, iter);
		}
		wrap_read_unlock();
		if (THREAD_READER_DELAY)
			msleep(THREAD_READER_DELAY);
	} while (!kthread_should_stop());
	printk("trylock_reader_thread/%lu iterations : %lu, "
		"successful iterations : %lu\n",
		(unsigned long)data, iter, success_iter);
	return 0;
}

DEFINE_PER_CPU(cycles_t, int_delaymin);
DEFINE_PER_CPU(cycles_t, int_delayavg);
DEFINE_PER_CPU(cycles_t, int_delaymax);
DEFINE_PER_CPU(cycles_t, int_ipi_nr);

static void interrupt_reader_ipi(void *data)
{
	int i;
	int prev, cur;
	cycles_t time1, time2;
	cycles_t *delaymax, *delaymin, *delayavg, *ipi_nr, delay;

	/*
	 * Skip the ipi caller, not in irq context.
	 */
	if (!in_irq())
		return;

	delaymax = &per_cpu(int_delaymax, smp_processor_id());
	delaymin = &per_cpu(int_delaymin, smp_processor_id());
	delayavg = &per_cpu(int_delayavg, smp_processor_id());
	ipi_nr = &per_cpu(int_ipi_nr, smp_processor_id());

	rdtsc_barrier();
	time1 = get_cycles();
	rdtsc_barrier();

	wrap_read_lock_irq();

	rdtsc_barrier();
	time2 = get_cycles();
	rdtsc_barrier();
	delay = time2 - time1;
	*delaymax = max(*delaymax, delay);
	*delaymin = min(*delaymin, delay);
	*delayavg += delay;
	(*ipi_nr)++;
	prev = var[0];
	for (i = 1; i < NR_VARS; i++) {
		cur = var[i];
		if (cur != prev)
			printk(KERN_ALERT
			"Unequal cur %d/prev %d at i %d in interrupt\n",
				cur, prev, i);
	}
	wrap_read_unlock_irq();
}

DEFINE_PER_CPU(unsigned long, trylock_int_iter);
DEFINE_PER_CPU(unsigned long, trylock_int_success);

static void trylock_interrupt_reader_ipi(void *data)
{
	int i;
	int prev, cur;

	/*
	 * Skip the ipi caller, not in irq context.
	 */
	if (!in_irq())
		return;

	per_cpu(trylock_int_iter, smp_processor_id())++;
	while (!wrap_read_trylock_irq())
		per_cpu(trylock_int_iter, smp_processor_id())++;
	per_cpu(trylock_int_success, smp_processor_id())++;
	prev = var[0];
	for (i = 1; i < NR_VARS; i++) {
		cur = var[i];
		if (cur != prev)
			printk(KERN_ALERT
			"Unequal cur %d/prev %d at i %d in interrupt\n",
				cur, prev, i);
	}
	wrap_read_unlock_irq();
}


static int interrupt_reader_thread(void *data)
{
	unsigned long iter = 0;
	int i;

	for_each_online_cpu(i) {
		per_cpu(int_delaymax, i) = 0;
		per_cpu(int_delaymin, i) = ULLONG_MAX;
		per_cpu(int_delayavg, i) = 0;
		per_cpu(int_ipi_nr, i) = 0;
	}
	do {
		iter++;
		on_each_cpu(interrupt_reader_ipi, NULL, 0);
		if (INTERRUPT_READER_DELAY)
			msleep(INTERRUPT_READER_DELAY);
	} while (!kthread_should_stop());
	printk("interrupt_reader_thread/%lu iterations : %lu\n",
			(unsigned long)data, iter);
	for_each_online_cpu(i) {
		if (!per_cpu(int_ipi_nr, i))
			continue;
		per_cpu(int_delayavg, i) /= per_cpu(int_ipi_nr, i);
		printk("interrupt readers on CPU %i, "
			"lock delay [min,avg,max] %llu,%llu,%llu cycles\n",
			i,
			calibrate_cycles(per_cpu(int_delaymin, i)),
			calibrate_cycles(per_cpu(int_delayavg, i)),
			calibrate_cycles(per_cpu(int_delaymax, i)));
	}
	return 0;
}

static int trylock_interrupt_reader_thread(void *data)
{
	unsigned long iter = 0;
	int i;

	do {
		iter++;
		on_each_cpu(trylock_interrupt_reader_ipi, NULL, 0);
		if (INTERRUPT_READER_DELAY)
			msleep(INTERRUPT_READER_DELAY);
	} while (!kthread_should_stop());
	printk("trylock_interrupt_reader_thread/%lu iterations : %lu\n",
			(unsigned long)data, iter);
	for_each_online_cpu(i) {
		printk("trylock interrupt readers on CPU %i, "
			"iterations %lu, "
			"successful iterations : %lu\n",
			i, per_cpu(trylock_int_iter, i),
			per_cpu(trylock_int_success, i));
		per_cpu(trylock_int_iter, i) = 0;
		per_cpu(trylock_int_success, i) = 0;
	}
	return 0;
}

static int writer_thread(void *data)
{
	int i;
	int new;
	unsigned long iter = 0;
	cycles_t time1, time2, delay, delaymax = 0, delaymin = ULLONG_MAX,
		delayavg = 0;

	printk("writer_thread/%lu runnning\n", (unsigned long)data);
	do {
		iter++;
		//preempt_disable();	/* for get_cycles accuracy */
		rdtsc_barrier();
		time1 = get_cycles();
		rdtsc_barrier();

		wrap_write_lock();

		rdtsc_barrier();
		time2 = get_cycles();
		rdtsc_barrier();
		delay = time2 - time1;
		delaymax = max(delaymax, delay);
		delaymin = min(delaymin, delay);
		delayavg += delay;
		new = (int)get_cycles();
		for (i = 0; i < NR_VARS; i++) {
			var[i] = new;
		}

		wrap_write_unlock();

		//preempt_enable();	/* for get_cycles accuracy */
		if (WRITER_DELAY > 0)
			udelay(WRITER_DELAY);
	} while (!kthread_should_stop());
	delayavg /= iter;
	printk("writer_thread/%lu iterations : %lu, "
		"lock delay [min,avg,max] %llu,%llu,%llu cycles\n",
		(unsigned long)data, iter,
		calibrate_cycles(delaymin),
		calibrate_cycles(delayavg),
		calibrate_cycles(delaymax));
	return 0;
}

#if (TEST_STD_RWLOCK)
static int trylock_writer_thread(void *data)
{
	int i;
	int new;
	unsigned long iter = 0, success = 0, fail = 0;

	printk("trylock_writer_thread/%lu runnning\n", (unsigned long)data);
	do {
#if (TEST_INTERRUPTS)
		/* std write trylock cannot disable interrupts. */
		local_irq_disable();
#endif

#if (TRYLOCK_WRITERS_FAIL_ITER == -1)
		for (;;) {
			iter++;
			if (write_trylock(&std_rw_lock))
				goto locked;
		}
#else
		for (i = 0; i < TRYLOCK_WRITERS_FAIL_ITER; i++) {
			iter++;
			if (write_trylock(&std_rw_lock))
				goto locked;
		}
#endif
		fail++;
#if (TEST_INTERRUPTS)
		local_irq_enable();
#endif
		goto loop;
locked:
		success++;
		new = (int)get_cycles();
		for (i = 0; i < NR_VARS; i++) {
			var[i] = new;
		}
#if (TEST_INTERRUPTS)
		write_unlock_irq(&std_rw_lock);
#else
		write_unlock(&std_rw_lock);
#endif
loop:
		if (TRYLOCK_WRITER_DELAY > 0)
			udelay(TRYLOCK_WRITER_DELAY);
	} while (!kthread_should_stop());
	printk("trylock_writer_thread/%lu iterations : "
		"[try,success,fail after %d try], "
		"%lu,%lu,%lu\n",
		(unsigned long)data, TRYLOCK_WRITERS_FAIL_ITER,
		iter, success, fail);
	return 0;
}

#else /* !TEST_STD_RWLOCK */

static int trylock_writer_thread(void *data)
{
	int i;
	int new;
	unsigned long iter = 0, success = 0, fail = 0;

	printk("trylock_writer_thread/%lu runnning\n", (unsigned long)data);
	do {
		iter++;
#if (TEST_INTERRUPTS)
		if (wbias_write_trylock_irq_else_subscribe(&wbiasrwlock))
#else
		if (wbias_write_trylock_else_subscribe(&wbiasrwlock))
#endif
			goto locked;

#if (TRYLOCK_WRITERS_FAIL_ITER == -1)
		for (;;) {
			iter++;
#if (TEST_INTERRUPTS)
			if (wbias_write_trylock_irq_subscribed(&wbiasrwlock))
#else
			if (wbias_write_trylock_subscribed(&wbiasrwlock))
#endif
				goto locked;
		}
#else
		for (i = 0; i < TRYLOCK_WRITERS_FAIL_ITER - 1; i++) {
			iter++;
#if (TEST_INTERRUPTS)
			if (wbias_write_trylock_irq_subscribed(&wbiasrwlock))
#else
			if (wbias_write_trylock_subscribed(&wbiasrwlock))
#endif
				goto locked;
		}
#endif
		fail++;
		wbias_write_unsubscribe(&wbiasrwlock);
		goto loop;
locked:
		success++;
		new = (int)get_cycles();
		for (i = 0; i < NR_VARS; i++) {
			var[i] = new;
		}
#if (TEST_INTERRUPTS)
		wbias_write_unlock_irq(&wbiasrwlock);
#else
		wbias_write_unlock(&wbiasrwlock);
#endif
loop:
		if (TRYLOCK_WRITER_DELAY > 0)
			udelay(TRYLOCK_WRITER_DELAY);
	} while (!kthread_should_stop());
	printk("trylock_writer_thread/%lu iterations : "
		"[try,success,fail after %d try], "
		"%lu,%lu,%lu\n",
		(unsigned long)data, TRYLOCK_WRITERS_FAIL_ITER,
		iter, success, fail);
	return 0;
}

#endif	/* TEST_STD_RWLOCK */

static void wbias_rwlock_create(void)
{
	unsigned long i;

	for (i = 0; i < NR_PREADERS; i++) {
		printk("starting preemptable reader thread %lu\n", i);
		preader_threads[i] = kthread_run(preader_thread, (void *)i,
			"wbiasrwlock_preader");
		BUG_ON(!preader_threads[i]);
	}

	for (i = 0; i < NR_NPREADERS; i++) {
		printk("starting non-preemptable reader thread %lu\n", i);
		npreader_threads[i] = kthread_run(npreader_thread, (void *)i,
			"wbiasrwlock_npreader");
		BUG_ON(!npreader_threads[i]);
	}

	for (i = 0; i < NR_TRYLOCK_READERS; i++) {
		printk("starting trylock reader thread %lu\n", i);
		trylock_reader_threads[i] = kthread_run(trylock_reader_thread,
			(void *)i, "wbiasrwlock_trylock_reader");
		BUG_ON(!trylock_reader_threads[i]);
	}
	for (i = 0; i < NR_INTERRUPT_READERS; i++) {
		printk("starting interrupt reader %lu\n", i);
		interrupt_reader[i] = kthread_run(interrupt_reader_thread,
			(void *)i,
			"wbiasrwlock_interrupt_reader");
	}
	for (i = 0; i < NR_TRYLOCK_INTERRUPT_READERS; i++) {
		printk("starting trylock interrupt reader %lu\n", i);
		trylock_interrupt_reader[i] =
			kthread_run(trylock_interrupt_reader_thread,
			(void *)i, "wbiasrwlock_trylock_interrupt_reader");
	}
	for (i = 0; i < NR_WRITERS; i++) {
		printk("starting writer thread %lu\n", i);
		writer_threads[i] = kthread_run(writer_thread, (void *)i,
			"wbiasrwlock_writer");
		BUG_ON(!writer_threads[i]);
	}
	for (i = 0; i < NR_TRYLOCK_WRITERS; i++) {
		printk("starting trylock writer thread %lu\n", i);
		trylock_writer_threads[i] = kthread_run(trylock_writer_thread,
			(void *)i, "wbiasrwlock_trylock_writer");
		BUG_ON(!trylock_writer_threads[i]);
	}
}

static void wbias_rwlock_stop(void)
{
	unsigned long i;

	for (i = 0; i < NR_WRITERS; i++)
		kthread_stop(writer_threads[i]);
	for (i = 0; i < NR_TRYLOCK_WRITERS; i++)
		kthread_stop(trylock_writer_threads[i]);
	for (i = 0; i < NR_NPREADERS; i++)
		kthread_stop(npreader_threads[i]);
	for (i = 0; i < NR_PREADERS; i++)
		kthread_stop(preader_threads[i]);
	for (i = 0; i < NR_TRYLOCK_READERS; i++)
		kthread_stop(trylock_reader_threads[i]);
	for (i = 0; i < NR_INTERRUPT_READERS; i++)
		kthread_stop(interrupt_reader[i]);
	for (i = 0; i < NR_TRYLOCK_INTERRUPT_READERS; i++)
		kthread_stop(trylock_interrupt_reader[i]);
}


static void perform_test(const char *name, void (*callback)(void))
{
	printk("%s\n", name);
	callback();
}

static int my_open(struct inode *inode, struct file *file)
{
	unsigned long i;
	cycles_t time1, time2, delay;

	printk("** get_cycles calibration **\n");
	cycles_calibration_min = ULLONG_MAX;
	cycles_calibration_avg = 0;
	cycles_calibration_max = 0;

	local_irq_disable();
	for (i = 0; i < 10; i++) {
		rdtsc_barrier();
		time1 = get_cycles();
		rdtsc_barrier();
		rdtsc_barrier();
		time2 = get_cycles();
		rdtsc_barrier();
		delay = time2 - time1;
		cycles_calibration_min = min(cycles_calibration_min, delay);
		cycles_calibration_avg += delay;
		cycles_calibration_max = max(cycles_calibration_max, delay);
	}
	cycles_calibration_avg /= 10;
	local_irq_enable();

	printk("get_cycles takes [min,avg,max] %llu,%llu,%llu cycles, "
		"results calibrated on avg\n",
		cycles_calibration_min,
		cycles_calibration_avg,
		cycles_calibration_max);
	printk("\n");

	printk("** Single writer test, no contention **\n");
	wbias_rwlock_profile_latency_reset();
	writer_threads[0] = kthread_run(writer_thread, (void *)0,
		"wbiasrwlock_writer");
	BUG_ON(!writer_threads[0]);
	ssleep(SINGLE_WRITER_TEST_DURATION);
	kthread_stop(writer_threads[0]);
	printk("\n");

	wbias_rwlock_profile_latency_print();

	printk("** Single trylock writer test, no contention **\n");
	wbias_rwlock_profile_latency_reset();
	trylock_writer_threads[0] = kthread_run(trylock_writer_thread,
		(void *)0,
		"trylock_wbiasrwlock_writer");
	BUG_ON(!trylock_writer_threads[0]);
	ssleep(SINGLE_WRITER_TEST_DURATION);
	kthread_stop(trylock_writer_threads[0]);
	printk("\n");

	wbias_rwlock_profile_latency_print();

	printk("** Single preemptable reader test, no contention **\n");
	wbias_rwlock_profile_latency_reset();
	preader_threads[0] = kthread_run(preader_thread, (void *)0,
		"wbiasrwlock_preader");
	BUG_ON(!preader_threads[0]);
	ssleep(SINGLE_READER_TEST_DURATION);
	kthread_stop(preader_threads[0]);
	printk("\n");

	wbias_rwlock_profile_latency_print();

	printk("** Single non-preemptable reader test, no contention **\n");
	wbias_rwlock_profile_latency_reset();
	npreader_threads[0] = kthread_run(npreader_thread, (void *)0,
		"wbiasrwlock_npreader");
	BUG_ON(!npreader_threads[0]);
	ssleep(SINGLE_READER_TEST_DURATION);
	kthread_stop(npreader_threads[0]);
	printk("\n");

	wbias_rwlock_profile_latency_print();

	printk("** Multiple p/non-p readers test, no contention **\n");
	wbias_rwlock_profile_latency_reset();
	for (i = 0; i < NR_PREADERS; i++) {
		printk("starting preader thread %lu\n", i);
		preader_threads[i] = kthread_run(preader_thread, (void *)i,
			"wbiasrwlock_preader");
		BUG_ON(!preader_threads[i]);
	}
	for (i = 0; i < NR_NPREADERS; i++) {
		printk("starting npreader thread %lu\n", i);
		npreader_threads[i] = kthread_run(npreader_thread, (void *)i,
			"wbiasrwlock_npreader");
		BUG_ON(!npreader_threads[i]);
	}
	ssleep(SINGLE_READER_TEST_DURATION);
	for (i = 0; i < NR_NPREADERS; i++)
		kthread_stop(npreader_threads[i]);
	for (i = 0; i < NR_PREADERS; i++)
		kthread_stop(preader_threads[i]);
	printk("\n");

	wbias_rwlock_profile_latency_print();

	printk("** High contention test **\n");
	wbias_rwlock_profile_latency_reset();
	perform_test("wbias-rwlock-create", wbias_rwlock_create);
	ssleep(TEST_DURATION);
	perform_test("wbias-rwlock-stop", wbias_rwlock_stop);
	printk("\n");
	wbias_rwlock_profile_latency_print();

	return -EPERM;
}


static struct file_operations my_operations = {
	.open = my_open,
};

int init_module(void)
{
	pentry = create_proc_entry("testwbiasrwlock", 0444, NULL);
	if (pentry)
		pentry->proc_fops = &my_operations;

	printk("PTHREAD_ROFFSET : %lX\n", PTHREAD_ROFFSET);
	printk("PTHREAD_RMASK : %lX\n", PTHREAD_RMASK);
	printk("NPTHREAD_ROFFSET : %lX\n", NPTHREAD_ROFFSET);
	printk("NPTHREAD_RMASK : %lX\n", NPTHREAD_RMASK);
	printk("SOFTIRQ_ROFFSET : %lX\n", SOFTIRQ_ROFFSET);
	printk("SOFTIRQ_RMASK : %lX\n", SOFTIRQ_RMASK);
	printk("HARDIRQ_ROFFSET : %lX\n", HARDIRQ_ROFFSET);
	printk("HARDIRQ_RMASK : %lX\n", HARDIRQ_RMASK);
	printk("SUBSCRIBERS_WOFFSET : %lX\n", SUBSCRIBERS_WOFFSET);
	printk("SUBSCRIBERS_WMASK : %lX\n", SUBSCRIBERS_WMASK);
	printk("WRITER_MUTEX : %lX\n", WRITER_MUTEX);
	printk("NPTHREAD_WMASK : %lX\n", SOFTIRQ_WMASK);
	printk("SOFTIRQ_WMASK : %lX\n", SOFTIRQ_WMASK);
	printk("HARDIRQ_WMASK : %lX\n", HARDIRQ_WMASK);

	return 0;
}

void cleanup_module(void)
{
	remove_proc_entry("testwbiasrwlock", NULL);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("wbias rwlock test");
