#ifndef MODULES_H
#define MODULES_H

#include <gmodule.h>

/* lttv modules are shared object files, to be loaded dynamically, which
   interact with the main module to provide additional capabilities. They
   typically register hooks to be called at various places, read and add
   attributes...

   Each lttv module must define a function named "init" with
   the following signature. The init function may itself require other
   modules using lttv_module_require. 

   It should also define a function named "destroy" to free the
   resources reserved during execution.

   Most modules will not use the command line arguments passed as init 
   arguments. It is easier to simply register command line options 
   to be parsed by the main module. However, some modules
   may require an "early access" to these arguments, for example an embedded
   python interpreter module which needs to know the modules written in
   python to load. */

typedef struct _LttvModule LttvModule;

typedef void (*LttvModuleInit)(LttvModule *self, int argc, char **argv);

typedef void (*LttvModuleDestroy)();


/* Additional module search paths may be defined. */

void lttv_module_path_add(const char *name);


/* Load (or increment its reference count if already loaded) the named module.
   The init function of the module is executed upon loading. */

LttvModule *lttv_module_load(const char *name, int argc, char **argv);


/* Module m depends on the named module. The named module will be loaded,
   remembered by m as a dependent, and unloaded when m is unloaded. */

LttvModule *lttv_module_require(LttvModule *m, const char *name, int argc,
    char **argv);


/* Decrement the reference count of the specified module and unload it if 0.
   The destroy function of the module is executed before unloading. 
   Dependent modules are unloaded. */

void lttv_module_unload(LttvModule *m) ;


/* List the loaded modules. The returned array contains nb elements and
   must be freed with g_free. */

LttvModule **lttv_module_list(guint *nb);


/* Obtain information about a module. The list of dependent module is
   returned and must be freed with g_free. */

LttvModule **lttv_module_info(LttvModule *m, const char **name, 
    guint *ref_count, guint *load_count, guint *nb_dependents);

#endif // MODULES_H
