
/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Michel Dagenais
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
 * MA 02111-1307, USA.
 */


/* module.c : Implementation of the module loading/unloading mechanism. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lttv/module.h>
#include <gmodule.h>
#include <stdlib.h>

struct _LttvLibrary
{
  LttvLibraryInfo info;
  GPtrArray *modules;
  GModule *gm;
  guint locked_loaded;
};


struct _LttvModule
{
  LttvModuleInfo info;
  char **prerequisites_names;
  GPtrArray *prerequisites;
};


/* Modules are searched by name. However, a library may be loaded which
   provides a module with the same name as an existing one. A stack of
   modules is thus maintained for each name. 

   Libraries correspond to glib modules. The g_module function is 
   responsible for loading each library only once. */

static GHashTable *modules_by_name = NULL;

static GPtrArray *libraries = NULL;

static GHashTable *libraries_by_g_module = NULL;

static GPtrArray *library_paths = NULL;

static gboolean initialized = FALSE;

static gboolean destroyed = TRUE;

static struct _LttvModuleDescription *builtin_chain = NULL;

static struct _LttvModuleDescription *module_chain = NULL;

static struct _LttvModuleDescription **module_next = &module_chain;

static GQuark lttv_module_error;

static void init();

static void finish_destroy();

static void module_release(LttvModule *m);


static LttvLibrary *library_add(char *name, char *path, GModule *gm)
{
  LttvLibrary *l;

  LttvModule *m;

  struct _LttvModuleDescription *link;

  GPtrArray *modules;

  l = g_new(LttvLibrary, 1);
  l->modules = g_ptr_array_new();
  l->gm = gm;
  l->locked_loaded = 0;
  l->info.name = g_strdup(name);
  l->info.path = g_strdup(path);
  l->info.load_count = 0;

  g_ptr_array_add(libraries, l);
  g_hash_table_insert(libraries_by_g_module, gm, l);

  *module_next = NULL;
  for(link = module_chain; link != NULL; link = link->next) {
    m = g_new(LttvModule, 1);
    g_ptr_array_add(l->modules, m);

    modules = g_hash_table_lookup(modules_by_name, link->name);
    if(modules == NULL) {
      modules = g_ptr_array_new();
      g_hash_table_insert(modules_by_name, g_strdup(link->name), modules);
    }
    g_ptr_array_add(modules, m);

    m->prerequisites_names = link->prerequisites;
    m->prerequisites = g_ptr_array_new();
    m->info.name = link->name;
    m->info.short_description = link->short_description;
    m->info.description = link->description;
    m->info.init = link->init;
    m->info.destroy = link->destroy;
    m->info.library = l;
    m->info.require_count = 0;
    m->info.use_count = 0;
    m->info.prerequisites_number = link->prerequisites_number;
  }
  return l;
}


static void library_remove(LttvLibrary *l)
{
  LttvModule *m;

  GPtrArray *modules;
  GPtrArray **modules_ptr = &modules; /* for strict aliasing */
  guint i;

  char *key;
  char **key_ptr = &key; /* for strict aliasing */

  for(i = 0 ; i < l->modules->len ; i++) {
    m = (LttvModule *)(l->modules->pdata[i]);

    g_hash_table_lookup_extended(modules_by_name, m->info.name, 
				 (gpointer *)key_ptr, (gpointer *)modules_ptr);
    g_assert(modules != NULL);
    g_ptr_array_remove(modules, m);
    if(modules->len == 0) {
      g_hash_table_remove(modules_by_name, m->info.name);
      g_ptr_array_free(modules, TRUE);
      g_free(key);
    }

    g_ptr_array_free(m->prerequisites, TRUE);
    g_free(m);
  }

  g_ptr_array_remove(libraries, l);
  g_hash_table_remove(libraries_by_g_module, l->gm);
  g_ptr_array_free(l->modules, TRUE);
  g_free(l->info.name);
  g_free(l->info.path);
  g_free(l);
}


static LttvLibrary *library_load(char *name, GError **error)
{
  GModule *gm = NULL;

  int i, nb;

  /* path is always initialized, checked */
  char *path = NULL, *pathname;

  LttvLibrary *l;

  GString *messages = g_string_new("");

  /* insure that module.c is initialized */

  init();

  /* Try to find the library along all the user specified paths */

  g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Load library %s", name);
  nb = lttv_library_path_number();
  for(i = 0 ; i <= nb ; i++) {
    if(i < nb) path = lttv_library_path_get(i);
    else path = NULL;

    pathname = g_module_build_path(path ,name);
    g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Try path %s", pathname);
    module_chain = NULL;
    module_next = &module_chain;
    gm = g_module_open(pathname,0);
    g_free(pathname);
    
    if(gm != NULL) break;

    messages = g_string_append(messages, g_module_error());
    messages = g_string_append(messages, "\n");
    g_log(G_LOG_DOMAIN,G_LOG_LEVEL_INFO,"Trial failed, %s", g_module_error());
  }

  /* Module cannot be found */

  if(gm == NULL) {
    g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Failed to load %s", name); 
    g_set_error(error, lttv_module_error, LTTV_MODULE_NOT_FOUND,
          "Cannot load library %s: %s", name, messages->str);
    g_string_free(messages, TRUE);
    return NULL;
  }
  g_string_free(messages, TRUE);

  /* Check if the library was already loaded */

  l = g_hash_table_lookup(libraries_by_g_module, gm);

  /* This library was not already loaded */

  if(l == NULL) {
    g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Library %s (%s) loaded", name, 
        g_module_name(gm));
    l = library_add(name, path, gm);
  }
  return l;
}


LttvLibrary *lttv_library_load(char *name, GError **error)
{
  LttvLibrary *l = library_load(name, error);
  if(l != NULL) l->info.load_count++;
  return l;
}

/* Returns < 0 if still in use, 0 if freed */
static gint library_unload(LttvLibrary *l)
{
  guint i;

  GModule *gm;

  LttvModule *m;

  if(l->locked_loaded > 0) {
    g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Unload library %s: locked loaded", 
        l->info.name);
    return 1;
  }

  if(l->info.load_count > 0) {
    g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Unload library %s: load count %d", 
	l->info.name, l->info.load_count);
    return l->info.load_count;
  }

  /* Check if all its modules have been released */

  for(i = 0 ; i < l->modules->len ; i++) {
    m = (LttvModule *)(l->modules->pdata[i]);
    if(m->info.use_count > 0) {
      g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO,"Unload library %s: module %s used",
	   l->info.name, m->info.name);
      return 1;
    }
  }

  g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Unload library %s: close the GModule",
       l->info.name);
  gm = l->gm;
  library_remove(l);
  if(gm != NULL) g_module_close(gm);

  /* insure that module.c will be finalized */

  finish_destroy();
  return 0;
}


gint lttv_library_unload(LttvLibrary *l)
{
  /* In the case where we wait for a module to release, the load count is 0
   * and should not be decremented. */
  if(l->info.load_count != 0) {
    l->info.load_count--;
    return l->info.load_count;
  } else {
    library_unload(l);
    return 0;
  }
}


static void library_lock_loaded(LttvLibrary *l)
{
  l->locked_loaded++;
}


static gint library_unlock_loaded(LttvLibrary *l)
{
  l->locked_loaded--;
  return library_unload(l);
}


static LttvModule *module_require(char *name, GError **error)
{
  GError *tmp_error = NULL;

  guint i, j;

  LttvModule *m, *required;

  LttvLibrary *l = NULL;

  GPtrArray *modules;

  /* Insure that module.c is initialized */

  init();

  /* Check if the module is already loaded */

  modules = g_hash_table_lookup(modules_by_name, name);

  /* Try to load a library having the module name */

  if(modules == NULL) {
    l = library_load(name, error);
    if(l == NULL) return NULL;
    else library_lock_loaded(l);

    /* A library was found, does it contain the named module */

    modules = g_hash_table_lookup(modules_by_name, name);
    if(modules == NULL) {
      g_set_error(error, lttv_module_error, LTTV_MODULE_NOT_FOUND,
          "Module %s not found in library %s", name, l->info.name);
      library_unlock_loaded(l);
      return NULL;
    }
  }
  m = (LttvModule *)(modules->pdata[modules->len - 1]);

  /* We have the module */

  m->info.use_count++;

  /* First use of the module. Initialize after getting the prerequisites */

  if(m->info.use_count == 1) {
    for(i = 0 ; i < m->info.prerequisites_number ; i++) {
      required = module_require(m->prerequisites_names[i], &tmp_error);

      /* A prerequisite could not be found, undo everything and fail */

      if(required == NULL) {
        for(j = 0 ; j < m->prerequisites->len ; j++) {
          module_release((LttvModule *)(m->prerequisites->pdata[j]));
        }
        g_ptr_array_set_size(m->prerequisites, 0);
        if(l != NULL) library_unlock_loaded(l);
        g_set_error(error, lttv_module_error, LTTV_MODULE_NOT_FOUND,
            "Cannot find prerequisite for module %s: %s", name, 
	    tmp_error->message);
        g_clear_error(&tmp_error);
        return NULL;
      }
      g_ptr_array_add(m->prerequisites, required);
    }
    g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Module %s: init()", m->info.name);
    m->info.init();
  }

  /* Decrement the load count of the library. It will not really be 
     unloaded since it contains a currently used module. */

  if(l != NULL) library_unlock_loaded(l);

  return(m);
}


/* The require_count for a module is the number of explicit calls to 
   lttv_module_require, while the use_count also counts the number of times
   a module is needed as a prerequisite. */

LttvModule *lttv_module_require(char *name, GError **error)
{
  LttvModule *m = module_require(name, error);
  if(m != NULL) m->info.require_count++;
  return(m);
}


static void module_release(LttvModule *m)
{
  guint i;

  library_lock_loaded(m->info.library);

  m->info.use_count--;
  if(m->info.use_count == 0) {
    g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Module %s: destroy()",m->info.name);
    m->info.destroy();
    for(i = 0 ; i < m->prerequisites->len ; i++) {
      module_release((LttvModule *)(m->prerequisites->pdata[i]));
    }
    g_ptr_array_set_size(m->prerequisites, 0);
  }
  library_unlock_loaded(m->info.library);
}


void lttv_module_release(LttvModule *m)
{
  m->info.require_count--;
  module_release(m);
}


void lttv_module_info(LttvModule *m, LttvModuleInfo *info)
{
  *info = m->info;
}


unsigned lttv_module_prerequisite_number(LttvModule *m)
{
  return m->prerequisites->len;
}


LttvModule *lttv_module_prerequisite_get(LttvModule *m, unsigned i)
{
  return (LttvModule *)(m->prerequisites->pdata[i]);
}


void lttv_library_info(LttvLibrary *l, LttvLibraryInfo *info)
{
  *info = l->info;
}


unsigned lttv_library_module_number(LttvLibrary *l)
{
  return l->modules->len;
}


LttvModule *lttv_library_module_get(LttvLibrary *l, unsigned i)
{
  return (LttvModule *)(l->modules->pdata[i]);
}


unsigned lttv_library_number()
{
  return libraries->len;  
}


LttvLibrary *lttv_library_get(unsigned i)
{
  return (LttvLibrary *)(libraries->pdata[i]);
}


void lttv_library_path_add(const char *name)
{
  g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Add library path %s", name);
  g_ptr_array_add(library_paths,(char*)g_strdup(name));
}


void lttv_library_path_remove(const char *name) 
{
  guint i;

  for(i = 0 ; i < library_paths->len ; i++) {
    if(g_str_equal(name, library_paths->pdata[i])) {
      g_free(library_paths->pdata[i]);
      g_ptr_array_remove_index(library_paths,i);
      return;
    }
  }
}


unsigned lttv_library_path_number()
{
  return library_paths->len;
}


char *lttv_library_path_get(unsigned i)
{
  return (char *)(library_paths->pdata[library_paths->len - i - 1]);
}


void lttv_module_register(struct _LttvModuleDescription *d)
{
  *module_next = d;
  module_next = &(d->next);
}


static void init() 
{
  if(initialized) return;
  g_assert(destroyed);

  g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Init module.c");

  initialized = TRUE;
  destroyed = FALSE;
  lttv_module_error = g_quark_from_string("LTTV_MODULE_ERROR");
  modules_by_name = g_hash_table_new(g_str_hash, g_str_equal);
  libraries = g_ptr_array_new();
  libraries_by_g_module = g_hash_table_new(g_direct_hash, g_direct_equal);
  library_paths = g_ptr_array_new();

  if(builtin_chain == NULL) builtin_chain = module_chain;
  module_chain = builtin_chain;
  library_add("builtin", NULL, NULL);
}


static void finish_destroy()
{
  guint i;

  if(initialized) return;
  g_assert(!destroyed);

  g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Finish destroy module.c");
  g_hash_table_destroy(modules_by_name);
  g_ptr_array_free(libraries, TRUE);
  g_hash_table_destroy(libraries_by_g_module);
  for(i = 0 ; i < library_paths->len ; i++) {
    g_free(library_paths->pdata[i]);
  }
  g_ptr_array_free(library_paths, TRUE);
  destroyed = TRUE;
}


static void destroy() 
{  
  guint i, j, nb;

  LttvLibrary *l, **locked_libraries;

  LttvModule *m;

  g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Destroy module.c");

  /* Unload all libraries */

  nb = libraries->len;
  locked_libraries = g_new(LttvLibrary *, nb);

  for(i = 0 ; i < nb ; i++) {
    //g_assert(nb == libraries->len);
    l = (LttvLibrary *)(libraries->pdata[i]);
    locked_libraries[i] = l;
    library_lock_loaded(l);
    for(j = 0 ; j < l->modules->len ; j++) {
      m = (LttvModule *)(l->modules->pdata[j]);
      while(m->info.require_count > 0) lttv_module_release(m);
    }
    if(library_unlock_loaded(l) > 0)
      while(lttv_library_unload(l) > 0);

    /* If the number of librairies loaded have changed, restart from the
     * beginning */
    if(nb != libraries->len) {
      i = 0;
      nb = libraries->len;
    }

  }

  for(i = 0 ; i < nb ; i++) {
    l = locked_libraries[i];
    library_unlock_loaded(l);
  }
  g_free(locked_libraries);

  /* The library containing module.c may be locked by our caller */

  g_assert(libraries->len <= 1); 

  initialized = FALSE;

  exit(0);
}

LTTV_MODULE("module", "Modules in libraries",			      \
    "Load libraries, list, require and initialize contained modules", \
    init, destroy)

