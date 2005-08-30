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

#ifndef MODULES_H
#define MODULES_H

#include <glib.h>

/* A module contains some functionality which becomes available atfer it is
   initialized and before it is destroyed. A module is characterized by a name,
   a description (short and long), a list of names of other modules on which 
   it depends, and an initialization and a destruction function.

   A library contains one or more modules and may be loaded dynamically.
   The modules contained in a library are automatically registered through
   constructors which are called when the library is loaded. For modules
   directly linked with the main program (builtin), the constructors are
   called before the main program starts. (However, neither malloc nor glib 
   functions are used during the registration process).

   The library loading path is a set of directories, where requested
   libraries and modules are searched for.
*/

typedef struct _LttvModule LttvModule;

typedef struct _LttvLibrary LttvLibrary;

typedef void (*LttvModuleInit)();

typedef void (*LttvModuleDestroy)();

typedef struct _LttvModuleInfo
{
  char *name;
  char *short_description;
  char *description;
  LttvModuleInit init;
  LttvModuleDestroy destroy;
  LttvLibrary *library;
  unsigned require_count;
  unsigned use_count;
  unsigned prerequisites_number;
} LttvModuleInfo;


typedef struct _LttvLibraryInfo
{
  char *name;
  char *path;
  unsigned load_count;
} LttvLibraryInfo;


typedef enum _LttvModuleError 
{
  LTTV_MODULE_NOT_FOUND,
  LTTV_MODULE_NO_INIT
} LttvModuleError;


/* Insure that a module is loaded and initialized. Require count 
   (number of times the module was required) and use count 
   (number of times required or used as prerequisite) serve to 
   insure that a module is destroyed only after it has been released 
   as many times as it was required (and prerequired).

   The module is searched among the modules currently loaded, then as a 
   similarly named library to load which should contain the named module.
   If the module cannot be found or loaded, NULL is returned and an
   explanation is provided in error. */

LttvModule *lttv_module_require(char *name, GError **error);

void lttv_module_release(LttvModule *m);


/* Obtain information about the module, including the containing library */

void lttv_module_info(LttvModule *m, LttvModuleInfo *info);


/* List the modules on which this module depends */

unsigned lttv_module_prerequisite_number(LttvModule *m);

LttvModule *lttv_module_prerequisite_get(LttvModule *m, unsigned i);


/* Insure that a library is loaded. A load count insures that a library 
   is unloaded only after it has been asked to unload as
   many times as it was loaded, and its modules are not in use. The library
   is searched along the library path if name is a relative pathname. 
   If the library cannot be found or loaded, NULL is returned and an 
   explanation is provided in error. */

LttvLibrary *lttv_library_load(char *name, GError **error);

/* Returns 0 if library is unloaded, > 0 otherwise */
gint lttv_library_unload(LttvLibrary *l);


/* Obtain information about the library */

void lttv_library_info(LttvLibrary *l, LttvLibraryInfo *info);


/* List the modules contained in a library */

unsigned lttv_library_module_number(LttvLibrary *l);

LttvModule *lttv_library_module_get(LttvLibrary *l, unsigned i);


/* List the currently loaded libraries */

unsigned lttv_library_number();

LttvLibrary *lttv_library_get(unsigned i);



/* Add or remove directory names to the library search path */

void lttv_library_path_add(const char *name);

void lttv_library_path_remove(const char *name);


/* List the directory names in the library search path */

unsigned lttv_library_path_number();

char *lttv_library_path_get(unsigned i);


/* To define a module, simply call the LTTV_MODULE macro with the needed
   arguments: single word name, one line short description, larger
   description, initialization function, destruction function, and
   list of names for required modules (e.g., "moduleA", "moduleB").
   This will insure that the module is registered at library load time.

   Example:

   LTTV_MODULE("option", "Command line options processing", "...", \
       init, destroy, "moduleA", "moduleB") 
*/

#define LTTV_MODULE(name, short_desc, desc, init, destroy, ...) \
  \
  static void _LTTV_MODULE_REGISTER(__LINE__)() \
      __attribute__((constructor));		 \
  \
  static void _LTTV_MODULE_REGISTER(__LINE__)() \
  { \
    static char *module_prerequisites[] = { __VA_ARGS__ };	\
    \
    static struct _LttvModuleDescription module = { \
        name, short_desc, desc, init, destroy, \
        sizeof(module_prerequisites) / sizeof(char *), \
        module_prerequisites, NULL}; \
        \
    lttv_module_register(&module); \
  }


/* Internal structure and function used to register modules, called by 
   LTTV_MODULE */

#define __LTTV_MODULE_REGISTER(line) _lttv_module_register_ ## line
#define _LTTV_MODULE_REGISTER(line) __LTTV_MODULE_REGISTER(line)

struct _LttvModuleDescription
{
  char *name;
  char *short_description;
  char *description;
  LttvModuleInit init;
  LttvModuleDestroy destroy;
  unsigned prerequisites_number;
  char **prerequisites;
  struct _LttvModuleDescription *next;
};

void lttv_module_register(struct _LttvModuleDescription *d);

#endif // MODULES_H






