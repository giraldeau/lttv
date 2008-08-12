/* Sample module for Linux Trace Toolkit new generation User Interface */

/* Created by Mathieu Desnoyers, may 2003 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <lttv/module.h>

static void init() {
	g_critical("Sample module 2 init()");
}

static void destroy() {
	g_critical("Sample module 2 destroy()");
}


LTTV_MODULE("samplemodule2", "Medium...", "Long...", init, destroy, {})

