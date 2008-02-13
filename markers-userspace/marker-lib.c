
#include "marker.h"
#include <stdio.h>
#include <errno.h>

__attribute__ ((visibility ("protected")))
extern struct marker __start___markers[];

__attribute__ ((visibility ("protected")))
extern struct marker __stop___markers[];

/**
 * __mark_empty_function - Empty probe callback
 * @probe_private: probe private data
 * @call_private: call site private data
 * @fmt: format string
 * @...: variable argument list
 *
 * Empty callback provided as a probe to the markers. By providing this to a
 * disabled marker, we make sure the  execution flow is always valid even
 * though the function pointer change and the marker enabling are two distinct
 * operations that modifies the execution flow of preemptible code.
 */
__attribute__ ((visibility ("protected")))
void __mark_empty_function(void *probe_private, void *call_private,
	const char *fmt, va_list *args)
{
}

/*
 * marker_probe_cb Callback that prepares the variable argument list for probes.
 * @mdata: pointer of type struct marker
 * @call_private: caller site private data
 * @fmt: format string
 * @...:  Variable argument list.
 *
 */
__attribute__ ((visibility ("protected")))
void marker_probe_cb(const struct marker *mdata, void *call_private,
	const char *fmt, ...)
{
	static unsigned int count = 0;

	printf("Test probe function %u\n", count++);
}

//FIXME : imv_read won't work with optimized immediate values.
//will need to issue one sys_marker call for each immediate value.
__attribute__ ((visibility ("protected")))
void testip(void)
{
	printf("addr : %p\n", __builtin_return_address(0));
}

__attribute__((constructor, visibility ("protected")))
void marker_init(void)
{
	struct marker *iter;
	int ret;

	printf("Marker section : from %p to %p (init)\n",
		__start___markers, __stop___markers);
	testip();
	for (iter = __start___markers; iter < __stop___markers; iter++) {
		printf("Marker : %s\n", iter->name);
		ret = sys_marker(iter->name, iter->format,
			&imv_read(iter->state), 1);
		if (ret)
			perror("Error connecting markers");
	}
}

__attribute__((destructor, visibility ("protected")))
void marker_fini(void)
{
	struct marker *iter;
	int ret;

	printf("Marker section : from %p to %p (fini)\n",
		__start___markers, __stop___markers);
	for (iter = __start___markers; iter < __stop___markers; iter++) {
		printf("Marker : %s\n", iter->name);
		ret = sys_marker(iter->name, iter->format,
			&imv_read(iter->state), 0);
		if (ret)
			perror("Error disconnecting markers");
	}
}
