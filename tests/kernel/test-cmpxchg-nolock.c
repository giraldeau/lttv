/* test-cmpxchg.c
 *
 * Test time spent in a LTTng instrumentation probe.
 */


#include <linux/config.h>
#include <linux/jiffies.h>
#include <linux/compiler.h>
#include <linux/init.h>
#include <linux/module.h>

#if 0
#define cmpxchg_up(ptr,o,n)\
	((__typeof__(*(ptr)))__cmpxchg_up((ptr),(unsigned long)(o),\
					(unsigned long)(n),sizeof(*(ptr))))
static inline unsigned long __cmpxchg_up(volatile void *ptr, unsigned long old,
				      unsigned long new, int size)
{
	unsigned long prev;
	switch (size) {
	case 1:
		__asm__ __volatile__("cmpxchgb %b1,%2"
				     : "=a"(prev)
				     : "q"(new), "m"(*__xg(ptr)), "0"(old)
				     : "memory");
		return prev;
	case 2:
		__asm__ __volatile__("cmpxchgw %w1,%2"
				     : "=a"(prev)
				     : "r"(new), "m"(*__xg(ptr)), "0"(old)
				     : "memory");
		return prev;
	case 4:
		__asm__ __volatile__("cmpxchgl %1,%2"
				     : "=a"(prev)
				     : "r"(new), "m"(*__xg(ptr)), "0"(old)
				     : "memory");
		return prev;
	}
	return old;
}
#endif //0

#define NR_LOOPS 20000

volatile int test_val = 100;


static inline void do_test(void)
{
	int val, ret;

	val = test_val;
	
	ret = cmpxchg_up(&test_val, val, val+1);
}

static int ltt_test_init(void)
{
	unsigned int i;
	cycles_t time1, time2, time;
	cycles_t max_time = 0, min_time = 18446744073709551615ULL; /* (2^64)-1 */
	cycles_t tot_time = 0;
	unsigned long flags;
	printk(KERN_ALERT "test init\n");
	
	local_irq_save(flags);
	time1 = get_cycles();
	for(i=0; i<NR_LOOPS; i++) {
		//for(int j=0; j<10; j++) {
		do_test();
		//}
		//max_time = max(max_time, time);
		//min_time = min(min_time, time);
		//printk("val : %d\n", test_val);
	}
	time2 = get_cycles();
	local_irq_restore(flags);
	time = time2 - time1;
	tot_time += time;

	printk(KERN_ALERT "test results : time non locked cmpxchg\n");
	printk(KERN_ALERT "number of loops : %d\n", NR_LOOPS);
	printk(KERN_ALERT "total time : %llu\n", tot_time);
	//printk(KERN_ALERT "min : %llu\n", min_time);
	//printk(KERN_ALERT "max : %llu\n", max_time);

	printk(KERN_ALERT "test end\n");
	
	return -EAGAIN; /* Fail will directly unload the module */
}

static void ltt_test_exit(void)
{
	printk(KERN_ALERT "test exit\n");
}

module_init(ltt_test_init)
module_exit(ltt_test_exit)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Linux Trace Toolkit Test");

