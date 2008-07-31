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

#ifndef LTTV_PLUGIN_XFV_H
#define LTTV_PLUGIN_XFV_H

#include <lttvwindow/lttv_plugin.h>
#include <lttvwindow/mainwindow-private.h>
#include "xfv.h"

/*
 * Type macros.
 */

#define LTTV_TYPE_PLUGIN_XFV		  (lttv_plugin_xfv_get_type ())
#define LTTV_PLUGIN_XFV(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LTTV_TYPE_PLUGIN_XFV, LttvPluginXFV))
#define LTTV_PLUGIN_XFV_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), LTTV_TYPE_PLUGIN_XFV, LttvPluginXFVClass))
#define LTTV_IS_PLUGIN_XFV(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LTTV_TYPE_PLUGIN_XFV))
#define LTTV_IS_PLUGIN_XFV_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LTTV_TYPE_PLUGIN_XFV))
#define LTTV_PLUGIN_XFV_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LTTV_TYPE_PLUGIN_XFV, LttvPluginXFVClass))

typedef struct _LttvPluginXFV LttvPluginXFV;
typedef struct _LttvPluginXFVClass LttvPluginXFVClass;

struct _LttvPluginXFV {
  LttvPlugin parent;

  /* instance members */
  XenoLTTData *xfd;

  /* private */
};

struct _LttvPluginXFVClass {
  LttvPluginClass parent;

  /* class members */
};

/* used by LTTV_PLUGIN_TAB_TYPE */
GType lttv_plugin_xfv_get_type (void);

/*
 * Method definitions.
 */


#endif
