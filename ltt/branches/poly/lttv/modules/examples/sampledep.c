/* Sample module for Linux Trace Toolkit new generation User Interface */

/* Created by Mathieu Desnoyers, may 2003 */

#include <glib.h>

/* Include module.h from lttv headers for module loading */
#include <lttv/module.h>

static void init() {
	g_critical("Sample module dependant init()");
}

static void destroy() {
	g_critical("Sample module dependant destroy()");
}


LTTV_MODULE("sampledep", "Medium desc...", "Long desc...", init, destroy, \
	    { "samplemodule" })

