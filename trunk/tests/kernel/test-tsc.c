/* test-tsc.c
 *
 * Test TSC synchronization
 */


#include <linux/module.h>
#include <linux/timer.h>
#include <asm/timex.h>
#include <linux/jiffies.h>
#include <linux/cpu.h>
#include <linux/kthread.h>

#define MAX_CYCLES_DELTA 1000ULL

static DEFINE_PER_CPU(cycles_t, tsc_count) = 0;

static atomic_t wait_sync;
static atomic_t wait_end_sync;

/* Mark it noinline so we make sure it is not unrolled.
 * Wait until value is reached. */
static noinline void tsc_barrier(int value)
{
	sync_core();
	atomic_dec(&wait_sync);
	do {
		barrier();
	} while(unlikely(atomic_read(&wait_sync) > value));
	__get_cpu_var(tsc_count) = get_cycles_sync();
}

/* worker thread called on each CPU.
 * First wait with interrupts enabled, then wait with interrupt disabled,
 * for precision. We are already bound to one CPU. */
static void test_sync(void *arg)
{
	unsigned long flags;

	local_irq_save(flags);
	tsc_barrier(2);	/* Make sure the instructions are in I-CACHE */
	tsc_barrier(0);
	atomic_dec(&wait_end_sync);
	do {
		barrier();
	} while(unlikely(atomic_read(&wait_end_sync)));
	local_irq_restore(flags);
}

/* Do loops (making sure no unexpected event changes the timing), keep the
 * best one. The result of each loop is the highest tsc delta between the
 * master CPU and the slaves. */
static int test_synchronization(void)
{
	int cpu, master;
	cycles_t max_diff = 0, diff, best_loop, worse_loop = 0;
	int i;

	preempt_disable();
	master = smp_processor_id();
	for_each_online_cpu(cpu) {
		if (master == cpu)
			continue;
		best_loop = ULLONG_MAX;
		for (i = 0; i < 10; i++) {
			/* Each CPU (master and slave) must decrement the
			 * wait_sync value twice (one for priming in cache) */
			atomic_set(&wait_sync, 4);
			atomic_set(&wait_end_sync, 2);
			smp_call_function_single(cpu, test_sync, NULL, 1, 0);
			test_sync(NULL);
			diff = abs(per_cpu(tsc_count, cpu)
				- per_cpu(tsc_count, master));
			best_loop = min(best_loop, diff);
			worse_loop = max(worse_loop, diff);
		}
		max_diff = max(best_loop, max_diff);
	}
	preempt_enable();
	if (max_diff >= MAX_CYCLES_DELTA) {
		printk("Synchronization tsc : %llu cycles delta is over "
			"threshold %llu\n", max_diff, MAX_CYCLES_DELTA);
		printk("Synchronization tsc (worse loop) : %llu cycles delta\n",
			worse_loop);
	}
	return max_diff < MAX_CYCLES_DELTA;
}

static int __init test_init(void)
{
	int ret;

	ret = test_synchronization();
	return 0;
}

static void __exit test_exit(void)
{
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("sync tsc test");

