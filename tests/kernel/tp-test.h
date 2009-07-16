#include <linux/tracepoint.h>

DECLARE_TRACE(kernel_test,
	TP_PROTO(void *a, void *b),
		TP_ARGS(a, b));
