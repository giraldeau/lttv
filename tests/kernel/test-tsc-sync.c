/* test-tsc-sync.c
 *
 * Test TSC sync
 */


#include <linux/module.h>
#include <linux/timer.h>
#include <asm/timex.h>
#include <linux/jiffies.h>

static DEFINE_PER_CPU(cycles_t, count) = 0;

static struct timer_list test_timer;

/* IPI called on each CPU. */
static void test_each(void *info)
{
	__get_cpu_var(count) = get_cycles();
}

static void do_test_timer(unsigned long data)
{
	int cpu;

	/* Increment the counters */
	on_each_cpu(test_each, NULL, 0, 1);
	/* Read all the counters */
	printk("Counters read from CPU %d\n", smp_processor_id());
	for_each_online_cpu(cpu) {
		printk("Read : CPU %d, count %llu\n", cpu,
			per_cpu(count, cpu));
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
MODULE_DESCRIPTION("sync tsc test");

