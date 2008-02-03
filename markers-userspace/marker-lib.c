
#include "marker.h"

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
void marker_probe_cb(const struct marker *mdata, void *call_private,
	const char *fmt, ...)
{


}
