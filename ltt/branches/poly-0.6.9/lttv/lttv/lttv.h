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

#ifndef LTTV_H
#define LTTV_H

#include <lttv/attribute.h>

/* The modules in the visualizer communicate with the main module and
   with each other through attributes. There is a global set of attributes */

LttvAttribute *lttv_global_attributes();

extern gboolean lttv_profile_memory;

extern int lttv_argc;

extern char **lttv_argv;

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

#ifndef g_debug
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)
#endif

#endif // LTTV_H
