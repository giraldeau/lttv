/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2006 Mathieu Desnoyers
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

/* The LttvPlugin class is a pure virtual class. It only contains the functions
 * available for interaction with a plugin (tab or viewer).
 */

#ifndef LTTV_PLUGIN_H
#define LTTV_PLUGIN_H

#include <gtk/gtk.h>
#include <lttv/filter.h>

/*
 * Type macros.
 */

#define LTTV_TYPE_PLUGIN		  (lttv_plugin_get_type ())
#define LTTV_PLUGIN(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LTTV_TYPE_PLUGIN, LttvPlugin))
#define LTTV_PLUGIN_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), LTTV_TYPE_PLUGIN, LttvPluginClass))
#define LTTV_IS_PLUGIN(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LTTV_TYPE_PLUGIN))
#define LTTV_IS_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LTTV_TYPE_PLUGIN))
#define LTTV_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LTTV_TYPE_PLUGIN, LttvPluginClass))

typedef struct _LttvPlugin LttvPlugin;
typedef struct _LttvPluginClass LttvPluginClass;

struct _LttvPlugin {
  GObject parent;
  /* instance members */
  GtkWidget *top_widget;

  /* private */
};

struct _LttvPluginClass {
  GObjectClass parent;

  void (*update_filter) (LttvPlugin *self, LttvFilter *filter);
  
  /* class members */
};

/* used by LTTV_PLUGIN_TYPE */
GType lttv_plugin_get_type (void);

/*
 * Method definitions.
 */

/* declaration in the header. */
void lttv_plugin_update_filter (LttvPlugin *self, LttvFilter *filter);




#endif
