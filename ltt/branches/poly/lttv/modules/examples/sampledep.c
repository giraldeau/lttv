/* Sample module for Linux Trace Toolkit new generation User Interface */

/* Created by Mathieu Desnoyers, may 2003 */

#include <glib.h>
#include <gmodule.h>

/* Include module.h from lttv headers for module loading */
#include <lttv/module.h>

G_MODULE_EXPORT void init(int argc, char * argv[], LttvModule *self) {
	g_critical("Sample module dependant init()");

	lttv_module_require(self, "samplemodule",argc,argv);
}

G_MODULE_EXPORT void destroy() {
	g_critical("Sample module dependant destroy()");
}
	
