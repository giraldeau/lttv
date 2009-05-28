/*
 * test-ipi.c
 *
 * Copyright 2009 - Mathieu Desnoyers <mathieu.desnoyers@polymtl.ca>
 * Distributed under GPLv2
 */

#include <linux/jiffies.h>
#include <linux/compiler.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/math64.h>
#include <linux/spinlock.h>
#include <linux/seqlock.h>
#include <linux/cpumask.h>
#include <linux/smp.h>
#include <asm/timex.h>
#include <asm/system.h>

#ifdef CONFIG_ARM
#include <linux/trace-clock.h>
#define get_timestamp	trace_clock_read64
#else
#define get_timestamp	get_cycles
#endif

#define NR_LOOPS 20000

int test_val;

static void do_testbaseline(void)
{
	unsigned long flags;
	unsigned int i;
	cycles_t time1, time2, time;
	u32 rem;

	local_irq_save(flags);
	preempt_disable();
	time1 = get_timestamp();
	for (i = 0; i < NR_LOOPS; i++) {
		asm volatile ("");
	}
	time2 = get_timestamp();
	local_irq_restore(flags);
	preempt_enable();
	time = time2 - time1;

	printk(KERN_ALERT "test results: time for baseline\n");
	printk(KERN_ALERT "number of loops: %d\n", NR_LOOPS);
	printk(KERN_ALERT "total time: %llu\n", (unsigned long long)time);
	time = div_u64_rem(time, NR_LOOPS, &rem);
	printk(KERN_ALERT "-> baseline takes %llu cycles\n", (unsigned long long)time);
	printk(KERN_ALERT "test end\n");
}

static void remote_wmb(void *info)
{
	smp_wmb();
}

static void do_test_ipi(void)
{
	unsigned int i;
	int cpu;
	cycles_t time1, time2, time;
	u32 rem;

	preempt_disable();
	cpu = smp_processor_id();
	if (cpu == 0)
		cpu = 1;
	else
		cpu = 0;
	time1 = get_timestamp();
	for (i = 0; i < NR_LOOPS; i++) {
		smp_call_function_single(cpu, remote_wmb, NULL, 1);
	}
	time2 = get_timestamp();
	preempt_enable();
	time = time2 - time1;

	printk(KERN_ALERT "test results: time for ipi\n");
	printk(KERN_ALERT "number of loops: %d\n", NR_LOOPS);
	printk(KERN_ALERT "total time: %llu\n", (unsigned long long)time);
	time = div_u64_rem(time, NR_LOOPS, &rem);
	printk(KERN_ALERT "-> ipi takes %llu cycles\n", (unsigned long long)time);
	printk(KERN_ALERT "test end\n");
}

static void do_test_wmb(void)
{
	unsigned int i;
	cycles_t time1, time2, time;
	u32 rem;

	preempt_disable();
	time1 = get_timestamp();
	for (i = 0; i < NR_LOOPS; i++) {
		wmb();
	}
	time2 = get_timestamp();
	preempt_enable();
	time = time2 - time1;

	printk(KERN_ALERT "test results: time for ipi\n");
	printk(KERN_ALERT "number of loops: %d\n", NR_LOOPS);
	printk(KERN_ALERT "total time: %llu\n", (unsigned long long)time);
	time = div_u64_rem(time, NR_LOOPS, &rem);
	printk(KERN_ALERT "-> ipi takes %llu cycles\n", (unsigned long long)time);
	printk(KERN_ALERT "test end\n");
}

static int ltt_test_init(void)
{
	printk(KERN_ALERT "test init\n");
	
	printk(KERN_ALERT "Number of active CPUs : %d\n", num_online_cpus());
	do_testbaseline();
	do_test_ipi();
	do_test_wmb();
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
MODULE_DESCRIPTION("Test read lock speed");
