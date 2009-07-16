/* Sample module for Linux Trace Toolkit new generation User Interface */

/* Created by Mathieu Desnoyers, may 2003 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <lttv/module.h>

static void init() {
	g_critical("Sample module init()");
}

static void destroy() {
	g_critical("Sample module destroy()");
}


LTTV_MODULE("sampledep", "Medium...", "Long...", init, destroy, {})

