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


#include "lttv_plugin_cfv.h"
#include <lttvwindow/lttvwindow.h>
#include "drawing.h"
#include "eventhooks.h"

/* 
 * forward definitions
 */

/*
 * Implementation
 */

static void cfv_update_filter(LttvPlugin *parent, LttvFilter *filter)
{
  LttvPluginCFV *self = LTTV_PLUGIN_CFV(parent);
  g_message("In CFV update filter.");
  lttv_filter_destroy(self->cfd->filter);
  self->cfd->filter = filter;
  redraw_notify(self->cfd, NULL);
}


static void
lttv_plugin_cfv_class_init (LttvPluginCFVClass *klass)
{
  LttvPluginClass *parent_klass;
  parent_klass = &klass->parent;
  parent_klass->update_filter = cfv_update_filter;
  g_type_class_add_private (klass, sizeof (ControlFlowData));
}


static void
lttv_plugin_cfv_init (GTypeInstance *instance, gpointer g_class)
{
  LttvPluginCFV *self = LTTV_PLUGIN_CFV (instance);
  self->cfd = G_TYPE_INSTANCE_GET_PRIVATE (self,
      LTTV_TYPE_PLUGIN_CFV, ControlFlowData);
}


GType
lttv_plugin_cfv_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (LttvPluginCFVClass),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      lttv_plugin_cfv_class_init,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      sizeof (LttvPluginCFV),
      0,      /* n_preallocs */
      lttv_plugin_cfv_init    /* instance_init */
      };
      type = g_type_register_static (G_TYPE_OBJECT,
                                     "LttvPluginRVType",
                                     &info, 0);
    }
    return type;
}


