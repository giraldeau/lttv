#include <linux/tracepoint.h>

DECLARE_TRACE(kernel_test,
	TPPROTO(void *a, void *b),
		TPARGS(a, b));
