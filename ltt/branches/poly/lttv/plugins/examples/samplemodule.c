/* Sample module for Linux Trace Toolkit new generation User Interface */

/* Created by Mathieu Desnoyers, may 2003 */

#include <glib-2.0/glib.h>
#include <glib-2.0/gmodule.h>

G_MODULE_EXPORT void init() {
	g_critical("Sample module init()");
}

G_MODULE_EXPORT void destroy() {
	g_critical("Sample module destroy()");
}
	
