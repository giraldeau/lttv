/* test-nop-speed.c
 *
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/timex.h>
#include <asm/atomic.h>
#include <asm/ptrace.h>

#define NR_TESTS 10000000

int var, var2;
atomic64_t atomicval;

struct proc_dir_entry *pentry = NULL;


static inline long test_nop_atomic64_add_return(long i, atomic64_t *v)
{
	long __i = i;
	asm volatile(".byte 0x90;"
		     "xaddq %0, %1;"
		     : "+r" (i), "+m" (v->counter)
		     : : "memory");
	return i + __i;
}

static inline long test_prefix_atomic64_add_return(long i, atomic64_t *v)
{
	long __i = i;
	asm volatile(".byte 0x3E;"
		     "xaddq %0, %1;"
		     : "+r" (i), "+m" (v->counter)
		     : : "memory");
	return i + __i;
}

static inline long test_lock_atomic64_add_return(long i, atomic64_t *v)
{
	long __i = i;
	asm volatile(".byte 0xf0;"
		     "xaddq %0, %1;"
		     : "+r" (i), "+m" (v->counter)
		     : : "memory");
	return i + __i;
}



void empty(void)
{
	asm volatile ("");
	var += 50;
	var /= 10;
	var *= var2;
}

void testnop(void)
{
	test_nop_atomic64_add_return(5, &atomicval);
	var += 50;
	var /= 10;
	var *= var2;
}

void testprefix(void)
{
	test_prefix_atomic64_add_return(5, &atomicval);
	var += 50;
	var /= 10;
	var *= var2;
}

void testlock(void)
{
	test_lock_atomic64_add_return(5, &atomicval);
	var += 50;
	var /= 10;
	var *= var2;
}



void perform_test(const char *name, void (*callback)(void))
{
	unsigned int i;
	cycles_t cycles1, cycles2;
	unsigned long flags;

	local_irq_save(flags);
	rdtsc_barrier();
	cycles1 = get_cycles();
	rdtsc_barrier();
	for(i=0; i<NR_TESTS; i++) {
		callback();
	}
	rdtsc_barrier();
	cycles2 = get_cycles();
	rdtsc_barrier();
	local_irq_restore(flags);
	printk("test %s cycles : %llu\n", name, cycles2-cycles1);
}

static int my_open(struct inode *inode, struct file *file)
{
	printk("NR_TESTS %d\n", NR_TESTS);

	perform_test("empty", empty);
	perform_test("test 1-byte nop xadd", testnop);
	perform_test("test DS override prefix xadd", testprefix);
	perform_test("test LOCK xadd", testlock);

	return -EPERM;
}


static struct file_operations my_operations = {
	.open = my_open,
};

int init_module(void)
{
	pentry = create_proc_entry("testprefix", 0444, NULL);
	if (pentry)
		pentry->proc_fops = &my_operations;

	return 0;
}

void cleanup_module(void)
{
	remove_proc_entry("testprefix", NULL);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("prefix test");
