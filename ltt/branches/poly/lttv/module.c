
/* module.c : Implementation of the module loading/unloading mechanism.
 * 
 */

/* Initial draft by Michel Dagenais May 2003
 * Reworked by Mathieu Desnoyers, May 2003
 */

#include "lttv.h"
#include "module.h"
#include <popt.h>

/* Table of loaded modules and paths where to search for modules */

static GHashTable *modules = NULL;

static GPtrArray *modulesStandalone = NULL;

static GPtrArray *modulesPaths = NULL;

void lttv_module_init(int argc, char **argv) {
  modules = g_hash_table_new(g_str_hash, g_str_equal);
  modulesStandalone = g_ptr_array_new();
  modulesPaths = g_ptr_array_new();
}

void lttv_module_destroy() {
  
  int i;

  /* Unload all modules */
  lttv_module_unload_all();

  /* Free the modules paths pointer array as well as the elements */
  for(i = 0; i< modulesPaths->len; i++) {
    g_free(modulesPaths->pdata[i]);
  }
  g_ptr_array_free(modulesPaths,TRUE) ;
  g_ptr_array_free(modulesStandalone,TRUE) ;
  modulesPaths = NULL;
  modulesStandalone = NULL;
  
  /* destroy the hash table */
  g_hash_table_destroy(modules) ;
  modules = NULL;
}

/* Add a new pathname to the modules loading search path */

void lttv_module_path_add(const char *name) {
  g_ptr_array_add(modulesPaths,(char*)g_strdup(name));
}


/* Load (if not already loaded) the named module. Its init function is
   called. We pass the options of the command line to it in case it has
   preliminary things to get from it. Note that the normal way to add a
   command line option for a module is through the options parsing mecanism.
   */

lttv_module_info *lttv_module_load(const char *name, int argc, char **argv, loadtype load) {

  GModule *gmodule;

  lttv_module_info *moduleInfo;

  int i;

  char *pathname;
  
  lttv_module_load_init init_Function;

  /* Find and load the module, It will increase the usage counter
   * If the module is already loaded, only the reference counter will
   * be incremented. It's part of the gmodule architecture. Very useful
   * for modules dependencies.
   */

  g_assert(name != NULL);

  for(i = 0 ; i < modulesPaths->len ; i++) {
    pathname = g_module_build_path(modulesPaths->pdata[i],name);
    gmodule = g_module_open(pathname,0) ;
    
    
    if(gmodule != NULL) {
      g_message("Loading module %s ... found!",pathname);

      /* Was the module already opened? */
      moduleInfo = g_hash_table_lookup(modules,g_module_name(gmodule));

      /* First time the module is opened */

      if(moduleInfo == NULL ) {
        moduleInfo = g_new(lttv_module_info, 1);
        moduleInfo->module = gmodule;
        moduleInfo->pathname = g_module_name(gmodule);
        moduleInfo->directory = modulesPaths->pdata[i];
        moduleInfo->name = (char *)g_strdup(name);
	moduleInfo->ref_count = 0;
	moduleInfo->index_standalone = -1;
        g_hash_table_insert(modules, moduleInfo->pathname, moduleInfo);
        if(!g_module_symbol(gmodule, "init", (gpointer) &init_Function)) {
 	  g_critical("module %s (%s) does not have init function",
            moduleInfo->pathname,moduleInfo->name);
	}
        else {
          init_Function(argc,argv);
        }
      }

      /* Add the module in the standalone array if the module is
       * standalone and not in the array. Otherwise, set index to
       * -1 (dependant only).
       */
      if(load == STANDALONE) {
	      
        if(moduleInfo->index_standalone == -1) {
		
          g_ptr_array_add(modulesStandalone, moduleInfo);
          moduleInfo->index_standalone = modulesStandalone->len - 1;

	  moduleInfo->ref_count++ ;  
        }
	else {
          g_warning("Module %s is already loaded standalone.",pathname);
	  /* Decrease the gmodule use_count. Has previously been increased in the g_module_open. */
  	  g_module_close(moduleInfo->module) ;
	}
      }
      else { /* DEPENDANT */
	  moduleInfo->ref_count++ ;
      }

      return moduleInfo;
    }
    g_message("Loading module %s ... missing.",pathname);
    g_free(pathname);
  }
  g_critical("module %s not found",name);
  return NULL;
}

/* Unload the named module. */

int lttv_module_unload_pathname(const char *pathname, loadtype load) {

  lttv_module_info *moduleInfo;

  moduleInfo = g_hash_table_lookup(modules, pathname);

  /* If no module of that name is loaded, nothing to unload. */
  if(moduleInfo != NULL) {
    g_message("Unloading module %s : is loaded.\n", pathname) ;
    lttv_module_unload(moduleInfo, load) ;
    return 1;
  }
  else {
    g_message("Unloading module %s : is not loaded.\n", pathname) ;
    return 0;
  }
 
}

int lttv_module_unload_name(const char *name, loadtype load) {

  int i;

  char *pathname;
  
  /* Find and load the module, It will increase the usage counter
   * If the module is already loaded, only the reference counter will
   * be incremented. It's part of the gmodule architecture. Very useful
   * for modules dependencies.
   */

  g_assert(name != NULL);

  for(i = 0 ; i < modulesPaths->len ; i++) {

    pathname = g_module_build_path(modulesPaths->pdata[i],name);
    
    if(lttv_module_unload_pathname(pathname, load) == TRUE)
      return TRUE ;
  }
  g_critical("module %s not found",name);
  return FALSE;
}



/* Unload the module. We use a call_gclose boolean to keep the g_module_close call
 * after the call to the module's destroy function. */
  
int lttv_module_unload(lttv_module_info *moduleInfo, loadtype load) {

  lttv_module_unload_destroy destroy_Function;

  char *moduleName ;

  gboolean call_gclose = FALSE;

  if(moduleInfo == NULL) return FALSE;

 /* Closing the module decrements the usage counter if previously higher than
  * 1. If 1, it unloads the module.
  */

  /* Add the module in the standalone array if the module is
   * standalone and not in the array. Otherwise, set index to
   * -1 (dependant only).
   */
  if(load == STANDALONE) {
     
    if(moduleInfo->index_standalone == -1) {
	
      g_warning("Module %s is not loaded standalone.",moduleInfo->pathname);
    }
    else {
      /* We do not remove the element of the array, it would change
       * the index orders. We will have to check if index is -1 in
       * unload all modules.
       */
      moduleInfo->index_standalone = -1;
      g_message("Unloading module %s, reference count passes from %u to %u",
  	    moduleInfo->pathname,moduleInfo->ref_count,
	    moduleInfo->ref_count-1);

      moduleInfo->ref_count-- ;
      call_gclose = TRUE ;
    }
  }
  else { /* DEPENDANT */
    g_message("Unloading module %s, reference count passes from %u to %u",
              moduleInfo->pathname,
              moduleInfo->ref_count,moduleInfo->ref_count-1);

              moduleInfo->ref_count-- ;
	      call_gclose = TRUE ;
  }

  /* The module is really closing if ref_count is 0 */
  if(!moduleInfo->ref_count) {
    g_message("Unloading module %s : closing module.",moduleInfo->pathname);

    /* Call the destroy function of the module */
    if(!g_module_symbol(moduleInfo->module, "destroy", (gpointer) &destroy_Function)) {
       g_critical("module %s (%s) does not have destroy function",
       		moduleInfo->pathname,moduleInfo->name);
    }
    else {
       destroy_Function();
    }
  
    /* If the module will effectively be closed, remove the moduleInfo from
     * the hash table and free the module name.
     */
    g_free(moduleInfo->name) ;

    g_hash_table_remove(modules, moduleInfo->pathname);
  }
  
  if(call_gclose) g_module_close(moduleInfo->module) ;

  return TRUE ;
}

#define MODULE_I ((lttv_module_info *)modulesStandalone->pdata[i])

/* unload all the modules in the hash table, calling module_destroy for
 * each of them.
 *
 * We first take all the moduleInfo in the hash table, put it in an
 * array. We use qsort on the array to have the use count of 1 first.
 */
void lttv_module_unload_all() {
	
  int i = 0;

  /* call the unload for each module.
   */
  for(i = 0; i < modulesStandalone->len; i++) {
	  
    if(MODULE_I->index_standalone != -1) {
    lttv_module_unload(MODULE_I,STANDALONE) ;
    }
  }
 
}
