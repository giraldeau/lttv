/* test-mark.c
 *
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/timex.h>
#include <linux/marker.h>
#include <asm/ptrace.h>
#include <asm/system.h>

static void pmc_flush_cache(void)
  {
    /* write back and invalidate cache (a serializing instruction) */

    __asm__ __volatile__ ( "wbinvd" : : : "memory" );

    /* The wbinvd instruction does not wait for the external caches
     * to be flushed, but only requests that it be done.  The loop
     * is to be sure that enough time has elapsed, but the compiler
     * might simplify or even remove it.  The loop bound is for a
     * 512 KB L2 cache.  On a Pentium Pro/II/III, the loop uses
     * 2 cycles per iteration.
     *
     * Does wbinvd also cause the TLB to be flushed?
     * A comment in mtrr.c suggests that it does.
     */
    { register int i; for (i = 0; i < 512*1024; i++) { } }
  }



static void noinline test2(const struct marker *mdata,
        void *call_private, ...)
{
	unsigned char *ins = __builtin_return_address(0) - 5;
#if 0
	/* not called */
	printk("ip %p\n", __builtin_return_address(0));
	printk("prev_ins %hX %hX %hX %hX %hX\n",
		ins[0], ins[1], ins[2], ins[3], ins[4]);
#endif //0
	ins[0] = 0x90;
	ins[1] = 0x8d;
	ins[2] = 0x74;
	ins[3] = 0x26;
	ins[4] = 0x00;
}

/*
 * Generic marker flavor always available.
 * Note : the empty asm volatile with read constraint is used here instead of a
 * "used" attribute to fix a gcc 4.1.x bug.
 * Make sure the alignment of the structure in the __markers section will
 * not add unwanted padding between the beginning of the section and the
 * structure. Force alignment to the same alignment as the section start.
 */
#define __my_trace_mark(generic, name, call_private, format, args...)	\
	do {								\
		static const char __mstrtab_##name[]			\
		__attribute__((section("__markers_strings")))		\
		= #name "\0" format;					\
		static struct marker __mark_##name			\
		__attribute__((section("__markers"), aligned(8))) =	\
		{ __mstrtab_##name, &__mstrtab_##name[sizeof(#name)],	\
		0, 0, marker_probe_cb,					\
		{ __mark_empty_function, NULL}, NULL };			\
		__mark_check_format(format, ## args);			\
		if (!generic) {						\
			if (unlikely(imv_read(__mark_##name.state)))	\
				test2			\
					(&__mark_##name, call_private,	\
					## args);		\
		} else {						\
			if (unlikely(_imv_read(__mark_##name.state)))	\
				test2			\
					(&__mark_##name, call_private,	\
					## args);		\
		}							\
	} while (0)

	//asm volatile ("");
struct proc_dir_entry *pentry = NULL;

char temp0[8192];
int temp[8192] __cacheline_aligned;
char temp5[8192];

static inline void test(unsigned long arg, unsigned long arg2)
{
#ifdef CACHEFLUSH
	pmc_flush_cache();
#endif
	temp[2] = (temp[0] + 60) << 10;
	temp[3] = (temp[2] + 60) << 10;
	temp[4] = (temp[3] + 60) << 10;
	temp[0] = (temp[4] + 60) << 10;
	//asm volatile ("");
	//__my_trace_mark(1, kernel_debug_test, NULL, "%d %d %ld %ld", 2, current->pid, arg, arg2);
	barrier();
	test2(NULL, NULL, 2, current->pid, arg, arg2);
	barrier();
	//__my_trace_mark(0, kernel_debug_test, NULL, "%d %d %ld %ld", 2, current->pid, arg, arg2);
}

static int my_open(struct inode *inode, struct file *file)
{
	unsigned int i;
	cycles_t cycles1, cycles2;
	unsigned long flags;

	local_irq_save(flags);
#ifdef CACHEFLUSH
	pmc_flush_cache();	/* initial write back, without cycle count */
	msleep(20);	/* wait for L2 flush */
#endif
	rdtsc_barrier();
	cycles1 = get_cycles();
	rdtsc_barrier();
	for(i=0; i<2000; i++) {
		test(i, i);
		test(i, i);
		test(i, i);
		test(i, i);
		test(i, i);
		test(i, i);
		test(i, i);
		test(i, i);
		test(i, i);
		test(i, i);
	}
	rdtsc_barrier();
	cycles2 = get_cycles();
	rdtsc_barrier();
	local_irq_restore(flags);
	printk("cycles : %llu\n", cycles2-cycles1);
	return -EPERM;
}


static struct file_operations my_operations = {
	.open = my_open,
};

int init_module(void)
{
	pentry = create_proc_entry("testmark", 0444, NULL);
	if (pentry)
		pentry->proc_fops = &my_operations;

	return 0;
}

void cleanup_module(void)
{
	remove_proc_entry("testmark", NULL);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Marker Test");
MODULE_VERSION("1.0");

