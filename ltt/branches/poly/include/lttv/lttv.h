#ifndef LTTV_H
#define LTTV_H

#include <lttv/attribute.h>

/* The modules in the visualizer communicate with the main module and
   with each other through attributes. There is a global set of attributes */

LttvAttribute *lttv_global_attributes();


/* A number of global attributes are initialized before modules are
   loaded, for example hooks lists. More global attributes are defined
   in individual mudules to store information or to communicate with other
   modules (GUI windows, menus...).

   The hooks lists (lttv_hooks) are initialized in the main module and may be 
   used by other modules. Each corresponds to a specific location in the main
   module processing loop. The attribute key and typical usage for each 
   is indicated.

   /hooks/options/before
       Good place to define new command line options to be parsed.

   /hooks/options/after
       Read the values set by the command line options.

   /hooks/main/before

   /hooks/main/after

*/

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)

#endif // LTTV_H
