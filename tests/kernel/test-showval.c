/* test-slub.c
 *
 * Compare local cmpxchg with irq disable / enable with cmpxchg_local for slub.
 */


#include <linux/jiffies.h>
#include <linux/compiler.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/calc64.h>
#include <asm/timex.h>
#include <asm/system.h>

extern atomic_t slub_fast_count;
extern atomic_t slub_slow_count;

static int slub_test_init(void)
{
	printk("Fast slub free: %u\n", atomic_read(&slub_fast_count));
	printk("Slow slub free: %u\n", atomic_read(&slub_slow_count));
	return -EAGAIN; /* Fail will directly unload the module */
}

static void slub_test_exit(void)
{
	printk(KERN_ALERT "test exit\n");
}

module_init(slub_test_init)
module_exit(slub_test_exit)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("SLUB test");
