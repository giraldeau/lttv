/*
 * test-trace.c
 *
 * Test tracepoint probes.
 */

#include <linux/module.h>
#include "tp-test.h"
#include <linux/ltt-type-serializer.h>

/* kernel_trap_entry specialized tracepoint probe */

struct serialize_long_long {
	unsigned long f1;
	unsigned long f2;
	unsigned char end_field[0];
} LTT_ALIGN;

void probe_test(void *a, void *b);

DEFINE_MARKER_TP(kernel, test, kernel_test,
	probe_test, "f1 %p f2 %p");

notrace void probe_test(void *a, void *b)
{
	struct marker *marker;
	struct serialize_long_long data;

	data.f1 = (long)a;
	data.f2 = (long)b;

	marker = &GET_MARKER(kernel, test);
	ltt_specialized_trace(marker, marker->single.probe_private,
		&data, serialize_sizeof(data), sizeof(long));
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Test Tracepoint Probes");
