/* test-async-tsc.c
 *
 * test async tsc on AMD.
 */


#include <asm/atomic.h>
#include <linux/module.h>
#include <asm/timex.h>
#include <asm/hpet.h>
#include <asm/io.h>

static int __init test_init(void)
{
	int i;
	cycles_t time1, time2;
	volatile unsigned long myval;

	time1 = get_cycles();
	for (i=0; i<1; i++) {
		//printk("time %llu\n", ltt_tsc_read());
		//myval = ltt_tsc_read();
		myval = hpet_readl(HPET_COUNTER);
	}
	time2 = get_cycles();
	printk("timediff %llu\n", time2-time1);
	return -EPERM;
}

static void __exit test_exit(void)
{
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("sync async tsc");

