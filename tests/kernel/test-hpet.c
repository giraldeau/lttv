/* test-async-tsc.c
 *
 * test async tsc on AMD.
 */


#include <asm/atomic.h>
#include <linux/module.h>
#include <asm/timex.h>
#include <asm/hpet.h>
#include <asm/io.h>
#include <linux/clocksource.h>
#include <linux/ltt.h>
#include <asm/cacheflush.h>

//#define NR_LOOPS 10000
#define NR_LOOPS 10

extern cycle_t read_hpet(void);

static int __init test_init(void)
{
	int i;
	cycles_t time1, time2;
	volatile unsigned long myval;
	int sync_save;	/* racy */

	sync_save = ltt_tsc_is_sync;
	ltt_tsc_is_sync = 0;
	//ltt_tsc_is_sync = 1;
	return -EPERM; //TEST !
	myval = ltt_get_timestamp64();
	time1 = get_cycles();
	for (i=0; i < NR_LOOPS; i++) {
		//printk("time %llu\n", ltt_tsc_read());
		//get_cycles_barrier();
		//myval = get_cycles();
		//get_cycles_barrier();
		myval = read_hpet();
		//clflush(&ltt_last_tsc);
		//myval = ltt_get_timestamp64();
		printk("val : %llu\n", (unsigned long long)myval);
	}
	time2 = get_cycles();
	printk("timediff %llu\n", (time2-time1)/NR_LOOPS);
	ltt_tsc_is_sync = sync_save;
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

