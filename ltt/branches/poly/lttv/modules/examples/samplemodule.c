/* Sample module for Linux Trace Toolkit new generation User Interface */

/* Created by Mathieu Desnoyers, may 2003 */

#include <glib.h>
#include <lttv/module.h>

static void init() {
	g_critical("Sample module init()");
}

static void destroy() {
	g_critical("Sample module destroy()");
}


LTTV_MODULE("sampledep", "Medium...", "Long...", init, destroy, {})

