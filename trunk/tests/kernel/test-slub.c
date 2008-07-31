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

#define TEST_COUNT 10000

static int slub_test_init(void)
{
	void **v = kmalloc(TEST_COUNT * sizeof(void *), GFP_KERNEL);
	unsigned int i;
	cycles_t time1, time2, time;
	long rem;
	int size;

	printk(KERN_ALERT "test init\n");

	printk(KERN_ALERT "SLUB Performance testing\n");
	printk(KERN_ALERT "========================\n");
	printk(KERN_ALERT "1. Kmalloc: Repeatedly allocate then free test\n");
	for (size = 8; size <= PAGE_SIZE << 2; size <<= 1) {
		time1 = get_cycles();
		for (i = 0; i < TEST_COUNT; i++) {
			v[i] = kmalloc(size, GFP_KERNEL);
		}
		time2 = get_cycles();
		time = time2 - time1;

		printk(KERN_ALERT "%i times kmalloc(%d) = \n", i, size);
		printk(KERN_ALERT "number of loops: %d\n", TEST_COUNT);
		printk(KERN_ALERT "total time: %llu\n", time);
		time = div_long_long_rem(time, TEST_COUNT, &rem);
		printk(KERN_ALERT "-> %llu cycles\n", time);

		time1 = get_cycles();
		for (i = 0; i < TEST_COUNT; i++) {
			kfree(v[i]);
		}
		time2 = get_cycles();
		time = time2 - time1;

		printk(KERN_ALERT "%i times kfree = \n", i);
		printk(KERN_ALERT "number of loops: %d\n", TEST_COUNT);
		printk(KERN_ALERT "total time: %llu\n", time);
		time = div_long_long_rem(time, TEST_COUNT, &rem);
		printk(KERN_ALERT "-> %llu cycles\n", time);
	}

	printk(KERN_ALERT "2. Kmalloc: alloc/free test\n");
	for (size = 8; size <= PAGE_SIZE << 2; size <<= 1) {
		time1 = get_cycles();
		for (i = 0; i < TEST_COUNT; i++) {
			kfree(kmalloc(size, GFP_KERNEL));
		}
		time2 = get_cycles();
		time = time2 - time1;

		printk(KERN_ALERT "%i times kmalloc(%d)/kfree = \n", i, size);
		printk(KERN_ALERT "number of loops: %d\n", TEST_COUNT);
		printk(KERN_ALERT "total time: %llu\n", time);
		time = div_long_long_rem(time, TEST_COUNT, &rem);
		printk(KERN_ALERT "-> %llu cycles\n", time);
	}
#if 0
	printk(KERN_ALERT "3. kmem_cache_alloc: Repeatedly allocate then free test\n");
	for (size = 3; size <= PAGE_SHIFT; size ++) {
		time1 = get_cycles();
		for (i = 0; i < TEST_COUNT; i++) {
			v[i] = kmem_cache_alloc(kmalloc_caches + size, GFP_KERNEL);
		}
		time2 = get_cycles();
		time = time2 - time1;

		printk(KERN_ALERT "%d times kmem_cache_alloc(%d) = \n", i, 1 << size);
		printk(KERN_ALERT "number of loops: %d\n", TEST_COUNT);
		printk(KERN_ALERT "total time: %llu\n", time);
		time = div_long_long_rem(time, TEST_COUNT, &rem);
		printk(KERN_ALERT "-> %llu cycles\n", time);

		time1 = get_cycles();
		for (i = 0; i < TEST_COUNT; i++) {
			kmem_cache_free(kmalloc_caches + size, v[i]);
		}
		time2 = get_cycles();
		time = time2 - time1;

		printk(KERN_ALERT "%i times kmem_cache_free = \n", i);
		printk(KERN_ALERT "number of loops: %d\n", TEST_COUNT);
		printk(KERN_ALERT "total time: %llu\n", time);
		time = div_long_long_rem(time, TEST_COUNT, &rem);
		printk(KERN_ALERT "-> %llu cycles\n", time);
	}

	printk(KERN_ALERT "4. kmem_cache_alloc: alloc/free test\n");
	for (size = 3; size <= PAGE_SHIFT; size++) {
		time1 = get_cycles();
		for (i = 0; i < TEST_COUNT; i++) {
			kmem_cache_free(kmalloc_caches + size,
				kmem_cache_alloc(kmalloc_caches + size,
							GFP_KERNEL));
		}
		time2 = get_cycles();
		time = time2 - time1;

		printk(KERN_ALERT "%d times kmem_cache_alloc(%d)/kmem_cache_free = \n", i, 1 << size);
		printk(KERN_ALERT "number of loops: %d\n", TEST_COUNT);
		printk(KERN_ALERT "total time: %llu\n", time);
		time = div_long_long_rem(time, TEST_COUNT, &rem);
		printk(KERN_ALERT "-> %llu cycles\n", time);
	}
	printk(KERN_ALERT "5. kmem_cache_zalloc: Repeatedly allocate then free test\n");
	for (size = 3; size <= PAGE_SHIFT; size ++) {
		time1 = get_cycles();
		for (i = 0; i < TEST_COUNT; i++) {
			v[i] = kmem_cache_zalloc(kmalloc_caches + size, GFP_KERNEL);
		}
		time2 = get_cycles();
		time = time2 - time1;

		printk(KERN_ALERT "%d times kmem_cache_zalloc(%d) = \n", i, 1 << size);
		printk(KERN_ALERT "number of loops: %d\n", TEST_COUNT);
		printk(KERN_ALERT "total time: %llu\n", time);
		time = div_long_long_rem(time, TEST_COUNT, &rem);
		printk(KERN_ALERT "-> %llu cycles\n", time);

		time1 = get_cycles();
		for (i = 0; i < TEST_COUNT; i++) {
			kmem_cache_free(kmalloc_caches + size, v[i]);
		}
		time2 = get_cycles();
		time = time2 - time1;

		printk(KERN_ALERT "%i times kmem_cache_free = \n", i);
		printk(KERN_ALERT "number of loops: %d\n", TEST_COUNT);
		printk(KERN_ALERT "total time: %llu\n", time);
		time = div_long_long_rem(time, TEST_COUNT, &rem);
		printk(KERN_ALERT "-> %llu cycles\n", time);
	}

	printk(KERN_ALERT "6. kmem_cache_zalloc: alloc/free test\n");
	for (size = 3; size <= PAGE_SHIFT; size++) {
		time1 = get_cycles();
		for (i = 0; i < TEST_COUNT; i++) {
			kmem_cache_free(kmalloc_caches + size,
				kmem_cache_zalloc(kmalloc_caches + size,
							GFP_KERNEL));
		}
		time2 = get_cycles();
		time = time2 - time1;

		printk(KERN_ALERT "%d times kmem_cache_zalloc(%d)/kmem_cache_free = \n", i, 1 << size);
		printk(KERN_ALERT "number of loops: %d\n", TEST_COUNT);
		printk(KERN_ALERT "total time: %llu\n", time);
		time = div_long_long_rem(time, TEST_COUNT, &rem);
		printk(KERN_ALERT "-> %llu cycles\n", time);

	}
#endif //0
	kfree(v);
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
