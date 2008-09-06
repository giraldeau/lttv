/* test-nop-speed.c
 *
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/timex.h>
#include <linux/marker.h>
#include <asm/ptrace.h>

#define NR_TESTS 10000000

int var, var2;

struct proc_dir_entry *pentry = NULL;

void empty(void)
{
	asm volatile ("");
	var += 50;
	var /= 10;
	var *= var2;
}

void twobytesjump(void)
{
	asm volatile ("jmp 1f\n\t"
		".byte 0x00, 0x00, 0x00\n\t"
		"1:\n\t");
	var += 50;
	var /= 10;
	var *= var2;
}

void fivebytesjump(void)
{
	asm volatile (".byte 0xe9, 0x00, 0x00, 0x00, 0x00\n\t");
	var += 50;
	var /= 10;
	var *= var2;
}

void threetwonops(void)
{
	asm volatile (".byte 0x66,0x66,0x90,0x66,0x90\n\t");
	var += 50;
	var /= 10;
	var *= var2;
}

void fivebytesnop(void)
{
	asm volatile (".byte 0x66,0x66,0x66,0x66,0x90\n\t");
	var += 50;
	var /= 10;
	var *= var2;
}

void fivebytespsixnop(void)
{
	asm volatile (".byte 0x0f,0x1f,0x44,0x00,0\n\t");
	var += 50;
	var /= 10;
	var *= var2;
}

/*
 * GENERIC_NOP1 GENERIC_NOP4,
 * 1: nop
 * _not_ nops in 64-bit mode.
 * 4: leal 0x00(,%esi,1),%esi
 */
void genericfivebytesonefournops(void)
{
	asm volatile (".byte 0x90,0x8d,0x74,0x26,0x00\n\t");
	var += 50;
	var /= 10;
	var *= var2;
}

/*
 * K7_NOP4 ASM_NOP1
 * 1: nop
 * assumed _not_ to be nops in 64-bit mode.
 * leal 0x00(,%eax,1),%eax
 */
void k7fivebytesonefournops(void)
{
	asm volatile (".byte 0x90,0x8d,0x44,0x20,0x00\n\t");
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
	perform_test("2-bytes jump", twobytesjump);
	perform_test("5-bytes jump", fivebytesjump);
	perform_test("3/2 nops", threetwonops);
	perform_test("5-bytes nop with long prefix", fivebytesnop);
	perform_test("5-bytes P6 nop", fivebytespsixnop);
#ifdef CONFIG_X86_32
	perform_test("Generic 1/4 5-bytes nops", genericfivebytesonefournops);
	perform_test("K7 1/4 5-bytes nops", k7fivebytesonefournops);
#endif

	return -EPERM;
}


static struct file_operations my_operations = {
	.open = my_open,
};

int init_module(void)
{
	pentry = create_proc_entry("testnops", 0444, NULL);
	if (pentry)
		pentry->proc_fops = &my_operations;

	return 0;
}

void cleanup_module(void)
{
	remove_proc_entry("testnops", NULL);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("NOP Test");