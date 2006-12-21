/* rdtsc-smp
 *
 * Test TSC
 */


#include <linux/jiffies.h>
#include <linux/compiler.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h>

static atomic_t busy_wait;
static struct delayed_work testwork[NR_CPUS];


static void do_work(struct work_struct *work)
{
	cycles_t val;
	int cpu1, cpu2;
	int num_online = num_online_cpus();
	int copybusy;
	int i;

	cpu1 = smp_processor_id();
	printk("Busy waiting on cpu %d\n", cpu1);

	/* Prime busy_wait in cache */
	for(i=0; i<100; i++) {
		copybusy = atomic_read(&busy_wait);
	}
	barrier();
	atomic_inc(&busy_wait);
	while(atomic_read(&busy_wait) != num_online) {
		barrier();
	}
	
	val = get_cycles();

	cpu2 = smp_processor_id();
	BUG_ON(cpu1 != cpu2);
	printk("Cycle count on cpu %d is %llu\n", cpu1, val);
}


static int ltt_test_init(void)
{
	int cpu;

	printk(KERN_ALERT "test init\n");
	
	atomic_set(&busy_wait, 0);
	for_each_online_cpu(cpu) {
		INIT_DELAYED_WORK(&testwork[cpu], do_work);
		schedule_delayed_work_on(cpu, &testwork[cpu], 0);
	}
	return 0;
}

static void ltt_test_exit(void)
{
	/* Test program... wait for output before unload */
	printk(KERN_ALERT "test exit\n");
}

module_init(ltt_test_init)
module_exit(ltt_test_exit)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Linux Trace Toolkit Test");

