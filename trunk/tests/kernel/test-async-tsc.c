/* test-async-tsc.c
 *
 * test async tsc on AMD.
 */


#include <asm/atomic.h>
#include <linux/module.h>
#include <asm/timex.h>

//LTT #define LTT_MIN_PROBE_DURATION 400
//get_cycles
#define LTT_MIN_PROBE_DURATION 200

static atomic_long_t ltt_last_tsc = ATOMIC_LONG_INIT(0);

/* When the local TSC is discovered to lag behind the highest TSC counter, we
 * increment the TSC count of an amount that should be, ideally, lower than the
 * execution time of this routine, in cycles : this is the granularity we look
 * for : we must be able to order the events. */

cycles_t ltt_tsc_read(void)
{
	cycles_t new_tsc;
	cycles_t last_tsc;

	new_tsc = get_cycles_sync();
        if (cpu_has(&cpu_data[smp_processor_id()], X86_FEATURE_CONSTANT_TSC))
                return new_tsc;

	do {
		last_tsc = atomic_long_read(&ltt_last_tsc);
		if (new_tsc < last_tsc) {
			/* last_tsc may only have incremented since last read,
			 * therefore the condition new_tsc < last_tsc still
			 * applies even if it has been updated. Therefore, we
			 * can use add_return, cheaper than cmpxchg here. */
			new_tsc = atomic_long_add_return(LTT_MIN_PROBE_DURATION,
					&ltt_last_tsc);
			break;
		}
		/* cmpxchg will fail if ltt_last_tsc has been concurrently
		 * updated by add_return or set to a lower tsc value by a
		 * concurrent CPU at the same time. cmpxchg will succeed if
		 * the other CPUs update the ltt_last_tsc with a cmpxchg or
		 * add_return to a value higher than the new_tsc : it's ok
		 * since the current get_cycles happened before the one that
		 * causes the ltt_last_tsc to become higher than new_tsc.
		 * It also succeeds if we write to the memory location
		 * successfully without concurrent modification. */
	} while (atomic_long_cmpxchg(&ltt_last_tsc, last_tsc, new_tsc)
		< new_tsc);
	return new_tsc;
}

static int __init test_init(void)
{
	int i;
	cycles_t time1, time2;
	volatile cycles_t myval;

	time1 = get_cycles();
	for (i=0; i<200; i++) {
		//printk("time %llu\n", ltt_tsc_read());
		//myval = ltt_tsc_read();
		myval = get_cycles_sync();
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

