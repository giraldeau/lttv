/* test-local.c
 *
 * Sample module for local.h usage.
 */


#include <asm/local.h>
#include <linux/module.h>
#include <linux/timer.h>

static DEFINE_PER_CPU(local_t, counters) = LOCAL_INIT(0);

static struct timer_list test_timer;

/* IPI called on each CPU. */
static void test_each(void *info)
{
	/* Increment the counter from a non preemptible context */
	printk("Increment on cpu %d\n", smp_processor_id());
	local_inc(&__get_cpu_var(counters));

	/* This is what incrementing the variable would look like within a
	 * preemptible context (it disables preemption) :
	 *
	 * local_inc(&get_cpu_var(counters));
	 * put_cpu_var(counters);
	 */
}

static void do_test_timer(unsigned long data)
{
	int cpu;

	/* Increment the counters */
	on_each_cpu(test_each, NULL, 0, 1);
	/* Read all the counters */
	printk("Counters read from CPU %d\n", smp_processor_id());
	for_each_online_cpu(cpu) {
		printk("Read : CPU %d, count %ld\n", cpu,
			local_read(&per_cpu(counters, cpu)));
	}
	del_timer(&test_timer);
	test_timer.expires = jiffies + 1000;
	add_timer(&test_timer);
}

static int __init test_init(void)
{
	/* initialize the timer that will increment the counter */
	init_timer(&test_timer);
	test_timer.function = do_test_timer;
	test_timer.expires = jiffies + 1;
	add_timer(&test_timer);

	return 0;
}

static void __exit test_exit(void)
{
	del_timer_sync(&test_timer);
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Local Atomic Ops");

