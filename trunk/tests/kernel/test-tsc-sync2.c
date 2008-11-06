/* test-async-tsc.c
 *
 * test async tsc on AMD.
 */


#include <asm/atomic.h>
#include <linux/module.h>
#include <asm/timex.h>

static int __init test_init(void)
{
	test_tsc_synchronization();
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

