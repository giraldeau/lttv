/* Sample module for Linux Trace Toolkit new generation User Interface */

/* Created by Mathieu Desnoyers, may 2003 */

#include <glib.h>
#include <gmodule.h>

G_MODULE_EXPORT void init() {
	g_critical("Sample module 2 init()");
}

G_MODULE_EXPORT void destroy() {
	g_critical("Sample module 2 destroy()");
}
	
