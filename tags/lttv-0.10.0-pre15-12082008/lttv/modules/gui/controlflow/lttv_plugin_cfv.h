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

#ifndef LTTV_PLUGIN_CFV_H
#define LTTV_PLUGIN_CFV_H

#include <lttvwindow/lttv_plugin.h>
#include <lttvwindow/mainwindow-private.h>
#include "cfv.h"

/*
 * Type macros.
 */

#define LTTV_TYPE_PLUGIN_CFV		  (lttv_plugin_cfv_get_type ())
#define LTTV_PLUGIN_CFV(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LTTV_TYPE_PLUGIN_CFV, LttvPluginCFV))
#define LTTV_PLUGIN_CFV_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), LTTV_TYPE_PLUGIN_CFV, LttvPluginCFVClass))
#define LTTV_IS_PLUGIN_CFV(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LTTV_TYPE_PLUGIN_CFV))
#define LTTV_IS_PLUGIN_CFV_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LTTV_TYPE_PLUGIN_CFV))
#define LTTV_PLUGIN_CFV_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LTTV_TYPE_PLUGIN_CFV, LttvPluginCFVClass))

typedef struct _LttvPluginCFV LttvPluginCFV;
typedef struct _LttvPluginCFVClass LttvPluginCFVClass;

struct _LttvPluginCFV {
  LttvPlugin parent;

  /* instance members */
  ControlFlowData *cfd;

  /* private */
};

struct _LttvPluginCFVClass {
  LttvPluginClass parent;

  /* class members */
};

/* used by LTTV_PLUGIN_TAB_TYPE */
GType lttv_plugin_cfv_get_type (void);

/*
 * Method definitions.
 */


#endif
