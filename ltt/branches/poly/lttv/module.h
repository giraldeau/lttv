#ifndef MODULES_H
#define MODULES_H

#include <gmodule.h>

/* lttv modules are shared object files, to be loaded dynamically, which
   interact with the main module to provide additional capabilities. They
   typically register hooks to be called at various places, read and add
   global or trace attributes, and add menu items and tabbed windows to the
   graphical user interface. Both the hooks lists and the menus and windows
   are accessed as global attributes. */


/* Each lttv module must define a function named "init" with
   the following signature. The init function may itself load pre-requisite 
   modules using lttv_module_load. 

   It should also define a function named "destroy", which free the
   resources reserved during execution.

   Most modules will not use the command line arguments passed as init 
   arguments. It is easier to simply register command line options 
   to be parsed by the main module. However, some modules
   may require an "early access" to these arguments, for example a embedded
   python interpreter module which needs to know the modules written in
   python to load. */

/* Initial draft by Michel Dagenais May 2003
 * Reworked by Mathieu Desnoyers, May 2003
 */

/* index_standalone is the index of the module in the modulesStanalone array.
 * If the module is only loaded "DEPENDANT", index is -1.
 */

typedef struct lttv_module_info_ {
  GModule *module;
  char *name;
  char *directory;
  char *pathname;
  guint ref_count;
  gint index_standalone;
} lttv_module_info;

/* Loading type of modules : 
 * STANDALONE : the program takes care of unloading the moduels
 * DEPENDANT : The module that load this module is required to unload
 * 		it in it's destroy function.
 */

typedef enum _loadtype
{ STANDALONE, DEPENDANT
} loadtype;

typedef void (*lttv_module_load_init)(int argc, char **argv) ;


/* Load (if not already loaded) the named module. The init function of the
   module is executed upon loading. */

lttv_module_info *lttv_module_load(const char *name, int argc, char **argv,loadtype);



/* Unload (if already loaded) the named module. The destroy function of the
   module is executed before unloading. */

typedef void (*lttv_module_unload_destroy)() ;

int lttv_module_unload_pathname(const char *pathname,loadtype) ;

int lttv_module_unload_name(const char *name,loadtype) ;

int lttv_module_unload(lttv_module_info *moduleInfo,loadtype);

/* Unload all the modules */
void lttv_module_unload_all();

/* Additional module search paths may be defined. */

void lttv_module_path_add(const char *name);

#endif // MODULES_H
