#ifndef LTTV_H
#define LTTV_H

#include "attribute.h"

/* Initial draft by Michel Dagenais May 2003
 * Reworked by Mathieu Desnoyers, May 2003
 */


/* The modules in the visualizer communicate with the main module and
   with each other through attributes. There is a global set of attributes as
   well as attributes attached to each trace set, trace and tracefile. */

lttv_attributes *lttv_global_attributes();



/* Modules are allowed to define new command line options.

   Each option has a long name (--long_name), a short one character 
   name (-c), a descriptive text, the argument type, and a
   pointer to where the argument value will be stored. For an option of
   type LTTV_OPT_NONE, the argument is a boolean value set to true when the
   option is present. */

/* Those are already in option.h, cause conflict */
//typedef enum _lttv_option_type 
//{LTTV_OPT_NONE, LTTV_OPT_STRING, LTTV_OPT_INT, LTTV_OPT_LONG } 
//lttv_option_type;


//void lttv_option_add(char *long_name, char char_name, char *description, 
//                    lttv_option_type t, void *p);



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

   /hooks/trace_set/before
       Before any analysis.

   /hooks/trace_set/after
       After all traces were analyzed.

   /hooks/trace/before
       Before each trace.

   /hooks/trace/after
       After each trace.

   /hooks/tracefile/before
       Before each tracefile.

   /hooks/tracefile/after
       After each tracefile.

   /hooks/event
       Called for each event

   /hooks/event_id
       This attribute contains an lttv_hooks_by_id, where the hooks for each
       id are to be called when an event of the associated type are found.

*/

#endif // LTTV_H
