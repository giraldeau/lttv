/* Sample module for Linux Trace Toolkit new generation User Interface */

/* Created by Mathieu Desnoyers, may 2003 */

#include <glib.h>
#include <gmodule.h>

/* Include module.h from lttv headers for module loading */
#include "../../include/lttv/module.h"

G_MODULE_EXPORT void init() {
	g_critical("Sample module dependant init()");

	lttv_module_load("samplemodule",0,NULL,DEPENDANT);
}

G_MODULE_EXPORT void destroy() {
	g_critical("Sample module dependant destroy()");
	lttv_module_unload_name("samplemodule",DEPENDANT);
}
	
